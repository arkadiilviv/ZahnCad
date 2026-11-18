#include "zahn/document.hpp"
#include <QUndoCommand>
#include <functional>
#include <cmath>
namespace zahn {
namespace {
class Edit : public QUndoCommand {
  std::function<void()> forward_, backward_;
public:
  Edit(QString title, std::function<void()> forward, std::function<void()> backward)
    : QUndoCommand(title), forward_(std::move(forward)), backward_(std::move(backward)) {}
  void redo() override { forward_(); }
  void undo() override { backward_(); }
};
}
void Document::reset(Project value, bool recovered) {
  history.clear(); project = std::move(value); recovered_ = recovered; history.setClean();
}
void Document::addScan(Scan scan) {
  if(!scan.mesh) throw std::runtime_error("Scan geometry is missing");
  history.push(new Edit("Import scan", [this,scan]{project.scans.push_back(scan);},
    [this]{project.scans.pop_back();}));
}
void Document::removeScan(size_t index) {
  auto scan = project.scans.at(index);
  history.push(new Edit("Remove scan", [this,index]{project.scans.erase(project.scans.begin()+index);},
    [this,index,scan]{project.scans.insert(project.scans.begin()+index,scan);}));
}
void Document::setDisplay(size_t index, bool visible, double opacity) {
  if(!std::isfinite(opacity)||opacity<0||opacity>1) throw std::runtime_error("Invalid opacity");
  auto before=project.scans.at(index);
  if(before.visible==visible && before.opacity==opacity) return;
  history.push(new Edit("Change scan display", [this,index,visible,opacity]{auto& s=project.scans.at(index);s.visible=visible;s.opacity=opacity;},
    [this,index,before]{auto& s=project.scans.at(index);s.visible=before.visible;s.opacity=before.opacity;}));
}
void Document::replaceScan(size_t index,Scan scan,QString label){
  if(!scan.mesh)throw std::runtime_error("Edited geometry is missing");auto before=project.scans.at(index);
  history.push(new Edit(label,[this,index,scan]{project.scans.at(index)=scan;},[this,index,before]{project.scans.at(index)=before;}));
}
}
