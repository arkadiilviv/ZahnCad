#include "zahn/document.hpp"
#include "zahn/display.hpp"
#include "zahn/surface.hpp"
#include "zahn/interaction.hpp"
#include "zahn/preparation.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QSpinBox>
#include <QKeySequence>
#include <QJsonObject>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QApplication>
#include <QSurfaceFormat>
#include <QMainWindow>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QButtonGroup>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QTabWidget>
#include <QToolButton>
#include <QTimer>
#include <QStandardPaths>
#include <QSysInfo>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QCloseEvent>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QAction>
#include <QUndoCommand>
#include <QVariant>
#include <QSignalBlocker>
#include <QSqlDatabase>
#include <QSqlError>
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
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkPlane.h>
#include <vtkDistanceRepresentation2D.h>
#include <vtkHandleRepresentation.h>
#include <vtkCallbackCommand.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkNew.h>
#include <vtkWeakPointer.h>
#include <vtkCellPicker.h>
#include <vtkBoxWidget2.h>
#include <vtkBoxRepresentation.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkIdList.h>
#include <set>
#include <map>
#include <optional>

enum class SmokeMode { None, Viewer, Preparation, Analysis, Crown, Reference, RecoveryWrite, RecoveryRead };

static void styleWorkbench(QApplication& app) {
  app.setStyle(QStyleFactory::create("Fusion"));
  QPalette palette=app.palette();
  palette.setColor(QPalette::Window,QColor("#151A21"));
  palette.setColor(QPalette::Base,QColor("#151A21"));
  palette.setColor(QPalette::AlternateBase,QColor("#242C36"));
  palette.setColor(QPalette::Button,QColor("#242C36"));
  palette.setColor(QPalette::WindowText,QColor("#E5EBF0"));
  palette.setColor(QPalette::Text,QColor("#E5EBF0"));
  palette.setColor(QPalette::ButtonText,QColor("#E5EBF0"));
  palette.setColor(QPalette::ToolTipBase,QColor("#242C36"));
  palette.setColor(QPalette::ToolTipText,QColor("#E5EBF0"));
  palette.setColor(QPalette::Highlight,QColor("#38C9B8"));
  palette.setColor(QPalette::HighlightedText,QColor("#151A21"));
  palette.setColor(QPalette::Disabled,QPalette::Text,QColor("#8995A3"));
  palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor("#8995A3"));
  palette.setColor(QPalette::Disabled,QPalette::WindowText,QColor("#8995A3"));
  app.setPalette(palette);
  app.setStyleSheet(R"(
    QMenuBar { background: #242C36; border-bottom: 1px solid #343F4B; padding: 3px 7px; }
    QMenuBar::item { background: transparent; padding: 5px 9px; }
    QMenuBar::item:selected, QMenu::item:selected { background: #38C9B8; color: #151A21; }
    QMenu { background: #242C36; border: 1px solid #343F4B; padding: 4px; }
    QMenu::item { padding: 6px 28px 6px 22px; }
    QMenu::separator { background: #343F4B; height: 1px; margin: 4px 8px; }
    QFrame[panel="true"], QTabWidget::pane {
      background: #242C36; border: 1px solid #343F4B; border-radius: 9px;
    }
    QFrame#viewportFrame { background: #151A21; }
    QLabel[heading="true"] { font-weight: 600; font-size: 14px; }
    QLabel[muted="true"] { color: #AEBBC8; }
    QPushButton, QToolButton, QComboBox {
      background: #242C36; border: 1px solid #343F4B; border-radius: 6px;
      padding: 6px 9px;
    }
    QPushButton:hover:enabled, QToolButton:hover:enabled, QComboBox:hover:enabled { background: #303B48; }
    QPushButton:focus, QToolButton:focus, QComboBox:focus,
    QListWidget:focus { border: 1px solid #38C9B8; }
    QToolButton:checked, QToolButton[primary="true"]:enabled {
      background: #38C9B8; border-color: #38C9B8; color: #151A21; font-weight: 600;
    }
    QToolButton[primary="true"]:hover:enabled { background: #64D7C9; }
    QPushButton[stage="true"] { padding: 0; border-radius: 8px; }
    QPushButton[stage="true"]:checked {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #205650, stop:1 #21746A);
      border-color: #38C9B8;
    }
    QLabel[stageNumber="true"] {
      border: 1px solid #8995A3; border-radius: 14px; color: #AEBBC8;
      font-size: 16px; font-weight: 600;
    }
    QPushButton[stage="true"]:checked QLabel[stageNumber="true"] {
      border-color: #A1F3E9; color: #E5EBF0;
    }
    QPushButton[stage="true"]:checked QLabel[muted="true"] { color: #D2EFEB; }
    QPushButton[tooth="true"][marked="true"] { border-color: #38C9B8; }
    QPushButton[tooth="true"]:checked { background: #185D57; border: 1px solid #38C9B8; }
    QPushButton[tooth="true"]:disabled { color: #778491; background: #1C232B; }
    QListWidget {
      background: #151A21; border: 1px solid #343F4B; border-radius: 6px; outline: 0;
      padding: 3px;
    }
    QListWidget::item { padding: 7px 5px; border-radius: 4px; }
    QListWidget::item:hover { background: #303B48; }
    QListWidget::item:selected { background: #205650; color: #E5EBF0; }
    QTabBar::tab { padding: 9px 13px; color: #AEBBC8; border-bottom: 2px solid transparent; }
    QTabBar::tab:selected { color: #E5EBF0; border-bottom-color: #38C9B8; font-weight: 600; }
    QSlider::groove:horizontal { height: 4px; background: #343F4B; border-radius: 2px; }
    QSlider::sub-page:horizontal { background: #38C9B8; border-radius: 2px; }
    QSlider::handle:horizontal {
      background: #38C9B8; border: 2px solid #E5EBF0; width: 10px; margin: -5px 0; border-radius: 7px;
    }
    QSlider::handle:horizontal:disabled { background: #343F4B; border-color: #8995A3; }
    QStatusBar { background: #242C36; border-top: 1px solid #343F4B; min-height: 26px; }
    QStatusBar::item { border: none; }
    QStatusBar QLabel { padding: 0 8px; }
  )");
}

static const QColor brushHighlight("#38C9B8");
static const QColor marginHighlight("#FFC04D");

class ViewportOverlay : public QWidget {
  QColor border;
public:
  QPointF center;
  double radius=30;
  bool brushActive=false,inside=false,erase=false;
  explicit ViewportOverlay(QWidget* parent):QWidget(parent){
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
  }
  void setBorderColor(const QColor& color) {
    if(border==color)return;
    border=color;
    update();
  }
protected:
  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if(border.isValid()){
      painter.setPen(QPen(border,4));
      painter.drawRoundedRect(QRectF(rect()).adjusted(2,2,-2,-2),4,4);
    }
    if(brushActive&&inside){
      auto color=erase?QColor(255,140,30):border;
      painter.setPen(QPen(color,2));
      color.setAlpha(28);
      painter.setBrush(color);
      painter.drawEllipse(center,radius,radius);
    }
  }
};

class MarginPointEdit final : public QUndoCommand {
  std::vector<zahn::Vec3>& points;
  size_t index;
  std::optional<zahn::Vec3> before, after;

  void apply(const std::optional<zahn::Vec3>& oldPoint,
             const std::optional<zahn::Vec3>& newPoint) {
    auto position=points.begin()+index;
    if(oldPoint&&newPoint)*position=*newPoint;
    else if(newPoint)points.insert(position,*newPoint);
    else points.erase(position);
  }
public:
  MarginPointEdit(std::vector<zahn::Vec3>& points, size_t index,
                  std::optional<zahn::Vec3> before, std::optional<zahn::Vec3> after)
      : points(points), index(index), before(before), after(after) {}
  void undo() override {apply(after,before);}
  void redo() override {apply(before,after);}
};

class Window : public QMainWindow {
  zahn::Document document;
  QString path, directory;
  const SmokeMode smokeMode;
  QListWidget* scans;
  QSlider* opacity;
  QWidget* workbench;
  QListWidget* margins;
  QLabel *scanHeading, *marginHeading, *cornersLabel, *trianglesLabel, *sizeLabel;
  QLabel *activeToolLabel, *selectionLabel, *toolHeading;
  QTabWidget* inspector;
  QAction *undoAction, *redoAction;
  QVTKOpenGLNativeWidget* view;
  vtkNew<vtkGenericOpenGLRenderWindow> render;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkOrientationMarkerWidget> orientation;
  vtkNew<vtkPlane> plane;
  vtkNew<vtkDistanceRepresentation2D> distance;
  bool measuring = false;
  int rulerPoints = 0;
  vtkWeakPointer<vtkActor> rulerActor;
  int rulerHistoryIndex=-1;
  bool rulerOnSingleScan=false, referenceReopened=false;
  vtkNew<vtkBoxWidget2> boxWidget;
  vtkNew<vtkBoxRepresentation> boxRepresentation;
  vtkNew<vtkActor> selectionActor;
  vtkNew<vtkActor> normalsActor;
  vtkNew<vtkActor> traceActor;
  vtkNew<vtkActor> otherMarginsActor;
  vtkNew<vtkActor> undercutActor;
  std::shared_ptr<const zahn::Mesh> analyzedMesh;
  enum class PointTool { None, Margin, MovingLandmarks, FixedLandmarks };
  PointTool pointTool=PointTool::None;
  int toolHistoryIndex=0;
  int movingRow=-1, fixedRow=-1, draftTooth=0;
  std::vector<zahn::Vec3> draft, movingLandmarks;
  QUndoStack marginHistory;
  bool editingSavedMargin=false;
  const zahn::Mesh* selectionMesh=nullptr;
  using VertexKey=std::array<double,3>;
  std::set<VertexKey> selectedVertices, sculptVertices;
  bool sculpting=false;
  QAction* sculptAction=nullptr;
  bool picking=false,showNormals=false,showColors=true;
  vtkNew<zahn::InteractionStyle> inputStyle;
  ViewportOverlay* viewportOverlay=nullptr;
  QSpinBox* brushRadius=nullptr;
  QAction* brushAction=nullptr;
  QAction *navigationAction, *boxAction, *measureAction, *clipAction;
  QAction *removeDraftPointAction, *applyMarginAction;
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
  bool busy = false, clip = false, autosaving = false, closePending = false;
  quint64 recoveryGeneration = 0;
  std::shared_ptr<std::atomic_bool> cancellation;
  QSqlDatabase db;

  void rememberCase() {
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO cases(path,name) VALUES(?,?)");
    query.addBindValue(path);
    query.addBindValue(document.project.id);
    if(!query.exec())statusBar()->showMessage("Could not update Recent Cases: "+query.lastError().text(),6000);
  }
  void reportFailure(const QString& label, const QString& error) {
    if (smokeMode!=SmokeMode::None) {
      qCritical("%s", qPrintable(error));
      QApplication::exit(5);
      return;
    }
    QMessageBox::critical(this, label, error);
  }
  template<class F> void guarded(F fn) {
    try { fn(); } catch(const std::exception& e) {
      reportFailure("Operation failed", QString::fromUtf8(e.what()));
    }
  }
  void refreshTitle() {
    setWindowTitle(document.project.id + (document.dirty() ? " *" : "") + " — ZahnCAD Research");
    refreshUndoActions();
  }
  QUndoStack& activeHistory() {
    return pointTool==PointTool::Margin?marginHistory:document.history;
  }
  void refreshUndoActions() {
    auto& history=activeHistory();
    undoAction->setEnabled(!busy&&history.canUndo());
    redoAction->setEnabled(!busy&&history.canRedo());
  }
  void stepHistory(bool redo) {
    auto& history=activeHistory();
    if(redo)history.redo();else history.undo();
    if(pointTool==PointTool::Margin){
      updateTrace();
      render->Render();
    }
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
    menuBar()->setEnabled(false); workbench->setEnabled(false);refreshTitle();
    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
      [this,watcher,progress,flag,finish,label] {
        const QString error = watcher->result();
        const bool cancelled = flag->load();
        progress->reset(); progress->deleteLater(); watcher->deleteLater();
        busy = false; cancellation.reset();
        menuBar()->setEnabled(true); workbench->setEnabled(true);
        if(!cancelled && error.isEmpty()) guarded(finish);
        else if(!cancelled) reportFailure(label,error);
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
  void autosave(std::function<void()> after = {}) {
    if(busy||autosaving||!document.dirty())return;
    autosaving=true;auto snapshot=document.project;auto dir=directory;auto generation=recoveryGeneration;
    auto* watcher=new QFutureWatcher<QString>(this);
    connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,generation,after]{
      auto error=watcher->result();watcher->deleteLater();autosaving=false;
      if(generation!=recoveryGeneration||!document.dirty())clearRecovery();
      if(!error.isEmpty())statusBar()->showMessage("Autosave failed — save the case manually: "+error);
      if(after){
        if(error.isEmpty())guarded(after);
        else reportFailure("Autosave failed",error);
      }
      if(closePending){closePending=false;QTimer::singleShot(0,this,[this]{close();});}
    });
    watcher->setFuture(QtConcurrent::run([snapshot,dir]()->QString{
      try{zahn::saveRecovery(snapshot,dir);return {};}catch(const std::exception& e){return QString::fromUtf8(e.what());}
    }));
  }
  void saveTo(const QString& destination) {
    zahn::saveProject(document.project, destination);
    path = QFileInfo(destination).absoluteFilePath();
    document.markSaved();
    clearRecovery();
    rememberCase();
    refreshTitle();
  }
  bool save(bool saveAs = false) {
    QString destination = saveAs ? QString() : path;
    if(destination.isEmpty()) destination = QFileDialog::getSaveFileName(this,"Save project",{},"ZahnCAD (*.zahncad)");
    if(destination.isEmpty()) return false;
    saveTo(destination);
    return true;
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
    item.actor->GetProperty()->SetColor(0.78,0.82,0.86);
    cache.emplace(item.geometry.source.get(),std::move(item));
  }
  void syncOpacity() {
    QSignalBlocker blocked(opacity);int i=scans->currentRow();
    if(i>=int(document.project.scans.size()))i=-1;
    opacity->setEnabled(i>=0);opacity->setValue(i>=0?int(document.project.scans[i].opacity*100):100);
  }
  void syncScene() {
    if(pointTool!=PointTool::None&&document.history.index()!=toolHistoryIndex)clearPointTool();
    int selected=scans->currentRow();
    {
      QSignalBlocker blocked(scans);
      scans->clear();
      margins->clear();
      quint64 triangles=0, bytes=0;
      for(auto actor:active)renderer->RemoveActor(actor);active.clear();
      for(const auto& scan:document.project.scans){
        triangles+=scan.mesh->triangles.size();
        bytes+=scan.original.size();
        auto found=cache.find(scan.mesh.get());if(found==cache.end())continue;
        auto& item=found->second;item.mapper->RemoveAllClippingPlanes();if(clip)item.mapper->AddClippingPlane(plane);
        item.actor->SetVisibility(scan.visible);item.actor->GetProperty()->SetOpacity(scan.opacity);
        renderer->AddActor(item.actor);active.push_back(item.actor);
        const auto tooth=scan.toothFdi?QString("Tooth %1  ·  ").arg(scan.toothFdi):QString{};
        auto* row=new QListWidgetItem(tooth+scan.role+"  ·  "+scan.name,scans);
        row->setToolTip(row->text());
        row->setCheckState(scan.visible?Qt::Checked:Qt::Unchecked);
        for(const auto& [number,points]:scan.toothMargins){
          if(points.empty())continue;
          const auto label=number?QString("Tooth %1").arg(number):QString("Unassigned");
          auto* margin=new QListWidgetItem(label+"  ·  "+scan.role+" margin  ·  "+scan.name,margins);
          margin->setToolTip(margin->text());
          margin->setData(Qt::UserRole,scans->count()-1);
          margin->setData(Qt::UserRole+1,number);
        }
      }
      if(scans->count())scans->setCurrentRow(std::clamp(selected,0,scans->count()-1));
      scanHeading->setText(QString("Scans (%1)").arg(scans->count()));
      marginHeading->setText(QString("Saved margins (%1)").arg(margins->count()));
      margins->setVisible(margins->count()>0);
      QLocale locale;
      cornersLabel->setText(locale.toString(3*triangles));
      trianglesLabel->setText(locale.toString(triangles));
      sizeLabel->setText(locale.formattedDataSize(bytes));
    }
    syncOpacity();refreshTitle();render->Render();
    updateSelection();
    // Drop display caches after their undo commands and document references expire.
    for(auto it=cache.begin();it!=cache.end();) {
      if(it->second.geometry.source.use_count()==1)it=cache.erase(it);else ++it;
    }
  }
  void navigation(bool enabled) {
    for(auto& [key,item]:cache)
      item.mapper->SetInputData(enabled&&!measuring&&pointTool==PointTool::None?item.geometry.navigation:item.geometry.full);
    if(!enabled)render->Render();
  }
  void setMeasuring(bool enabled) {
    measuring=enabled;
    rulerPoints=0;
    rulerActor=nullptr;
    rulerOnSingleScan=false;
    distance->VisibilityOff();
    refreshToolUi();
    navigation(false);
    if(enabled)statusBar()->showMessage("Click two points on visible scans to measure their distance",6000);
  }
  bool measureAt(int x,int y) {
    if(pointTool!=PointTool::None){
      guarded([&]{pickToolPoint(x,y);});
      return true;
    }
    if(!measuring)return false;
    if(busy)return true;
    vtkNew<vtkCellPicker> picker;
    picker->PickFromListOn();
    picker->SetTolerance(0);
    for(const auto& actor:active)
      if(actor->GetVisibility()&&actor->GetProperty()->GetOpacity()>0)picker->AddPickList(actor);
    if(!picker->Pick(x,y,0,renderer)){
      statusBar()->showMessage("Choose a point on a visible scan",4000);
      return true;
    }
    double point[3];
    picker->GetPickPosition(point);
    if(rulerPoints==1){
      distance->SetPoint2WorldPosition(point);
      distance->VisibilityOn();
      rulerPoints=2;
      rulerOnSingleScan=rulerActor.GetPointer()==picker->GetActor()&&rulerHistoryIndex==document.history.index();
    }else{
      distance->SetPoint1WorldPosition(point);
      distance->VisibilityOff();
      rulerPoints=1;
      rulerActor=picker->GetActor();
      rulerHistoryIndex=document.history.index();
      statusBar()->showMessage("Choose the second measurement point",4000);
    }
    render->Render();
    return true;
  }
  static double readingSpread(const zahn::ReferenceMeasurement& reference) {
    const auto& values=reference.readings;
    auto [minimum,maximum]=std::minmax_element(values.begin(),values.end(),[](const auto& a,const auto& b){
      return a.measuredMm<b.measuredMm;
    });
    return maximum->measuredMm-minimum->measuredMm;
  }
  void setupReferenceMeasurement() {
    if(document.project.scans.empty())throw std::runtime_error("Import a scan before setting up a reference measurement");
    const auto existing=document.project.reference.value_or(zahn::ReferenceMeasurement{});
    QDialog dialog(this);
    dialog.setObjectName("reference-measurement");
    dialog.setWindowTitle("Reference measurement");
    auto* form=new QFormLayout(&dialog);
    auto textField=[&](const QString& label,const QString& value,const QString& name,int length){
      auto* field=new QLineEdit(value);
      field->setObjectName(name);
      field->setMaxLength(length);
      form->addRow(label,field);
      return field;
    };
    auto numberField=[&](const QString& label,double value,double minimum,const QString& name){
      auto* field=new QDoubleSpinBox;
      field->setObjectName(name);
      field->setDecimals(6);
      field->setRange(minimum,1e8);
      field->setSuffix(" mm");
      field->setSpecialValueText("Set value");
      field->setValue(std::max(value,minimum));
      form->addRow(label,field);
      return field;
    };
    auto* landmarks=textField("&Landmarks",existing.landmarks,"landmarks",1024);
    auto* method=textField("Reference &method",existing.method,"method",1024);
    auto* expected=numberField("Reference &distance",existing.expectedMm,0,"expected");
    auto* tolerance=numberField("&Tolerance",document.project.reference?existing.toleranceMm:-0.000001,-0.000001,"tolerance");
    auto* notes=textField("Reviewer &notes",existing.notes,"notes",2048);
    if(!existing.readings.empty()){
      for(auto* field:{landmarks,method})field->setReadOnly(true);
      for(auto* field:{expected,tolerance})field->setReadOnly(true);
      QString result=QString("%1 recorded").arg(existing.readings.size());
      if(existing.readings.size()>1)result+=QString(" · Spread %1 mm").arg(readingSpread(existing),0,'f',3);
      form->addRow("Readings",new QLabel(result));
    }
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
    form->addRow(buttons);
    auto validate=[&]{buttons->button(QDialogButtonBox::Save)->setEnabled(
      !landmarks->text().trimmed().isEmpty()&&!method->text().trimmed().isEmpty()&&expected->value()>0&&tolerance->value()>=0);};
    connect(landmarks,&QLineEdit::textChanged,&dialog,validate);
    connect(method,&QLineEdit::textChanged,&dialog,validate);
    connect(expected,&QDoubleSpinBox::valueChanged,&dialog,validate);
    connect(tolerance,&QDoubleSpinBox::valueChanged,&dialog,validate);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    validate();
    if(dialog.exec()!=QDialog::Accepted)return;
    auto reference=existing;
    if(existing.readings.empty()){
      reference.landmarks=landmarks->text().trimmed();
      reference.method=method->text().trimmed();
      reference.expectedMm=expected->value();
      reference.toleranceMm=tolerance->value();
    }
    reference.notes=notes->text().trimmed();
    document.setReference(std::move(reference),"Set up reference measurement");
    statusBar()->showMessage("Measure the landmarks, then choose Analyze → Record ruler reading.",8000);
  }
  void recordReferenceReading() {
    if(!document.project.reference)throw std::runtime_error("Set up a reference measurement first");
    const auto& scans=document.project.scans;
    if(rulerPoints!=2||!rulerOnSingleScan||rulerHistoryIndex!=document.history.index())
      throw std::runtime_error("Pick both landmarks on the same scan with the ruler before recording");
    const auto scan=std::find_if(scans.begin(),scans.end(),[this](const auto& item){
      const auto display=cache.find(item.mesh.get());
      return display!=cache.end()&&display->second.actor.GetPointer()==rulerActor.GetPointer();
    });
    if(scan==scans.end()||scan->mesh!=scan->sourceMesh||scan->original.isEmpty())
      throw std::runtime_error("Record reference measurements on the original imported scan");
    auto reference=*document.project.reference;
    if(reference.readings.size()>=zahn::ReferenceMeasurement::readingLimit)throw std::runtime_error("Reference reading limit reached");
    const auto sourceHash=QString::fromLatin1(QCryptographicHash::hash(scan->original,QCryptographicHash::Sha256).toHex());
    if(!reference.readings.empty()&&(sourceHash!=reference.readings.front().sourceSha256||scan->scale!=reference.readings.front().scaleToMm))
      throw std::runtime_error("Use the same source scan and import scale as the first reading");
    distance->BuildRepresentation();
    const double measured=distance->GetDistance(), error=measured-reference.expectedMm;
    if(!std::isfinite(measured)||measured<0)throw std::runtime_error("The ruler did not produce a finite distance");
    reference.readings.push_back({scan->name.left(1024),scan->role.left(1024),sourceHash,
      QApplication::applicationVersion(),QSysInfo::prettyProductName().left(256),
      QDateTime::currentDateTimeUtc().toString(Qt::ISODate),scan->scale,measured,referenceReopened});
    QString report=QString("Reading %1: %2 mm · Error %3 mm · %4")
      .arg(reference.readings.size()).arg(measured,0,'f',3).arg(error,0,'f',3)
      .arg(std::abs(error)<=reference.toleranceMm?"Within tolerance":"Outside tolerance");
    if(reference.readings.size()>1)report+=QString(" · Spread %1 mm").arg(readingSpread(reference),0,'f',3);
    document.setReference(std::move(reference),"Record reference reading");
    rulerOnSingleScan=false;
    statusBar()->showMessage(report,10000);
  }
  void exportReferenceCsv(const QString& file) {
    const auto& reference=*document.project.reference;
    auto quote=[](QString text){
      const auto trimmed=text.trimmed();
      if(!trimmed.isEmpty()&&QStringLiteral("=+-@").contains(trimmed.front()))text.prepend('\'');
      text.replace('"',"\"\"");
      return '"'+text+'"';
    };
    auto number=[](double value){return QString::number(value,'g',17);};
    QByteArray csv="case_code,landmarks,reference_method,reviewer_notes,reference_mm,tolerance_mm,attempt,phase,";
    csv+="scan,scan_role,scale_to_mm,measured_mm,error_mm,within_tolerance,repeat_spread_mm,source_sha256,app_version,platform,captured_utc\r\n";
    const auto spread=reference.readings.size()>1?number(readingSpread(reference)):QString{};
    for(size_t i=0;i<reference.readings.size();++i){
      const auto& reading=reference.readings[i];
      const double error=reading.measuredMm-reference.expectedMm;
      const QStringList columns{quote(document.project.id),quote(reference.landmarks),quote(reference.method),
        quote(reference.notes),number(reference.expectedMm),number(reference.toleranceMm),QString::number(i+1),
        reading.afterReopen?"after_reopen":"initial_session",quote(reading.scanName),quote(reading.scanRole),
        number(reading.scaleToMm),number(reading.measuredMm),number(error),
        std::abs(error)<=reference.toleranceMm?"yes":"no",spread,quote(reading.sourceSha256),
        quote(reading.appVersion),quote(reading.platform),quote(reading.capturedUtc)};
      csv+=columns.join(',').toUtf8()+"\r\n";
    }
    QSaveFile output(file);
    if(!output.open(QIODevice::WriteOnly)||output.write(csv)!=csv.size()||!output.commit())
      throw std::runtime_error("Could not save the reference measurement CSV");
  }
  void saveScreenshot(const QString& file) {
    navigation(false);
    QSaveFile output(file);
    if(!output.open(QIODevice::WriteOnly)||!view->grabFramebuffer().save(&output,"PNG")||!output.commit())
      throw std::runtime_error("Could not save screenshot");
  }
  void updateSelection() {
    int i=scans->currentRow();auto mesh=i>=0&&size_t(i)<document.project.scans.size()?document.project.scans[i].mesh:std::shared_ptr<const zahn::Mesh>();
    if(mesh.get()!=selectionMesh){selectedVertices.clear();selectionMesh=mesh.get();boxWidget->Off();setBrushActive(false);}
    renderer->RemoveActor(selectionActor);renderer->RemoveActor(normalsActor);
    updateTrace();
    if(analyzedMesh!=mesh)clearInsertion();
    else if(mesh){
      const auto& scan=document.project.scans[i];
      undercutActor->SetVisibility(scan.visible&&scan.opacity>0);
      if(auto* mapper=undercutActor->GetMapper()){
        mapper->RemoveAllClippingPlanes();
        if(clip)mapper->AddClippingPlane(plane);
      }
    }
    if(!mesh){render->Render();return;}
    auto found=cache.find(mesh.get());if(found==cache.end())return;
    const auto& highlighted=sculpting?sculptVertices:selectedVertices;
    if(!highlighted.empty()){
      vtkNew<vtkPoints> points;points->SetDataTypeToDouble();vtkNew<vtkCellArray> cells;for(auto vertex:highlighted){auto id=points->InsertNextPoint(vertex.data());cells->InsertNextCell(1,&id);}
      vtkNew<vtkPolyData> data;data->SetPoints(points);data->SetVerts(cells);
      vtkNew<vtkPolyDataMapper> mapper;mapper->SetInputData(data);mapper->ScalarVisibilityOff();mapper->SetResolveCoincidentTopologyToPolygonOffset();mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1,-1);
      selectionActor->SetMapper(mapper);selectionActor->GetProperty()->SetColor(1,0.45,0.05);selectionActor->GetProperty()->SetPointSize(7);selectionActor->PickableOff();renderer->AddActor(selectionActor);
    }
    if(showNormals){vtkNew<vtkPoints> points;points->SetDataTypeToDouble();vtkNew<vtkCellArray> lines;size_t stride=std::max(size_t(1),mesh->triangles.size()/2000);
      for(size_t face=0;face<mesh->triangles.size();face+=stride){auto v=mesh->triangles[face].v;zahn::Vec3 a{v[1].x-v[0].x,v[1].y-v[0].y,v[1].z-v[0].z},b{v[2].x-v[0].x,v[2].y-v[0].y,v[2].z-v[0].z};zahn::Vec3 n{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};double length=std::hypot(n.x,n.y,n.z);if(length<1e-16)continue;zahn::Vec3 center{(v[0].x+v[1].x+v[2].x)/3,(v[0].y+v[1].y+v[2].y)/3,(v[0].z+v[1].z+v[2].z)/3};vtkIdType ids[2]{points->InsertNextPoint(center.x,center.y,center.z),points->InsertNextPoint(center.x+0.5*n.x/length,center.y+0.5*n.y/length,center.z+0.5*n.z/length)};lines->InsertNextCell(2,ids);}
      vtkNew<vtkPolyData> data;data->SetPoints(points);data->SetLines(lines);vtkNew<vtkPolyDataMapper> mapper;mapper->SetInputData(data);normalsActor->SetMapper(mapper);normalsActor->GetProperty()->SetColor(0.1,0.9,1);normalsActor->PickableOff();renderer->AddActor(normalsActor);
    }
    render->Render();
  }
  void refreshToolUi() {
    const bool hasBox=boxWidget->GetEnabled();
    const bool navigating=!picking&&!hasBox&&!measuring&&pointTool==PointTool::None;
    navigationAction->setChecked(navigating);
    boxAction->setChecked(hasBox);
    measureAction->setChecked(measuring);
    clipAction->setChecked(clip);
    const bool tracing=pointTool==PointTool::Margin;
    viewportOverlay->setBorderColor(tracing?marginHighlight:sculpting?QColor("#BE91FF"):picking?brushHighlight:QColor{});
    removeDraftPointAction->setEnabled(tracing&&!draft.empty());
    applyMarginAction->setEnabled(tracing&&draft.size()>=3);
    refreshUndoActions();
    QString name="Navigate";
    if(pointTool==PointTool::Margin)name=draftTooth?QString("Margin trace · Tooth %1").arg(draftTooth):QString("Margin trace");
    else if(pointTool!=PointTool::None)name="Landmark registration";
    else if(sculpting)name="Freeform smooth";
    else if(picking)name="Vertex brush";
    else if(hasBox)name="Crop / selection box";
    else if(measuring)name="Distance measurement";
    if(toolHeading->text()!=name){
      toolHeading->setText(name);
      activeToolLabel->setText("Active tool · "+name);
    }
    const auto count=selectedVertices.size();
    selectionLabel->setText(QString("Selected · %1 %2")
      .arg(QLocale().toString(quint64(count)),count==1?"vertex":"vertices"));
  }
  void setBrushActive(bool enabled,bool sculpt=false){
    sculpting=enabled&&sculpt;
    sculptVertices.clear();
    if(sculptAction)sculptAction->setChecked(sculpting);
    picking=enabled;inputStyle->setBrush(enabled);strokeDepth.clear();
    if(brushRadius)inputStyle->radius=brushRadius->value()*view->devicePixelRatioF();
    if(brushAction)brushAction->setChecked(enabled&&!sculpt);
    viewportOverlay->brushActive=enabled;
    if(!enabled)viewportOverlay->inside=false;
    viewportOverlay->update();
    if(enabled){boxWidget->Off();setMeasuring(false);}
    refreshToolUi();
  }
  void toggleBrush(bool sculpt=false){
    cancelPointTool();
    if(picking&&sculpting==sculpt){setBrushActive(false);updateSelection();return;}
    setBrushActive(false);
    const int row=scans->currentRow();
    if(row<0)return;
    const auto& scan=document.project.scans.at(row);
    if(sculpt&&(scan.role!="Restoration candidate"||!scan.visible||scan.opacity<=0||scan.hasMargins()))
      throw std::runtime_error("Select a visible restoration candidate without traced margins");
    auto mesh=scan.mesh;
    if(cache.at(mesh.get()).brush){setBrushActive(true,sculpt);updateSelection();return;}
    auto data=cache.at(mesh.get()).geometry.full;auto index=std::make_shared<zahn::BrushIndex>();
    runJob("Preparing vertex brush",[data,index](const zahn::Checkpoint& check){*index=zahn::prepareBrush(data,check);},[this,mesh,index,sculpt]{cache.at(mesh.get()).brush=index;setBrushActive(true,sculpt);updateSelection();});
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
      VertexKey key{point[0],point[1],point[2]};if(sculpting)sculptVertices.insert(key);else if(erase)selectedVertices.erase(key);else selectedVertices.insert(key);
    }
  }
  void finishSculptStroke(){
    if(!sculpting||sculptVertices.empty()||busy)return;
    const int row=scans->currentRow();
    const auto scan=document.project.scans.at(row);
    if(scan.role!="Restoration candidate"||!scan.visible||scan.opacity<=0)return;
    auto points=std::move(sculptVertices);
    auto display=std::make_shared<zahn::DisplayMesh>();
    auto index=std::make_shared<zahn::BrushIndex>();
    runJob("Smoothing brush stroke",[mesh=scan.mesh,points=std::move(points),display,index](const zahn::Checkpoint& check){
      std::map<VertexKey,std::map<VertexKey,unsigned>> neighbors;
      size_t processed=0;
      for(const auto& face:mesh->triangles){
        if(processed++%1024==0)check();
        for(int i=0;i<3;++i){
          const auto p=face.v[i];
          VertexKey key{p.x,p.y,p.z};
          if(!points.contains(key))continue;
          auto& edges=neighbors[key];
          for(int j=1;j<=2;++j){
            const auto q=face.v[(i+j)%3];
            ++edges[{q.x,q.y,q.z}];
          }
        }
      }
      std::map<VertexKey,zahn::Vec3> moved;
      for(const auto& [key,edges]:neighbors){
        // Keep boundary and non-manifold edges fixed.
        if(edges.empty()||std::any_of(edges.begin(),edges.end(),[](const auto& edge){return edge.second!=2;}))continue;
        zahn::Vec3 delta{};
        for(const auto& [q,uses]:edges){
          delta.x+=q[0]-key[0];delta.y+=q[1]-key[1];delta.z+=q[2]-key[2];
        }
        const double weight=0.12/edges.size();
        if(std::hypot(delta.x,delta.y,delta.z)*weight>1e-12)
          moved[key]={key[0]+weight*delta.x,key[1]+weight*delta.y,key[2]+weight*delta.z};
      }
      if(moved.empty())return;
      auto result=std::make_shared<zahn::Mesh>(*mesh);
      for(auto& face:result->triangles){
        if(processed++%1024==0)check();
        for(auto& p:face.v){
          auto it=moved.find({p.x,p.y,p.z});
          if(it!=moved.end())p=it->second;
        }
      }
      *display=zahn::prepareDisplay(result,check);
      *index=zahn::prepareBrush(display->full,check);
    },[this,row,scan,display,index]{
      if(!display->source){updateSelection();return;}
      auto edited=scan;
      edited.mesh=display->source;
      commitGeometry(row,edited,std::move(*display),"Freeform smooth",{});
      cache.at(edited.mesh.get()).brush=index;
      setBrushActive(true,true);
      updateSelection();
    });
  }
  zahn::Box currentBox(){if(!boxWidget->GetEnabled())throw std::runtime_error("Enable the crop/selection box first");vtkNew<vtkPolyData> data;boxRepresentation->GetPolyData(data);auto* b=data->GetBounds();return {{b[0],b[2],b[4]},{b[1],b[3],b[5]}};}
  int captureRow() const {
    return pointTool==PointTool::FixedLandmarks?fixedRow:movingRow;
  }
  void drawTraces(vtkActor* actor,const std::vector<const std::vector<zahn::Vec3>*>& traces,bool closed) {
    vtkNew<vtkPoints> coordinates;
    coordinates->SetDataTypeToDouble();
    vtkNew<vtkCellArray> markers,strip;
    for(const auto* points:traces){
      const auto start=coordinates->GetNumberOfPoints();
      for(const auto& p:*points){
        const auto id=coordinates->InsertNextPoint(p.x,p.y,p.z);
        markers->InsertNextCell(1,&id);
      }
      if(points->size()>1){
        const bool loop=closed&&points->size()>=3;
        strip->InsertNextCell(int(points->size())+int(loop));
        for(vtkIdType i=0;i<vtkIdType(points->size());++i)strip->InsertCellPoint(start+i);
        if(loop)strip->InsertCellPoint(start);
      }
    }
    if(!coordinates->GetNumberOfPoints())return;
    vtkNew<vtkPolyData> data;
    data->SetPoints(coordinates);
    data->SetVerts(markers);
    data->SetLines(strip);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(data);
    mapper->SetResolveCoincidentTopologyToPolygonOffset();
    mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-2,-2);
    mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-2);
    if(clip)mapper->AddClippingPlane(plane);
    actor->SetMapper(mapper);
    actor->VisibilityOn();
  }
  void updateTrace() {
    refreshToolUi();
    traceActor->VisibilityOff();
    otherMarginsActor->VisibilityOff();
    const int row=pointTool==PointTool::None?scans->currentRow():captureRow();
    if(row<0||size_t(row)>=document.project.scans.size())return;
    const auto& scan=document.project.scans[row];
    if(pointTool==PointTool::MovingLandmarks||pointTool==PointTool::FixedLandmarks){
      for(size_t i=0;i<document.project.scans.size();++i){
        const auto& item=document.project.scans[i];
        auto& actor=cache.at(item.mesh.get()).actor;
        actor->SetVisibility(int(i)==row);
        if(int(i)==row)actor->GetProperty()->SetOpacity(1);
      }
      drawTraces(traceActor,{&draft},false);
      return;
    }
    const int tooth=pointTool==PointTool::Margin?draftTooth:scan.toothFdi;
    int activeMargin=-1;
    for(int i=0;i<margins->count();++i){
      const auto* item=margins->item(i);
      if(item->data(Qt::UserRole).toInt()==row&&item->data(Qt::UserRole+1).toInt()==tooth){activeMargin=i;break;}
    }
    margins->setCurrentRow(activeMargin);
    if(!scan.visible||scan.opacity<=0)return;
    std::vector<const std::vector<zahn::Vec3>*> others;
    if(pointTool==PointTool::Margin)drawTraces(traceActor,{&draft},true);
    for(const auto& [number,points]:scan.toothMargins){
      if(points.empty())continue;
      if(number!=tooth)others.push_back(&points);
      else if(pointTool==PointTool::None)drawTraces(traceActor,{&points},true);
    }
    drawTraces(otherMarginsActor,others,true);
  }
  void clearPointTool() {
    pointTool=PointTool::None;
    movingRow=fixedRow=-1;
    draftTooth=0;
    marginHistory.clear();
    editingSavedMargin=false;
    draft.clear();
    movingLandmarks.clear();
  }
  void cancelPointTool() {
    if(pointTool==PointTool::None)return;
    clearPointTool();
    syncScene();
    statusBar()->clearMessage();
  }
  void focusCapture() {
    scans->setCurrentRow(captureRow());
    updateTrace();
    renderer->ResetCamera(cache.at(document.project.scans[captureRow()].mesh.get()).geometry.full->GetBounds());
    render->Render();
    statusBar()->showMessage(pointTool==PointTool::MovingLandmarks?
      "Pick three landmarks on the moving scan":
      "Pick the same three landmarks on the fixed scan, in the same order");
  }
  void startMargin() {
    const int row=scans->currentRow();
    if(row<0||pointTool==PointTool::Margin)return;
    cancelPointTool();
    const auto& scan=document.project.scans[row];
    if(!scan.visible||scan.opacity<=0)throw std::runtime_error("Show the selected scan before tracing its margin");
    setBrushActive(false);
    setMeasuring(false);
    boxWidget->Off();
    movingRow=row;
    draftTooth=scan.toothFdi;
    if(const auto saved=scan.toothMargins.find(draftTooth);saved!=scan.toothMargins.end())draft=saved->second;
    editingSavedMargin=draft.size()>=3;
    toolHistoryIndex=document.history.index();
    pointTool=PointTool::Margin;
    updateTrace();
    render->Render();
    statusBar()->showMessage(editingSavedMargin?
      "Click a margin segment to insert a point. Shift+click moves a point. Apply margin saves edits.":
      "Click the scan to add points in order. Shift+click moves a point. Apply margin saves the trace.");
  }
  void applyMargin(bool clear=false) {
    const int row=pointTool==PointTool::Margin?captureRow():scans->currentRow();
    if(row<0)return;
    if(!clear&&(pointTool!=PointTool::Margin||draft.size()<3))
      throw std::runtime_error("Trace at least three surface points first");
    auto scan=document.project.scans[row];
    const int tooth=pointTool==PointTool::Margin?draftTooth:scan.toothFdi;
    if(clear){
      const auto saved=scan.toothMargins.find(tooth);
      if(saved!=scan.toothMargins.end()){
        if(tooth)saved->second.clear();
        else scan.toothMargins.erase(saved);
      }else if(pointTool!=PointTool::Margin)return;
    }else scan.toothMargins[tooth]=std::move(draft);
    cancelPointTool();
    document.replaceScan(size_t(row),std::move(scan),clear?"Clear margin":"Apply margin");
    syncScene();
    statusBar()->showMessage(clear?"Saved margin cleared":"Margin applied to the case",4000);
  }
  void activateTooth(int row,int tooth) {
    auto scan=document.project.scans.at(size_t(row));
    if(scan.toothFdi==tooth)return;
    if(tooth&&!scan.toothMargins.contains(tooth)){
      // The original workflow allowed tracing before assigning the first tooth.
      if(scan.toothMargins.size()==1&&scan.toothMargins.contains(0)){
        auto unassigned=scan.toothMargins.extract(0);
        unassigned.key()=tooth;
        scan.toothMargins.insert(std::move(unassigned));
      }else scan.toothMargins.try_emplace(tooth);
    }
    scan.toothFdi=tooth;
    document.replaceScan(size_t(row),std::move(scan),"Select tooth markup");
    syncScene();
  }
  void selectTooth() {
    const int row=scans->currentRow();
    if(row<0)throw std::runtime_error("Select a scan in the scene first");
    if(pointTool!=PointTool::None)throw std::runtime_error("Finish or cancel the active point tool first");
    auto scan=document.project.scans.at(size_t(row));
    if(scan.format=="zmesh")throw std::runtime_error("Select the preparation scan to assign the tooth");

    QDialog dialog(this);
    dialog.setObjectName("tooth-chart");
    dialog.setWindowTitle("Select tooth markup");
    auto* layout=new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("Permanent teeth (FDI) · patient right to left"));
    layout->addWidget(new QLabel("Choose a tooth to add or switch to it. Outlined teeth already have markup."));
    auto* chart=new QGridLayout;
    layout->addLayout(chart);
    chart->addWidget(new QLabel("Upper"),0,0);
    chart->addWidget(new QLabel("Lower"),1,0);
    int selected=scan.toothFdi;
    bool remove=false;
    for(int arch=0;arch<2;++arch){
      for(int position=0;position<16;++position){
        const int tooth=position<8?(arch==0?18:48)-position:(arch==0?21:31)+position-8;
        auto* button=new QPushButton(QString::number(tooth));
        button->setProperty("tooth",true);
        button->setProperty("marked",scan.toothMargins.contains(tooth));
        button->setObjectName(QString("tooth-%1").arg(tooth));
        button->setAccessibleName(QString("Select tooth %1").arg(tooth));
        const auto saved=scan.toothMargins.find(tooth);
        if(saved!=scan.toothMargins.end())button->setToolTip(saved->second.empty()?"Tooth added":"Saved margin");
        button->setMinimumWidth(38);
        button->setCheckable(true);
        button->setChecked(tooth==selected);
        button->setEnabled(arch==0?scan.role!="Lower jaw":scan.role!="Upper jaw");
        if(tooth==selected&&button->isEnabled())button->setFocus();
        chart->addWidget(button,arch,position+1+int(position>=8));
        connect(button,&QPushButton::clicked,&dialog,[&dialog,&selected,tooth]{selected=tooth;dialog.accept();});
      }
    }
    chart->setColumnMinimumWidth(9,14);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel);
    auto* clear=buttons->addButton("Remove selected tooth…",QDialogButtonBox::ResetRole);
    clear->setEnabled(selected!=0);
    connect(clear,&QPushButton::clicked,&dialog,[&dialog,&remove]{remove=true;dialog.accept();});
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    layout->addWidget(buttons);
    if(dialog.exec()!=QDialog::Accepted)return;
    if(!remove){activateTooth(row,selected);return;}
    if((!scan.toothMargins.at(selected).empty()||scan.insertionAxes.contains(selected))&&
       QMessageBox::question(this,"Remove tooth markup",
        QString("Remove tooth %1 and its saved markup from this scan?").arg(selected),
        QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
    scan.removeToothMarkup(selected);
    document.replaceScan(size_t(row),std::move(scan),"Remove tooth markup");
    syncScene();
  }
  QString scanLabel(int row) const {
    const auto& scan=document.project.scans.at(size_t(row));
    const auto name=scan.toothFdi?QString("Tooth %1 · %2").arg(scan.toothFdi).arg(scan.name):scan.name;
    return QString("%1. %2 [%3]").arg(row+1).arg(name,scan.role);
  }
  std::optional<int> chooseOtherScan(int row,const QString& title,const QString& prompt) {
    QStringList choices;
    std::vector<int> rows;
    for(int i=0;i<int(document.project.scans.size());++i){
      if(i==row)continue;
      choices<<scanLabel(i);
      rows.push_back(i);
    }
    bool ok=false;
    const auto item=QInputDialog::getItem(this,title,prompt,choices,0,false,&ok);
    if(!ok)return std::nullopt;
    return rows.at(size_t(choices.indexOf(item)));
  }
  void startRegistration() {
    const int row=scans->currentRow();
    if(row<0||document.project.scans.size()<2)throw std::runtime_error("Select a moving scan and import a fixed scan first");
    cancelPointTool();
    const auto fixed=chooseOtherScan(row,"Landmark registration","Fixed scan");
    if(!fixed)return;
    setBrushActive(false);
    setMeasuring(false);
    boxWidget->Off();
    beginRegistration(row,*fixed);
  }
  void beginRegistration(int moving,int fixed) {
    movingRow=moving;
    fixedRow=fixed;
    toolHistoryIndex=document.history.index();
    pointTool=PointTool::MovingLandmarks;
    focusCapture();
  }
  void editMarginPoint(zahn::Vec3 point,int x,int y) {
    const QPointF click(x,y);
    const double scale=view->devicePixelRatioF();
    auto project=[this](zahn::Vec3 p){
      renderer->SetWorldPoint(p.x,p.y,p.z,1);
      renderer->WorldToDisplay();
      const auto* display=renderer->GetDisplayPoint();
      return QPointF(display[0],display[1]);
    };
    if(render->GetInteractor()->GetShiftKey()&&!draft.empty()){
      auto nearest=draft.size();
      double best=24*scale;
      for(size_t i=0;i<draft.size();++i){
        const auto delta=project(draft[i])-click;
        const double distance=std::hypot(delta.x(),delta.y());
        if(distance<=best){best=distance;nearest=i;}
      }
      if(nearest==draft.size()){
        statusBar()->showMessage("Shift+click within 24 pixels of a margin point",4000);
        return;
      }
      marginHistory.push(new MarginPointEdit(draft,nearest,draft[nearest],point));
      return;
    }

    if(draft.size()>=10000)throw std::runtime_error("Margin point limit reached");
    size_t insertion=draft.size();
    if(editingSavedMargin&&draft.size()>=3){
      // Screen-space proximity chooses the segment. The inserted point comes from the scan pick.
      double best=12*scale;
      bool found=false;
      auto start=project(draft.back());
      for(size_t i=0;i<draft.size();++i){
        const auto end=project(draft[i]);
        const auto offset=click-end;
        if(std::hypot(offset.x(),offset.y())<=4*scale){
          statusBar()->showMessage("Shift+click to move an existing point",4000);
          return;
        }
        const auto edge=end-start;
        const double lengthSquared=QPointF::dotProduct(edge,edge);
        if(lengthSquared>0){
          const double t=std::clamp(QPointF::dotProduct(click-start,edge)/lengthSquared,0.,1.);
          const auto delta=click-(start+t*edge);
          const double distance=std::hypot(delta.x(),delta.y());
          if(distance<best){best=distance;insertion=i==0?draft.size():i;found=true;}
        }
        start=end;
      }
      if(!found){
        statusBar()->showMessage("Click near a margin segment to insert a point",4000);
        return;
      }
    }
    marginHistory.push(new MarginPointEdit(draft,insertion,std::nullopt,point));
  }
  void pickToolPoint(int x,int y) {
    if(busy)return;
    if(document.history.index()!=toolHistoryIndex){cancelPointTool();return;}
    const auto& scan=document.project.scans.at(size_t(captureRow()));
    const auto& display=cache.at(scan.mesh.get());
    vtkNew<vtkCellPicker> picker;
    picker->PickFromListOn();
    for(const auto& actor:active)
      if(actor->GetVisibility()&&actor->GetProperty()->GetOpacity()>0)picker->AddPickList(actor);
    picker->SetTolerance(0);
    if(!picker->Pick(x,y,0,renderer)||picker->GetActor()!=display.actor){
      statusBar()->showMessage("Choose a point on the active scan surface",4000);
      return;
    }
    const auto* p=picker->GetPickPosition();
    const zahn::Vec3 point{p[0],p[1],p[2]};
    if(pointTool==PointTool::Margin)editMarginPoint(point,x,y);
    else draft.push_back(point);
    if(pointTool==PointTool::MovingLandmarks&&draft.size()==3){
      movingLandmarks=std::move(draft);
      draft.clear();
      pointTool=PointTool::FixedLandmarks;
      focusCapture();
      return;
    }
    if(pointTool==PointTool::FixedLandmarks&&draft.size()==3){registerScans();return;}
    updateTrace();
    render->Render();
  }
  bool approveGeometry(const zahn::Scan& before,const zahn::DisplayMesh& candidate,
                       const QString& label,const QString& report,bool keepSource=false) {
    auto& old=cache.at(before.mesh.get());
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(candidate.full);
    mapper->SetScalarVisibility(showColors);
    if(clip)mapper->AddClippingPlane(plane);
    vtkNew<vtkActor> preview;
    preview->SetMapper(mapper);
    preview->GetProperty()->SetColor(0.5,0.9,0.65);
    const auto traceVisible=traceActor->GetVisibility();
    const auto otherMarginsVisible=otherMarginsActor->GetVisibility();
    const auto undercutVisible=undercutActor->GetVisibility();
    if(!keepSource)old.actor->VisibilityOff();
    selectionActor->VisibilityOff();
    normalsActor->VisibilityOff();
    if(!keepSource){traceActor->VisibilityOff();otherMarginsActor->VisibilityOff();}
    undercutActor->VisibilityOff();
    renderer->AddActor(preview);
    if(keepSource)renderer->ResetCameraClippingRange();
    render->Render();
    const auto answer=(smokeMode==SmokeMode::Preparation||smokeMode==SmokeMode::Analysis||smokeMode==SmokeMode::Crown)?QMessageBox::Apply:
      QMessageBox::question(this,label,report,QMessageBox::Apply|QMessageBox::Cancel,QMessageBox::Cancel);
    renderer->RemoveActor(preview);
    old.actor->SetVisibility(before.visible);
    selectionActor->VisibilityOn();
    normalsActor->VisibilityOn();
    traceActor->SetVisibility(traceVisible);
    otherMarginsActor->SetVisibility(otherMarginsVisible);
    undercutActor->SetVisibility(undercutVisible);
    if(keepSource)renderer->ResetCameraClippingRange();
    render->Render();
    return answer==QMessageBox::Apply;
  }
  void commitGeometry(int row,zahn::Scan scan,zahn::DisplayMesh display,
                      const QString& label,QJsonObject audit) {
    audit["operation"]=label;
    audit["timeUtc"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    scan.operations.append(audit);
    addDisplay(std::move(display));
    document.replaceScan(size_t(row),std::move(scan),label);
    syncScene();
  }
  void registerScans() {
    const int row=movingRow;
    const auto before=document.project.scans.at(size_t(row));
    const auto fixedName=document.project.scans.at(size_t(fixedRow)).name;
    zahn::LandmarkTriplet moving,fixed;
    std::copy_n(movingLandmarks.begin(),3,moving.begin());
    std::copy_n(draft.begin(),3,fixed.begin());
    cancelPointTool();
    const auto fit=zahn::alignLandmarks(moving,fixed);
    auto display=std::make_shared<zahn::DisplayMesh>();
    runJob("Registering scan",[before,fit,display](const zahn::Checkpoint& check){
      auto moved=std::make_shared<const zahn::Mesh>(zahn::transformMesh(*before.mesh,fit,check));
      *display=zahn::prepareDisplay(std::move(moved),check);
    },[this,row,before,fixedName,fit,display]{
      const QString label="Landmark registration";
      const auto report=QString("Apply rigid registration to %1?\nFixed scan: %2\n"
        "Landmark RMS: %3 mm\nMaximum landmark residual: %4 mm\n"
        "Check alignment away from the three landmarks before using the result.")
        .arg(before.name,fixedName).arg(fit.rms,0,'g',5).arg(fit.maximum,0,'g',5);
      if(!approveGeometry(before,*display,label,report))return;
      auto edited=before;
      edited.mesh=display->source;
      const auto& m=fit.matrix;
      for(auto& [tooth,axis]:edited.insertionAxes)
        axis={m[0]*axis.x+m[1]*axis.y+m[2]*axis.z,
              m[4]*axis.x+m[5]*axis.y+m[6]*axis.z,
              m[8]*axis.x+m[9]*axis.y+m[10]*axis.z};
      for(auto& entry:edited.toothMargins)
        for(auto& point:entry.second)point=fit.apply(point);
      QJsonArray matrix;
      for(double value:fit.matrix)matrix.append(value);
      commitGeometry(row,std::move(edited),std::move(*display),label,
                     {{"fixedScan",fixedName},{"matrixRowMajor",matrix},
                      {"landmarkRmsMm",fit.rms},{"landmarkMaxMm",fit.maximum}});
    });
  }
  zahn::Vec3 insertionAxis() {
    const auto* axis=renderer->GetActiveCamera()->GetDirectionOfProjection();
    return {-axis[0],-axis[1],-axis[2]};
  }
  zahn::Vec3 toothInsertionAxis(const zahn::Scan& scan) {
    const auto saved=scan.insertionAxes.find(scan.toothFdi);
    return saved==scan.insertionAxes.end()?insertionAxis():saved->second;
  }
  void setToothInsertionAxis() {
    const int row=scans->currentRow();
    if(row<0)throw std::runtime_error("Select a preparation scan first");
    auto scan=document.project.scans.at(size_t(row));
    if(!scan.toothFdi||scan.role=="Restoration candidate"||!scan.visible||scan.opacity<=0)
      throw std::runtime_error("Select a tooth on a visible preparation scan first");
    if(pointTool!=PointTool::None)throw std::runtime_error("Finish or cancel the active point tool first");
    scan.insertionAxes[scan.toothFdi]=insertionAxis();
    const int tooth=scan.toothFdi;
    clearInsertion();
    document.replaceScan(size_t(row),std::move(scan),"Set insertion direction");
    syncScene();
    statusBar()->showMessage(QString("Insertion direction set for tooth %1").arg(tooth),5000);
  }
  void createResearchCrown(std::optional<double> smokeHeight={},std::function<void()> after={},bool usePreparation=false) {
    const int row=scans->currentRow();
    if(row<0)throw std::runtime_error("Select the preparation scan first");
    if(pointTool!=PointTool::None)throw std::runtime_error("Apply or cancel the active point tool first");
    const auto before=document.project.scans.at(size_t(row));
    if(!before.visible||before.opacity<=0||before.role=="Restoration candidate")
      throw std::runtime_error("Select a visible preparation scan");
    const auto selected=before.toothMargins.find(before.toothFdi);
    if(!before.toothFdi||selected==before.toothMargins.end()||selected->second.size()<3)
      throw std::runtime_error("Select a tooth and apply its margin before building a research crown");
    document.requireRoomForScan();
    const auto axis=toothInsertionAxis(before);
    bool ok=true;
    const auto height=smokeHeight?*smokeHeight:QInputDialog::getDouble(this,"Build research crown",
      QString("Height above the highest margin point (mm)\nDirection: %1").arg(before.insertionAxes.contains(before.toothFdi)?"saved tooth axis":"current view"),5,2,12,2,&ok);
    if(!ok)return;
    auto display=std::make_shared<zahn::DisplayMesh>();
    runJob("Building research crown",[margin=selected->second,axis,height,display,preparation=usePreparation?before.mesh:nullptr](const zahn::Checkpoint& check){
      auto mesh=std::make_shared<const zahn::Mesh>(zahn::researchCrown(margin,axis,height,check,preparation.get()));
      *display=zahn::prepareDisplay(std::move(mesh),check);
    },[this,before,axis,height,display,after,usePreparation]{
      const auto report=QString("Add the hollow research crown for tooth %1 as a restoration candidate?")
        .arg(before.toothFdi);
      if(!approveGeometry(before,*display,"Research crown",report,true))return;
      zahn::Scan crown;
      crown.name=usePreparation?"Prepared-tooth crown":"Research crown";
      crown.role="Restoration candidate";
      crown.format="zmesh";
      crown.toothFdi=before.toothFdi;
      crown.toothMargins.try_emplace(crown.toothFdi);
      crown.sourceMesh=crown.mesh=display->source;
      crown.operations.append(QJsonObject{{"operation","Build research crown"},{"preparationScan",before.name},
        {"axis",QJsonArray{axis.x,axis.y,axis.z}},{"heightMm",height},{"cavity",usePreparation?"sampledPreparation":"marginOnly"},
        {"timeUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}});
      addDisplay(std::move(*display));
      document.addScan(std::move(crown),"Add research crown");
      syncScene();
      scans->setCurrentRow(scans->count()-1);
      renderer->ResetCameraClippingRange();
      render->Render();
      if(after)after();
    });
  }
  void clearInsertion() {
    analyzedMesh.reset();
    undercutActor->VisibilityOff();
    undercutActor->SetMapper(nullptr);
  }
  void measureSeparation() {
    const int row=scans->currentRow();
    if(row<0||document.project.scans.size()<2)
      throw std::runtime_error("Select a scan and import a second surface before measuring separation");
    if(const auto other=chooseOtherScan(row,"Sampled surface separation","Comparison surface"))
      measureSeparationBetween(row,*other);
  }
  void measureSeparationBetween(int firstRow,int secondRow,
    std::function<void(const zahn::SampledSeparation&)> after={}) {
    const auto& first=document.project.scans.at(size_t(firstRow));
    const auto& second=document.project.scans.at(size_t(secondRow));
    auto result=std::make_shared<zahn::SampledSeparation>();
    const auto firstLabel=scanLabel(firstRow);
    const auto secondLabel=scanLabel(secondRow);
    runJob("Measuring surface separation",
      [a=first.mesh,b=second.mesh,result](const zahn::Checkpoint& check){
        *result=zahn::sampledSeparation(*a,*b,check);
      },[this,result,firstLabel,secondLabel,after]{
        if(after){after(*result);return;}
        auto coordinates=[](zahn::Vec3 point){
          return QString("(%1, %2, %3) mm")
            .arg(point.x,0,'g',9).arg(point.y,0,'g',9).arg(point.z,0,'g',9);
        };
        QMessageBox dialog(QMessageBox::Information,"Sampled surface separation",
          QString("Smallest sampled separation: %1 mm\n%2 sample queries across both surfaces\n\n"
                  "%3\n%4\n\n%5\n%6\n\n"
                  "Distances are unsigned. Sampling can miss closer areas and intersections.")
            .arg(result->minimum,0,'g',6).arg(result->samples)
            .arg(firstLabel,coordinates(result->onFirst),secondLabel,coordinates(result->onSecond)),
          QMessageBox::Ok,this);
        dialog.setTextFormat(Qt::PlainText);
        dialog.setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);
        dialog.exec();
      });
  }
  void analyzeCurrentInsertion(std::function<void(zahn::Vec3)> after={}) {
    const int row=scans->currentRow();
    if(row<0)return;
    if(pointTool!=PointTool::None)throw std::runtime_error("Apply or cancel the active point tool before checking undercuts");
    const auto mesh=document.project.scans[row].mesh;
    const auto axis=toothInsertionAxis(document.project.scans[row]);
    struct Result { zahn::InsertionReport report; vtkSmartPointer<vtkPolyData> overlay; };
    auto result=std::make_shared<Result>();
    runJob("Analyzing insertion",[mesh,axis,result](const zahn::Checkpoint& check){
      result->report=zahn::analyzeInsertion(*mesh,axis,check);
      if(result->report.undercuts)result->overlay=zahn::toPolyData(result->report.shadowed,check);
    },[this,mesh,result,axis,after]{
      clearInsertion();
      if(result->overlay){
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(result->overlay);
        mapper->SetResolveCoincidentTopologyToPolygonOffset();
        mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-2,-2);
        if(clip)mapper->AddClippingPlane(plane);
        undercutActor->SetMapper(mapper);
        analyzedMesh=mesh;
        updateSelection();
      }else render->Render();
      if(after){after(axis);return;}
      const auto& report=result->report;
      QMessageBox::information(this,"Insertion direction analysis",
        QString("%1 of %2 sampled face centroids are shadowed.\nLargest sampled axial depth: %3 mm.\n"
                "Orbit the scan to see the shadowed faces in red.")
          .arg(report.undercuts).arg(report.sampled).arg(report.maximumDepth,0,'g',5));
    });
  }
  void blockoutCurrent() {
    const int row=scans->currentRow();
    if(row<0)return;
    cancelPointTool();
    bool ok=false;
    const auto spacing=QInputDialog::getDouble(this,"Directional blockout","Grid spacing (mm)",
      0.5,0.05,5,2,&ok);
    if(!ok)return;
    blockoutScan(row,insertionAxis(),spacing);
  }
  void blockoutScan(int row,zahn::Vec3 axis,double spacing,std::function<void()> after={}) {
    const auto before=document.project.scans[row];
    auto display=std::make_shared<zahn::DisplayMesh>();
    runJob("Building blockout",[before,axis,spacing,display](const zahn::Checkpoint& check){
      auto blocked=zahn::blockout(*before.mesh,axis,spacing,check);
      if(blocked.triangles.empty())throw std::runtime_error("The grid missed the surface. Use finer spacing.");
      if(!zahn::inspect(blocked,check).closed())
        throw std::runtime_error("The sampled envelope did not produce a closed manifold. Try a different view or spacing.");
      *display=zahn::prepareDisplay(std::make_shared<const zahn::Mesh>(std::move(blocked)),check);
    },[this,row,axis,spacing,before,display,after]{
      const QString label="Directional blockout";
      auto report=QString("Apply the sampled envelope at %1 mm grid spacing?\n"
                          "This derived mesh adds a planar base and omits vertex colors.").arg(spacing);
      if(before.hasMargins())report+="\nSaved margins on this scan will be cleared for the edited mesh.";
      if(!approveGeometry(before,*display,label,report))return;
      auto edited=before;
      edited.mesh=display->source;
      edited.clearMargins();
      commitGeometry(row,std::move(edited),std::move(*display),label,
                     {{"axis",QJsonArray{axis.x,axis.y,axis.z}},{"gridMm",spacing}});
      if(after)after();
    });
  }
  void editSurface(zahn::Parameters parameters,QString label,double tolerance=-1,std::function<void()> after={}){
    cancelPointTool();
    int i=scans->currentRow();if(i<0)return;auto before=document.project.scans[i];
    struct Result{std::shared_ptr<const zahn::Mesh> mesh;zahn::DisplayMesh display;zahn::Deviation deviation;zahn::Report oldReport,newReport;};auto result=std::make_shared<Result>();
    runJob(label,[before,parameters,result](const zahn::Checkpoint& check){
      result->mesh=std::make_shared<const zahn::Mesh>(zahn::processSurface(*before.mesh,parameters,check));
      result->deviation=zahn::deviation(*before.mesh,*result->mesh,check);result->oldReport=zahn::inspect(*before.mesh,check);result->newReport=zahn::inspect(*result->mesh,check);
      result->display=zahn::prepareDisplay(result->mesh,check);
    },[this,i,before,parameters,result,label,tolerance,after]{
      if(tolerance>=0&&result->deviation.maximum>tolerance){QMessageBox::information(this,label,QString("Not applied: sampled maximum deviation %1 mm exceeds %2 mm. Choose different parameters or tolerance.").arg(result->deviation.maximum,0,'g',6).arg(tolerance));return;}
      QString report=QString("Preview — apply this derived scan?\nTriangles: %1 → %2\nBoundary edges: %3 → %4\nDegenerate faces: %5 → %6\nNon-manifold edges: %7 → %8\nSampled bidirectional deviation: max %9 mm; RMS %10 mm (%11 samples).\n\nSampling is not a certified error bound. Original source data stays unchanged; this edit can be undone.")
        .arg(result->oldReport.triangles).arg(result->newReport.triangles).arg(result->oldReport.boundaryEdges).arg(result->newReport.boundaryEdges).arg(result->oldReport.degenerate).arg(result->newReport.degenerate).arg(result->oldReport.nonManifoldEdges).arg(result->newReport.nonManifoldEdges).arg(result->deviation.maximum,0,'g',6).arg(result->deviation.rms,0,'g',6).arg(result->deviation.samples);
      if(parameters.operation==zahn::Operation::FillHoles)report+="\nHole filling can also close a scan's outer boundary and create overlapping faces. Review the preview carefully.";
      if(before.hasMargins())report+="\nSaved margins on this scan will be cleared for the edited mesh.";
      if(!approveGeometry(before,result->display,label,report))return;
      auto edited=before;edited.mesh=result->mesh;
      edited.clearMargins();
      QJsonObject audit{{"value",parameters.value},{"iterations",parameters.iterations},{"selectedFaceCount",qint64(parameters.selected.size())},{"selectedVertexCount",qint64(parameters.selectedVertices.size())},{"sampledMaxMm",result->deviation.maximum},{"sampledRmsMm",result->deviation.rms},{"samples",qint64(result->deviation.samples)}};
      if(parameters.operation==zahn::Operation::Crop)audit["boxMm"]=QJsonArray{parameters.box.minimum.x,parameters.box.maximum.x,parameters.box.minimum.y,parameters.box.maximum.y,parameters.box.minimum.z,parameters.box.maximum.z};
      commitGeometry(i,std::move(edited),std::move(result->display),label,std::move(audit));
      if(after)after();
    });
  }
  void loadScan(QString file, QString role, double factor, std::function<void()> after = {}) {
    document.requireRoomForScan();
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
  void loadCase(QString file, bool recovery = false, std::function<void()> after = {}) {
    if((file.isEmpty()&&!recovery)||!leave())return;
    if(!recovery)file=QFileInfo(file).absoluteFilePath();
    struct Result {zahn::Project project;std::vector<zahn::DisplayMesh> display;};auto result=std::make_shared<Result>();
    const QString dir=directory;
    runJob(recovery?"Recovering case":"Opening case",[result,file,recovery,dir](const zahn::Checkpoint& check){
      result->project=recovery?zahn::loadRecovery(dir,check):zahn::loadProject(file,check);
      for(auto& scan:result->project.scans)result->display.push_back(zahn::prepareDisplay(scan.mesh,check));
    },[this,result,file,recovery,after]{
      clearPointTool();
      clearInsertion();
      for(auto actor:active)renderer->RemoveActor(actor);active.clear();cache.clear();
      for(auto& display:result->display)addDisplay(std::move(display));
      referenceReopened=!recovery&&result->project.reference.has_value();
      document.reset(std::move(result->project),recovery);path=recovery?QString():file;
      if(!recovery){
        clearRecovery();
        rememberCase();
      }
      setMeasuring(false);
      syncScene();renderer->ResetCamera();render->Render();
      if(after)after();
    });
  }
  void commitOpacity(int value) {
    int i=scans->currentRow();if(i<0||busy)return;
    auto& scan=document.project.scans[i];document.setDisplay(size_t(i),scan.visible,value/100.);
  }
  void importScanWithOptions(const QString& file, std::function<void()> after={}) {
    QDialog dialog(this);
    dialog.setObjectName("scan-import-options");
    dialog.setWindowTitle("Import scan");
    auto* form=new QFormLayout(&dialog);
    auto* filename=new QLabel(QFileInfo(file).fileName());
    filename->setWordWrap(true);
    form->addRow("File",filename);
    auto* role=new QComboBox;
    role->setObjectName("scan-role");
    role->addItems({"Upper jaw","Lower jaw","Antagonist","Bite","Reference","Restoration candidate"});
    form->addRow("&Scan role",role);
    auto* scale=new QDoubleSpinBox;
    scale->setObjectName("scan-scale");
    scale->setDecimals(6);
    scale->setRange(0.000001,1000000);
    scale->setValue(1);
    form->addRow("Scale to &mm",scale);
    auto* units=new QLabel("Use 1 if the scan is already in millimetres.");
    units->setWordWrap(true);
    form->addRow(units);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText("Import");
    connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    form->addRow(buttons);
    if(dialog.exec()==QDialog::Accepted)loadScan(file,role->currentText(),scale->value(),std::move(after));
  }
  void buildWorkbench(const std::map<QString,QAction*>& actions,
                      const std::array<QMenu*,5>& stages, QDoubleSpinBox* elevation) {
    auto heading=[](const QString& title) {
      auto* label=new QLabel(title);
      label->setProperty("heading",true);
      return label;
    };
    auto panel=[] {
      auto* frame=new QFrame;
      frame->setProperty("panel",true);
      return frame;
    };
    auto button=[this](QAction* action,const QString& caption) {
      auto* control=new QToolButton;
      control->setText(caption);
      control->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
      control->setAccessibleName(action->text());
      const auto shortcut=action->shortcut().toString(QKeySequence::NativeText);
      control->setToolTip(shortcut.isEmpty()?action->text():action->text()+" ("+shortcut+")");
      auto sync=[control,action]{
        control->setEnabled(action->isEnabled());
        control->setCheckable(action->isCheckable());
        control->setChecked(action->isChecked());
      };
      sync();
      connect(action,&QAction::changed,control,sync);
      connect(control,&QToolButton::clicked,action,&QAction::trigger);
      return control;
    };

    workbench=new QWidget;
    auto* shell=new QVBoxLayout(workbench);
    shell->setContentsMargins(12,10,12,10);
    shell->setSpacing(12);
    auto* stageRow=new QHBoxLayout;
    stageRow->setSpacing(6);
    shell->addLayout(stageRow);

    auto* body=new QHBoxLayout;
    body->setSpacing(10);
    shell->addLayout(body,1);
    auto* left=new QVBoxLayout;
    left->setSpacing(10);
    body->addLayout(left);

    auto* scene=panel();
    scene->setMinimumWidth(210);
    scene->setMaximumWidth(270);
    auto* sceneLayout=new QVBoxLayout(scene);
    sceneLayout->addWidget(heading("Scene"));
    scanHeading=new QLabel("Scans (0)");
    sceneLayout->addWidget(scanHeading);
    scans->setAccessibleName("Scans");
    scans->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sceneLayout->addWidget(scans,1);
    marginHeading=new QLabel("Saved margins (0)");
    sceneLayout->addWidget(marginHeading);
    margins=new QListWidget;
    margins->setAccessibleName("Saved margins");
    margins->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    margins->setMaximumHeight(118);
    margins->hide();
    sceneLayout->addWidget(margins);
    auto selectMargin=[this](QListWidgetItem* item){
      if(busy)return;
      const int row=item->data(Qt::UserRole).toInt(), tooth=item->data(Qt::UserRole+1).toInt();
      if(pointTool!=PointTool::None){
        statusBar()->showMessage("Finish or cancel the active point tool before switching margins",6000);
        updateTrace();
        return;
      }
      scans->setCurrentRow(row);
      guarded([&]{activateTooth(row,tooth);});
    };
    connect(margins,&QListWidget::itemClicked,this,selectMargin);
    connect(margins,&QListWidget::itemActivated,this,selectMargin);
    left->addWidget(scene,1);

    auto* info=panel();
    auto* infoLayout=new QVBoxLayout(info);
    infoLayout->addWidget(heading("Case info"));
    auto* stats=new QFormLayout;
    cornersLabel=new QLabel("0");
    trianglesLabel=new QLabel("0");
    sizeLabel=new QLabel("0 B");
    stats->addRow("Triangle corners",cornersLabel);
    stats->addRow("Triangles",trianglesLabel);
    stats->addRow("Source files",sizeLabel);
    sizeLabel->setToolTip("Combined size of the original imported scan files");
    infoLayout->addLayout(stats);
    left->addWidget(info);

    auto* viewport=panel();
    viewport->setObjectName("viewportFrame");
    auto* viewportLayout=new QGridLayout(viewport);
    viewportLayout->setContentsMargins(2,2,2,2);
    view->setMinimumSize(360,300);
    viewportLayout->addWidget(view,0,0);
    body->addWidget(viewport,1);

    auto* projection=new QComboBox;
    projection->setAccessibleName("Camera projection");
    projection->addItems({"Perspective","Orthographic"});
    connect(projection,&QComboBox::activated,this,[this,action=actions.at("Orthographic / perspective")](int index){
      if((index==1)!=bool(renderer->GetActiveCamera()->GetParallelProjection()))action->trigger();
    });
    connect(actions.at("Orthographic / perspective"),&QAction::triggered,this,[this,projection]{
      projection->setCurrentIndex(renderer->GetActiveCamera()->GetParallelProjection()?1:0);
    });
    auto* viewportTools=panel();
    auto* topTools=new QHBoxLayout(viewportTools);
    topTools->setContentsMargins(7,6,7,6);
    topTools->setSpacing(4);
    topTools->addWidget(projection);
    topTools->addWidget(button(navigationAction,"Navigate"));
    topTools->addWidget(button(brushAction,"Brush"));
    topTools->addWidget(button(boxAction,"Box"));
    topTools->addWidget(button(measureAction,"Measure"));
    topTools->addWidget(button(clipAction,"Clip"));
    auto* overlays=new QGridLayout;
    overlays->setContentsMargins(10,10,10,10);
    overlays->addWidget(viewportTools,0,0,Qt::AlignTop|Qt::AlignLeft);
    auto* cameraTools=new QWidget;
    auto* cameraLayout=new QVBoxLayout(cameraTools);
    cameraLayout->setContentsMargins(0,0,0,0);
    cameraLayout->setSpacing(4);
    cameraLayout->addWidget(button(actions.at("Fit"),"Fit"));
    cameraLayout->addWidget(button(actions.at("Zoom in"),"+"));
    cameraLayout->addWidget(button(actions.at("Zoom out"),"−"));
    overlays->addWidget(cameraTools,1,0,Qt::AlignBottom|Qt::AlignRight);
    overlays->setRowStretch(1,1);
    viewportLayout->addLayout(overlays,0,0);

    inspector=new QTabWidget;
    inspector->setMinimumWidth(265);
    inspector->setMaximumWidth(310);
    body->addWidget(inspector);
    auto* properties=new QWidget;
    auto* propertiesLayout=new QVBoxLayout(properties);
    propertiesLayout->setContentsMargins(12,14,12,12);
    propertiesLayout->setSpacing(10);
    toolHeading=heading({});
    propertiesLayout->addWidget(toolHeading);
    auto* form=new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    form->setVerticalSpacing(7);
    auto* radiusControls=new QHBoxLayout;
    auto* radiusSlider=new QSlider(Qt::Horizontal);
    radiusSlider->setAccessibleName("Brush radius");
    radiusSlider->setRange(brushRadius->minimum(),brushRadius->maximum());
    radiusSlider->setValue(brushRadius->value());
    brushRadius->setSuffix(" px");
    brushRadius->setAccessibleName("Brush radius in pixels");
    connect(radiusSlider,&QSlider::valueChanged,brushRadius,&QSpinBox::setValue);
    connect(brushRadius,&QSpinBox::valueChanged,radiusSlider,&QSlider::setValue);
    radiusControls->addWidget(radiusSlider,1);
    radiusControls->addWidget(brushRadius);
    form->addRow("Brush radius",radiusControls);
    opacity->setAccessibleName("Scan opacity");
    form->addRow("Scan opacity",opacity);
    elevation->setSuffix(" mm");
    elevation->setAccessibleName("Clipping Z position");
    form->addRow("Clipping Z",elevation);
    propertiesLayout->addLayout(form);
    propertiesLayout->addWidget(heading("Vertex selection"));
    propertiesLayout->addWidget(button(actions.at("Keep selected vertices…"),"Keep selection…"));
    propertiesLayout->addWidget(button(actions.at("Delete selected vertices and incident faces…"),"Delete selection…"));
    propertiesLayout->addWidget(button(actions.at("Clear vertex selection"),"Clear selection"));
    propertiesLayout->addStretch();
    const int propertiesTab=inspector->addTab(properties,"Tool Properties");

    auto* workflow=new QWidget;
    auto* workflowLayout=new QVBoxLayout(workflow);
    workflowLayout->setContentsMargins(8,8,8,8);
    auto* mode=new QComboBox;
    mode->setObjectName("workflow-mode");
    mode->setAccessibleName("Workflow mode");
    mode->addItems({"Guided workflow","Expert tools"});
    workflowLayout->addWidget(mode);
    auto* workflowPages=new QStackedWidget;
    workflowPages->setObjectName("workflow-pages");
    workflowLayout->addWidget(workflowPages,1);
    const int workflowTab=inspector->addTab(workflow,"Workflow");
    auto* group=new QButtonGroup(workbench);
    const std::array<QString,5> descriptions{
      "Import and register","Clean and repair","Select teeth and design","Inspect surfaces","Research STL"
    };
    const std::array<QString,5> instructions{
      "Open a case or import scans. Confirm the role and scale for each scan.",
      "Select a scan in the scene. Use these tools if it needs preparation.",
      "Select the preparation scan and tooth. Trace and apply the margin. Orient the scan and set the tooth’s insertion direction before checking undercuts or building the crown.",
      "Select a scan to inspect, or compare surfaces. Use the ruler for a point-to-point measurement.",
      "Save the case to keep editing it later. Select an object in the scene to export its research STL."
    };
    const std::array<QStringList,5> guidedActions{{
      {"Open project…","Recent cases…","Import scan…","Register using landmarks…"},
      {"Vertex brush","Toggle crop/selection box","Crop to box…","Remove duplicate/degenerate faces…"},
      {"Select tooth markup…","Trace or edit margin","Remove last draft point","Apply margin",
       "Exit active point tool","Set insertion direction from view","Check insertion undercuts","Clear insertion overlay","Crown over prepared tooth…","Build research crown…","Freeform smooth"},
      {"Inspect selected scan","Toggle distance measurement","Measure surface separation…",
       "Set up reference measurement…","Record ruler reading","Export reference CSV…"},
      {"Save","Export research STL…"}
    }};
    const std::map<QString,QString> compactLabels{
      {"Delete selected vertices and incident faces…","Delete selection…"},
      {"Keep largest connected component…","Keep largest component…"},
      {"Keep component touched by selection…","Keep selected component…"},
      {"Remove duplicate/degenerate faces…","Remove duplicate / bad faces…"},
      {"Orient connected faces consistently…","Orient faces consistently…"},
      {"Smooth with fixed boundary…","Smooth scan…"},
      {"Simplify with topology preservation…","Simplify scan…"},
      {"Toggle crop/selection box","Crop / selection box"},
      {"Toggle distance measurement","Distance ruler"},
      {"Set insertion direction from view","Set direction from view"}
    };
    for(size_t i=0;i<stages.size();++i){
      auto* stage=new QPushButton;
      stage->setProperty("stage",true);
      stage->setCheckable(true);
      stage->setAccessibleName(stages[i]->title());
      stage->setMinimumHeight(59);
      stage->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
      auto* tileLayout=new QHBoxLayout(stage);
      tileLayout->setContentsMargins(10,6,10,6);
      tileLayout->setSpacing(8);
      auto* number=new QLabel(QString::number(i+1));
      number->setProperty("stageNumber",true);
      number->setAlignment(Qt::AlignCenter);
      number->setFixedSize(29,29);
      number->setAttribute(Qt::WA_TransparentForMouseEvents);
      tileLayout->addWidget(number);
      auto* stageText=new QVBoxLayout;
      stageText->setSpacing(2);
      tileLayout->addLayout(stageText,1);
      auto* title=heading(stages[i]->title());
      title->setAttribute(Qt::WA_TransparentForMouseEvents);
      stageText->addWidget(title);
      auto* description=new QLabel(descriptions[i]);
      description->setProperty("muted",true);
      description->setAttribute(Qt::WA_TransparentForMouseEvents);
      description->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
      stageText->addWidget(description);
      stage->setToolTip(descriptions[i]);
      group->addButton(stage,int(i));
      stageRow->addWidget(stage,1);

      auto* views=new QStackedWidget;
      views->setObjectName("workflow-stage-"+QString::number(i));
      for(int expert=0;expert<2;++expert){
        auto* page=new QWidget;
        auto* pageLayout=new QVBoxLayout(page);
        pageLayout->setContentsMargins(4,8,4,8);
        pageLayout->setSpacing(5);
        if(!expert){
          auto* instruction=new QLabel(instructions[i]);
          instruction->setWordWrap(true);
          pageLayout->addWidget(instruction);
        }
        auto addControl=[&](QAction* action){
          const auto compact=compactLabels.find(action->text());
          auto* control=button(action,compact==compactLabels.end()?action->text():compact->second);
          control->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
          pageLayout->addWidget(control);
        };
        if(expert){
          for(auto* action:stages[i]->actions())if(!action->isSeparator())addControl(action);
        }else{
          for(const auto& title:guidedActions[i])addControl(actions.at(title));
        }
        pageLayout->addStretch();
        auto* scroll=new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setWidget(page);
        views->addWidget(scroll);
      }
      connect(mode,&QComboBox::currentIndexChanged,views,&QStackedWidget::setCurrentIndex);
      workflowPages->addWidget(views);
    }
    auto* steps=new QWidget;
    auto* stepLayout=new QHBoxLayout(steps);
    stepLayout->setContentsMargins(0,0,0,0);
    auto* back=new QPushButton("Back");
    back->setObjectName("workflow-back");
    auto* next=new QPushButton("Next");
    next->setObjectName("workflow-next");
    stepLayout->addWidget(back);
    stepLayout->addWidget(next);
    workflowLayout->addWidget(steps);
    group->button(0)->setChecked(true);
    inspector->setCurrentIndex(workflowTab);
    back->setEnabled(false);
    connect(back,&QPushButton::clicked,this,[group]{group->button(group->checkedId()-1)->click();});
    connect(next,&QPushButton::clicked,this,[group]{group->button(group->checkedId()+1)->click();});
    connect(mode,&QComboBox::currentIndexChanged,steps,[steps](int index){steps->setVisible(index==0);});
    connect(workflowPages,&QStackedWidget::currentChanged,this,[workflowPages,back,next](int index){
      back->setEnabled(index>0);
      next->setEnabled(index+1<workflowPages->count());
    });
    connect(group,&QButtonGroup::idClicked,this,[this,workflowPages,workflowTab](int index){
      workflowPages->setCurrentIndex(index);
      inspector->setCurrentIndex(workflowTab);
    });
    connect(sculptAction,&QAction::triggered,this,[this,propertiesTab]{inspector->setCurrentIndex(propertiesTab);});
    connect(brushAction,&QAction::triggered,this,[this,propertiesTab]{inspector->setCurrentIndex(propertiesTab);});
    stageRow->addSpacing(5);
    stageRow->addWidget(button(undoAction,"Undo"));
    stageRow->addWidget(button(redoAction,"Redo"));
    auto* saveButton=button(actions.at("Save"),"Save");
    saveButton->setProperty("primary",true);
    stageRow->addWidget(saveButton);

    connect(statusBar(),&QStatusBar::messageChanged,this,[this](const QString& message){
      if(message.isEmpty())statusBar()->showMessage("Ready");
    });
    activeToolLabel=new QLabel;
    selectionLabel=new QLabel;
    activeToolLabel->setMinimumWidth(activeToolLabel->fontMetrics().horizontalAdvance("Active tool · Landmark registration")+20);
    statusBar()->addPermanentWidget(activeToolLabel);
    statusBar()->addPermanentWidget(selectionLabel);
    refreshToolUi();
    setCentralWidget(workbench);
  }
protected:
  bool eventFilter(QObject* watched,QEvent* event) override {
    if(watched==view){
      if(event->type()==QEvent::Resize)viewportOverlay->setGeometry(view->rect());
      if(event->type()==QEvent::Leave){
        viewportOverlay->inside=false;
        viewportOverlay->update();
      }
    }
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
    orientation->SetEnabled(0);
    orientation->SetInteractor(nullptr);
    inputStyle->setBrush(false);inputStyle->paint={};inputStyle->hover={};inputStyle->changed={};inputStyle->strokeEnded={};inputStyle->strokeCommitted={};inputStyle->measure={};
    db.close();
  }
  Window(const QString& dataDirectory, SmokeMode mode) : directory(dataDirectory), smokeMode(mode) {
    resize(1320,780);
    menuBar()->setNativeMenuBar(false);
    db=QSqlDatabase::addDatabase("QSQLITE");db.setDatabaseName(directory+"/cases.sqlite");
    if(db.open()){QSqlQuery q(db);q.exec("CREATE TABLE IF NOT EXISTS cases(path TEXT PRIMARY KEY,name TEXT)");}
    view=new QVTKOpenGLNativeWidget(this);view->setRenderWindow(render);render->AddRenderer(renderer);
    viewportOverlay=new ViewportOverlay(view);viewportOverlay->setGeometry(view->rect());viewportOverlay->show();view->setMouseTracking(true);view->installEventFilter(this);
    render->GetInteractor()->SetInteractorStyle(inputStyle);
    inputStyle->measure=[this](int x,int y){return measureAt(x,y);};
    inputStyle->paint=[this](int x,int y,bool erase){brushStamp(x,y,erase);};inputStyle->changed=[this]{updateSelection();};inputStyle->strokeEnded=[this]{strokeDepth.clear();sculptVertices.clear();};
    inputStyle->strokeCommitted=[this]{guarded([this]{finishSculptStroke();});};
    inputStyle->hover=[this](int x,int y,bool erase){
      if(!picking)return;
      const double ratio=view->devicePixelRatioF();
      viewportOverlay->center={x/ratio,(render->GetSize()[1]-1-y)/ratio};
      viewportOverlay->inside=true;
      viewportOverlay->erase=erase&&!sculpting;
      viewportOverlay->update();
    };
    renderer->SetBackground(0.12,0.16,0.20);
    renderer->SetBackground2(0.065,0.085,0.11);
    renderer->GradientBackgroundOn();
    vtkNew<vtkAxesActor> axes;
    orientation->SetOrientationMarker(axes);
    orientation->SetInteractor(render->GetInteractor());
    orientation->SetDefaultRenderer(renderer);
    orientation->SetViewport(0.015,0.018,0.14,0.18);
    orientation->SetEnabled(1);
    orientation->InteractiveOff();
    traceActor->PickableOff();
    traceActor->GetProperty()->SetColor(1,0.75,0.2);
    traceActor->GetProperty()->SetPointSize(8);
    traceActor->GetProperty()->SetLineWidth(3);
    traceActor->VisibilityOff();
    renderer->AddActor(traceActor);
    otherMarginsActor->PickableOff();
    otherMarginsActor->GetProperty()->SetColor(0.22,0.79,0.72);
    otherMarginsActor->GetProperty()->SetPointSize(4);
    otherMarginsActor->GetProperty()->SetLineWidth(2);
    otherMarginsActor->VisibilityOff();
    renderer->AddActor(otherMarginsActor);
    undercutActor->PickableOff();
    undercutActor->GetProperty()->SetColor(1,0.16,0.12);
    undercutActor->VisibilityOff();
    renderer->AddActor(undercutActor);
    distance->SetRenderer(renderer);
    distance->InstantiateHandleRepresentation();
    distance->GetPoint1Representation()->SetRenderer(renderer);
    distance->GetPoint2Representation()->SetRenderer(renderer);
    distance->SetLabelFormat("%.3f mm");
    distance->VisibilityOff();
    renderer->AddViewProp(distance);
    plane->SetNormal(0,0,1);
    boxWidget->SetInteractor(render->GetInteractor());boxWidget->SetRepresentation(boxRepresentation);boxWidget->SetDefaultRenderer(renderer);boxWidget->RotationEnabledOff();
    vtkNew<vtkCallbackCommand> interaction;interaction->SetClientData(this);
    interaction->SetCallback([](vtkObject*,unsigned long event,void* data,void*){
      static_cast<Window*>(data)->navigation(event==vtkCommand::StartInteractionEvent);
    });
    render->GetInteractor()->AddObserver(vtkCommand::StartInteractionEvent,interaction);
    render->GetInteractor()->AddObserver(vtkCommand::EndInteractionEvent,interaction);
    scans=new QListWidget;
    opacity=new QSlider(Qt::Horizontal);
    opacity->setRange(0,100);opacity->setValue(100);
    brushRadius=new QSpinBox;brushRadius->setRange(5,200);brushRadius->setValue(30);
    connect(brushRadius,&QSpinBox::valueChanged,this,[this](int radius){viewportOverlay->radius=radius;inputStyle->radius=radius*view->devicePixelRatioF();viewportOverlay->update();});
    auto* z=new QDoubleSpinBox;z->setRange(-10000,10000);
    connect(z,&QDoubleSpinBox::valueChanged,this,[this](double v){plane->SetOrigin(0,0,v);render->Render();});
    connect(opacity,&QSlider::valueChanged,this,[this](int value){
      int i=scans->currentRow();if(i<0||busy)return;
      if(opacity->isSliderDown()){cache.at(document.project.scans[i].mesh.get()).actor->GetProperty()->SetOpacity(value/100.);render->Render();}
      else commitOpacity(value);
    });
    connect(opacity,&QSlider::sliderReleased,this,[this]{commitOpacity(opacity->value());});
    connect(scans,&QListWidget::currentRowChanged,this,[this](int row){
      if(pointTool!=PointTool::None&&row!=captureRow()){
        QSignalBlocker blocked(scans);
        scans->setCurrentRow(captureRow());
      }
      syncOpacity();updateSelection();
    });
    connect(scans,&QListWidget::itemChanged,this,[this](QListWidgetItem* row){
      int i=scans->row(row);if(i>=0&&!busy){auto& s=document.project.scans[i];document.setDisplay(size_t(i),row->checkState()==Qt::Checked,s.opacity);}
    });
    std::map<QString,QAction*> actions;
    auto action=[this,&actions](QMenu* menu,QString title,auto fn){
      auto* a=menu->addAction(title);
      actions.emplace(title,a);
      connect(a,&QAction::triggered,this,[this,fn]{
        if(!busy){strokeDepth.clear();guarded(fn);refreshToolUi();}
      });
      return a;
    };
    auto* menu=menuBar()->addMenu("File");
    auto* edit=menuBar()->addMenu("Edit");
    auto* camera=menuBar()->addMenu("View");
    auto* scan=menuBar()->addMenu("Scan");
    auto* preparation=menuBar()->addMenu("Prepare");
    auto* margin=menuBar()->addMenu("Design");
    auto* analysis=menuBar()->addMenu("Analyze");
    auto* exportMenu=menuBar()->addMenu("Export");
    undoAction=action(edit,"Undo",[this]{stepHistory(false);});undoAction->setShortcut(QKeySequence::Undo);
    redoAction=action(edit,"Redo",[this]{stepHistory(true);});redoAction->setShortcut(QKeySequence::Redo);
    action(edit,"Remove selected scan",[this]{int i=scans->currentRow();if(i>=0)document.removeScan(size_t(i));});
    connect(&document.history,&QUndoStack::indexChanged,this,[this]{clearInsertion();syncScene();},Qt::QueuedConnection);
    action(menu,"New case",[this]{if(!leave())return;bool ok;auto id=QInputDialog::getText(this,"Case identifier","Use a non-identifying case code",QLineEdit::Normal,{},&ok);if(!ok)return;
      zahn::Project project;project.id=id;clearPointTool();clearInsertion();referenceReopened=false;document.reset(std::move(project),true);path.clear();clearRecovery();cache.clear();setMeasuring(false);syncScene();});
    action(menu,"Open project…",[this]{loadCase(QFileDialog::getOpenFileName(this,"Open project",{},"ZahnCAD (*.zahncad)"));});
    action(menu,"Recent cases…",[this]{QStringList names,paths;QSqlQuery q("SELECT name,path FROM cases ORDER BY rowid DESC LIMIT 30",db);
      if(!q.isActive())throw std::runtime_error(q.lastError().text().toStdString());
      while(q.next()){names<<q.value(0).toString()+" — "+q.value(1).toString();paths<<q.value(1).toString();}
      if(names.isEmpty()){statusBar()->showMessage("No recent cases yet",4000);return;}
      bool ok;auto name=QInputDialog::getItem(this,"Local cases","Case",names,0,false,&ok);if(ok&&names.contains(name))loadCase(paths[names.indexOf(name)]);});
    action(menu,"Save",[this]{save();})->setShortcut(QKeySequence::Save);
    action(menu,"Save as…",[this]{save(true);});
    action(menu,"Recover autosaved case…",[this]{loadCase({},true);});
    action(scan,"Import scan…",[this]{
      const auto file=QFileDialog::getOpenFileName(this,"Import scan",{},"Scans (*.stl *.ply *.obj)");
      if(!file.isEmpty())importScanWithOptions(file);
    });
    action(analysis,"Inspect selected scan",[this]{int i=scans->currentRow();if(i<0)return;auto mesh=document.project.scans[i].mesh;auto result=std::make_shared<QString>();
      runJob("Inspecting geometry",[mesh,result](const zahn::Checkpoint& check){*result=QString::fromStdString(zahn::inspect(*mesh,check).text());},
        [this,result]{QMessageBox::information(this,"Geometry diagnostics",*result);});});
    action(exportMenu,"Export research STL…",[this]{int i=scans->currentRow();if(i<0)return;auto file=QFileDialog::getSaveFileName(this,"Export research geometry",{},"STL (*.stl)");if(file.isEmpty())return;
      auto mesh=document.project.scans[i].mesh;runJob("Exporting research STL",[mesh,file](const zahn::Checkpoint&){zahn::writeStl(*mesh,zahn::nativePath(file));},[]{},false);});
    navigationAction=action(camera,"Navigate",[this]{cancelPointTool();setBrushActive(false);boxWidget->Off();setMeasuring(false);});
    navigationAction->setCheckable(true);
    action(camera,"Toggle scan colors",[this]{showColors=!showColors;for(auto& [key,item]:cache)item.mapper->SetScalarVisibility(showColors);render->Render();});
    action(camera,"Toggle face normals",[this]{showNormals=!showNormals;updateSelection();});
    sculptAction=action(margin,"Freeform smooth",[this]{toggleBrush(true);});sculptAction->setCheckable(true);
    brushAction=action(preparation,"Vertex brush",[this]{toggleBrush();});brushAction->setCheckable(true);
    action(preparation,"Exit active point tool",[this]{cancelPointTool();setBrushActive(false);});
    action(preparation,"Smaller brush",[this]{brushRadius->setValue(brushRadius->value()-5);});
    action(preparation,"Larger brush",[this]{brushRadius->setValue(brushRadius->value()+5);});
    action(preparation,"Clear vertex selection",[this]{selectedVertices.clear();updateSelection();});
    boxAction=action(preparation,"Toggle crop/selection box",[this]{int i=scans->currentRow();if(i<0)return;if(boxWidget->GetEnabled()){boxWidget->Off();render->Render();return;}cancelPointTool();setBrushActive(false);setMeasuring(false);auto* bounds=cache.at(document.project.scans[i].mesh.get()).geometry.full->GetBounds();double b[6];std::copy(bounds,bounds+6,b);for(int k=0;k<3;++k)if(b[k*2+1]-b[k*2]<0.001){b[k*2]-=0.001;b[k*2+1]+=0.001;}boxRepresentation->SetPlaceFactor(1);boxRepresentation->PlaceWidget(b);boxWidget->On();render->Render();});
    boxAction->setCheckable(true);
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
    action(preparation,"Restore original scan",[this]{
      cancelPointTool();
      const int row=scans->currentRow();
      if(row<0)return;
      auto scan=document.project.scans[row];
      if(!scan.sourceMesh||scan.mesh==scan.sourceMesh)return;
      scan.mesh=scan.sourceMesh;
      scan.operations=scan.format=="zmesh"&&!scan.operations.isEmpty()?QJsonArray{scan.operations.first()}:QJsonArray{};
      scan.clearMargins();
      scan.insertionAxes.clear();
      auto result=std::make_shared<zahn::DisplayMesh>();
      runJob("Restoring original",[mesh=scan.mesh,result](const zahn::Checkpoint& check){
        *result=zahn::prepareDisplay(mesh,check);
      },[this,row,scan,result]{
        addDisplay(std::move(*result));
        document.replaceScan(size_t(row),scan,"Restore original scan");
        syncScene();
      });
    });
    preparation->addSeparator();
    action(scan,"Register using landmarks…",[this]{startRegistration();});
    action(margin,"Select tooth markup…",[this]{selectTooth();});
    action(margin,"Trace or edit margin",[this]{startMargin();});
    removeDraftPointAction=action(margin,"Remove last draft point",[this]{
      if(pointTool!=PointTool::Margin||draft.empty())return;
      marginHistory.push(new MarginPointEdit(draft,draft.size()-1,draft.back(),std::nullopt));
      updateTrace();
      render->Render();
    });
    applyMarginAction=action(margin,"Apply margin",[this]{applyMargin();});
    action(margin,"Clear saved margin",[this]{applyMargin(true);});
    action(margin,"Set insertion direction from view",[this]{setToothInsertionAxis();});
    action(margin,"Crown over prepared tooth…",[this]{createResearchCrown({}, {},true);});
    action(margin,"Build research crown…",[this]{createResearchCrown();});
    action(analysis,"Check insertion undercuts",[this]{analyzeCurrentInsertion();});
    action(analysis,"Measure surface separation…",[this]{measureSeparation();});
    action(analysis,"Set up reference measurement…",[this]{setupReferenceMeasurement();});
    action(analysis,"Record ruler reading",[this]{recordReferenceReading();});
    action(analysis,"Export reference CSV…",[this]{
      if(!document.project.reference||document.project.reference->readings.empty())
        throw std::runtime_error("Record a reference reading before exporting");
      const auto file=QFileDialog::getSaveFileName(this,"Export reference measurements",{},"CSV (*.csv)");
      if(!file.isEmpty())exportReferenceCsv(file);
    });
    action(analysis,"Clear insertion overlay",[this]{clearInsertion();render->Render();});
    action(preparation,"Block out from view…",[this]{blockoutCurrent();});
    action(camera,"Fit",[this]{renderer->ResetCamera();render->Render();});
    action(camera,"Orthographic / perspective",[this]{auto* c=renderer->GetActiveCamera();c->SetParallelProjection(!c->GetParallelProjection());render->Render();});
    auto orient=[this](double x,double y,double z){auto* c=renderer->GetActiveCamera();c->SetFocalPoint(0,0,0);c->SetPosition(x,y,z);c->SetViewUp(0,z?1:0,z?0:1);renderer->ResetCamera();render->Render();};
    action(camera,"Occlusal",[orient]{orient(0,0,1);});action(camera,"Front",[orient]{orient(0,-1,0);});action(camera,"Side",[orient]{orient(1,0,0);});
    measureAction=action(camera,"Toggle distance measurement",[this]{cancelPointTool();setBrushActive(false);boxWidget->Off();setMeasuring(!measuring);});
    measureAction->setCheckable(true);
    clipAction=action(camera,"Toggle axial clipping",[this]{clip=!clip;syncScene();});
    clipAction->setCheckable(true);
    action(camera,"Screenshot…",[this]{auto file=QFileDialog::getSaveFileName(this,"Screenshot",{},"PNG (*.png)");if(!file.isEmpty())saveScreenshot(file);});
    action(camera,"Orbit left",[this]{renderer->GetActiveCamera()->Azimuth(-15);render->Render();});
    action(camera,"Orbit right",[this]{renderer->GetActiveCamera()->Azimuth(15);render->Render();});
    action(camera,"Orbit up",[this]{renderer->GetActiveCamera()->Elevation(15);renderer->GetActiveCamera()->OrthogonalizeViewUp();render->Render();});
    action(camera,"Orbit down",[this]{renderer->GetActiveCamera()->Elevation(-15);renderer->GetActiveCamera()->OrthogonalizeViewUp();render->Render();});
    auto zoom=[this](double amount){auto* camera=renderer->GetActiveCamera();if(camera->GetParallelProjection())camera->SetParallelScale(camera->GetParallelScale()/amount);else camera->Dolly(amount);renderer->ResetCameraClippingRange();render->Render();};
    action(camera,"Zoom in",[zoom]{zoom(1.2);});action(camera,"Zoom out",[zoom]{zoom(1/1.2);});
    const std::map<QString,QString> bindings{
      {"Fit","F"},{"Front","F1"},{"Side","F2"},{"Occlusal","F3"},{"Orthographic / perspective","F4"},{"Zoom in","F5"},{"Zoom out","F6"},
      {"Orbit left","Alt+Left"},{"Orbit right","Alt+Right"},{"Orbit up","Alt+Up"},{"Orbit down","Alt+Down"},
      {"Vertex brush","B"},{"Exit active point tool","Esc"},{"Smaller brush","["},{"Larger brush","]"},{"Clear vertex selection","Ctrl+Shift+A"},
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
    buildWorkbench(actions,{scan,preparation,margin,analysis,exportMenu},z);
    auto* timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this]{autosave();});timer->start(60000);
    statusBar()->showMessage("Research preview · Full-resolution data retained for measurement and export",5500);refreshTitle();syncOpacity();
    if(smokeMode==SmokeMode::None &&
      (QFile::exists(directory+"/recovery.zahncad")||QFile::exists(directory+"/recovery.previous.zahncad")))
      QTimer::singleShot(0,this,[this]{if(QMessageBox::question(this,"Recovery available","Recover the last autosaved case? If the latest is damaged, the previous snapshot will be tried.")==QMessageBox::Yes)guarded([this]{loadCase({},true);});});
  }
  void checkViewportBorder(const QColor& color,bool visible) {
    const auto frame=grab();
    const auto sample=view->mapTo(this,QPoint(view->width()/2,view->height()-2));
    const auto pixel=frame.toImage().pixelColor((QPointF(sample)*frame.devicePixelRatio()).toPoint());
    if((pixel==color)!=visible)
      throw std::runtime_error(visible?"Active tool border was not visible":"Tool border remained after exit");
  }
  void stage2Smoke(const QString& source,const QString& screenshot){
    QTimer::singleShot(0,this,[this]{
      auto* dialog=findChild<QDialog*>("scan-import-options");
      auto* role=dialog?dialog->findChild<QComboBox*>("scan-role"):nullptr;
      auto* scale=dialog?dialog->findChild<QDoubleSpinBox*>("scan-scale"):nullptr;
      if(!role||!scale||scale->value()!=1){
        reportFailure("Import smoke","Scan role or millimetre scale was unavailable");
        return;
      }
      role->setCurrentText("Reference");
      dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });
    importScanWithOptions(source,[this,screenshot]{guarded([this,screenshot]{
      if(document.project.scans.at(0).role!="Reference")
        throw std::runtime_error("Scan import ignored the selected role");
      auto* stages=workbench->findChild<QButtonGroup*>();
      auto* workflow=findChild<QStackedWidget*>("workflow-pages");
      stages->button(0)->click();
      if(!workflow||inspector->currentIndex()!=1||workflow->currentIndex()!=0)
        throw std::runtime_error("Scan stage did not open its workflow");
      stages->button(1)->click();
      if(workflow->currentIndex()!=1)throw std::runtime_error("Prepare workflow did not open");

      const auto buttons=workbench->findChildren<QToolButton*>();
      const auto brushButton=std::find_if(buttons.begin(),buttons.end(),[](const auto* button){
        return button->accessibleName()=="Vertex brush";
      });
      if(brushButton==buttons.end())throw std::runtime_error("Brush button missing");
      auto original=document.project.scans[0].mesh;auto vertex=original->triangles[0].v;
      cache.at(original.get()).brush=std::make_shared<zahn::BrushIndex>(zahn::prepareBrush(cache.at(original.get()).geometry.full));
      (*brushButton)->click();
      if(!picking || inspector->currentIndex()!=0)throw std::runtime_error("Brush button did not activate Tool Properties");
      QEvent leave(QEvent::Leave);
      QApplication::sendEvent(view,&leave);
      checkViewportBorder(brushHighlight,true);
      view->grabFramebuffer();
      auto* camera=renderer->GetActiveCamera();double cameraPosition[3],focalPoint[3],viewUp[3];camera->GetPosition(cameraPosition);camera->GetFocalPoint(focalPoint);camera->GetViewUp(viewUp);
      auto first=vertex[0];renderer->SetWorldPoint(first.x,first.y,first.z,1);renderer->WorldToDisplay();auto* position=renderer->GetDisplayPoint();int px=int(std::lround(position[0])),py=int(std::lround(position[1]));
      auto* interactor=render->GetInteractor();interactor->SetEventPosition(px,py);interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);interactor->SetEventPosition(px+8,py+6);interactor->InvokeEvent(vtkCommand::MouseMoveEvent);interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
      for(int i=0;i<3;++i)if(std::abs(camera->GetPosition()[i]-cameraPosition[i])>1e-10||std::abs(camera->GetFocalPoint()[i]-focalPoint[i])>1e-10)throw std::runtime_error("Left brush drag moved camera");
      if(!selectedVertices.contains({first.x,first.y,first.z}))throw std::runtime_error("Vertex brush failed");
      if(!grab().save(screenshot+".brush.png"))throw std::runtime_error("Could not save brush screenshot");
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
      checkViewportBorder(brushHighlight,false);
      std::set<QString> shortcuts;size_t shortcutCount=0;for(auto* top:menuBar()->actions())if(top->menu())for(auto* action:top->menu()->actions())for(auto key:action->shortcuts()){auto name=key.toString(QKeySequence::PortableText);if(name.isEmpty())continue;if(!shortcuts.insert(name).second)throw std::runtime_error("Duplicate shortcut");++shortcutCount;}
      if(shortcutCount<30||brushAction->shortcut()!=QKeySequence("B"))throw std::runtime_error("Tool/camera shortcuts missing");
      auto b=zahn::inspect(*original);double x=b.maximum.x-b.minimum.x,y=b.maximum.y-b.minimum.y;zahn::Parameters p;p.operation=zahn::Operation::Crop;p.box={{b.minimum.x+x*0.1,b.minimum.y+y*0.1,b.minimum.z-1},{b.maximum.x-x*0.1,b.maximum.y-y*0.1,b.maximum.z+1}};
      double box[6]{p.box.minimum.x,p.box.maximum.x,p.box.minimum.y,p.box.maximum.y,p.box.minimum.z,p.box.maximum.z};boxRepresentation->SetPlaceFactor(1);boxRepresentation->PlaceWidget(box);boxWidget->On();p.box=currentBox();if(std::abs(p.box.minimum.x-box[0])>1e-10)throw std::runtime_error("Crop widget bounds changed");
      showNormals=true;editSurface(p,"Smoke crop",-1,[this,original,screenshot]{
        auto edited=document.project.scans[0].mesh;if(edited==original)throw std::runtime_error("Crop was not applied");document.history.undo();if(document.project.scans[0].mesh!=original)throw std::runtime_error("Crop undo failed");document.history.redo();
        const auto path=screenshot+".zahncad";
        zahn::saveProject(document.project,path);
        const auto reopened=zahn::loadProject(path);
        const auto& saved=reopened.scans.at(0);
        if(saved.mesh->triangles.size()!=edited->triangles.size()||saved.mesh->colors!=edited->colors||
           saved.sourceMesh==saved.mesh||saved.sourceMesh->triangles.size()!=original->triangles.size()||
           saved.sourceMesh->colors!=original->colors||saved.original!=document.project.scans[0].original||
           saved.operations.isEmpty())throw std::runtime_error("Edited case did not reopen with its original and colors");
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
    loadScan(source,"Reference",1,[this,source,screenshot]{
      guarded([this,source,screenshot]{
        auto mesh=document.project.scans.at(0).mesh;
        document.setDisplay(0,false,0.5);document.history.undo();
        if(!document.project.scans[0].visible||document.project.scans[0].opacity!=1)throw std::runtime_error("Undo failed");
        document.history.redo();document.history.undo();document.removeScan(0);document.history.undo();
        if(document.project.scans[0].mesh!=mesh)throw std::runtime_error("Geometry changed in history");
        QTimer::singleShot(1500,this,[this,mesh,source,screenshot]{
          guarded([this,mesh,source,screenshot]{
            auto& item=cache.at(mesh.get());
            navigation(true);
            if(item.mapper->GetInput()!=item.geometry.navigation)
              throw std::runtime_error("Navigation geometry was not displayed");
            render->Render();QElapsedTimer elapsed;elapsed.start();
            for(int i=0;i<60;++i){renderer->GetActiveCamera()->Azimuth(1);render->Render();}
            qInfo("Navigation render exercise: 60 render calls in %lld ms; presented-frame FPS not measured",static_cast<long long>(elapsed.elapsed()));
            renderer->GetActiveCamera()->Azimuth(-60);navigation(false);
            const auto restored=item.mapper->GetInput()->GetNumberOfCells();
            if(item.mapper->GetInput()!=item.geometry.full||
               restored!=vtkIdType(mesh->triangles.size())||document.project.scans.at(0).mesh!=mesh)
              throw std::runtime_error("Full-resolution geometry was not restored after navigation");
            qInfo("Restored %lld full-resolution triangles",static_cast<long long>(restored));
            if(document.project.scans.at(0).name=="triangle.stl"&&mesh->triangles.size()==1){
              checkRuler(screenshot);
              loadScan(source,"Antagonist",2,[this,screenshot]{checkCaseRoundtrip(screenshot);});
              return;
            }
            QApplication::exit(grab().save(screenshot)?0:2);
          });
        });
      });
    });
  }
  void checkRuler(const QString& screenshot) {
    setMeasuring(true);
    clip=true;
    plane->SetOrigin(0,0,1);
    syncScene();
    clickSurface({2,2,0});
    if(rulerPoints!=0)throw std::runtime_error("Ruler accepted a clipped point");
    clip=false;
    plane->SetOrigin(0,0,0);
    syncScene();
    clickSurface({8,8,0});
    if(rulerPoints!=0)throw std::runtime_error("Ruler accepted a background click");
    clickSurface({2,2,0});
    clickSurface({7,2,0});
    distance->BuildRepresentation();
    if(rulerPoints!=2||std::abs(distance->GetDistance()-5)>0.05)
      throw std::runtime_error("Ruler did not measure the expected 5 mm");
    qInfo("Synthetic surface measurement: %.3f mm (expected 5 mm)",distance->GetDistance());
    saveScreenshot(screenshot+".ruler.png");
    setMeasuring(false);
  }
  void clickSurface(zahn::Vec3 point,bool shift=false) {
    renderer->SetWorldPoint(point.x,point.y,point.z,1);
    renderer->WorldToDisplay();
    const auto* pixel=renderer->GetDisplayPoint();
    auto* interactor=render->GetInteractor();
    const auto previousShift=interactor->GetShiftKey();
    interactor->SetShiftKey(shift);
    interactor->SetEventPosition(int(std::lround(pixel[0])),int(std::lround(pixel[1])));
    interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent);
    interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
    interactor->SetShiftKey(previousShift);
  }
  void checkCaseRoundtrip(const QString& screenshot) {
    document.setDisplay(1,false,0.35);
    saveTo(screenshot+".zahncad");
    QSqlQuery recent(db);
    if(!recent.exec("SELECT path FROM cases ORDER BY rowid DESC LIMIT 1")||!recent.next()||
       recent.value(0).toString()!=path)
      throw std::runtime_error("Recent Cases did not index the saved project");
    loadCase(recent.value(0).toString(),false,[this,screenshot]{
      const auto& reopened=document.project.scans;
      if(reopened.size()!=2||reopened[0].role!="Reference"||reopened[0].scale!=1||
         reopened[1].role!="Antagonist"||reopened[1].scale!=2||reopened[1].visible||
         reopened[1].opacity!=0.35||zahn::inspect(*reopened[1].mesh).maximum.x!=20)
        throw std::runtime_error("Reopened case lost scan roles, scale, or display settings");
      QApplication::exit(grab().save(screenshot)?0:2);
    });
  }
  void recoveryWriteSmoke(const QString& source) {
    loadScan(source,"Reference",1,[this]{
      document.project.id="Recovery first";
      autosave([this]{
        document.project.id="Recovery second";
        document.setDisplay(0,false,0.35);
        autosave([this]{
          QSaveFile ready(QDir(directory).filePath("ready"));
          if(!ready.open(QIODevice::WriteOnly)||ready.write("ready")!=5||!ready.commit())
            throw std::runtime_error("Could not signal recovery readiness");
        });
      });
    });
  }
  void recoveryReadSmoke(const QString& output) {
    loadCase({},true,[this,output]{
      if(!document.dirty()||!path.isEmpty()||document.project.scans.size()!=1)
        throw std::runtime_error("Recovered case must require Save As");
      saveTo(output);
      QApplication::exit(0);
    });
  }
  void analysisSmoke(const QString& source,const QString& screenshot) {
    constexpr zahn::LandmarkTriplet points{{{2,2,0},{7,2,0},{2,7,0}}};
    loadScan(source,"Upper jaw",1,[this,source,screenshot,points]{
      if(smokeMode==SmokeMode::Crown){
        auto* mode=findChild<QComboBox*>("workflow-mode");
        auto* pages=findChild<QStackedWidget*>("workflow-pages");
        auto* back=findChild<QPushButton*>("workflow-back");
        auto* next=findChild<QPushButton*>("workflow-next");
        if(!mode||!pages||!back||!next||back->isEnabled()||pages->currentIndex()!=0)
          throw std::runtime_error("Guided workflow did not start at Scan");
        next->click();
        next->click();
        auto* design=findChild<QStackedWidget*>("workflow-stage-2");
        if(!design||pages->currentWidget()!=design||design->currentIndex()!=0)
          throw std::runtime_error("Next did not open the guided Design stage");
        mode->setCurrentIndex(1);
        if(design->currentIndex()!=1||next->isVisibleTo(inspector)||back->isVisibleTo(inspector))
          throw std::runtime_error("Expert tools did not open on the current stage");
        mode->setCurrentIndex(0);
        back->click();
        next->click();
        if(pages->currentWidget()!=design||design->currentIndex()!=0||
           !next->isVisibleTo(inspector)||!back->isVisibleTo(inspector))
          throw std::runtime_error("Returning to guided mode did not preserve navigation");
        selectToothForSmoke(16);
        document.history.undo();
        if(document.project.scans.at(0).toothFdi!=0)throw std::runtime_error("Tooth markup undo failed");
        document.history.redo();
        if(document.project.scans.at(0).toothFdi!=16)throw std::runtime_error("Tooth chart selection or redo failed");
      }
      if(smokeMode==SmokeMode::Crown){
        useGuidedDesignTool("Trace or edit margin");
        if(pointTool!=PointTool::Margin||applyMarginAction->isEnabled())
          throw std::runtime_error("Guided margin tool started in the wrong state");
      }else startMargin();
      checkViewportBorder(marginHighlight,true);
      for(const auto& point:points)clickSurface(point);
      if(!undoAction->isEnabled()||redoAction->isEnabled())
        throw std::runtime_error("Margin undo/redo availability was incorrect");
      undoAction->trigger();
      if(draft.size()!=2||applyMarginAction->isEnabled()||!redoAction->isEnabled())
        throw std::runtime_error("Margin undo did not remove the last added point");
      redoAction->trigger();
      if(draft.size()!=3||!applyMarginAction->isEnabled())
        throw std::runtime_error("Margin redo did not restore the point");
      clickSurface({2.1,2,0},true);
      undoAction->trigger();
      if(draft.front().x>2.05)throw std::runtime_error("Margin undo did not restore the moved point");
      redoAction->trigger();
      if(smokeMode==SmokeMode::Crown){
        if(!applyMarginAction->isEnabled())throw std::runtime_error("The traced margin could not be applied");
        useGuidedDesignTool("Apply margin");
        if(applyMarginAction->isEnabled())throw std::runtime_error("Apply margin stayed available after committing");
        if(!grab().save(screenshot+".workflow.png"))throw std::runtime_error("Could not capture the guided workflow");
      }else applyMargin();
      checkViewportBorder(marginHighlight,false);
      const auto& first=document.project.scans[0].toothMargins.at(smokeMode==SmokeMode::Crown?16:0);
      if(first.size()!=3||first[0].x<2.05)
        throw std::runtime_error("Margin trace or point move failed");
      const int appliedIndex=document.history.index();
      startMargin();
      if(undoAction->isEnabled()||redoAction->isEnabled())
        throw std::runtime_error("A new draft inherited a previous undo history");
      clickSurface({4.5,2,0});
      if(draft.size()!=4||std::abs(draft[1].x-4.5)>0.05)
        throw std::runtime_error("Clicking a saved margin segment did not insert its point in order");
      undoAction->trigger();
      redoAction->trigger();
      if(draft.size()!=4||std::abs(draft[1].x-4.5)>0.05)
        throw std::runtime_error("Redo did not restore the inserted point at its segment");
      if(smokeMode==SmokeMode::Crown&&!grab().save(screenshot+".insert.png"))
        throw std::runtime_error("Could not capture the inserted margin point");
      removeDraftPointAction->trigger();
      if(draft.size()!=3)throw std::runtime_error("Removing a draft point failed");
      undoAction->trigger();
      if(draft.size()!=4)throw std::runtime_error("Undo did not restore the removed point");
      clickSurface({2.05,4.5,0});
      if(draft.size()!=5||std::abs(draft.back().y-4.5)>0.05||redoAction->isEnabled())
        throw std::runtime_error("The closing segment or redo after a new edit was incorrect");
      cancelPointTool();
      checkViewportBorder(marginHighlight,false);
      if(document.history.index()!=appliedIndex||
         document.project.scans[0].toothMargins.at(smokeMode==SmokeMode::Crown?16:0).size()!=3)
        throw std::runtime_error("Draft edits changed the committed margin");
      if(smokeMode==SmokeMode::Crown){
        selectToothForSmoke(17,screenshot+".teeth.png");
        startMargin();
        if(!draft.empty())throw std::runtime_error("A new tooth inherited the previous margin");
        for(const auto& point:zahn::LandmarkTriplet{{{6,6,0},{8,6,0},{6,8,0}}})clickSurface(point);
        applyMargin();
        applyMargin(true);
        const auto& cleared=document.project.scans.at(0).toothMargins;
        if(cleared.at(16).size()!=3||!cleared.at(17).empty())
          throw std::runtime_error("Clearing tooth 17 changed another tooth's margin");
        document.history.undo();
        syncScene();
        if(scans->count()!=1||margins->count()!=2||!otherMarginsActor->GetVisibility())
          throw std::runtime_error("Two saved margins did not appear on the same scan");
        margins->itemClicked(margins->item(0));
        if(document.project.scans.at(0).toothFdi!=16)
          throw std::runtime_error("Selecting a saved margin did not activate its tooth");
        saveScreenshot(screenshot+".two-margins.png");
      }
      loadScan(source,"Restoration candidate",1,[this,screenshot,points]{
        auto fixed=document.project.scans[1];
        zahn::RigidAlignment translation;
        translation.matrix={1,0,0,0,0,1,0,0,0,0,1,2,0,0,0,1};
        fixed.mesh=std::make_shared<const zahn::Mesh>(zahn::transformMesh(*fixed.mesh,translation));
        addDisplay(zahn::prepareDisplay(fixed.mesh));
        document.replaceScan(1,std::move(fixed),"Place synthetic fixed scan");
        syncScene();
        measureSeparationBetween(1,0,[this,screenshot,points](const auto& report){
          if(std::abs(report.minimum-2)>1e-9||report.samples!=16||
             report.onFirst.z!=2||report.onSecond.z!=0)
            throw std::runtime_error("Sampled surface separation did not match the 2 mm offset");
          qInfo("Sampled surface separation: %.3f mm (expected 2 mm)",report.minimum);
          beginRegistration(0,1);
          for(const auto& point:points)clickSurface(point);
          for(auto point:points){point.z=2;clickSurface(point);}
          if(!busy)throw std::runtime_error("Landmark picking did not start registration");
          auto* timer=new QTimer(this);
          connect(timer,&QTimer::timeout,this,[this,timer,screenshot]{
            if(busy)return;
            timer->stop();
            timer->deleteLater();
            guarded([this,screenshot]{verifyAnalysis(screenshot);});
          });
          timer->start(20);
        });
      });
    });
  }
  void useGuidedDesignTool(const QString& name) {
    auto* design=findChild<QStackedWidget*>("workflow-stage-2");
    for(auto* tool:design->currentWidget()->findChildren<QToolButton*>()){
      if(tool->accessibleName()!=name)continue;
      tool->click();
      return;
    }
    throw std::runtime_error("Guided Design tool is missing");
  }
  void selectToothForSmoke(int tooth,const QString& screenshot={}) {
    QTimer::singleShot(0,this,[this,tooth,screenshot]{
      auto* chart=findChild<QDialog*>("tooth-chart");
      if(!chart){reportFailure("Crown smoke","Tooth chart did not open");return;}
      auto* selected=chart->findChild<QPushButton*>(QString("tooth-%1").arg(tooth));
      auto* lower=chart->findChild<QPushButton*>("tooth-46");
      if(selected&&lower&&!lower->isEnabled()&&(screenshot.isEmpty()||chart->grab().save(screenshot,"PNG")))selected->click();
      else chart->reject();
    });
    useGuidedDesignTool("Select tooth markup…");
    if(document.project.scans.at(0).toothFdi!=tooth)throw std::runtime_error("Tooth chart selection failed");
  }
  void referenceSmoke(const QString& source,const QString& screenshot) {
    loadScan(source,"Reference",1,[this,screenshot]{
      QTimer::singleShot(0,this,[this]{
        auto* dialog=findChild<QDialog*>("reference-measurement");
        dialog->findChild<QLineEdit*>("landmarks")->setText("(2, 2) to (7, 2)");
        dialog->findChild<QLineEdit*>("method")->setText("Synthetic geometry");
        dialog->findChild<QLineEdit*>("notes")->setText("=Synthetic, \"control\"");
        dialog->findChild<QDoubleSpinBox*>("expected")->setValue(5);
        dialog->findChild<QDoubleSpinBox*>("tolerance")->setValue(0.05);
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
      });
      setupReferenceMeasurement();
      setMeasuring(true);
      clickSurface({2,2,0});
      clickSurface({7,2,0});
      recordReferenceReading();
      const auto project=screenshot+".zahncad";
      saveTo(project);
      loadCase(project,false,[this,screenshot]{
        setMeasuring(true);
        clickSurface({2,2,0});
        clickSurface({7,2,0});
        recordReferenceReading();
        clickSurface({2,2,0});
        clickSurface({6.8,2,0});
        recordReferenceReading();
        const auto& review=*document.project.reference;
        const auto& readings=review.readings;
        if(readings.size()!=3||readings[0].afterReopen||!readings[1].afterReopen||
           std::abs(readings[0].measuredMm-5)>0.05||std::abs(readings[1].measuredMm-5)>0.05||
           readingSpread(review)<0.1||readingSpread(review)>0.3)
          throw std::runtime_error("Reference reading or reopen checks failed");
        exportReferenceCsv(screenshot+".csv");
        saveTo(screenshot+".zahncad");
        QApplication::exit(grab().save(screenshot)?0:2);
      });
    });
  }
  void verifyAnalysis(const QString& screenshot) {
    auto& scan=document.project.scans[0];
    if(!scan.operations.size()||!registeredMarginsMatch(scan)||
       scan.mesh->colors!=scan.sourceMesh->colors)
      throw std::runtime_error("Registration did not transform the margin and preserve colors");
    document.history.undo();
    if(document.project.scans[0].mesh!=document.project.scans[0].sourceMesh)
      throw std::runtime_error("Registration undo did not restore the source");
    document.history.redo();
    document.setDisplay(1,false,1);
    scans->setCurrentRow(0);
    syncScene();
    saveScreenshot(screenshot);
    saveTo(screenshot+".zahncad");
    const auto reopened=zahn::loadProject(screenshot+".zahncad");
    if(reopened.scans.at(1).role!="Restoration candidate")
      throw std::runtime_error("Restoration candidate role failed to reopen");
    if(!registeredMarginsMatch(reopened.scans.at(0)))
      throw std::runtime_error("Saved registered margin failed to reopen");
    blockoutScan(0,{0,0,1},0.5,[this,screenshot]{
      if(document.project.scans[0].hasMargins()||document.project.scans[0].operations.size()!=2)
        throw std::runtime_error("Blockout did not record its derived geometry");
      saveTo(screenshot+".blockout.zahncad");
      saveScreenshot(screenshot+".blockout.png");
      if(smokeMode==SmokeMode::Crown){
        document.history.undo();
        syncScene();
        verifyCrown(screenshot);
      }else QApplication::exit(0);
    });
  }
  bool registeredMarginsMatch(const zahn::Scan& scan) const {
    const bool multi=smokeMode==SmokeMode::Crown;
    auto matches=[&](int tooth,double minX,double maxX){
      const auto it=scan.toothMargins.find(tooth);
      return it!=scan.toothMargins.end()&&it->second.size()==3&&it->second[0].x>=minX&&it->second[0].x<=maxX&&
        std::all_of(it->second.begin(),it->second.end(),[](auto p){return std::abs(p.z-2)<0.001;});
    };
    return scan.toothMargins.size()==(multi?2u:1u)&&matches(multi?16:0,2.05,2.2)&&(!multi||matches(17,5.9,6.1));
  }
  void verifyCrown(const QString& screenshot) {
    const auto preparation=document.project.scans.at(0);
    if(preparation.toothFdi!=16||!registeredMarginsMatch(preparation)||preparation.operations.size()!=1)
      throw std::runtime_error("Tooth markup and margin did not survive registration and blockout undo");
    auto* camera=renderer->GetActiveCamera();
    camera->SetPosition(0,0,1);
    camera->SetFocalPoint(0,0,0);
    camera->SetViewUp(0,1,0);
    renderer->ResetCamera();
    setToothInsertionAxis();
    const auto file=screenshot+".axis.zahncad";
    saveTo(file);
    loadCase(file,false,[this,screenshot]{
      scans->setCurrentRow(0);
      renderer->GetActiveCamera()->Azimuth(65);
      if(std::abs(insertionAxis().z)>0.95)throw std::runtime_error("Test camera did not orbit");
      analyzeCurrentInsertion([this,screenshot](zahn::Vec3 axis){
        if(std::abs(axis.z-1)>1e-6)throw std::runtime_error("Undercut analysis lost the saved axis");
        verifyCrownGeometry(screenshot);
      });
    });
  }
  void verifyCrownGeometry(const QString& screenshot) {
    const auto preparation=document.project.scans.at(0);
    createResearchCrown(5,[this,screenshot,preparation]{
      if(document.project.scans.size()!=3||document.project.scans.at(0).mesh!=preparation.mesh)
        throw std::runtime_error("Adding a crown changed the preparation scan");
      document.history.undo();
      if(document.project.scans.size()!=2)throw std::runtime_error("Crown undo failed");
      document.history.redo();
      syncScene();
      auto* camera=renderer->GetActiveCamera();
      camera->Azimuth(25);
      camera->Elevation(35);
      camera->OrthogonalizeViewUp();
      renderer->ResetCamera();
      saveScreenshot(screenshot+".crown.png");
      const auto project=screenshot+".crown.zahncad";
      saveTo(project);
      loadCase(project,false,[this,screenshot]{
        const auto& base=document.project.scans.at(0);
        const auto& crown=document.project.scans.at(2);
        const auto geometry=zahn::inspect(*crown.mesh);
        if(base.toothFdi!=16||!registeredMarginsMatch(base)||crown.toothFdi!=16||crown.format!="zmesh"||
           crown.role!="Restoration candidate"||crown.sourceMesh!=crown.mesh||crown.operations.size()!=1||
           !geometry.closed()||std::abs(geometry.maximum.z-7)>0.001||geometry.maximum.x>7.1)
          throw std::runtime_error("Crown geometry, provenance or tooth markup did not reopen");
        measureSeparationBetween(2,0,[this,screenshot](const auto& report){
          if(report.minimum>0.001)throw std::runtime_error("Crown rim did not meet the registered margin plane");
          verifySculptStroke(screenshot);
        });
      });
    });
  }
  void verifySculptStroke(const QString& screenshot){
    scans->setCurrentRow(2);
    const auto original=document.project.scans.at(2).mesh;
    auto& display=cache.at(original.get());
    display.brush=std::make_shared<zahn::BrushIndex>(zahn::prepareBrush(display.geometry.full));
    sculptAction->trigger();
    checkViewportBorder(QColor("#BE91FF"),true);
    auto* camera=renderer->GetActiveCamera();
    camera->SetPosition(0,0,1);camera->SetFocalPoint(0,0,0);camera->SetViewUp(0,1,0);
    renderer->ResetCamera();render->Render();
    const auto& upper=original->triangles.at(2);
    const auto a=upper.v[0], center=upper.v[2];
    clickSurface({0.97*a.x+0.03*center.x,0.97*a.y+0.03*center.y,a.z});
    if(!busy)throw std::runtime_error("Sculpt stroke did not start");
    auto* timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,[this,timer,original,screenshot]{
      if(busy)return;
      timer->stop();timer->deleteLater();
      guarded([this,original,screenshot]{
        const auto edited=document.project.scans.at(2).mesh;
        if(edited==original||!sculpting||!zahn::inspect(*edited).closed())
          throw std::runtime_error("Local smoothing did not produce a closed edited crown");
        if(!grab().save(screenshot+".sculpt.png"))throw std::runtime_error("Sculpt screenshot failed");
        document.history.undo();
        if(document.project.scans.at(2).mesh!=original)throw std::runtime_error("Sculpt undo failed");
        document.history.redo();
        if(document.project.scans.at(2).mesh!=edited)throw std::runtime_error("Sculpt redo failed");
        const auto path=screenshot+".sculpt.zahncad";
        saveTo(path);
        const auto reopened=zahn::loadProject(path);
        const auto& candidate=reopened.scans.at(2);
        if(candidate.mesh==candidate.sourceMesh||candidate.operations.last().toObject()["operation"]!="Freeform smooth")
          throw std::runtime_error("Sculpted geometry failed to reopen");
        qInfo("Freeform smooth stroke, undo, redo and reopen passed");
        scans->setCurrentRow(0);
        createResearchCrown(5,[this,screenshot]{
          const auto& crown=document.project.scans.back();
          if(crown.operations.first().toObject()["cavity"]!="sampledPreparation"||
             !zahn::inspect(*crown.mesh).closed())throw std::runtime_error("Prepared-tooth crown failed");
          saveTo(screenshot+".prepared.zahncad");
          qInfo("Prepared-tooth case saved");
          QApplication::exit(0);
        },true);
      });
    });
    timer->start(20);
  }

};

int main(int argc,char** argv) {
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());QApplication app(argc,argv);
  const auto args=app.arguments();
  auto mode=SmokeMode::None;
  if(args.size()==4){
    if(args[1]=="--smoke-test")mode=SmokeMode::Viewer;
    if(args[1]=="--stage2-smoke-test")mode=SmokeMode::Preparation;
    if(args[1]=="--m2-smoke-test")mode=SmokeMode::Analysis;
    if(args[1]=="--m3-smoke-test")mode=SmokeMode::Crown;
    if(args[1]=="--m0-reference-smoke-test")mode=SmokeMode::Reference;
    if(args[1]=="--recovery-smoke-write")mode=SmokeMode::RecoveryWrite;
    if(args[1]=="--recovery-smoke-read")mode=SmokeMode::RecoveryRead;
  }
  app.setApplicationName("ZahnCAD");
  app.setApplicationVersion(ZAHNCAD_VERSION);
  styleWorkbench(app);
  auto directory=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  std::optional<QTemporaryDir> testDirectory;
  if(mode==SmokeMode::RecoveryWrite||mode==SmokeMode::RecoveryRead){
    directory=QFileInfo(args[2]).absoluteFilePath();
    if(!QFile::exists(QDir(directory).filePath(".zahncad-recovery-test"))){
      qCritical("Recovery smoke test requires a disposable marked profile");
      return 1;
    }
  }else if(mode!=SmokeMode::None){
    testDirectory.emplace();
    if(!testDirectory->isValid()){qCritical("Could not create a smoke-test profile");return 1;}
    directory=testDirectory->path();
  }
  if(!QDir().mkpath(directory)){qCritical("Could not create the ZahnCAD data directory");return 1;}
  QLockFile lock(directory+"/session.lock");
  if(!lock.tryLock()){
    if(mode!=SmokeMode::None)qCritical("Could not lock the ZahnCAD smoke-test session");
    else QMessageBox::information(nullptr,"ZahnCAD already running","Close the other ZahnCAD instance before opening another case.");
    return 1;
  }
  Window window(directory,mode);
  window.show();
  if(mode==SmokeMode::Viewer)window.smokeTest(args[2],args[3]);
  if(mode==SmokeMode::Preparation)window.stage2Smoke(args[2],args[3]);
  if(mode==SmokeMode::Analysis||mode==SmokeMode::Crown)window.analysisSmoke(args[2],args[3]);
  if(mode==SmokeMode::Reference)window.referenceSmoke(args[2],args[3]);
  if(mode==SmokeMode::RecoveryWrite)window.recoveryWriteSmoke(args[3]);
  if(mode==SmokeMode::RecoveryRead)window.recoveryReadSmoke(args[3]);
  return app.exec();
}
