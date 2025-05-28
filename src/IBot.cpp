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

      // Fetch Process
      //while (1)
      //   {
      //     RssManager rssManager;
      //     // fetch in interval
      //     int itemCount = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/abc.rss");
      //     +rssManager.fetchFeed ("https://www.root.cz/rss/clanky/");

      //     if (itemCount > 0) {
      //       RSSItem randomItem = rssManager.getRandomItem ();
      //       if (!randomItem.title_.empty ()) {
      //         LOG_I_STREAM << rssManager.printItem (randomItem) << std::endl;
      //       }
      //     }
      //     std::this_thread::sleep_for (std::chrono::seconds (2));
      //   }

      //   // Just pick up random item from the feed
      //   while (1) {
      //     RssManager rssManager;
      //     RSSItem randomItem = rssManager.getRandomItem ();
      //     if (!randomItem.title_.empty ()) {
      //       LOG_I_STREAM << rssManager.printItem (randomItem) << std::endl;
      //     }
      //     else
      //     {
      //       LOG_W_STREAM << "No items found in the feed queue." << std::endl;
      //     }
      //     std::this_thread::sleep_for (std::chrono::seconds (2));
      //   }
    }
  }

  IBot::~IBot () {
    LOG_D_STREAM << libName_ << " ... destructed" << std::endl;
  }

} // namespace dotname