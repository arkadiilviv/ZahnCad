#include "zahn/mesh.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>
namespace zahn {
namespace {
constexpr size_t limit=10000000;
uint32_t u32(const unsigned char* p) { return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24; }
float f32(const unsigned char* p) { return std::bit_cast<float>(u32(p)); }
void valid(Vec3 p) { if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})>1e8) throw std::runtime_error("Non-finite or excessive coordinate"); }
Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
Vec3 sub(Vec3 a,Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
double norm(Vec3 a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
void token(std::istream& in,const char* expected) { std::string s; if(!(in>>s)||s!=expected) throw std::runtime_error(std::string("Malformed ASCII STL: expected ")+expected); }
void put(std::ostream& out,uint32_t u) { for(int i=0;i<4;++i) out.put(char((u>>(8*i))&255)); }
void putf(std::ostream& out,double d) { put(out,std::bit_cast<uint32_t>(float(d))); }
}
Mesh readStl(const std::filesystem::path& path, const Checkpoint& checkpoint) {
  if(checkpoint) checkpoint();
  std::ifstream in(path,std::ios::binary); if(!in) throw std::runtime_error("Cannot open STL");
  auto size=std::filesystem::file_size(path); if(size>84ull+50ull*limit) throw std::runtime_error("STL exceeds 10 million triangle import limit");
  unsigned char header[84]{}; in.read(reinterpret_cast<char*>(header),84); auto count=u32(header+80);
  Mesh mesh;
  if(size>=84 && size==84ull+50ull*count) {
    if(!count||count>limit) throw std::runtime_error("Invalid triangle count"); mesh.triangles.reserve(count);
    for(uint32_t i=0;i<count;++i) { if(checkpoint && i%1024==0) checkpoint(); unsigned char b[50]; if(!in.read(reinterpret_cast<char*>(b),50)) throw std::runtime_error("Truncated STL"); Triangle t;
      for(int j=0;j<3;++j) { auto p=b+12+j*12; t.v[j]={f32(p),f32(p+4),f32(p+8)}; valid(t.v[j]); } mesh.triangles.push_back(t); }
  } else {
    in.clear(); in.seekg(0); token(in,"solid"); std::string line; std::getline(in,line); bool ended=false; std::string s;
    while(in>>s) { if(checkpoint && mesh.triangles.size()%1024==0) checkpoint(); if(s=="endsolid") { std::getline(in,line); ended=true; break; } if(s!="facet") throw std::runtime_error("Invalid STL encoding or truncated binary STL");
      token(in,"normal"); Vec3 n; if(!(in>>n.x>>n.y>>n.z)) throw std::runtime_error("Invalid normal"); valid(n); token(in,"outer"); token(in,"loop"); Triangle t;
      for(auto& p:t.v) { token(in,"vertex"); if(!(in>>p.x>>p.y>>p.z)) throw std::runtime_error("Invalid vertex"); valid(p); }
      token(in,"endloop"); token(in,"endfacet"); mesh.triangles.push_back(t); if(mesh.triangles.size()>limit) throw std::runtime_error("Too many triangles");
    }
    if(!ended||mesh.triangles.empty()||(in>>s)) throw std::runtime_error("Incomplete or trailing ASCII STL data");
  }
  return mesh;
}
void writeStl(const Mesh& mesh,const std::filesystem::path& path) {
  if(mesh.triangles.empty()||mesh.triangles.size()>limit) throw std::runtime_error("Invalid export triangle count");
  for(auto& t:mesh.triangles) for(auto p:t.v) valid(p);
  std::ofstream out(path,std::ios::binary); if(!out) throw std::runtime_error("Cannot create STL");
  std::string header="ZahnCAD research geometry; units millimetres"; header.resize(80,0); out.write(header.data(),80); put(out,uint32_t(mesh.triangles.size()));
  for(auto& t:mesh.triangles) { auto n=cross(sub(t.v[1],t.v[0]),sub(t.v[2],t.v[0])); double len=norm(n); if(len>0) {n.x/=len;n.y/=len;n.z/=len;} putf(out,n.x);putf(out,n.y);putf(out,n.z);for(auto p:t.v) {putf(out,p.x);putf(out,p.y);putf(out,p.z);}out.put(0);out.put(0); }
  out.flush(); if(!out) throw std::runtime_error("STL write failed");
}
Report inspect(const Mesh& mesh, const Checkpoint& checkpoint) {
  Report r; r.triangles=mesh.triangles.size(); if(!r.triangles) return r;
  r.minimum={INFINITY,INFINITY,INFINITY}; r.maximum={-INFINITY,-INFINITY,-INFINITY};
  using Key=std::tuple<double,double,double>; using Edge=std::pair<Key,Key>; std::map<Edge,std::pair<size_t,int>> edges;
  size_t checked=0;
  for(auto& t:mesh.triangles) { if(checkpoint && checked++%1024==0) checkpoint(); for(auto p:t.v) {valid(p);r.minimum.x=std::min(r.minimum.x,p.x);r.minimum.y=std::min(r.minimum.y,p.y);r.minimum.z=std::min(r.minimum.z,p.z);r.maximum.x=std::max(r.maximum.x,p.x);r.maximum.y=std::max(r.maximum.y,p.y);r.maximum.z=std::max(r.maximum.z,p.z);}
    if(norm(cross(sub(t.v[1],t.v[0]),sub(t.v[2],t.v[0])))<=1e-16) ++r.degenerate;
    for(int i=0;i<3;++i) {auto a=t.v[i],b=t.v[(i+1)%3];Key ka{a.x,a.y,a.z},kb{b.x,b.y,b.z}; bool forward=ka<kb; auto& e=edges[forward?Edge{ka,kb}:Edge{kb,ka}];++e.first;e.second+=forward?1:-1;}
  }
  for(auto& [edge,e]:edges) {if(e.first==1)++r.boundaryEdges;else if(e.first!=2)++r.nonManifoldEdges;else if(e.second)++r.inconsistentEdges;} return r;
}
std::string Report::text() const { std::ostringstream s;s<<"Triangles: "<<triangles<<"\nDegenerate: "<<degenerate<<"\nBoundary edges: "<<boundaryEdges<<"\nNon-manifold edges: "<<nonManifoldEdges<<"\nInconsistent edges: "<<inconsistentEdges<<"\nBounds (mm): "<<minimum.x<<","<<minimum.y<<","<<minimum.z<<" to "<<maximum.x<<","<<maximum.y<<","<<maximum.z<<"\nClosed topology: "<<(closed()?"yes":"no")<<"\nSelf-intersections: NOT CHECKED\nClinical/manufacturing validation: NOT PERFORMED\n";return s.str(); }
Mesh scaled(const Mesh& input,double factor,const Checkpoint& checkpoint) {if(checkpoint)checkpoint();if(!std::isfinite(factor)||factor<=0||factor>1e6)throw std::runtime_error("Scale must be positive and finite");Mesh m; m.triangles.reserve(input.triangles.size());size_t checked=0;for(auto t:input.triangles){if(checkpoint && checked++%1024==0)checkpoint();for(auto& p:t.v){p={p.x*factor,p.y*factor,p.z*factor};valid(p);}m.triangles.push_back(t);}m.colors=input.colors;return m;}
}
