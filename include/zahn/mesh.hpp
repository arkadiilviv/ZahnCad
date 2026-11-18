#pragma once
#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
namespace zahn {
struct Vec3 { double x{}, y{}, z{}; };
struct Triangle { std::array<Vec3,3> v; };
using Color = std::array<unsigned char,3>;
struct Mesh { std::vector<Triangle> triangles; std::vector<std::array<Color,3>> colors; };
struct Report {
  size_t triangles{}, degenerate{}, boundaryEdges{}, nonManifoldEdges{}, inconsistentEdges{};
  Vec3 minimum{}, maximum{};
  bool closed() const { return triangles && !degenerate && !boundaryEdges && !nonManifoldEdges && !inconsistentEdges; }
  std::string text() const;
};
using Checkpoint = std::function<void()>;
Mesh readStl(const std::filesystem::path& path, const Checkpoint& checkpoint = {});
void writeStl(const Mesh&, const std::filesystem::path&);
Report inspect(const Mesh&, const Checkpoint& checkpoint = {});
Mesh scaled(const Mesh&, double factor, const Checkpoint& checkpoint = {});
}
