// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <IBot/IBot.hpp>
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <RssReader/RssReader.hpp>

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

      // // testy
      // RssReader rssReader;
      // std::string rssFeed1 = rssReader.feedRandomFromUrl ("www.abclinuxu.cz/auto/abc.rss", 1);
      // if (rssFeed1.empty ()) {
      //   LOG_E_STREAM << "Failed to fetch RSS feed." << std::endl;
      // } else {
      //   LOG_I_STREAM << "Fetched RSS feed successfully:\n" << rssFeed1 << std::endl;
      // }

      // std::string rssFeed2 = rssReader.feedRandomFromUrl ("https://www.root.cz/rss/clanky/");
      // if (rssFeed2.empty ()) {
      //   LOG_E_STREAM << "Failed to fetch RSS feed." << std::endl;
      // } else {
      //   LOG_I_STREAM << "Fetched RSS feed successfully:\n" << rssFeed2 << std::endl;
      // }

    }
  }

  IBot::~IBot () {
    LOG_D_STREAM << libName_ << " ... destructed" << std::endl;
  }

} // namespace dotname