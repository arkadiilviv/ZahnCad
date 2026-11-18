#include "zahn/project.hpp"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include "zahn/surface.hpp"
#include <cmath>
#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <map>
#include <memory>
#include <stdexcept>
namespace zahn {
namespace {
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
// Verify ZIP CRCs before rotating a recovery file. A damaged latest snapshot
// must never overwrite the last readable previous snapshot.
bool validRecoveryArchive(const QString& path) {
  struct Reader {QFile file;char block[65536];explicit Reader(const QString& p):file(p){}} reader(path);
  if(!reader.file.open(QIODevice::ReadOnly))return false;
  std::unique_ptr<archive,decltype(&archive_read_free)> a(archive_read_new(),archive_read_free);
  archive_read_support_format_zip(a.get());
  auto read=[](archive*,void* data,const void** output)->la_ssize_t{auto& r=*static_cast<Reader*>(data);*output=r.block;return r.file.read(r.block,sizeof(r.block));};
  if(archive_read_open(a.get(),&reader,nullptr,read,nullptr)!=ARCHIVE_OK)return false;
  archive_entry* entry=nullptr;int status;size_t entries=0,total=0;QByteArray manifest;
  while((status=archive_read_next_header(a.get(),&entry))==ARCHIVE_OK){
    if(++entries>128)return false;bool isManifest=QString::fromUtf8(archive_entry_pathname(entry))=="manifest.json";
    char block[65536];la_ssize_t n;
    while((n=archive_read_data(a.get(),block,sizeof(block)))>0){total+=size_t(n);if(total>1024ull*1024*1024)return false;if(isManifest){if(manifest.size()+n>1048576)return false;manifest.append(block,qsizetype(n));}}
    if(n<0)return false;
  }
  auto document=QJsonDocument::fromJson(manifest);auto root=document.object();
  return status==ARCHIVE_EOF&&root["format"]=="ZahnCAD"&&(root["version"].toInt(-1)==1||root["version"].toInt(-1)==2)&&root["scans"].isArray();
}
Mesh parse(const QByteArray& b, const QString& format,const Checkpoint& checkpoint) {QTemporaryFile f;check(f.open(),"Temporary surface failed");for(qsizetype offset=0;offset<b.size();offset+=1048576){if(checkpoint)checkpoint();auto n=std::min(qsizetype(1048576),b.size()-offset);check(f.write(b.constData()+offset,n)==n,"Temporary surface write failed");}check(f.flush(),"Temporary surface flush failed");return readSurface(nativePath(f.fileName()),format.toStdString(),checkpoint);}
QByteArray encodeMesh(const Mesh& mesh){
  check(mesh.triangles.size()>0&&mesh.triangles.size()<=10000000,"Invalid current geometry");bool colored=!mesh.colors.empty();check(!colored||mesh.colors.size()==mesh.triangles.size(),"Invalid mesh colors");
  check(14ull+mesh.triangles.size()*(colored?81ull:72ull)<=512ull*1024*1024,"Derived mesh exceeds entry limit");
  QByteArray bytes;QDataStream out(&bytes,QIODevice::WriteOnly);out.setByteOrder(QDataStream::LittleEndian);out.setFloatingPointPrecision(QDataStream::DoublePrecision);out.writeRawData("ZMSH1",5);out<<quint64(mesh.triangles.size())<<quint8(colored);
  for(size_t i=0;i<mesh.triangles.size();++i){for(auto p:mesh.triangles[i].v)out<<p.x<<p.y<<p.z;if(colored)for(auto color:mesh.colors[i])for(auto component:color)out<<quint8(component);}check(out.status()==QDataStream::Ok,"Current geometry serialization failed");return bytes;
}
Mesh decodeMesh(const QByteArray& bytes,const Checkpoint& checkpoint){
  check(bytes.startsWith("ZMSH1"),"Unsupported mesh encoding");QDataStream in(bytes);in.setByteOrder(QDataStream::LittleEndian);in.setFloatingPointPrecision(QDataStream::DoublePrecision);char magic[5];in.readRawData(magic,5);quint64 count=0;quint8 colored=0;in>>count>>colored;
  check(count>0&&count<=10000000&&colored<=1&&14ull+count*(colored?81ull:72ull)==quint64(bytes.size()),"Invalid derived mesh size");Mesh mesh;mesh.triangles.resize(size_t(count));if(colored)mesh.colors.resize(size_t(count));
  for(size_t i=0;i<count;++i){if(checkpoint&&i%1024==0)checkpoint();for(auto& p:mesh.triangles[i].v){in>>p.x>>p.y>>p.z;check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})<=1e8,"Invalid derived mesh coordinate");}if(colored){char colorBytes[9]{};check(in.readRawData(colorBytes,9)==9,"Truncated mesh colors");for(size_t corner=0;corner<3;++corner)for(size_t channel=0;channel<3;++channel)mesh.colors.at(i).at(corner).at(channel)=static_cast<unsigned char>(colorBytes[corner*3+channel]);}}
  check(in.status()==QDataStream::Ok,"Truncated derived mesh");return mesh;
}
}
std::filesystem::path nativePath(const QString& value) {
#ifdef _WIN32
  return std::filesystem::path(value.toStdWString());
#else
  return std::filesystem::path(value.toUtf8().constData());
#endif
}
Scan importScan(const QString& path,const QString& role,double scale,const Checkpoint& checkpoint) {
  QFile file(path);check(file.open(QIODevice::ReadOnly),"Cannot open scan");check(file.size()<=500000084,"Scan exceeds import limit");
  Scan scan;scan.name=QFileInfo(file).fileName();scan.role=role;scan.scale=scale;scan.format=QFileInfo(file).suffix().toLower();check(scan.format=="stl"||scan.format=="ply"||scan.format=="obj","Supported scan formats: STL, PLY, OBJ");
  while(!file.atEnd()){if(checkpoint)checkpoint();auto chunk=file.read(1048576);check(!chunk.isEmpty(),"Cannot read scan");scan.original+=chunk;}
  scan.mesh=std::make_shared<const Mesh>(scaled(parse(scan.original,scan.format,checkpoint),scale,checkpoint));scan.sourceMesh=scan.mesh;return scan;
}
void saveProject(const Project& p,const QString& path) {
  check(p.scans.size()<64,"Too many scans for this project format");
  quint64 total=0;for(auto& scan:p.scans){check(scan.original.size()<=512ll*1024*1024,"Scan exceeds project entry limit");total+=quint64(scan.original.size());if(scan.sourceMesh&&scan.mesh!=scan.sourceMesh){check(bool(scan.mesh),"Missing current geometry");total+=14+scan.mesh->triangles.size()*(scan.mesh->colors.empty()?72ull:81ull);}}
  check(total<=1000ull*1024*1024,"Project exceeds safe expanded-size limit");
  QSaveFile file(path);check(file.open(QIODevice::WriteOnly),"Cannot save project");
  std::unique_ptr<archive,decltype(&archive_write_free)> a(archive_write_new(),archive_write_free);
  check(archive_write_set_format_zip(a.get())==ARCHIVE_OK,"ZIP setup failed");
  auto writer=[](archive*,void* data,const void* b,size_t n)->la_ssize_t{return static_cast<QSaveFile*>(data)->write(static_cast<const char*>(b),qint64(n));};
  check(archive_write_open(a.get(),&file,nullptr,writer,nullptr)==ARCHIVE_OK,"ZIP open failed");
  auto add=[&](const QString& name,const QByteArray& bytes){auto* e=archive_entry_new();auto encoded=name.toUtf8();archive_entry_set_pathname(e,encoded.constData());archive_entry_set_size(e,bytes.size());archive_entry_set_filetype(e,AE_IFREG);archive_entry_set_perm(e,0600);int status=archive_write_header(a.get(),e);archive_entry_free(e);check(status==ARCHIVE_OK,"ZIP header failed");check(archive_write_data(a.get(),bytes.data(),bytes.size())==bytes.size(),"ZIP write failed");};
  QJsonArray scans;for(size_t i=0;i<p.scans.size();++i){auto& s=p.scans[i];QString entry=QString("sources/%1.%2").arg(i).arg(s.format);QJsonObject item{{"name",s.name},{"role",s.role},{"source",entry},{"sourceFormat",s.format},{"scaleToMm",s.scale},{"visible",s.visible},{"opacity",s.opacity},{"operations",s.operations}};add(entry,s.original);if(s.sourceMesh&&s.mesh!=s.sourceMesh){QString current=QString("meshes/%1.zmesh").arg(i);add(current,encodeMesh(*s.mesh));item["current"]=current;}scans.append(item);}
  add("manifest.json",QJsonDocument(QJsonObject{{"format","ZahnCAD"},{"version",2},{"caseId",p.id},{"units","mm"},{"scans",scans}}).toJson());
  check(archive_write_close(a.get())==ARCHIVE_OK,"ZIP close failed");check(file.commit(),"Atomic project commit failed");
}
Project loadProject(const QString& path,const Checkpoint& checkpoint) {
  if(checkpoint)checkpoint();
  QFile file(path);check(file.open(QIODevice::ReadOnly),"Cannot open project");check(file.size()<=1024ll*1024*1024,"Project exceeds 1 GiB limit");QByteArray bytes;
  while(!file.atEnd()){if(checkpoint)checkpoint();auto chunk=file.read(1048576);check(!chunk.isEmpty(),"Cannot read project");bytes+=chunk;}
  std::unique_ptr<archive,decltype(&archive_read_free)> a(archive_read_new(),archive_read_free);archive_read_support_format_zip(a.get());check(archive_read_open_memory(a.get(),bytes.data(),bytes.size())==ARCHIVE_OK,"Invalid ZIP");
  std::map<QString,QByteArray> entries;archive_entry* e=nullptr;int status;size_t total=0;
  while((status=archive_read_next_header(a.get(),&e))==ARCHIVE_OK){if(checkpoint)checkpoint();auto n=archive_entry_size(e);check(n>=0&&n<=512ll*1024*1024&&total+size_t(n)<=1024ull*1024*1024,"Project decompression limit exceeded");total+=size_t(n);check(entries.size()<128,"Too many archive entries");QString name=QString::fromUtf8(archive_entry_pathname(e));check(!entries.contains(name),"Duplicate archive entry");QByteArray b; b.reserve(qsizetype(n));char block[65536];for(;;){if(checkpoint)checkpoint();auto count=archive_read_data(a.get(),block,sizeof(block));check(count>=0,"Corrupt ZIP entry");if(!count)break;check(b.size()+count<=n,"Invalid ZIP entry size");b.append(block,qsizetype(count));}check(b.size()==n,"Truncated ZIP entry");entries.emplace(name,std::move(b));}
  check(status==ARCHIVE_EOF,"Corrupt ZIP");QJsonParseError error;auto doc=QJsonDocument::fromJson(entries["manifest.json"],&error);check(error.error==QJsonParseError::NoError&&doc.isObject(),"Invalid manifest");auto root=doc.object();int version=root["version"].toInt(-1);check(root["format"]=="ZahnCAD"&&(version==1||version==2),"Unsupported project format/version");check(root["units"]=="mm"&&root["scans"].isArray(),"Invalid project metadata");
  Project p;p.id=root["caseId"].toString();for(auto value:root["scans"].toArray()){if(checkpoint)checkpoint();auto o=value.toObject();Scan s;s.name=o["name"].toString();s.role=o["role"].toString();s.scale=o["scaleToMm"].toDouble(0);s.opacity=o["opacity"].toDouble(-1);s.visible=o["visible"].toBool();s.format=version==1?"stl":o["sourceFormat"].toString();check(s.format=="stl"||s.format=="ply"||s.format=="obj","Unsupported source format");check(s.opacity>=0&&s.opacity<=1,"Invalid opacity");auto it=entries.find(o["source"].toString());check(it!=entries.end(),"Missing scan");s.original=it->second;s.sourceMesh=std::make_shared<const Mesh>(scaled(parse(s.original,s.format,checkpoint),s.scale,checkpoint));s.mesh=s.sourceMesh;s.operations=o["operations"].toArray();if(o.contains("current")){auto current=entries.find(o["current"].toString());check(current!=entries.end(),"Missing current geometry");s.mesh=std::make_shared<const Mesh>(decodeMesh(current->second,checkpoint));}p.scans.push_back(std::move(s));}return p;
}
void saveRecovery(const Project& project,const QString& directory){
  auto current=QDir(directory).filePath("recovery.zahncad");
  if(validRecoveryArchive(current)) {QFile previous(current);check(previous.open(QIODevice::ReadOnly),"Cannot read recovery backup");QSaveFile backup(QDir(directory).filePath("recovery.previous.zahncad"));check(backup.open(QIODevice::WriteOnly),"Cannot create recovery backup");while(!previous.atEnd()){auto bytes=previous.read(1048576);check(!bytes.isEmpty()&&backup.write(bytes)==bytes.size(),"Recovery backup failed");}check(backup.commit(),"Recovery backup commit failed");}
  saveProject(project,current);
}
Project loadRecovery(const QString& directory,const Checkpoint& checkpoint){
  QString errors;
  for(auto name:{"recovery.zahncad","recovery.previous.zahncad"}){if(checkpoint)checkpoint();try{return loadProject(QDir(directory).filePath(name),checkpoint);}catch(const std::exception& e){if(checkpoint)checkpoint();errors+=QString::fromUtf8(e.what())+"; ";}}
  throw std::runtime_error(("No readable recovery: "+errors).toStdString());
}
}
