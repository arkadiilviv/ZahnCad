#include "zahn/display.hpp"
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
  auto mesh=std::make_shared<zahn::Mesh>();
  for(int x=0;x<350;++x)for(int y=0;y<350;++y){
    zahn::Vec3 a{double(x),double(y),0},b{double(x+1),double(y),0},c{double(x),double(y+1),0},d{double(x+1),double(y+1),0};
    mesh->triangles.push_back({{a,b,c}});mesh->triangles.push_back({{b,d,c}});
  }
  QFutureWatcher<zahn::DisplayMesh> watcher;int ticks=0;QTimer timer;
  QObject::connect(&timer,&QTimer::timeout,[&]{++ticks;});timer.start(1);
  int result=1;
  QObject::connect(&watcher,&QFutureWatcher<zahn::DisplayMesh>::finished,[&]{try{
    auto display=watcher.result();
    if(display.full->GetNumberOfCells()!=245000||display.navigation->GetNumberOfCells()>=245000||!ticks)
      throw std::runtime_error("LOD or background responsiveness failed");
    if(display.source!=mesh||mesh->triangles.size()!=245000||mesh->triangles.back().v[2].x!=349)
      throw std::runtime_error("Display modified source geometry");
    bool cancelled=false;int checkpoints=0;
    try{zahn::prepareDisplay(mesh,[&]{if(++checkpoints==3)throw std::runtime_error("cancel");});}catch(...){cancelled=true;}
    if(!cancelled)throw std::runtime_error("Display ignored cancellation");
    result=0;std::cout<<"Background responsiveness, LOD and cancellation passed\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';}app.quit();});
  watcher.setFuture(QtConcurrent::run([mesh]{return zahn::prepareDisplay(mesh);}));app.exec();return result;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
