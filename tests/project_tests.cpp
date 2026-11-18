#include "zahn/document.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QLockFile>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
  QTemporaryDir dir;zahn::Project p;p.id="TEST-001";zahn::Scan s;s.name="Upper scan";s.role="Upper jaw";s.scale=2;
  s.original="solid test\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid test\n";p.scans.push_back(s);
  {QFile source(dir.filePath(QString::fromUtf8("scan-ä.stl")));if(!source.open(QIODevice::WriteOnly)||source.write(s.original)!=s.original.size())throw std::runtime_error("Fixture write failed");source.close();auto imported=zahn::importScan(source.fileName(),"Upper jaw",2);if(imported.original!=s.original||zahn::inspect(*imported.mesh).maximum.x!=2)throw std::runtime_error("Unicode scan import failed");}
  {QLockFile one(dir.filePath("session.lock")),two(dir.filePath("session.lock"));if(!one.tryLock()||two.tryLock())throw std::runtime_error("Session lock failed");}
  auto path=dir.filePath("case.zahncad");zahn::saveProject(p,path);auto loaded=zahn::loadProject(path);
  if(loaded.id!=p.id||loaded.scans.size()!=1||loaded.scans[0].original!=s.original||zahn::inspect(*loaded.scans[0].mesh).maximum.x!=2)throw std::runtime_error("Project roundtrip failed");
  zahn::Document document;document.reset(loaded);auto geometry=document.project.scans[0].mesh;
  document.setDisplay(0,false,0.3);if(!document.dirty())throw std::runtime_error("Edit not dirty");
  document.history.undo();if(document.dirty()||!document.project.scans[0].visible)throw std::runtime_error("Undo did not restore clean state");
  document.history.redo();document.markSaved();document.history.undo();if(!document.dirty())throw std::runtime_error("Saved-index tracking failed");
  document.history.redo();if(document.dirty())throw std::runtime_error("Redo clean tracking failed");
  document.removeScan(0);document.history.undo();if(document.project.scans[0].mesh!=geometry)throw std::runtime_error("History copied geometry");
  document.addScan(loaded.scans[0]);document.history.undo();document.history.redo();if(document.project.scans.size()!=2)throw std::runtime_error("Import redo failed");
  document.reset(loaded,true);if(!document.dirty())throw std::runtime_error("Recovery must require saving");document.markSaved();if(document.dirty())throw std::runtime_error("Recovered save state failed");
  zahn::saveRecovery(loaded,dir.path());loaded.id="NEWER";zahn::saveRecovery(loaded,dir.path());
  if(zahn::loadRecovery(dir.path()).id!="NEWER")throw std::runtime_error("Latest recovery failed");
  {QFile corrupt(dir.filePath("recovery.zahncad"));if(!corrupt.open(QIODevice::WriteOnly))throw std::runtime_error("Fixture failed");corrupt.write("broken");}
  if(zahn::loadRecovery(dir.path()).id!="TEST-001")throw std::runtime_error("Recovery fallback failed");
  zahn::saveRecovery(loaded,dir.path());
  {QFile corrupt(dir.filePath("recovery.zahncad"));if(!corrupt.open(QIODevice::WriteOnly))throw std::runtime_error("Fixture failed");corrupt.write("broken again");}
  if(zahn::loadRecovery(dir.path()).id!="TEST-001")throw std::runtime_error("Corrupt latest overwrote valid backup");
  bool cancelled=false;try{zahn::loadProject(path,[]{throw std::runtime_error("cancel");});}catch(...){cancelled=true;}if(!cancelled)throw std::runtime_error("Load ignored cancellation");
  p.id="REPLACED";zahn::saveProject(p,path);if(zahn::loadProject(path).id!=p.id)throw std::runtime_error("Replacement failed");
  QFile file(path);if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("Fixture open failed");file.write("not a project");file.close();bool rejected=false;try{zahn::loadProject(path);}catch(...){rejected=true;}if(!rejected)throw std::runtime_error("Corrupt project accepted");
  std::cout<<"Project roundtrip/replacement/corruption checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
