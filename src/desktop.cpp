#include "zahn/document.hpp"
#include "zahn/display.hpp"
#include "zahn/surface.hpp"
#include "zahn/interaction.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QSpinBox>
#include <QKeySequence>
#include <QJsonObject>
#include <QDateTime>
#include <QTemporaryDir>
#include <QJsonArray>
#include <QApplication>
#include <QSurfaceFormat>
#include <QMainWindow>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QListWidget>
#include <QSlider>
#include <QToolBar>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QCloseEvent>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QAction>
#include <QVariant>
#include <QSignalBlocker>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QElapsedTimer>
#include <algorithm>
#include <atomic>
#include <functional>
#include <cmath>
#include <QLockFile>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkPlane.h>
#include <vtkDistanceWidget.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkCallbackCommand.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkNew.h>
#include <vtkCellPicker.h>
#include <vtkBoxWidget2.h>
#include <vtkBoxRepresentation.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkIdList.h>
#include <set>
#include <atomic>
#include <map>

class BrushOverlay : public QWidget {
public:
  QPointF center;double radius=30;bool active=false,inside=false,erase=false;
  explicit BrushOverlay(QWidget* parent):QWidget(parent){setAttribute(Qt::WA_TransparentForMouseEvents);setAttribute(Qt::WA_TranslucentBackground);}
protected:
  void paintEvent(QPaintEvent*) override {if(!active||!inside)return;QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);QColor color=erase?QColor(80,190,255):QColor(255,140,30);painter.setPen(QPen(color,2));color.setAlpha(20);painter.setBrush(color);painter.drawEllipse(center,radius,radius);}
};
class Window : public QMainWindow {
  zahn::Document document;
  QString path, directory;
  QListWidget* scans;
  QSlider* opacity;
  QToolBar* tools;
  QAction *undoAction, *redoAction;
  QVTKOpenGLNativeWidget* view;
  vtkNew<vtkGenericOpenGLRenderWindow> render;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkPlane> plane;
  vtkNew<vtkDistanceWidget> distance;
  vtkNew<vtkBoxWidget2> boxWidget;
  vtkNew<vtkBoxRepresentation> boxRepresentation;
  vtkNew<vtkActor> selectionActor;
  vtkNew<vtkActor> normalsActor;
  const zahn::Mesh* selectionMesh=nullptr;
  using VertexKey=std::array<double,3>;
  std::set<VertexKey> selectedVertices;
  bool picking=false,showNormals=false,showColors=true;
  vtkNew<zahn::InteractionStyle> inputStyle;
  BrushOverlay* brushOverlay=nullptr;
  QSpinBox* brushRadius=nullptr;
  QAction* brushAction=nullptr;
  std::vector<float> strokeDepth;
  int depthWidth=0,depthHeight=0;
  struct Display {
    zahn::DisplayMesh geometry;
    vtkSmartPointer<vtkPolyDataMapper> mapper;
    vtkSmartPointer<vtkActor> actor;
    std::shared_ptr<zahn::BrushIndex> brush;
  };
  std::map<const zahn::Mesh*, Display> cache;
  std::vector<vtkSmartPointer<vtkActor>> active;
  bool busy = false, clip = false, navigating = false, autosaving = false, closePending = false;
  quint64 recoveryGeneration = 0;
  std::shared_ptr<std::atomic_bool> cancellation;
  QSqlDatabase db;

  template<class F> void guarded(F fn) {
    try { fn(); } catch(const std::exception& e) {
      if(QApplication::arguments().contains("--stage2-smoke-test")){qCritical("%s",e.what());QApplication::exit(5);return;}
      QMessageBox::critical(this, "Operation failed", QString::fromUtf8(e.what()));
    }
  }
  void refreshTitle() {
    setWindowTitle(document.project.id + (document.dirty() ? " *" : "") + " — ZahnCAD Research");
    undoAction->setEnabled(!busy && document.history.canUndo());
    redoAction->setEnabled(!busy && document.history.canRedo());
  }
  // The worker owns its result; only the finished callback touches widgets or the scene.
  void runJob(QString label, std::function<void(const zahn::Checkpoint&)> worker,
              std::function<void()> finish, bool cancellable = true) {
    if(busy) return;
    busy = true;
    cancellation = std::make_shared<std::atomic_bool>(false);
    auto flag = cancellation;
    auto* progress = new QProgressDialog(label, cancellable ? "Cancel" : QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    if(!cancellable) progress->setCancelButton(nullptr);
    connect(progress, &QProgressDialog::canceled, this, [flag]{flag->store(true);});
    menuBar()->setEnabled(false); scans->setEnabled(false); tools->setEnabled(false);refreshTitle();
    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
      [this,watcher,progress,flag,finish,label] {
        const QString error = watcher->result();
        const bool cancelled = flag->load();
        progress->reset(); progress->deleteLater(); watcher->deleteLater();
        busy = false; cancellation.reset();
        menuBar()->setEnabled(true); scans->setEnabled(true); tools->setEnabled(true);
        if(!cancelled && error.isEmpty()) guarded(finish);
        else if(!cancelled) {if(QApplication::arguments().contains("--stage2-smoke-test")){qCritical("%s",error.toUtf8().constData());QApplication::exit(5);}else QMessageBox::critical(this, label, error);}
        statusBar()->showMessage(cancelled ? "Operation cancelled; current case preserved" :
          error.isEmpty() ? label + " completed" : "Operation failed; current case preserved", 6000);
        refreshTitle();
        if(closePending) {closePending=false;QTimer::singleShot(0,this,[this]{close();});}
      });
    watcher->setFuture(QtConcurrent::run([worker,flag]() -> QString {
      try {
        zahn::Checkpoint checkpoint = [flag]{if(flag->load()) throw std::runtime_error("Cancelled");};
        checkpoint(); worker(checkpoint); checkpoint(); return {};
      } catch(const std::exception& e) { return QString::fromUtf8(e.what()); }
      catch(...) { return "Unexpected worker failure"; }
    }));
  }
  void clearRecovery() {
    ++recoveryGeneration;
    for(auto name : {"recovery.zahncad", "recovery.previous.zahncad"}) QFile::remove(QDir(directory).filePath(name));
  }
  void autosave() {
    if(busy||autosaving||!document.dirty())return;
    autosaving=true;auto snapshot=document.project;auto dir=directory;auto generation=recoveryGeneration;
    auto* watcher=new QFutureWatcher<QString>(this);
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,generation]{
      auto error=watcher->result();watcher->deleteLater();autosaving=false;
      if(generation!=recoveryGeneration||!document.dirty())clearRecovery();
      if(!error.isEmpty())statusBar()->showMessage("Autosave failed — save the case manually: "+error);
      if(closePending){closePending=false;QTimer::singleShot(0,this,[this]{close();});}
    });
    watcher->setFuture(QtConcurrent::run([snapshot,dir]()->QString{
      try{zahn::saveRecovery(snapshot,dir);return {};}catch(const std::exception& e){return QString::fromUtf8(e.what());}
    }));
  }
  bool save(bool saveAs = false) {
    QString destination = saveAs ? QString() : path;
    if(destination.isEmpty()) destination = QFileDialog::getSaveFileName(this,"Save project",{},"ZahnCAD (*.zahncad)");
    if(destination.isEmpty()) return false;
    zahn::saveProject(document.project, destination);
    path = destination; document.markSaved(); clearRecovery();
    QSqlQuery q(db); q.prepare("INSERT OR REPLACE INTO cases(path,name) VALUES(?,?)");
    q.addBindValue(path); q.addBindValue(document.project.id); q.exec();refreshTitle();return true;
  }
  bool leave() {
    if(!document.dirty()) return true;
    auto answer = QMessageBox::question(this,"Unsaved case","Save this case before continuing?",
      QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
    if(answer==QMessageBox::Cancel) return false;
    return answer!=QMessageBox::Save || save();
  }
  void addDisplay(zahn::DisplayMesh value) {
    Display item;item.geometry=std::move(value);
    item.mapper=vtkSmartPointer<vtkPolyDataMapper>::New();item.mapper->SetInputData(item.geometry.full);
    item.mapper->SetScalarVisibility(showColors);
    item.actor=vtkSmartPointer<vtkActor>::New();item.actor->SetMapper(item.mapper);
    item.actor->GetProperty()->SetColor(0.86,0.79,0.62);
    cache.emplace(item.geometry.source.get(),std::move(item));
  }
  void syncOpacity() {
    QSignalBlocker blocked(opacity);int i=scans->currentRow();
    if(i>=int(document.project.scans.size()))i=-1;
    opacity->setEnabled(i>=0);opacity->setValue(i>=0?int(document.project.scans[i].opacity*100):100);
  }
  void syncScene() {
    int selected=scans->currentRow();
    {QSignalBlocker blocked(scans);scans->clear();
      for(auto actor:active)renderer->RemoveActor(actor);active.clear();
      for(auto& scan:document.project.scans){
        auto found=cache.find(scan.mesh.get());if(found==cache.end())continue;
        auto& item=found->second;item.mapper->RemoveAllClippingPlanes();if(clip)item.mapper->AddClippingPlane(plane);
        item.actor->SetVisibility(scan.visible);item.actor->GetProperty()->SetOpacity(scan.opacity);
        renderer->AddActor(item.actor);active.push_back(item.actor);
        auto* row=new QListWidgetItem(scan.name+" ["+scan.role+"]",scans);
        row->setCheckState(scan.visible?Qt::Checked:Qt::Unchecked);
      }
      if(scans->count())scans->setCurrentRow(std::clamp(selected,0,scans->count()-1));
    }
    syncOpacity();refreshTitle();render->Render();
    updateSelection();
    // Drop display caches after their undo commands and document references expire.
    for(auto it=cache.begin();it!=cache.end();) {
      if(it->second.geometry.source.use_count()==1)it=cache.erase(it);else ++it;
    }
  }
  void navigation(bool enabled) {
    navigating=enabled;
    for(auto& [key,item]:cache)item.mapper->SetInputData(enabled?item.geometry.navigation:item.geometry.full);
    if(!enabled)render->Render();
  }
  void updateSelection() {
    int i=scans->currentRow();auto mesh=i>=0&&size_t(i)<document.project.scans.size()?document.project.scans[i].mesh:std::shared_ptr<const zahn::Mesh>();
    if(mesh.get()!=selectionMesh){selectedVertices.clear();selectionMesh=mesh.get();boxWidget->Off();setBrushActive(false);}
    renderer->RemoveActor(selectionActor);renderer->RemoveActor(normalsActor);
    if(!mesh){render->Render();return;}
    auto found=cache.find(mesh.get());if(found==cache.end())return;
    if(!selectedVertices.empty()){
      vtkNew<vtkPoints> points;points->SetDataTypeToDouble();vtkNew<vtkCellArray> cells;for(auto vertex:selectedVertices){auto id=points->InsertNextPoint(vertex.data());cells->InsertNextCell(1,&id);}
      vtkNew<vtkPolyData> data;data->SetPoints(points);data->SetVerts(cells);
      vtkNew<vtkPolyDataMapper> mapper;mapper->SetInputData(data);mapper->ScalarVisibilityOff();mapper->SetResolveCoincidentTopologyToPolygonOffset();mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
      selectionActor->SetMapper(mapper);selectionActor->GetProperty()->SetColor(1,0.45,0.05);selectionActor->GetProperty()->SetPointSize(7);selectionActor->PickableOff();renderer->AddActor(selectionActor);
    }
    if(showNormals){vtkNew<vtkPoints> points;points->SetDataTypeToDouble();vtkNew<vtkCellArray> lines;size_t stride=std::max(size_t(1),mesh->triangles.size()/2000);
      for(size_t face=0;face<mesh->triangles.size();face+=stride){auto v=mesh->triangles[face].v;zahn::Vec3 a{v[1].x-v[0].x,v[1].y-v[0].y,v[1].z-v[0].z},b{v[2].x-v[0].x,v[2].y-v[0].y,v[2].z-v[0].z};zahn::Vec3 n{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};double length=std::hypot(n.x,n.y,n.z);if(length<1e-16)continue;zahn::Vec3 center{(v[0].x+v[1].x+v[2].x)/3,(v[0].y+v[1].y+v[2].y)/3,(v[0].z+v[1].z+v[2].z)/3};vtkIdType ids[2]{points->InsertNextPoint(center.x,center.y,center.z),points->InsertNextPoint(center.x+0.5*n.x/length,center.y+0.5*n.y/length,center.z+0.5*n.z/length)};lines->InsertNextCell(2,ids);}
      vtkNew<vtkPolyData> data;data->SetPoints(points);data->SetLines(lines);vtkNew<vtkPolyDataMapper> mapper;mapper->SetInputData(data);normalsActor->SetMapper(mapper);normalsActor->GetProperty()->SetColor(0.1,0.9,1);normalsActor->PickableOff();renderer->AddActor(normalsActor);
    }
    statusBar()->showMessage(QString("%1 vertices selected · Left brush / Shift+left erase · Right orbit / Shift+right pan · Wheel zoom").arg(selectedVertices.size()),4000);render->Render();
  }
  void setBrushActive(bool enabled){
    picking=enabled;inputStyle->setBrush(enabled);strokeDepth.clear();
    if(brushRadius)inputStyle->radius=brushRadius->value()*view->devicePixelRatioF();
    if(brushAction){QSignalBlocker block(brushAction);brushAction->setChecked(enabled);}
    if(brushOverlay){brushOverlay->active=enabled;brushOverlay->update();}
    if(enabled){boxWidget->Off();distance->Off();navigation(false);}
  }
  void toggleBrush(){
    if(picking){setBrushActive(false);return;}setBrushActive(false);int i=scans->currentRow();if(i<0)return;auto mesh=document.project.scans[i].mesh;
    if(cache.at(mesh.get()).brush){setBrushActive(true);return;}
    auto data=cache.at(mesh.get()).geometry.full;auto index=std::make_shared<zahn::BrushIndex>();
    runJob("Preparing vertex brush",[data,index](const zahn::Checkpoint& check){*index=zahn::prepareBrush(data,check);},[this,mesh,index]{cache.at(mesh.get()).brush=index;setBrushActive(true);});
  }
  void brushStamp(int x,int y,bool erase){
    if(!picking||busy)return;int row=scans->currentRow();if(row<0||size_t(row)>=document.project.scans.size())return;auto& item=cache.at(document.project.scans[row].mesh.get());if(!item.brush)return;
    auto* size=render->GetSize();if(x<0||y<0||x>=size[0]||y>=size[1])return;
    if(strokeDepth.empty()||depthWidth!=size[0]||depthHeight!=size[1]){
      depthWidth=size[0];depthHeight=size[1];strokeDepth.resize(size_t(depthWidth)*depthHeight);
      auto selectedVisible=selectionActor->GetVisibility(),normalsVisible=normalsActor->GetVisibility();auto opacity=item.actor->GetProperty()->GetOpacity();
      selectionActor->VisibilityOff();normalsActor->VisibilityOff();item.actor->GetProperty()->SetOpacity(1);navigation(false);view->grabFramebuffer();view->makeCurrent();
      int ok=render->GetZbufferData(0,0,depthWidth-1,depthHeight-1,strokeDepth.data());view->doneCurrent();
      selectionActor->SetVisibility(selectedVisible);normalsActor->SetVisibility(normalsVisible);item.actor->GetProperty()->SetOpacity(opacity);
      if(!ok){strokeDepth.clear();return;}
    }
    vtkNew<vtkCellPicker> picker;picker->PickFromListOn();picker->AddPickList(item.actor);picker->AddLocator(item.brush->cells);picker->SetTolerance(0.001);
    if(!picker->Pick(x,y,0,renderer))return;double center[3];picker->GetPickPosition(center);
    renderer->SetWorldPoint(center[0],center[1],center[2],1);renderer->WorldToDisplay();double z=renderer->GetDisplayPoint()[2];double radius=brushRadius->value()*view->devicePixelRatioF();
    renderer->SetDisplayPoint(x+radius,y,z);renderer->DisplayToWorld();auto* world=renderer->GetWorldPoint();if(std::abs(world[3])<1e-12)return;
    double worldRadius=std::sqrt(std::pow(world[0]/world[3]-center[0],2)+std::pow(world[1]/world[3]-center[1],2)+std::pow(world[2]/world[3]-center[2],2));
    vtkNew<vtkIdList> points;item.brush->points->FindPointsWithinRadius(worldRadius*1.05,center,points);
    for(vtkIdType i=0;i<points->GetNumberOfIds();++i){double point[3];item.geometry.full->GetPoint(points->GetId(i),point);renderer->SetWorldPoint(point[0],point[1],point[2],1);renderer->WorldToDisplay();auto* screen=renderer->GetDisplayPoint();
      if(screen[2]<0||screen[2]>1||std::hypot(screen[0]-x,screen[1]-y)>radius)continue;
      int px=int(std::lround(screen[0])),py=int(std::lround(screen[1]));double low=1,high=0;
      for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){int sx=px+dx,sy=py+dy;if(sx<0||sx>=depthWidth||sy<0||sy>=depthHeight)continue;double depth=strokeDepth[size_t(sy)*depthWidth+sx];if(depth<0.999999){low=std::min(low,depth);high=std::max(high,depth);}}
      if(high<low||screen[2]<low-0.00002||screen[2]>high+0.00002)continue;
      VertexKey key{point[0],point[1],point[2]};if(erase)selectedVertices.erase(key);else selectedVertices.insert(key);
    }
  }
  zahn::Box currentBox(){if(!boxWidget->GetEnabled())throw std::runtime_error("Enable the crop/selection box first");vtkNew<vtkPolyData> data;boxRepresentation->GetPolyData(data);auto* b=data->GetBounds();return {{b[0],b[2],b[4]},{b[1],b[3],b[5]}};}
  void editSurface(zahn::Parameters parameters,QString label,double tolerance=-1,std::function<void()> after={}){
    int i=scans->currentRow();if(i<0)return;auto before=document.project.scans[i];
    struct Result{std::shared_ptr<const zahn::Mesh> mesh;zahn::DisplayMesh display;zahn::Deviation deviation;zahn::Report oldReport,newReport;};auto result=std::make_shared<Result>();
    runJob(label,[before,parameters,result](const zahn::Checkpoint& check){
      result->mesh=std::make_shared<const zahn::Mesh>(zahn::processSurface(*before.mesh,parameters,check));
      result->deviation=zahn::deviation(*before.mesh,*result->mesh,check);result->oldReport=zahn::inspect(*before.mesh,check);result->newReport=zahn::inspect(*result->mesh,check);
      result->display=zahn::prepareDisplay(result->mesh,check);
    },[this,i,before,parameters,result,label,tolerance,after]{
      if(tolerance>=0&&result->deviation.maximum>tolerance){QMessageBox::information(this,label,QString("Not applied: sampled maximum deviation %1 mm exceeds %2 mm. Choose different parameters or tolerance.").arg(result->deviation.maximum,0,'g',6).arg(tolerance));return;}
      auto& old=cache.at(before.mesh.get());vtkNew<vtkPolyDataMapper> mapper;mapper->SetInputData(result->display.full);mapper->SetScalarVisibility(showColors);
      vtkNew<vtkActor> preview;preview->SetMapper(mapper);preview->GetProperty()->SetColor(0.5,0.9,0.65);old.actor->VisibilityOff();selectionActor->VisibilityOff();normalsActor->VisibilityOff();renderer->AddActor(preview);render->Render();
      QString report=QString("Preview — apply this derived scan?\nTriangles: %1 → %2\nBoundary edges: %3 → %4\nDegenerate faces: %5 → %6\nNon-manifold edges: %7 → %8\nSampled bidirectional deviation: max %9 mm; RMS %10 mm (%11 samples).\n\nSampling is not a certified error bound. Original source data stays unchanged; this edit can be undone.")
        .arg(result->oldReport.triangles).arg(result->newReport.triangles).arg(result->oldReport.boundaryEdges).arg(result->newReport.boundaryEdges).arg(result->oldReport.degenerate).arg(result->newReport.degenerate).arg(result->oldReport.nonManifoldEdges).arg(result->newReport.nonManifoldEdges).arg(result->deviation.maximum,0,'g',6).arg(result->deviation.rms,0,'g',6).arg(result->deviation.samples);
      if(parameters.operation==zahn::Operation::FillHoles)report+="\nHole filling can also close a scan's outer boundary and create overlapping faces. Review the preview carefully.";
      auto answer=QApplication::arguments().contains("--stage2-smoke-test")?QMessageBox::Apply:QMessageBox::question(this,label,report,QMessageBox::Apply|QMessageBox::Cancel,QMessageBox::Cancel);
      renderer->RemoveActor(preview);old.actor->SetVisibility(before.visible);selectionActor->VisibilityOn();normalsActor->VisibilityOn();render->Render();
      if(answer!=QMessageBox::Apply)return;auto edited=before;edited.mesh=result->mesh;
      QJsonObject audit{{"operation",label},{"timeUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"value",parameters.value},{"iterations",parameters.iterations},{"selectedFaceCount",qint64(parameters.selected.size())},{"selectedVertexCount",qint64(parameters.selectedVertices.size())},{"sampledMaxMm",result->deviation.maximum},{"sampledRmsMm",result->deviation.rms},{"samples",qint64(result->deviation.samples)}};
      if(parameters.operation==zahn::Operation::Crop)audit["boxMm"]=QJsonArray{parameters.box.minimum.x,parameters.box.maximum.x,parameters.box.minimum.y,parameters.box.maximum.y,parameters.box.minimum.z,parameters.box.maximum.z};
      edited.operations.append(audit);addDisplay(std::move(result->display));document.replaceScan(size_t(i),std::move(edited),label);syncScene();if(after)after();
    });
  }
  void loadScan(QString file, QString role, double factor, std::function<void()> after = {}) {
    struct Result {zahn::Scan scan;zahn::DisplayMesh display;};auto result=std::make_shared<Result>();
    runJob("Loading scan",[result,file,role,factor](const zahn::Checkpoint& check){
      result->scan=zahn::importScan(file,role,factor,check);
      result->display=zahn::prepareDisplay(result->scan.mesh,check);
    },[this,result,after]{
      addDisplay(std::move(result->display));document.addScan(std::move(result->scan));syncScene();
      scans->setCurrentRow(int(document.project.scans.size())-1);
      renderer->ResetCamera();render->Render();if(after)after();
    });
  }
  void loadCase(QString file, bool recovery = false) {
    if((file.isEmpty()&&!recovery)||!leave())return;
    struct Result {zahn::Project project;std::vector<zahn::DisplayMesh> display;};auto result=std::make_shared<Result>();
    const QString dir=directory;
    runJob(recovery?"Recovering case":"Opening case",[result,file,recovery,dir](const zahn::Checkpoint& check){
      result->project=recovery?zahn::loadRecovery(dir,check):zahn::loadProject(file,check);
      for(auto& scan:result->project.scans)result->display.push_back(zahn::prepareDisplay(scan.mesh,check));
    },[this,result,file,recovery]{
      for(auto actor:active)renderer->RemoveActor(actor);active.clear();cache.clear();
      for(auto& display:result->display)addDisplay(std::move(display));
      document.reset(std::move(result->project),recovery);path=recovery?QString():file;
      if(!recovery)clearRecovery();syncScene();renderer->ResetCamera();render->Render();
    });
  }
  void commitOpacity(int value) {
    int i=scans->currentRow();if(i<0||busy)return;
    auto& scan=document.project.scans[i];document.setDisplay(size_t(i),scan.visible,value/100.);
  }
protected:
  bool eventFilter(QObject* watched,QEvent* event) override {
    if(watched==view&&brushOverlay){if(event->type()==QEvent::Resize)brushOverlay->setGeometry(view->rect());if(event->type()==QEvent::Leave){brushOverlay->inside=false;brushOverlay->update();}}
    return QMainWindow::eventFilter(watched,event);
  }
  void closeEvent(QCloseEvent* event) override {
    event->ignore();
    if(busy||autosaving){closePending=true;if(cancellation)cancellation->store(true);statusBar()->showMessage("Finishing background work before closing");return;}
    guarded([&]{if(leave()){clearRecovery();event->accept();}});
  }
public:
  ~Window() override {
    // QUndoStack::clear emits indexChanged during destruction. The renderer's
    // members are destroyed before the document, so detach this callback first.
    disconnect(&document.history,nullptr,this,nullptr);
    inputStyle->setBrush(false);inputStyle->paint={};inputStyle->hover={};inputStyle->changed={};inputStyle->strokeEnded={};
  }
  Window() {
    resize(1280,850);directory=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(directory);db=QSqlDatabase::addDatabase("QSQLITE");db.setDatabaseName(directory+"/cases.sqlite");
    if(db.open()){QSqlQuery q(db);q.exec("CREATE TABLE IF NOT EXISTS cases(path TEXT PRIMARY KEY,name TEXT)");}
    view=new QVTKOpenGLNativeWidget(this);view->setRenderWindow(render);render->AddRenderer(renderer);
    brushOverlay=new BrushOverlay(view);brushOverlay->setGeometry(view->rect());brushOverlay->show();view->setMouseTracking(true);view->installEventFilter(this);
    render->GetInteractor()->SetInteractorStyle(inputStyle);
    inputStyle->paint=[this](int x,int y,bool erase){brushStamp(x,y,erase);};inputStyle->changed=[this]{updateSelection();};inputStyle->strokeEnded=[this]{strokeDepth.clear();};
    inputStyle->hover=[this](int x,int y,bool erase){double ratio=view->devicePixelRatioF();brushOverlay->center={x/ratio,(render->GetSize()[1]-1-y)/ratio};brushOverlay->inside=true;brushOverlay->erase=erase;brushOverlay->update();};
    renderer->SetBackground(0.08,0.11,0.15);setCentralWidget(view);
    distance->SetInteractor(render->GetInteractor());distance->CreateDefaultRepresentation();plane->SetNormal(0,0,1);
    boxWidget->SetInteractor(render->GetInteractor());boxWidget->SetRepresentation(boxRepresentation);boxWidget->SetDefaultRenderer(renderer);boxWidget->RotationEnabledOff();
    vtkNew<vtkCallbackCommand> interaction;interaction->SetClientData(this);
    interaction->SetCallback([](vtkObject*,unsigned long event,void* data,void*){
      static_cast<Window*>(data)->navigation(event==vtkCommand::StartInteractionEvent);
    });
    render->GetInteractor()->AddObserver(vtkCommand::StartInteractionEvent,interaction);
    render->GetInteractor()->AddObserver(vtkCommand::EndInteractionEvent,interaction);
    scans=new QListWidget;auto* dock=new QDockWidget("Scans",this);dock->setWidget(scans);addDockWidget(Qt::LeftDockWidgetArea,dock);
    tools=addToolBar("Display");tools->addWidget(new QLabel("Opacity "));opacity=new QSlider(Qt::Horizontal);
    opacity->setRange(0,100);opacity->setValue(100);opacity->setMaximumWidth(140);tools->addWidget(opacity);
    tools->addSeparator();tools->addWidget(new QLabel("Brush radius (px) "));brushRadius=new QSpinBox;brushRadius->setRange(5,200);brushRadius->setValue(30);tools->addWidget(brushRadius);
    connect(brushRadius,&QSpinBox::valueChanged,this,[this](int radius){brushOverlay->radius=radius;inputStyle->radius=radius*view->devicePixelRatioF();brushOverlay->update();});
    tools->addSeparator();tools->addWidget(new QLabel("Clip Z (mm) "));auto* z=new QDoubleSpinBox;z->setRange(-10000,10000);tools->addWidget(z);
    connect(z,&QDoubleSpinBox::valueChanged,this,[this](double v){plane->SetOrigin(0,0,v);render->Render();});
    connect(opacity,&QSlider::valueChanged,this,[this](int value){
      int i=scans->currentRow();if(i<0||busy)return;
      if(opacity->isSliderDown()){cache.at(document.project.scans[i].mesh.get()).actor->GetProperty()->SetOpacity(value/100.);render->Render();}
      else commitOpacity(value);
    });
    connect(opacity,&QSlider::sliderReleased,this,[this]{commitOpacity(opacity->value());});
    connect(scans,&QListWidget::currentRowChanged,this,[this]{syncOpacity();updateSelection();});
    connect(scans,&QListWidget::itemChanged,this,[this](QListWidgetItem* row){
      int i=scans->row(row);if(i>=0&&!busy){auto& s=document.project.scans[i];document.setDisplay(size_t(i),row->checkState()==Qt::Checked,s.opacity);}
    });
    auto action=[this](QMenu* menu,QString title,auto fn){auto* a=menu->addAction(title);
      connect(a,&QAction::triggered,this,[this,fn]{if(!busy){strokeDepth.clear();guarded(fn);}});return a;};
    auto* edit=menuBar()->addMenu("Edit");
    undoAction=action(edit,"Undo",[this]{document.history.undo();});undoAction->setShortcut(QKeySequence::Undo);
    redoAction=action(edit,"Redo",[this]{document.history.redo();});redoAction->setShortcut(QKeySequence::Redo);
    action(edit,"Remove selected scan",[this]{int i=scans->currentRow();if(i>=0)document.removeScan(size_t(i));});
    connect(&document.history,&QUndoStack::indexChanged,this,[this]{syncScene();},Qt::QueuedConnection);
    auto* menu=menuBar()->addMenu("Case");
    action(menu,"New case",[this]{if(!leave())return;bool ok;auto id=QInputDialog::getText(this,"Case identifier","Use a non-identifying case code",QLineEdit::Normal,{},&ok);if(!ok)return;
      zahn::Project project;project.id=id;document.reset(std::move(project),true);path.clear();clearRecovery();cache.clear();syncScene();});
    action(menu,"Open project…",[this]{loadCase(QFileDialog::getOpenFileName(this,"Open project",{},"ZahnCAD (*.zahncad)"));});
    action(menu,"Recent cases…",[this]{QStringList names,paths;QSqlQuery q("SELECT name,path FROM cases ORDER BY rowid DESC LIMIT 30",db);
      while(q.next()){names<<q.value(0).toString()+" — "+q.value(1).toString();paths<<q.value(1).toString();}
      if(names.isEmpty()){statusBar()->showMessage("No saved cases yet",4000);return;}
      bool ok;auto name=QInputDialog::getItem(this,"Local cases","Case",names,0,false,&ok);if(ok&&names.contains(name))loadCase(paths[names.indexOf(name)]);});
    action(menu,"Save",[this]{save();})->setShortcut(QKeySequence::Save);
    action(menu,"Save as…",[this]{save(true);});
    action(menu,"Recover autosaved case…",[this]{loadCase({},true);});
    action(menu,"Import scan…",[this]{auto file=QFileDialog::getOpenFileName(this,"Import scan",{},"Scans (*.stl *.ply *.obj)");if(file.isEmpty())return;
      bool ok;double factor=QInputDialog::getDouble(this,"Confirm units","Multiply source coordinates by this factor to obtain mm",1,0.000001,1000000,6,&ok);if(!ok)return;
      auto role=QInputDialog::getItem(this,"Assign scan","Role",{"Upper jaw","Lower jaw","Antagonist","Bite","Reference"},0,false,&ok);if(ok)loadScan(file,role,factor);});
    action(menu,"Inspect selected scan",[this]{int i=scans->currentRow();if(i<0)return;auto mesh=document.project.scans[i].mesh;auto result=std::make_shared<QString>();
      runJob("Inspecting geometry",[mesh,result](const zahn::Checkpoint& check){*result=QString::fromStdString(zahn::inspect(*mesh,check).text());},
        [this,result]{QMessageBox::information(this,"Geometry diagnostics",*result);});});
    action(menu,"Export research STL…",[this]{int i=scans->currentRow();if(i<0)return;auto file=QFileDialog::getSaveFileName(this,"Export research geometry",{},"STL (*.stl)");if(file.isEmpty())return;
      auto mesh=document.project.scans[i].mesh;runJob("Exporting research STL",[mesh,file](const zahn::Checkpoint&){zahn::writeStl(*mesh,zahn::nativePath(file));},[]{},false);});
    auto* camera=menuBar()->addMenu("View");
    action(camera,"Toggle scan colors",[this]{showColors=!showColors;for(auto& [key,item]:cache)item.mapper->SetScalarVisibility(showColors);render->Render();});
    action(camera,"Toggle face normals",[this]{showNormals=!showNormals;updateSelection();});
    auto* preparation=menuBar()->addMenu("Prepare scan");
    brushAction=action(preparation,"Vertex brush",[this]{toggleBrush();});brushAction->setCheckable(true);tools->addAction(brushAction);
    action(preparation,"Exit brush",[this]{setBrushActive(false);});
    action(preparation,"Smaller brush",[this]{brushRadius->setValue(brushRadius->value()-5);});
    action(preparation,"Larger brush",[this]{brushRadius->setValue(brushRadius->value()+5);});
    action(preparation,"Clear vertex selection",[this]{selectedVertices.clear();updateSelection();});
    action(preparation,"Toggle crop/selection box",[this]{int i=scans->currentRow();if(i<0)return;if(boxWidget->GetEnabled()){boxWidget->Off();return;}setBrushActive(false);auto* bounds=cache.at(document.project.scans[i].mesh.get()).geometry.full->GetBounds();double b[6];std::copy(bounds,bounds+6,b);for(int k=0;k<3;++k)if(b[k*2+1]-b[k*2]<0.001){b[k*2]-=0.001;b[k*2+1]+=0.001;}boxRepresentation->SetPlaceFactor(1);boxRepresentation->PlaceWidget(b);boxWidget->On();render->Render();});
    action(preparation,"Select vertices inside box",[this]{int i=scans->currentRow();if(i<0)return;auto mesh=document.project.scans[i].mesh;auto box=currentBox();auto selection=std::make_shared<std::set<VertexKey>>();runJob("Selecting vertices",[mesh,box,selection](const zahn::Checkpoint& check){size_t index=0;for(auto& triangle:mesh->triangles){if(index++%1024==0&&check)check();for(auto p:triangle.v)if(p.x>=box.minimum.x&&p.x<=box.maximum.x&&p.y>=box.minimum.y&&p.y<=box.maximum.y&&p.z>=box.minimum.z&&p.z<=box.maximum.z)selection->insert({p.x,p.y,p.z});}},[this,selection]{selectedVertices=*selection;updateSelection();});});
    auto selectedParameters=[this](zahn::Operation operation){zahn::Parameters p;p.operation=operation;for(auto v:selectedVertices)p.selectedVertices.push_back({v[0],v[1],v[2]});return p;};
    action(preparation,"Keep selected vertices…",[this,selectedParameters]{editSurface(selectedParameters(zahn::Operation::KeepSelected),"Keep selected vertices");});
    action(preparation,"Delete selected vertices and incident faces…",[this,selectedParameters]{editSurface(selectedParameters(zahn::Operation::DeleteSelected),"Delete selected vertices");});
    action(preparation,"Crop to box…",[this]{zahn::Parameters p;p.operation=zahn::Operation::Crop;p.box=currentBox();editSurface(p,"Crop to box");});
    action(preparation,"Keep largest connected component…",[this]{zahn::Parameters p;p.operation=zahn::Operation::LargestComponent;editSurface(p,"Keep largest component");});
    action(preparation,"Keep component touched by selection…",[this,selectedParameters]{editSurface(selectedParameters(zahn::Operation::SelectedComponent),"Keep selected component");});
    action(preparation,"Remove duplicate/degenerate faces…",[this]{zahn::Parameters p;p.operation=zahn::Operation::Clean;editSurface(p,"Clean scan");});
    action(preparation,"Orient connected faces consistently…",[this]{zahn::Parameters p;p.operation=zahn::Operation::Orient;editSurface(p,"Consistent winding");});
    action(preparation,"Fill small holes…",[this]{bool ok;double radius=QInputDialog::getDouble(this,"Fill small holes","Approximate maximum hole radius (mm); outer boundaries can also be filled",0.5,0.001,50,3,&ok);if(!ok)return;zahn::Parameters p;p.operation=zahn::Operation::FillHoles;p.value=radius;editSurface(p,"Fill small holes");});
    action(preparation,"Smooth with fixed boundary…",[this]{bool ok;int iterations=QInputDialog::getInt(this,"Smooth scan","Iterations (boundary vertices stay fixed)",10,1,100,1,&ok);if(!ok)return;double tolerance=QInputDialog::getDouble(this,"Deviation threshold","Maximum sampled deviation (mm); not a certified bound",0.05,0.000001,100,6,&ok);if(!ok)return;zahn::Parameters p;p.operation=zahn::Operation::Smooth;p.value=0.1;p.iterations=iterations;editSurface(p,"Smooth scan",tolerance);});
    action(preparation,"Simplify with topology preservation…",[this]{bool ok;int reduction=QInputDialog::getInt(this,"Simplify scan","Target triangle reduction (%); boundaries and topology constrain the result",50,1,95,1,&ok);if(!ok)return;double tolerance=QInputDialog::getDouble(this,"Deviation threshold","Maximum sampled deviation (mm); not a certified bound",0.05,0.000001,100,6,&ok);if(!ok)return;zahn::Parameters p;p.operation=zahn::Operation::Simplify;p.value=reduction/100.;editSurface(p,"Simplify scan",tolerance);});
    action(preparation,"Restore original scan",[this]{int i=scans->currentRow();if(i<0)return;auto scan=document.project.scans[i];if(!scan.sourceMesh||scan.mesh==scan.sourceMesh)return;scan.mesh=scan.sourceMesh;scan.operations={};auto result=std::make_shared<zahn::DisplayMesh>();runJob("Restoring original",[scan,result](const zahn::Checkpoint& check){*result=zahn::prepareDisplay(scan.mesh,check);},[this,i,scan,result]{addDisplay(std::move(*result));document.replaceScan(size_t(i),scan,"Restore original scan");syncScene();});});
    action(camera,"Fit",[this]{renderer->ResetCamera();render->Render();});
    action(camera,"Orthographic / perspective",[this]{auto* c=renderer->GetActiveCamera();c->SetParallelProjection(!c->GetParallelProjection());render->Render();});
    auto orient=[this](double x,double y,double z){auto* c=renderer->GetActiveCamera();c->SetFocalPoint(0,0,0);c->SetPosition(x,y,z);c->SetViewUp(0,z?1:0,z?0:1);renderer->ResetCamera();render->Render();};
    action(camera,"Occlusal",[orient]{orient(0,0,1);});action(camera,"Front",[orient]{orient(0,-1,0);});action(camera,"Side",[orient]{orient(1,0,0);});
    action(camera,"Toggle distance measurement",[this]{setBrushActive(false);navigation(false);distance->SetEnabled(!distance->GetEnabled());render->Render();});
    action(camera,"Toggle axial clipping",[this]{clip=!clip;syncScene();});
    action(camera,"Screenshot…",[this]{auto file=QFileDialog::getSaveFileName(this,"Screenshot",{},"PNG (*.png)");if(file.isEmpty())return;navigation(false);
      vtkNew<vtkWindowToImageFilter> image;image->SetInput(render);image->ReadFrontBufferOff();image->Update();vtkNew<vtkPNGWriter> writer;
      writer->SetFileName(file.toUtf8().constData());writer->SetInputConnection(image->GetOutputPort());writer->Write();});
    action(camera,"Orbit left",[this]{renderer->GetActiveCamera()->Azimuth(-15);render->Render();});
    action(camera,"Orbit right",[this]{renderer->GetActiveCamera()->Azimuth(15);render->Render();});
    action(camera,"Orbit up",[this]{renderer->GetActiveCamera()->Elevation(15);renderer->GetActiveCamera()->OrthogonalizeViewUp();render->Render();});
    action(camera,"Orbit down",[this]{renderer->GetActiveCamera()->Elevation(-15);renderer->GetActiveCamera()->OrthogonalizeViewUp();render->Render();});
    auto zoom=[this](double amount){auto* camera=renderer->GetActiveCamera();if(camera->GetParallelProjection())camera->SetParallelScale(camera->GetParallelScale()/amount);else camera->Dolly(amount);renderer->ResetCameraClippingRange();render->Render();};
    action(camera,"Zoom in",[zoom]{zoom(1.2);});action(camera,"Zoom out",[zoom]{zoom(1/1.2);});
    const std::map<QString,QString> bindings{
      {"Fit","F"},{"Front","F1"},{"Side","F2"},{"Occlusal","F3"},{"Orthographic / perspective","F4"},{"Zoom in","F5"},{"Zoom out","F6"},
      {"Orbit left","Alt+Left"},{"Orbit right","Alt+Right"},{"Orbit up","Alt+Up"},{"Orbit down","Alt+Down"},
      {"Vertex brush","B"},{"Exit brush","Esc"},{"Smaller brush","["},{"Larger brush","]"},{"Clear vertex selection","Ctrl+Shift+A"},
      {"Toggle crop/selection box","X"},{"Select vertices inside box","Shift+X"},{"Crop to box…","Ctrl+Shift+C"},
      {"Keep selected vertices…","Ctrl+Shift+K"},{"Delete selected vertices and incident faces…","Delete"},
      {"Keep largest connected component…","Ctrl+Alt+L"},{"Keep component touched by selection…","Ctrl+Alt+I"},
      {"Remove duplicate/degenerate faces…","Ctrl+Alt+C"},{"Orient connected faces consistently…","Ctrl+Alt+N"},
      {"Fill small holes…","Ctrl+Alt+H"},{"Smooth with fixed boundary…","Ctrl+Alt+S"},{"Simplify with topology preservation…","Ctrl+Alt+D"},{"Restore original scan","Ctrl+Alt+R"},
      {"Toggle scan colors","V"},{"Toggle face normals","N"},{"Toggle distance measurement","M"},{"Toggle axial clipping","C"},{"Screenshot…","Ctrl+Shift+P"},
      {"New case","Ctrl+N"},{"Open project…","Ctrl+O"},{"Import scan…","Ctrl+I"},{"Save as…","Ctrl+Shift+S"}
    };
    for(auto* top:menuBar()->actions())if(top->menu())for(auto* item:top->menu()->actions()){
      auto found=bindings.find(item->text());if(found!=bindings.end()){item->setShortcut(QKeySequence(found->second));item->setStatusTip(item->text()+" — "+found->second);}
    }
    redoAction->setShortcuts({QKeySequence("Ctrl+Y"),QKeySequence("Ctrl+Shift+Z")});
    auto* help=menuBar()->addMenu("Help");action(help,"Keyboard and mouse controls",[this,bindings]{QString text="Vertex brush: left-drag adds; Shift+left erases. Right-drag orbits; Shift+right pans; wheel zooms. Middle drag is disabled while brushing. Orange points are selected vertices.\n\n";for(auto& [label,key]:bindings)text+=key+" — "+label+"\n";QMessageBox::information(this,"Keyboard and mouse controls",text);})->setShortcut(QKeySequence("F12"));
    auto* timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this]{autosave();});timer->start(60000);
    statusBar()->showMessage("Research preview · Full-resolution data retained for measurement and export");refreshTitle();syncOpacity();
    if(!QApplication::arguments().contains("--smoke-test") && !QApplication::arguments().contains("--stage2-smoke-test") &&
      (QFile::exists(directory+"/recovery.zahncad")||QFile::exists(directory+"/recovery.previous.zahncad")))
      QTimer::singleShot(0,this,[this]{if(QMessageBox::question(this,"Recovery available","Recover the last autosaved case? If the latest is damaged, the previous snapshot will be tried.")==QMessageBox::Yes)guarded([this]{loadCase({},true);});});
  }
  void stage2Smoke(const QString& source,const QString& screenshot){
    loadScan(source,"Reference",1,[this,screenshot]{guarded([this,screenshot]{
      auto original=document.project.scans[0].mesh;auto vertex=original->triangles[0].v;
      cache.at(original.get()).brush=std::make_shared<zahn::BrushIndex>(zahn::prepareBrush(cache.at(original.get()).geometry.full));setBrushActive(true);view->grabFramebuffer();
      auto* camera=renderer->GetActiveCamera();double cameraPosition[3],focalPoint[3],viewUp[3];camera->GetPosition(cameraPosition);camera->GetFocalPoint(focalPoint);camera->GetViewUp(viewUp);
      auto first=vertex[0];renderer->SetWorldPoint(first.x,first.y,first.z,1);renderer->WorldToDisplay();auto* position=renderer->GetDisplayPoint();int px=int(std::lround(position[0])),py=int(std::lround(position[1]));
      auto* interactor=render->GetInteractor();interactor->SetEventPosition(px,py);interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);interactor->SetEventPosition(px+8,py+6);interactor->InvokeEvent(vtkCommand::MouseMoveEvent);interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
      for(int i=0;i<3;++i)if(std::abs(camera->GetPosition()[i]-cameraPosition[i])>1e-10||std::abs(camera->GetFocalPoint()[i]-focalPoint[i])>1e-10)throw std::runtime_error("Left brush drag moved camera");
      if(!selectedVertices.contains({first.x,first.y,first.z}))throw std::runtime_error("Vertex brush failed");
      grab().save(screenshot+".brush.png");
      if(document.project.scans[0].name=="brush-test.ply"){
        auto project=[this](double x,double y){renderer->SetWorldPoint(x,y,0,1);renderer->WorldToDisplay();auto* p=renderer->GetDisplayPoint();return std::array<int,2>{int(std::lround(p[0])),int(std::lround(p[1]))};};
        auto left=project(0,5),right=project(10,5);selectedVertices.clear();
        interactor->SetEventPosition(left[0],left[1]);interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);interactor->SetEventPosition(right[0],right[1]);interactor->InvokeEvent(vtkCommand::MouseMoveEvent);interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
        if(!selectedVertices.contains({5,5,0}))throw std::runtime_error("Fast brush stroke skipped intermediate vertex");
        for(int i=0;i<3;++i)if(std::abs(camera->GetPosition()[i]-cameraPosition[i])>1e-10)throw std::runtime_error("Fast left stroke moved camera");
      }
      if(document.project.scans[0].name=="brush-test.ply")for(auto selected:selectedVertices)if(selected[2]<-0.05)throw std::runtime_error("Brush selected hidden back layer");
      interactor->SetShiftKey(1);interactor->SetEventPosition(px,py);interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);interactor->SetShiftKey(0);
      if(selectedVertices.contains({first.x,first.y,first.z}))throw std::runtime_error("Shift brush did not erase");
      interactor->SetEventPosition(px,py);interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
      interactor->InvokeEvent(vtkCommand::RightButtonPressEvent);interactor->SetEventPosition(px+30,py+20);interactor->InvokeEvent(vtkCommand::MouseMoveEvent);interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent);
      double moved=0;for(int i=0;i<3;++i)moved+=std::abs(camera->GetPosition()[i]-cameraPosition[i]);if(moved<1e-8)throw std::runtime_error("Right drag did not orbit");
      double panBefore[3];camera->GetFocalPoint(panBefore);interactor->SetShiftKey(1);interactor->InvokeEvent(vtkCommand::RightButtonPressEvent);interactor->SetEventPosition(px+45,py+30);interactor->InvokeEvent(vtkCommand::MouseMoveEvent);interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent);interactor->SetShiftKey(0);double pan=0;for(int i=0;i<3;++i)pan+=std::abs(camera->GetFocalPoint()[i]-panBefore[i]);if(pan<1e-8)throw std::runtime_error("Shift+right did not pan");
      double middleBefore[3];camera->GetPosition(middleBefore);interactor->InvokeEvent(vtkCommand::MiddleButtonPressEvent);interactor->SetEventPosition(px+60,py+40);interactor->InvokeEvent(vtkCommand::MouseMoveEvent);interactor->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent);for(int i=0;i<3;++i)if(std::abs(camera->GetPosition()[i]-middleBefore[i])>1e-10)throw std::runtime_error("Middle drag moved camera in brush mode");
      double beforeZoom=camera->GetDistance();interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent);if(std::abs(camera->GetDistance()-beforeZoom)<1e-8)throw std::runtime_error("Wheel did not zoom");
      camera->SetPosition(cameraPosition);camera->SetFocalPoint(focalPoint);camera->SetViewUp(viewUp);renderer->ResetCameraClippingRange();setBrushActive(false);
      std::set<QString> shortcuts;size_t shortcutCount=0;for(auto* top:menuBar()->actions())if(top->menu())for(auto* action:top->menu()->actions())for(auto key:action->shortcuts()){auto name=key.toString(QKeySequence::PortableText);if(name.isEmpty())continue;if(!shortcuts.insert(name).second)throw std::runtime_error("Duplicate shortcut");++shortcutCount;}
      if(shortcutCount<30||brushAction->shortcut()!=QKeySequence("B"))throw std::runtime_error("Tool/camera shortcuts missing");
      auto b=zahn::inspect(*original);double x=b.maximum.x-b.minimum.x,y=b.maximum.y-b.minimum.y;zahn::Parameters p;p.operation=zahn::Operation::Crop;p.box={{b.minimum.x+x*0.1,b.minimum.y+y*0.1,b.minimum.z-1},{b.maximum.x-x*0.1,b.maximum.y-y*0.1,b.maximum.z+1}};
      double box[6]{p.box.minimum.x,p.box.maximum.x,p.box.minimum.y,p.box.maximum.y,p.box.minimum.z,p.box.maximum.z};boxRepresentation->SetPlaceFactor(1);boxRepresentation->PlaceWidget(box);boxWidget->On();p.box=currentBox();if(std::abs(p.box.minimum.x-box[0])>1e-10)throw std::runtime_error("Crop widget bounds changed");
      showNormals=true;editSurface(p,"Smoke crop",-1,[this,original,screenshot]{
        auto edited=document.project.scans[0].mesh;if(edited==original)throw std::runtime_error("Crop was not applied");document.history.undo();if(document.project.scans[0].mesh!=original)throw std::runtime_error("Crop undo failed");document.history.redo();
        QTemporaryDir directory;auto path=directory.filePath("edited.zahncad");zahn::saveProject(document.project,path);auto reopened=zahn::loadProject(path);if(reopened.scans[0].mesh->triangles.size()!=edited->triangles.size()||reopened.scans[0].original!=document.project.scans[0].original)throw std::runtime_error("Edited case did not reopen");zahn::saveProject(document.project,screenshot+".zahncad");
        syncScene();renderer->ResetCamera();QTimer::singleShot(300,this,[this,screenshot]{render->Render();QApplication::exit(grab().save(screenshot)?0:2);});
      });
    });});
  }
  void smokeTest(const QString& source,const QString& screenshot) {
    // Exercise cancellation through the same watcher path used by interactive import.
    runJob("Checking cancellation",[source](const zahn::Checkpoint& check){zahn::readStl(zahn::nativePath(source),check);},[]{});
    cancellation->store(true);
    auto* timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this,timer,source,screenshot]{
      if(busy)return;timer->stop();timer->deleteLater();
      if(!document.project.scans.empty()){QApplication::exit(4);return;}
      smokeLoad(source,screenshot);
    });timer->start(20);
  }
  void smokeLoad(const QString& source,const QString& screenshot) {
    loadScan(source,"Reference",1,[this,screenshot]{
      guarded([this,screenshot]{
        auto mesh=document.project.scans.at(0).mesh;
        document.setDisplay(0,false,0.5);document.history.undo();
        if(!document.project.scans[0].visible||document.project.scans[0].opacity!=1)throw std::runtime_error("Undo failed");
        document.history.redo();document.history.undo();document.removeScan(0);document.history.undo();
        if(document.project.scans[0].mesh!=mesh)throw std::runtime_error("Geometry changed in history");
        QTimer::singleShot(1500,this,[this,screenshot]{
          navigation(true);render->Render();QElapsedTimer elapsed;elapsed.start();
          for(int i=0;i<60;++i){renderer->GetActiveCamera()->Azimuth(1);render->Render();}
          qInfo("Navigation render exercise: 60 render calls in %lld ms; presented-frame FPS not measured",static_cast<long long>(elapsed.elapsed()));
          renderer->GetActiveCamera()->Azimuth(-60);navigation(false);render->Render();
          QApplication::exit(grab().save(screenshot)?0:2);
        });
      });
    });
  }
};

int main(int argc,char** argv) {
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());QApplication app(argc,argv);
  const bool stage2=argc==4&&QString::fromLocal8Bit(argv[1])=="--stage2-smoke-test";
  const bool smoke=argc==4&&QString::fromLocal8Bit(argv[1])=="--smoke-test";
  app.setApplicationName((smoke||stage2)?"ZahnCAD-build-verification":"ZahnCAD");
  auto directory=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);QDir().mkpath(directory);
  QLockFile lock(directory+"/session.lock");
  if(!lock.tryLock()){QMessageBox::information(nullptr,"ZahnCAD already running","Close the other ZahnCAD instance before opening another case.");return 1;}
  Window window;window.show();if(smoke)window.smokeTest(QString::fromLocal8Bit(argv[2]),QString::fromLocal8Bit(argv[3]));
  if(stage2)window.stage2Smoke(QString::fromLocal8Bit(argv[2]),QString::fromLocal8Bit(argv[3]));
  return app.exec();
}
