#ifndef __ASSETCONTEXT_H__
#define __ASSETCONTEXT_H__

#include <filesystem>

namespace AssetContext {
  void clearAssetsPath (void);
  void setAssetsPath (const std::filesystem::path& path);
  const std::filesystem::path& getAssetsPath ();
}

#endif // __ASSETCONTEXT_H__