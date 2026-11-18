#include "zahn/display.hpp"
#include <QCoreApplication>
#include <chrono>
#include <iostream>
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
  if(argc!=2){std::cerr<<"Usage: viewer_benchmark input.stl\n";return 1;}
  auto start=std::chrono::steady_clock::now();auto scan=zahn::importScan(QString::fromLocal8Bit(argv[1]),"Reference",1);
  auto loaded=std::chrono::steady_clock::now();auto display=zahn::prepareDisplay(scan.mesh);
  auto end=std::chrono::steady_clock::now();
  auto seconds=[](auto a,auto b){return std::chrono::duration<double>(b-a).count();};
  std::cout<<"triangles="<<scan.mesh->triangles.size()<<" import_seconds="<<seconds(start,loaded)
    <<" display_prepare_seconds="<<seconds(loaded,end)<<" total_seconds="<<seconds(start,end)
    <<" navigation_triangles="<<display.navigation->GetNumberOfCells()<<"\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
