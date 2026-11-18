#include "zahn/mesh.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
void require(bool ok){if(!ok)throw std::runtime_error("Test assertion failed");}
template<class F> void rejects(F f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}require(threw);}
int main(){try{
  auto dir=std::filesystem::temp_directory_path()/"zahncad-core-tests";std::filesystem::create_directories(dir);auto file=dir/"mesh.stl";
  zahn::Vec3 a{0,0,0},b{1,0,0},c{0,1,0},d{0,0,1};zahn::Mesh tetra{{{{a,c,b}},{{a,b,d}},{{b,c,d}},{{c,a,d}}}};
  auto report=zahn::inspect(tetra);require(report.closed());zahn::writeStl(tetra,file);auto loaded=zahn::readStl(file);require(zahn::inspect(loaded).closed());require(loaded.triangles.size()==4);
  auto large=zahn::scaled(loaded,25.4);require(std::abs(zahn::inspect(large).maximum.x-25.4)<1e-10);rejects([&]{zahn::scaled(loaded,0);});rejects([&]{zahn::scaled(loaded,NAN);});
  auto open=tetra;open.triangles.pop_back();require(zahn::inspect(open).boundaryEdges==3);auto flipped=tetra;std::swap(flipped.triangles[0].v[0],flipped.triangles[0].v[1]);require(zahn::inspect(flipped).inconsistentEdges==3);
  auto nonmanifold=tetra;nonmanifold.triangles.push_back(tetra.triangles[0]);require(zahn::inspect(nonmanifold).nonManifoldEdges==3);
  zahn::Mesh flat{{{{a,a,b}}}};require(zahn::inspect(flat).degenerate==1);
  {std::ofstream out(file);out<<"solid test\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid test\n";}require(zahn::readStl(file).triangles.size()==1);
  {std::ofstream out(file);out<<"solid test\nfacet normal 0 0 1\n";}rejects([&]{zahn::readStl(file);});
  zahn::writeStl(tetra,file);std::filesystem::resize_file(file,100);rejects([&]{zahn::readStl(file);});
  {std::ofstream out(file);out<<"solid test\nendsolid test\n";}rejects([&]{zahn::readStl(file);});
  zahn::Mesh many;for(int i=0;i<2000;++i)many.triangles.insert(many.triangles.end(),tetra.triangles.begin(),tetra.triangles.end());zahn::writeStl(many,file);
  int checkpoints=0;rejects([&]{zahn::readStl(file,[&]{if(++checkpoints==3)throw std::runtime_error("cancel");});});
  checkpoints=0;rejects([&]{zahn::scaled(many,2,[&]{if(++checkpoints==3)throw std::runtime_error("cancel");});});
  checkpoints=0;rejects([&]{zahn::inspect(many,[&]{if(++checkpoints==3)throw std::runtime_error("cancel");});});
  std::filesystem::remove(file);std::filesystem::remove(dir);std::cout<<"All core checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
