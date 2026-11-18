#include "zahn/display.hpp"
#include "zahn/surface.hpp"
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkNew.h>
#include <vtkQuadricClustering.h>
#include <vtkCallbackCommand.h>
#include <vtkAlgorithm.h>
#include <exception>
namespace zahn {
DisplayMesh prepareDisplay(std::shared_ptr<const Mesh> source,const Checkpoint& checkpoint){
  if(!source)throw std::runtime_error("Missing display geometry");
  if(checkpoint)checkpoint();
  DisplayMesh result;result.source=source;
  vtkNew<vtkPoints> points;points->SetDataTypeToDouble();
  points->SetNumberOfPoints(vtkIdType(source->triangles.size()*3));
  vtkNew<vtkCellArray> cells;cells->AllocateEstimate(vtkIdType(source->triangles.size()),3);
  vtkIdType index=0;
  for(auto& triangle:source->triangles){if(checkpoint&&index%3072==0)checkpoint();vtkIdType ids[3];for(int j=0;j<3;++j){ids[j]=index;auto p=triangle.v[j];points->SetPoint(index++,p.x,p.y,p.z);}cells->InsertNextCell(3,ids);}
  result.full=vtkSmartPointer<vtkPolyData>::New();result.full->SetPoints(points);result.full->SetPolys(cells);
  if(!source->colors.empty())result.full=toPolyData(*source,checkpoint);
  result.full->ComputeBounds();result.navigation=result.full;
  result.full->BuildCells();
  if(source->triangles.size()>200000){
    vtkNew<vtkQuadricClustering> decimate;decimate->SetInputData(result.full);
    decimate->SetNumberOfDivisions(64,64,64);decimate->UseInputPointsOn();
    struct Context {const Checkpoint* check;std::exception_ptr error;};Context ctx{&checkpoint,{}};
    vtkNew<vtkCallbackCommand> observer;observer->SetClientData(&ctx);
    observer->SetCallback([](vtkObject* object,unsigned long,void* data,void*){auto& ctx=*static_cast<Context*>(data);if(*ctx.check)try{(*ctx.check)();}catch(...){ctx.error=std::current_exception();static_cast<vtkAlgorithm*>(object)->SetAbortExecute(1);}});
    decimate->AddObserver(vtkCommand::ProgressEvent,observer);decimate->Update();
    if(ctx.error)std::rethrow_exception(ctx.error);if(checkpoint)checkpoint();
    if(decimate->GetOutput()->GetNumberOfCells()>0){result.navigation=vtkSmartPointer<vtkPolyData>::New();result.navigation->ShallowCopy(decimate->GetOutput());}
  }
  return result;
}
}
