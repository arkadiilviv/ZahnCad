#pragma once
#include "project.hpp"
#include <QUndoStack>
namespace zahn {
// Meshes and original bytes are shared across history; display edits never copy geometry.
class Document {
public:
  Project project;
  QUndoStack history;
  void reset(Project value = {}, bool recovered = false);
  void addScan(Scan scan, QString label="Import scan");
  void removeScan(size_t index);
  void setDisplay(size_t index, bool visible, double opacity);
  void replaceScan(size_t index, Scan scan, QString label);
  void setReference(ReferenceMeasurement reference, QString label);
  bool dirty() const { return recovered_ || !history.isClean(); }
  void markSaved() { recovered_ = false; history.setClean(); }
private:
  bool recovered_ = false;
};
}
