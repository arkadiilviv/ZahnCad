#pragma once
#include "mesh.hpp"
#include <QString>
#include <QByteArray>
#include <QJsonArray>
#include <vector>
#include <memory>
#include <optional>
#include <map>
namespace zahn {
struct Scan {
  QString name, role;
  QByteArray original;
  std::shared_ptr<const Mesh> mesh;
  double scale=1;
  bool visible=true;
  double opacity=1;
  QString format="stl";
  std::shared_ptr<const Mesh> sourceMesh;
  QJsonArray operations;
  // Key 0 preserves an unassigned margin. Numbered teeth may have empty traces.
  std::map<int,std::vector<Vec3>> toothMargins;
  int toothFdi=0;
  bool hasMargins() const;
  void clearMargins();
};
struct ReferenceReading {
  QString scanName, scanRole, sourceSha256, appVersion, platform, capturedUtc;
  double scaleToMm{}, measuredMm{};
  bool afterReopen{};
};
struct ReferenceMeasurement {
  static constexpr size_t readingLimit=200;
  QString landmarks, method, notes;
  double expectedMm{}, toleranceMm{};
  std::vector<ReferenceReading> readings;
};
struct Project {
  QString id="Untitled case";
  std::vector<Scan> scans;
  std::optional<ReferenceMeasurement> reference;
};
inline bool validToothFdi(int tooth) {
  return tooth>=11&&tooth<=48&&tooth%10>=1&&tooth%10<=8;
}
void saveProject(const Project&,const QString&);
Project loadProject(const QString&, const Checkpoint& checkpoint = {});
std::filesystem::path nativePath(const QString&);
Scan importScan(const QString&, const QString& role, double scale, const Checkpoint& checkpoint = {});
void saveRecovery(const Project&, const QString& directory);
Project loadRecovery(const QString& directory, const Checkpoint& checkpoint = {});
}
