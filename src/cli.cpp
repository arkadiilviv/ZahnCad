#include "zahn/mesh.hpp"
#include <iostream>
int main(int argc,char** argv) {
  try {
    if(argc<3) {std::cerr<<"ZahnCAD 0.3 research tools\nUsage: zahncad-cli inspect input.stl\n       zahncad-cli convert input.stl output.stl scale-to-mm\n";return 1;}
    auto m=zahn::readStl(argv[2]);std::string cmd=argv[1];
    if(cmd=="inspect"&&argc==3) {std::cout<<zahn::inspect(m).text();return 0;}
    if(cmd=="convert"&&argc==5) {size_t pos;double f=std::stod(argv[4],&pos);if(pos!=std::string(argv[4]).size())throw std::runtime_error("Invalid scale");if(std::filesystem::absolute(argv[2])==std::filesystem::absolute(argv[3]))throw std::runtime_error("Choose a distinct output path");zahn::writeStl(zahn::scaled(m,f),argv[3]);return 0;}
    throw std::runtime_error("Unknown command or wrong arguments");
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
