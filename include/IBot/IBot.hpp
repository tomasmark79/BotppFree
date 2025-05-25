// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#ifndef __IBOT_HPP
#define __IBOT_HPP

#include <IBot/version.h>
#include <filesystem>
#include <string>

// Public API

namespace dotname {

  class IBot {

    const std::string libName_ = std::string ("IBot v.") + IBOT_VERSION;

  public:
    IBot ();
    IBot (const std::filesystem::path& assetsPath);
    ~IBot ();
  };

} // namespace dotname

#endif // __IBOT_HPP