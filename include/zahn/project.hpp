#pragma once
#include "mesh.hpp"
#include <QString>
#include <QByteArray>
#include <QJsonArray>
#include <vector>
#include <memory>
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
  std::vector<Vec3> margin;
};
struct Project { QString id="Untitled case"; std::vector<Scan> scans; };
void saveProject(const Project&,const QString&);
Project loadProject(const QString&, const Checkpoint& checkpoint = {});
std::filesystem::path nativePath(const QString&);
Scan importScan(const QString&, const QString& role, double scale, const Checkpoint& checkpoint = {});
void saveRecovery(const Project&, const QString& directory);
Project loadRecovery(const QString& directory, const Checkpoint& checkpoint = {});
}
