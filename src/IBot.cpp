// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <IBot/IBot.hpp>
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <DiscordBot/DiscordBot.hpp>
#include <Utils/Utils.hpp>

#if defined(PLATFORM_WEB)
  #include <emscripten/emscripten.h>
#endif

namespace dotname {

  IBot::IBot () {
    LOG_D_STREAM << libName_ << " constructed ..." << std::endl;
    AssetContext::clearAssetsPath ();
  }

  IBot::IBot (const std::filesystem::path& assetsPath) : IBot () {
    if (!assetsPath.empty ()) {
      AssetContext::setAssetsPath (assetsPath);
      LOG_D_STREAM << "Assets path given to the library\n"
                   << "╰➤ " << AssetContext::getAssetsPath () << std::endl;
      auto logo = std::ifstream (AssetContext::getAssetsPath () / "logo.png");

      DiscordBot dbot;
      if (dbot.initCluster ()) {
        LOG_I_STREAM << "Discord bot initialized successfully." << std::endl;
      } else {
        LOG_E_STREAM << "Failed to initialize Discord bot." << std::endl;
      }
    }
  }

  IBot::~IBot () {
    LOG_D_STREAM << libName_ << " ... destructed" << std::endl;
  }

} // namespace dotname