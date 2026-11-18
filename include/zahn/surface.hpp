#pragma once
#include "mesh.hpp"
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkAlgorithm.h>
namespace zahn {
vtkSmartPointer<vtkPolyData> toPolyData(const Mesh&,const Checkpoint& = {});
Mesh fromPolyData(vtkPolyData*,const Checkpoint& = {});
Mesh readSurface(const std::filesystem::path&,std::string format,const Checkpoint& = {});
void updateAlgorithm(vtkAlgorithm*,const Checkpoint&);
struct Box {Vec3 minimum,maximum;};
enum class Operation { Crop, KeepSelected, DeleteSelected, LargestComponent, SelectedComponent, Clean, Orient, FillHoles, Smooth, Simplify };
struct Parameters {
  Operation operation=Operation::Clean;
  Box box{};
  std::vector<size_t> selected;
  std::vector<Vec3> selectedVertices;
  double value=0.5; // hole radius, smoothing relaxation, or target reduction
  int iterations=10;
};
struct Deviation {double maximum{},rms{};size_t samples{};};
Deviation deviation(const Mesh&,const Mesh&,const Checkpoint& = {});
Mesh processSurface(const Mesh&,const Parameters&,const Checkpoint& = {});
std::vector<size_t> selectBox(const Mesh&,Box,const Checkpoint& = {});
}
