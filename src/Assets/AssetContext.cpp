#include "AssetContext.hpp"

namespace {
  std::filesystem::path g_assetsPath;
}

namespace AssetContext {

  void clearAssetsPath (void) {
    g_assetsPath.clear ();
  }

  void setAssetsPath (const std::filesystem::path& path) {
    g_assetsPath = path;
  }

  const std::filesystem::path& getAssetsPath () {
    return g_assetsPath;
  }
}