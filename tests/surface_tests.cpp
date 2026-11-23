#include "zahn/surface.hpp"
#include "zahn/preparation.hpp"
#include "zahn/document.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonObject>
#include <QDataStream>
#include <archive.h>
#include <archive_entry.h>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <iostream>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void checkResearchCrown(zahn::Scan preparation) {
  std::vector<zahn::Vec3> outline{{0,0,0},{4,0,1},{4,4,0},{0,4,0}};
  auto mesh=std::make_shared<const zahn::Mesh>(zahn::researchCrown(outline,{0,0,1},5));
  auto report=zahn::inspect(*mesh);
  require(report.closed()&&report.triangles==32&&report.maximum.z==6,"Crown topology and height");
  std::reverse(outline.begin(),outline.end());
  require(zahn::inspect(zahn::researchCrown(outline,{0,0,1},5)).closed(),"Reversed crown margin rejected");
  auto crosses=outline;
  std::swap(crosses[1],crosses[2]);
  for(const auto& invalid:{crosses,std::vector<zahn::Vec3>{{0,0,0},{4,0,0},{1,1,0},{4,4,0},{0,4,0}}}){
    bool rejected=false;
    try{zahn::researchCrown(invalid,{0,0,1},5);}catch(const std::runtime_error&){rejected=true;}
    require(rejected,"Crossing or concave crown margin accepted");
  }

  preparation.toothFdi=16;
  preparation.toothMargins[16]=outline;
  zahn::Scan crown;
  crown.toothFdi=16;
  crown.toothMargins.try_emplace(16);
  crown.format="zmesh";
  crown.role="Restoration candidate";
  crown.sourceMesh=mesh;
  zahn::RigidAlignment move;
  move.matrix={1,0,0,1,0,1,0,0,0,0,1,0,0,0,0,1};
  crown.mesh=std::make_shared<const zahn::Mesh>(zahn::transformMesh(*mesh,move));
  QTemporaryDir recovery;
  zahn::Project project{"Crown",{crown,preparation}};
  zahn::saveRecovery(project,recovery.path());
  zahn::saveRecovery({"Second",{preparation}},recovery.path());
  QFile damaged(recovery.filePath("recovery.zahncad"));
  require(damaged.open(QIODevice::WriteOnly)&&damaged.write("broken")==6,"Damage recovery fixture");
  damaged.close();
  auto reopened=zahn::loadRecovery(recovery.path());
  const auto& restored=reopened.scans.at(0);
  require(reopened.id=="Crown"&&restored.toothFdi==16&&reopened.scans.at(1).toothMargins.at(16).size()==4&&
    zahn::inspect(*restored.mesh).minimum.x==1&&zahn::inspect(*restored.sourceMesh).minimum.x==0,
    "Version 4 mixed-case recovery lost markup or generated meshes");

  auto adjacent=outline;
  for(auto& point:adjacent)point.x+=5;
  preparation.toothMargins[17]=adjacent;
  preparation.toothMargins.try_emplace(18);
  preparation.toothFdi=17;
  project.scans={preparation,crown}; // Single-tooth scan follows the version 6 scan.
  project.reference=zahn::ReferenceMeasurement{"A and B","Synthetic geometry",{},5,0.05};
  const auto file=recovery.filePath("multi-tooth.zahncad");
  zahn::saveProject(project,file);
  const auto multi=zahn::loadProject(file);
  const auto& arch=multi.scans.at(0);
  require(multi.reference.has_value()&&arch.toothFdi==17&&arch.toothMargins.size()==3&&
    arch.toothMargins.at(16).front().x==outline.front().x&&
    arch.toothMargins.at(17).front().x==adjacent.front().x&&arch.toothMargins.at(18).empty(),
    "Multi-tooth project lost a margin, an empty assignment, or its active tooth");
}
void checkSampledSeparation() {
  zahn::Mesh large,small;
  large.triangles.push_back({{{{0,0,0},{12,0,0},{0,12,0}}}});
  small.triangles.push_back({{{{1,1,2},{2,1,2},{1,2,2}}}});
  // The closest sampled point must come from the reverse pass.
  const auto report=zahn::sampledSeparation(large,small);
  require(std::abs(report.minimum-2)<1e-9&&report.samples==8,"Sampled separation distance and count");
  require(report.onFirst.z==0&&report.onSecond.z==2&&
          std::hypot(report.onFirst.x-report.onSecond.x,report.onFirst.y-report.onSecond.y)<1e-9,
          "Sampled separation endpoints belong to the reported surfaces");
  require(zahn::sampledSeparation(large,large).minimum==0,"Coincident surface separation");
  bool rejected=false;
  try{zahn::sampledSeparation(large,{});}
  catch(const std::runtime_error&){rejected=true;}
  require(rejected,"Sampled separation accepted an empty surface");
}
void checkPreparation(zahn::Scan scan,const QTemporaryDir& directory) {
  const zahn::LandmarkTriplet moving{{{0,0,0},{2,0,0},{0,2,0}}};
  const zahn::LandmarkTriplet fixed{{{5,6,7},{5,8,7},{3,6,7}}};
  const auto fit=zahn::alignLandmarks(moving,fixed);
  require(fit.rms<1e-9&&fit.maximum<1e-9,"Rigid registration residual");
  const auto moved=zahn::transformMesh(*scan.mesh,fit);
  const auto point=moved.triangles[0].v[1];
  require(std::abs(point.x-5)<1e-9&&std::abs(point.y-8)<1e-9&&std::abs(point.z-7)<1e-9,
          "Rigid registration transform");
  require(moved.colors==scan.mesh->colors&&scan.mesh==scan.sourceMesh,"Registration lost original or colors");
  bool rejected=false;
  try{zahn::alignLandmarks({{{0,0,0},{1,0,0},{2,0,0}}},fixed);}
  catch(const std::runtime_error&){rejected=true;}
  require(rejected,"Collinear landmarks accepted");

  auto stacked=*scan.mesh;
  auto top=stacked.triangles.front();
  for(auto& p:top.v)p.z+=1;
  stacked.triangles.push_back(top);
  stacked.colors.clear();
  const auto report=zahn::analyzeInsertion(stacked,{0,0,1});
  require(report.sampled==2&&report.undercuts==1&&std::abs(report.maximumDepth-1)<1e-9,
          "Insertion shadow depth");
  require(zahn::analyzeInsertion(stacked,{0,0,-1}).shadowed.triangles[0].v[0].z==1,
          "Reversed insertion direction");
  const auto blocked=zahn::inspect(zahn::blockout(stacked,{0,0,1},0.25));
  require(blocked.closed()&&std::abs(blocked.maximum.z-1)<0.001&&
          std::abs(blocked.minimum.z+0.25)<0.001,"Closed blockout bounds");
  rejected=false;
  try{
    zahn::blockout(stacked,{0,0,1},0.25,[]{throw std::runtime_error("cancel");});
  }catch(const std::runtime_error&){rejected=true;}
  require(rejected,"Blockout ignored cancellation");

  zahn::Document document;
  document.addScan(scan);
  scan.toothMargins[0].assign(fixed.begin(),fixed.end());
  scan.mesh=std::make_shared<const zahn::Mesh>(moved);
  document.replaceScan(0,scan,"Trace and register");
  document.history.undo();
  require(!document.project.scans[0].hasMargins(),"Margin undo failed");
  document.history.redo();
  const auto path=directory.filePath("margin.zahncad");
  zahn::saveProject(document.project,path);
  const auto reopened=zahn::loadProject(path);
  const auto& saved=reopened.scans[0];
  require(saved.toothMargins.at(0).size()==3&&saved.toothMargins.at(0)[1].y==8&&saved.original==scan.original&&
          saved.mesh!=saved.sourceMesh,"Version 3 margin and geometry roundtrip");
  zahn::saveRecovery(document.project,directory.path());
  zahn::saveRecovery(document.project,directory.path());
  QFile corrupt(directory.filePath("recovery.zahncad"));
  require(corrupt.open(QIODevice::WriteOnly)&&corrupt.write("broken")==6,"Corruption fixture write");
  corrupt.close();
  require(zahn::loadRecovery(directory.path()).scans[0].toothMargins.at(0).size()==3,"Version 3 recovery fallback");
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
  if(argc==2){auto external=zahn::loadProject(QString::fromLocal8Bit(argv[1]));require(external.scans.size()==1,"Interop scan count");auto& scan=external.scans[0];require(scan.sourceMesh!=scan.mesh&&!scan.operations.isEmpty(),"Interop derived state");require(!scan.mesh->colors.empty()&&scan.mesh->colors.size()==scan.mesh->triangles.size(),"Interop colors");require(zahn::inspect(*scan.mesh).minimum.x>zahn::inspect(*scan.sourceMesh).minimum.x,"Interop crop bounds");}
  QTemporaryDir directory;auto write=[&](QString name,QByteArray data){QFile file(directory.filePath(name));require(file.open(QIODevice::WriteOnly),"fixture open");require(file.write(data)==data.size(),"fixture write");return file.fileName();};
  auto obj=write("colored.obj","v 0 0 0 1 0 0\nv 2 0 0 0 1 0\nv 0 2 0 0 0 1\nf -3 -2 -1\n");
  auto scan=zahn::importScan(obj,"Upper jaw",1);require(scan.mesh->colors.size()==1&&scan.mesh->colors[0][0][0]==255,"OBJ color import");
  checkPreparation(scan,directory);
  checkSampledSeparation();
  checkResearchCrown(scan);
  auto legacy=[&](int version){QByteArray buffer(8192,'\0');size_t used=0;auto* writer=archive_write_new();archive_write_set_format_zip(writer);require(archive_write_open_memory(writer,buffer.data(),buffer.size(),&used)==ARCHIVE_OK,"Legacy fixture ZIP");
    auto entry=[&](const char* name,QByteArray bytes){auto* item=archive_entry_new();archive_entry_set_pathname(item,name);archive_entry_set_size(item,bytes.size());archive_entry_set_filetype(item,AE_IFREG);archive_entry_set_perm(item,0600);require(archive_write_header(writer,item)==ARCHIVE_OK,"Legacy fixture header");archive_entry_free(item);require(archive_write_data(writer,bytes.data(),bytes.size())==bytes.size(),"Legacy fixture data");};
    entry("scan.stl","solid test\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid test\n");entry("manifest.json",QByteArray("{\"format\":\"ZahnCAD\",\"version\":")+QByteArray::number(version)+",\"units\":\"mm\",\"caseId\":\"legacy\",\"scans\":[{\"name\":\"Legacy\",\"role\":\"Upper jaw\",\"source\":\"scan.stl\",\"scaleToMm\":2,\"visible\":true,\"opacity\":1}]}" );require(archive_write_close(writer)==ARCHIVE_OK,"Legacy fixture close");archive_write_free(writer);buffer.resize(qsizetype(used));return write("legacy.zip",buffer);};
  auto migrated=zahn::loadProject(legacy(1));require(migrated.id=="legacy"&&migrated.scans[0].mesh==migrated.scans[0].sourceMesh&&zahn::inspect(*migrated.scans[0].mesh).maximum.x==2,"Version 1 migration");bool future=false;try{zahn::loadProject(legacy(99));}catch(...){future=true;}require(future,"Future project version accepted");
  auto ply=write("colored.ply","ply\nformat ascii 1.0\nelement vertex 3\nproperty double x\nproperty double y\nproperty double z\nproperty uchar red\nproperty uchar green\nproperty uchar blue\nelement face 1\nproperty list uchar int vertex_indices\nend_header\n0 0 0 255 0 0\n2 0 0 0 255 0\n0 2 0 0 0 255\n3 0 1 2\n");
  auto colored=zahn::importScan(ply,"Upper jaw",1);require(colored.mesh->colors.size()==1&&colored.mesh->colors[0][1][1]==255,"PLY color import");
  for(auto order:{QDataStream::LittleEndian,QDataStream::BigEndian}){
    QByteArray bytes="ply\nformat "+QByteArray(order==QDataStream::LittleEndian?"binary_little_endian":"binary_big_endian")+" 1.0\nelement vertex 3\nproperty double x\nproperty double y\nproperty double z\nproperty uchar red\nproperty uchar green\nproperty uchar blue\nelement face 1\nproperty list uchar int vertex_indices\nend_header\n";QByteArray body;QDataStream out(&body,QIODevice::WriteOnly);out.setByteOrder(order);out.setFloatingPointPrecision(QDataStream::DoublePrecision);
    out<<double(0.1234567890123)<<double(0)<<double(0)<<quint8(255)<<quint8(0)<<quint8(0);out<<double(2)<<double(0)<<double(0)<<quint8(0)<<quint8(255)<<quint8(0);out<<double(0)<<double(2)<<double(0)<<quint8(0)<<quint8(0)<<quint8(255);out<<quint8(3)<<qint32(0)<<qint32(1)<<qint32(2);bytes+=body;
    auto binary=write("binary.ply",bytes);auto loaded=zahn::importScan(binary,"Reference",1);require(loaded.mesh->triangles[0].v[0].x==0.1234567890123,"PLY double precision");require(loaded.mesh->colors[0][2][2]==255,"Binary PLY colors");bytes.chop(1);write("binary.ply",bytes);bool bad=false;try{zahn::importScan(binary,"Reference",1);}catch(...){bad=true;}require(bad,"Truncated binary PLY accepted");
  }
  bool rejected=false;auto bad=write("invalid.obj","v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 4\n");try{zahn::importScan(bad,"Reference",1);}catch(...){rejected=true;}require(rejected,"Invalid OBJ reference accepted");
  zahn::Parameters crop;crop.operation=zahn::Operation::Crop;crop.box={{0.25,0.25,-1},{0.75,0.75,1}};
  auto cropped=zahn::processSurface(*scan.mesh,crop);auto bounds=zahn::inspect(cropped);require(bounds.minimum.x>=0.25&&bounds.maximum.x<=0.75&&bounds.minimum.y>=0.25&&bounds.maximum.y<=0.75,"Exact crop bounds");require(cropped.colors.size()==cropped.triangles.size(),"Crop colors");require(scan.mesh->triangles.size()==1&&scan.mesh->triangles[0].v[1].x==2,"Original modified");
  auto touching=crop;touching.box={{0,0,-1},{1,1,1}};require(zahn::inspect(zahn::processSurface(*scan.mesh,touching)).degenerate==0,"Crop created zero-area faces at boundary");
  auto same=zahn::deviation(*scan.mesh,*scan.mesh);require(same.maximum<1e-10,"Identity deviation");auto shifted=*scan.mesh;for(auto& t:shifted.triangles)for(auto& p:t.v)p.z+=2;auto delta=zahn::deviation(*scan.mesh,shifted);require(std::abs(delta.maximum-2)<1e-10&&std::abs(delta.rms-2)<1e-10,"Analytic deviation");
  auto edited=scan;edited.mesh=std::make_shared<const zahn::Mesh>(cropped);edited.operations.append(QJsonObject{{"operation","Crop"}});zahn::Document document;document.addScan(scan);document.replaceScan(0,edited,"Crop");document.history.undo();require(document.project.scans[0].mesh==scan.sourceMesh,"Geometry undo");document.history.redo();require(document.project.scans[0].mesh==edited.mesh,"Geometry redo");
  zahn::saveProject(document.project,directory.filePath("edited.zahncad"));auto reopened=zahn::loadProject(directory.filePath("edited.zahncad"));require(reopened.scans[0].original==scan.original&&reopened.scans[0].format=="obj"&&reopened.scans[0].operations.size()==1,"Edited project provenance");require(reopened.scans[0].mesh->triangles[0].v[0].x==cropped.triangles[0].v[0].x&&reopened.scans[0].mesh->colors==cropped.colors,"Derived geometry roundtrip");
  zahn::Vec3 a{0,0,0},b{1,0,0},c{0,1,0},d{0,0,1};zahn::Mesh tetra{{{{a,c,b}},{{a,b,d}},{{b,c,d}},{{c,a,d}}}};auto damaged=tetra;damaged.triangles.push_back(tetra.triangles[0]);damaged.triangles.push_back({{a,a,b}});
  zahn::Parameters parameters;parameters.operation=zahn::Operation::Clean;auto clean=zahn::processSurface(damaged,parameters);require(clean.triangles.size()==4&&zahn::inspect(clean).closed(),"Clean duplicate/degenerate faces");
  auto open=tetra;open.triangles.pop_back();parameters.operation=zahn::Operation::FillHoles;parameters.value=2;require(zahn::processSurface(open,parameters).triangles.size()==4,"Limited hole fill");parameters.value=0.001;require(zahn::processSurface(open,parameters).triangles.size()==3,"Hole radius limit");
  auto separate=tetra;auto small=tetra;for(auto& t:small.triangles)for(auto& p:t.v)p.x+=10;separate.triangles.insert(separate.triangles.end(),small.triangles.begin(),small.triangles.end());parameters.operation=zahn::Operation::LargestComponent;require(zahn::processSurface(separate,parameters).triangles.size()==4,"Component isolation");
  zahn::Parameters vertexSelection;vertexSelection.operation=zahn::Operation::DeleteSelected;vertexSelection.selectedVertices={a};require(zahn::processSurface(separate,vertexSelection).triangles.size()==5,"Vertex deletion must remove incident faces");vertexSelection.operation=zahn::Operation::KeepSelected;vertexSelection.selectedVertices={a,b,c};require(zahn::processSurface(separate,vertexSelection).triangles.size()==1,"Keep must require all three vertices");vertexSelection.operation=zahn::Operation::SelectedComponent;vertexSelection.selectedVertices={{10,0,0},{11,0,0}};require(zahn::inspect(zahn::processSurface(separate,vertexSelection)).minimum.x==10,"Brush component seed");
  parameters.operation=zahn::Operation::SelectedComponent;parameters.selected={4};require(zahn::inspect(zahn::processSurface(separate,parameters)).minimum.x==10,"Selected component");parameters.operation=zahn::Operation::KeepSelected;require(zahn::processSurface(separate,parameters).triangles.size()==1,"Keep selection");parameters.operation=zahn::Operation::DeleteSelected;require(zahn::processSurface(separate,parameters).triangles.size()==7,"Delete selection");
  auto inverted=tetra;std::swap(inverted.triangles[0].v[0],inverted.triangles[0].v[1]);parameters.operation=zahn::Operation::Orient;require(zahn::inspect(zahn::processSurface(inverted,parameters)).inconsistentEdges==0,"Consistent winding");
  zahn::Mesh grid;for(int x=0;x<20;++x)for(int y=0;y<20;++y){zahn::Vec3 a{double(x),double(y),0},b{double(x+1),double(y),0},c{double(x),double(y+1),0},d{double(x+1),double(y+1),0};grid.triangles.push_back({{a,b,c}});grid.triangles.push_back({{b,d,c}});}
  parameters.operation=zahn::Operation::Simplify;parameters.value=0.5;auto reduced=zahn::processSurface(grid,parameters);require(reduced.triangles.size()<grid.triangles.size(),"Simplification did not reduce fixture");require(zahn::inspect(reduced).boundaryEdges==80,"Simplification changed fixed boundary");require(zahn::deviation(grid,reduced).maximum<1e-9,"Planar simplification deviated");
  auto raised=grid;for(auto& triangle:raised.triangles)for(auto& point:triangle.v)if(point.x==10&&point.y==10)point.z=1;
  parameters.operation=zahn::Operation::Smooth;parameters.value=0.1;parameters.iterations=5;auto smooth=zahn::processSurface(raised,parameters);require(zahn::inspect(smooth).maximum.z<1,"Smoothing had no effect");for(auto& triangle:smooth.triangles)for(auto point:triangle.v)if(point.x==0||point.x==20||point.y==0||point.y==20)require(point.z==0,"Smoothing moved boundary");
  bool cancelled=false;try{zahn::processSurface(tetra,parameters,[]{throw std::runtime_error("cancel");});}catch(...){cancelled=true;}require(cancelled,"Operation cancellation");
  std::cout<<"Surface import, color, crop, components, repair, deviation, history and persistence checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
