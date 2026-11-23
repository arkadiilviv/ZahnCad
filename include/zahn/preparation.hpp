#pragma once
#include "mesh.hpp"

namespace zahn {
using LandmarkTriplet = std::array<Vec3, 3>;

struct RigidAlignment {
  std::array<double, 16> matrix{};
  double rms{}, maximum{};
  Vec3 apply(Vec3 point) const;
};

RigidAlignment alignLandmarks(const LandmarkTriplet& moving, const LandmarkTriplet& fixed);
Mesh transformMesh(const Mesh&, const RigidAlignment&, const Checkpoint& = {});

struct InsertionReport {
  Mesh shadowed;
  size_t sampled{}, undercuts{};
  double maximumDepth{};
};

// The axis points toward the insertion viewer. The blockout extends to a base
// one grid spacing below the lowest source point in that coordinate frame.
InsertionReport analyzeInsertion(const Mesh&, Vec3 axis, const Checkpoint& = {});
Mesh blockout(const Mesh&, Vec3 axis, double spacing, const Checkpoint& = {});
Mesh researchCrown(const std::vector<Vec3>& margin, Vec3 axis, double height,
                   const Checkpoint& = {}, const Mesh* preparation=nullptr);
}
