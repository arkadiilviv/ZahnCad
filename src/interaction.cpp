#include "zahn/interaction.hpp"
#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCallbackCommand.h>
#include <algorithm>
#include <cmath>
namespace zahn {
vtkStandardNewMacro(InteractionStyle);
BrushIndex prepareBrush(vtkPolyData* data,const Checkpoint& check){
  if(check)check();BrushIndex result;
  result.points=vtkSmartPointer<vtkStaticPointLocator>::New();result.points->SetDataSet(data);result.points->BuildLocator();
  if(check)check();result.cells=vtkSmartPointer<vtkStaticCellLocator>::New();result.cells->SetDataSet(data);result.cells->BuildLocator();
  if(check)check();return result;
}
void InteractionStyle::setBrush(bool enabled){
  if(painting_){painting_=false;ReleaseFocus();if(strokeEnded)strokeEnded();}brush_=enabled;
}
void InteractionStyle::stamp(int x,int y){if(paint)paint(x,y,Interactor->GetShiftKey()!=0);}
void InteractionStyle::OnLeftButtonDown(){
  if(!brush_){Superclass::OnLeftButtonDown();return;}
  if(State!=VTKIS_NONE)return;auto* p=Interactor->GetEventPosition();FindPokedRenderer(p[0],p[1]);if(!CurrentRenderer)return;
  painting_=true;previousX_=p[0];previousY_=p[1];GrabFocus(EventCallbackCommand);stamp(p[0],p[1]);if(changed)changed();
}
void InteractionStyle::OnLeftButtonUp(){
  if(!brush_){Superclass::OnLeftButtonUp();return;}
  if(painting_){painting_=false;ReleaseFocus();if(strokeEnded)strokeEnded();}
}
void InteractionStyle::OnRightButtonDown(){
  if(painting_){painting_=false;ReleaseFocus();if(strokeEnded)strokeEnded();}
  auto* p=Interactor->GetEventPosition();FindPokedRenderer(p[0],p[1]);if(!CurrentRenderer)return;GrabFocus(EventCallbackCommand);
  if(Interactor->GetShiftKey())StartPan();else StartRotate();
}
void InteractionStyle::OnRightButtonUp(){
  if(State==VTKIS_ROTATE)EndRotate();else if(State==VTKIS_PAN)EndPan();ReleaseFocus();
}
void InteractionStyle::OnMiddleButtonDown(){if(!brush_)Superclass::OnMiddleButtonDown();}
void InteractionStyle::OnMiddleButtonUp(){if(!brush_)Superclass::OnMiddleButtonUp();}
void InteractionStyle::OnMouseMove(){
  auto* p=Interactor->GetEventPosition();if(hover)hover(p[0],p[1],Interactor->GetShiftKey()!=0);
  if(brush_&&painting_){
    const int x=p[0],y=p[1];double distance=std::hypot(double(x-previousX_),double(y-previousY_));
    int steps=std::max(1,int(std::ceil(distance/std::max(2.,radius/3.))));
    for(int i=1;i<=steps;++i)stamp(int(std::lround(previousX_+(x-previousX_)*double(i)/steps)),int(std::lround(previousY_+(y-previousY_)*double(i)/steps)));
    previousX_=x;previousY_=y;if(changed)changed();return;
  }
  Superclass::OnMouseMove();
}
void InteractionStyle::OnMouseWheelForward(){if(strokeEnded)strokeEnded();Superclass::OnMouseWheelForward();}
void InteractionStyle::OnMouseWheelBackward(){if(strokeEnded)strokeEnded();Superclass::OnMouseWheelBackward();}
}
