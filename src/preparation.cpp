#include "zahn/preparation.hpp"
#include "zahn/surface.hpp"
#include <vtkFlyingEdges3D.h>
#include <vtkImageData.h>
#include <vtkLandmarkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkStaticCellLocator.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace zahn {
namespace {
Vec3 add(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
Vec3 subtract(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
Vec3 multiply(Vec3 a, double n) { return {a.x*n, a.y*n, a.z*n}; }
double dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
double length(Vec3 a) { return std::hypot(a.x,a.y,a.z); }
Vec3 unit(Vec3 a) {
  const double n=length(a);
  if(!std::isfinite(n)||n<=0)throw std::runtime_error("Invalid insertion direction");
  return multiply(a,1/n);
}
void checkOutputPoint(Vec3 p) {
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||
     std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)})>1e8)
    throw std::runtime_error("Derived coordinates exceed the supported range");
}
void checkTriplet(const LandmarkTriplet& p) {
  const auto a=subtract(p[1],p[0]), b=subtract(p[2],p[0]);
  const double longest=std::max({length(a),length(b),length(subtract(p[2],p[1]))});
  const double height=length(cross(a,b))/longest;
  if(!std::isfinite(height)||longest<1e-6||height<longest*1e-4)
    throw std::runtime_error("Choose three distinct, well-separated, non-collinear landmarks on each scan");
}
struct InsertionSurface {
  Vec3 u,v,w,anchor;
  vtkSmartPointer<vtkPolyData> data;
  vtkNew<vtkStaticCellLocator> locator;
  double bounds[6]{};

  InsertionSurface(const Mesh& mesh, Vec3 axis, const Checkpoint& check) : w(unit(axis)) {
    u=unit(cross(std::abs(w.z)<0.9?Vec3{0,0,1}:Vec3{0,1,0},w));
    v=cross(w,u);
    data=toPolyData(mesh,check);
    if(!data->GetNumberOfCells())throw std::runtime_error("Select a scan with geometry");
    anchor=mesh.triangles.front().v.front();
    auto* points=data->GetPoints();
    for(vtkIdType i=0;i<points->GetNumberOfPoints();++i){
      if(check&&i%1024==0)check();
      double p[3];
      points->GetPoint(i,p);
      const auto point=project({p[0],p[1],p[2]});
      points->SetPoint(i,point.x,point.y,point.z);
    }
    points->Modified();
    data->GetBounds(bounds);
    locator->SetDataSet(data);
    locator->BuildLocator();
    if(check)check();
  }
  Vec3 project(Vec3 p) const {
    p=subtract(p,anchor);
    return {dot(u,p),dot(v,p),dot(w,p)};
  }
  Vec3 world(Vec3 p) const {
    return add(anchor,add(add(multiply(u,p.x),multiply(v,p.y)),multiply(w,p.z)));
  }
  bool top(double x,double y,double& z) const {
    const double padding=std::max(1.,bounds[5]-bounds[4]);
    const double from[3]{x,y,bounds[5]+padding}, to[3]{x,y,bounds[4]-padding};
    double t,hit[3],pcoords[3];
    int subId;
    if(!locator->IntersectWithLine(from,to,0,t,hit,pcoords,subId))return false;
    z=hit[2];
    return true;
  }
};
}

Vec3 RigidAlignment::apply(Vec3 p) const {
  return {matrix[0]*p.x+matrix[1]*p.y+matrix[2]*p.z+matrix[3],
          matrix[4]*p.x+matrix[5]*p.y+matrix[6]*p.z+matrix[7],
          matrix[8]*p.x+matrix[9]*p.y+matrix[10]*p.z+matrix[11]};
}

RigidAlignment alignLandmarks(const LandmarkTriplet& moving,const LandmarkTriplet& fixed) {
  checkTriplet(moving);
  checkTriplet(fixed);
  vtkNew<vtkPoints> from,to;
  from->SetDataTypeToDouble();
  to->SetDataTypeToDouble();
  for(size_t i=0;i<moving.size();++i){
    from->InsertNextPoint(moving[i].x,moving[i].y,moving[i].z);
    to->InsertNextPoint(fixed[i].x,fixed[i].y,fixed[i].z);
  }
  vtkNew<vtkLandmarkTransform> fit;
  fit->SetSourceLandmarks(from);
  fit->SetTargetLandmarks(to);
  fit->SetModeToRigidBody();
  auto* matrix=fit->GetMatrix();
  RigidAlignment result;
  for(int row=0;row<4;++row)
    for(int column=0;column<4;++column)
      result.matrix[row*4+column]=matrix->GetElement(row,column);
  for(size_t i=0;i<moving.size();++i){
    const auto error=length(subtract(result.apply(moving[i]),fixed[i]));
    result.maximum=std::max(result.maximum,error);
    result.rms+=error*error;
  }
  result.rms=std::sqrt(result.rms/moving.size());
  return result;
}

Mesh transformMesh(const Mesh& mesh,const RigidAlignment& fit,const Checkpoint& check) {
  Mesh result;
  result.triangles.reserve(mesh.triangles.size());
  for(const auto& triangle:mesh.triangles){
    if(check&&result.triangles.size()%1024==0)check();
    Triangle moved;
    std::transform(triangle.v.begin(),triangle.v.end(),moved.v.begin(),
      [&fit](Vec3 p){
        const auto result=fit.apply(p);
        checkOutputPoint(result);
        return result;
      });
    result.triangles.push_back(moved);
  }
  result.colors=mesh.colors;
  return result;
}

InsertionReport analyzeInsertion(const Mesh& mesh,Vec3 axis,const Checkpoint& check) {
  const InsertionSurface surface(mesh,axis,check);
  const double noise=std::max(1e-7,(surface.bounds[5]-surface.bounds[4])*1e-8);
  InsertionReport report;
  // Sample at most 100,000 face centroids; a narrow or unsampled undercut can be missed.
  const size_t step=std::max(size_t(1),(mesh.triangles.size()+99999)/100000);
  for(size_t i=0;i<mesh.triangles.size();i+=step){
    if(check&&report.sampled%256==0)check();
    const auto& face=mesh.triangles[i];
    const auto center=surface.project(multiply(add(add(face.v[0],face.v[1]),face.v[2]),1./3));
    double top;
    ++report.sampled;
    if(!surface.top(center.x,center.y,top))continue;
    const double depth=top-center.z;
    if(depth<=noise)continue;
    ++report.undercuts;
    report.maximumDepth=std::max(report.maximumDepth,depth);
    report.shadowed.triangles.push_back(face);
  }
  return report;
}

Mesh blockout(const Mesh& mesh,Vec3 axis,double spacing,const Checkpoint& check) {
  if(!std::isfinite(spacing)||spacing<0.05||spacing>5)
    throw std::runtime_error("Blockout grid spacing must be between 0.05 and 5 mm");
  const InsertionSurface surface(mesh,axis,check);
  int size[3]{};
  double origin[3]{};
  size_t samples=1;
  const double base=surface.bounds[4]-spacing;
  for(int i=0;i<3;++i){
    origin[i]=surface.bounds[2*i]-2*spacing;
    const double count=std::ceil((surface.bounds[2*i+1]-surface.bounds[2*i])/spacing)+5;
    if(!std::isfinite(count)||count>2000||samples*size_t(count)>8000000)
      throw std::runtime_error("Blockout grid is too large. Increase grid spacing or crop the region.");
    size[i]=int(count);
    samples*=size[i];
  }

  vtkNew<vtkImageData> image;
  image->SetDimensions(size);
  image->SetOrigin(origin);
  image->SetSpacing(spacing,spacing,spacing);
  image->AllocateScalars(VTK_FLOAT,1);
  auto* values=static_cast<float*>(image->GetScalarPointer());
  for(int y=0;y<size[1];++y){
    for(int x=0;x<size[0];++x){
      if(check&&x%64==0)check();
      double top=0;
      const bool hit=surface.top(origin[0]+x*spacing,origin[1]+y*spacing,top);
      for(int z=0;z<size[2];++z){
        const double depth=origin[2]+z*spacing;
        const double field=hit?std::clamp(std::min(depth-base,top-depth),-spacing,spacing):-spacing;
        values[(size_t(z)*size[1]+y)*size[0]+x]=float(field);
      }
    }
  }
  vtkNew<vtkFlyingEdges3D> contour;
  contour->SetInputData(image);
  contour->SetValue(0,-spacing*1e-4);
  contour->ComputeNormalsOff();
  contour->ComputeGradientsOff();
  contour->ComputeScalarsOff();
  updateAlgorithm(contour,check);
  auto result=fromPolyData(contour->GetOutput(),check);
  for(size_t i=0;i<result.triangles.size();++i){
    if(check&&i%1024==0)check();
    for(auto& p:result.triangles[i].v){
      p=surface.world(p);
      checkOutputPoint(p);
    }
  }
  return result;
}
}
