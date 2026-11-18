#pragma once
#include "project.hpp"
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
namespace zahn {
struct DisplayMesh {
  std::shared_ptr<const Mesh> source;
  vtkSmartPointer<vtkPolyData> full, navigation;
};
DisplayMesh prepareDisplay(std::shared_ptr<const Mesh>, const Checkpoint& checkpoint = {});
}
