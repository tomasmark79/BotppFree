// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <IBot/IBot.hpp>
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <RssManager/RssManager.hpp>

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

      // DiscordBot dbot;
      // if (dbot.initCluster ()) {
      //   LOG_I_STREAM << "Discord bot initialized successfully." << std::endl;
      // } else {
      //   LOG_E_STREAM << "Failed to initialize Discord bot." << std::endl;
      // }

      FeedFetcher feedFetcher;
      FeedPicker feedPicker;
      FeedPrinter feedPrinter;

      while (1) {
        // rssFeed = feedFetcher.feedFromUrl ("www.abclinuxu.cz/auto/abc.rss", 1);
        int result = feedFetcher.fetchFeed ("https://www.abclinuxu.cz/auto/abc.rss");

        // Pick a random item from the feed
        RSSItem randomItem = feedPicker.pickUpRandomItem ();

        // Check if the item is valid
        if (randomItem.title_.empty ()) {
          LOG_E_STREAM << "No more items available in the feed queue." << std::endl;
          std::this_thread::sleep_for (std::chrono::seconds (5));
          continue; // Skip to the next iteration if no items are available
        }

        // Print item
        LOG_I_STREAM << "Random item from feed:\n"
                     << feedPrinter.printItem (randomItem) << std::endl;


        // Simulate a delay for testing purposes
        std::this_thread::sleep_for (std::chrono::seconds (2));
      }

      // std::string rssFeed2 = feedFetcher.feedRandomFromUrl ("https://www.root.cz/rss/clanky/");
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