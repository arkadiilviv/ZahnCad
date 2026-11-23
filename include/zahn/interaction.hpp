#pragma once
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkStaticPointLocator.h>
#include <vtkStaticCellLocator.h>
#include <functional>
#include "mesh.hpp"
namespace zahn {
struct BrushIndex {
  vtkSmartPointer<vtkStaticPointLocator> points;
  vtkSmartPointer<vtkStaticCellLocator> cells;
};
BrushIndex prepareBrush(vtkPolyData*,const Checkpoint& = {});
class InteractionStyle : public vtkInteractorStyleTrackballCamera {
public:
  static InteractionStyle* New();
  vtkTypeMacro(InteractionStyle,vtkInteractorStyleTrackballCamera);
  std::function<void(int,int,bool)> paint;
  std::function<void(int,int,bool)> hover;
  std::function<bool(int,int)> measure;
  std::function<void()> changed,strokeEnded,strokeCommitted;
  void setBrush(bool enabled);
  bool brushEnabled() const {return brush_;}
  double radius=30;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnMouseMove() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;
  void OnChar() override {} // Qt actions own keyboard bindings; suppress VTK's conflicting legacy keys.
private:
  bool brush_=false,painting_=false;
  int previousX_=0,previousY_=0;
  void stamp(int,int);
};
}
