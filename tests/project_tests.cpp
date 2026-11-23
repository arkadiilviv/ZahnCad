#include "zahn/document.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QLockFile>
#include <iostream>

namespace {
void checkScanLimit(const zahn::Scan& scan,const QTemporaryDir& dir) {
  zahn::Document document;
  for(size_t i=0;i<zahn::Project::scanLimit;++i)document.addScan(scan);
  const auto historyIndex=document.history.index();
  bool rejected=false;
  try { document.addScan(scan); }
  catch(const std::runtime_error&) { rejected=true; }
  if(!rejected||document.history.index()!=historyIndex||
     document.project.scans.size()!=zahn::Project::scanLimit)
    throw std::runtime_error("Excess scan changed the case");
  const auto path=dir.filePath("full.zahncad");
  zahn::saveProject(document.project,path);
  if(zahn::loadProject(path).scans.size()!=zahn::Project::scanLimit)
    throw std::runtime_error("Full case did not reopen");
  document.removeScan(0);
  document.addScan(scan);
}

void checkToothRemoval(zahn::Scan scan,const QTemporaryDir& dir) {
  scan.toothFdi=16;
  scan.toothMargins[16]={{0,0,0},{1,0,0},{0,1,0}};
  scan.toothMargins.try_emplace(17);
  scan.insertionAxes={{16,{0,0,1}},{17,{1,0,0}}};
  zahn::Document document;
  document.addScan(scan);

  scan.removeToothMarkup(16);
  document.replaceScan(0,scan,"Remove tooth markup");
  document.history.undo();
  const auto& restored=document.project.scans.front();
  if(restored.toothFdi!=16||restored.toothMargins.at(16).size()!=3||
     !restored.insertionAxes.contains(16))
    throw std::runtime_error("Undo did not restore tooth markup and its insertion axis");
  document.history.redo();

  const auto path=dir.filePath("removed-tooth.zahncad");
  zahn::saveProject(document.project,path);
  const auto reopened=zahn::loadProject(path);
  const auto& remaining=reopened.scans.front();
  if(remaining.toothFdi!=17||remaining.toothMargins.size()!=1||
     remaining.insertionAxes.size()!=1||remaining.insertionAxes.at(17).x!=1)
    throw std::runtime_error("Saved case did not preserve only the remaining tooth and axis");
}

void checkRecoveryRotation(zahn::Project project) {
  QTemporaryDir dir;
  const auto latest=dir.filePath("recovery.zahncad");
  const auto previous=dir.filePath("recovery.previous.zahncad");
  project.id="Previous";
  zahn::saveRecovery(project,dir.path());
  project.id="Latest";
  zahn::saveRecovery(project,dir.path());

  auto invalid=project;
  invalid.scans.front().insertionAxes[16]={0,0,1};
  bool rejected=false;
  try { zahn::saveRecovery(invalid,dir.path()); }
  catch(const std::runtime_error&) { rejected=true; }
  if(!rejected||zahn::loadProject(latest).id!="Latest"||zahn::loadProject(previous).id!="Previous")
    throw std::runtime_error("Failed autosave changed an existing snapshot");

  // The writer produces a valid ZIP, but reopening its embedded scan fails.
  auto unreadable=project;
  unreadable.scans.front().original="Unparseable STL";
  zahn::saveProject(unreadable,latest);
  if(zahn::loadRecovery(dir.path()).id!="Previous")
    throw std::runtime_error("Unreadable source did not trigger recovery fallback");
  project.id="Replacement";
  zahn::saveRecovery(project,dir.path());
  if(zahn::loadProject(latest).id!="Replacement"||zahn::loadProject(previous).id!="Previous")
    throw std::runtime_error("Unreadable project overwrote the readable backup");
}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
  QTemporaryDir dir;zahn::Project p;p.id="TEST-001";zahn::Scan s;s.name="Upper scan";s.role="Upper jaw";s.scale=2;
  s.original="solid test\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid test\n";p.scans.push_back(s);
  {QFile source(dir.filePath(QString::fromUtf8("scan-ä.stl")));if(!source.open(QIODevice::WriteOnly)||source.write(s.original)!=s.original.size())throw std::runtime_error("Fixture write failed");source.close();auto imported=zahn::importScan(source.fileName(),"Upper jaw",2);if(imported.original!=s.original||zahn::inspect(*imported.mesh).maximum.x!=2)throw std::runtime_error("Unicode scan import failed");}
  {QLockFile one(dir.filePath("session.lock")),two(dir.filePath("session.lock"));if(!one.tryLock()||two.tryLock())throw std::runtime_error("Session lock failed");}
  auto path=dir.filePath("case.zahncad");zahn::saveProject(p,path);auto loaded=zahn::loadProject(path);
  if(loaded.id!=p.id||loaded.scans.size()!=1||loaded.scans[0].original!=s.original||zahn::inspect(*loaded.scans[0].mesh).maximum.x!=2)throw std::runtime_error("Project roundtrip failed");
  checkScanLimit(loaded.scans.front(),dir);
  checkToothRemoval(loaded.scans.front(),dir);
  checkRecoveryRotation(loaded);
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
