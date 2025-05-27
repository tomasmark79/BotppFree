#include "DiscordBot.hpp"
#include <Logger/Logger.hpp>
#include <IBot/version.h>
#include <RssManager/RssManager.hpp>

#include <thread>
#include <atomic>

#define IS_RELEASED_DISCORD_BOT

#define DISCORD_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.bot++.key"
const dpp::snowflake channelRss = 1375852042790244352;
const bool noEmbedded = false;

// Polling print feed every 47 minutes
std::atomic<bool> isPollingPrintFeedRunning (false);
std::atomic<bool> stopPollingPrintFeed (false);
#ifdef IS_RELEASED_DISCORD_BOT
int pollingPrintFeedIntervalInSec = 60 * 10;
#else
int pollingPrintFeedIntervalInSec = 3; // test purpose
#endif
bool DiscordBot::startPollingPrintFeed () {
  {
    std::thread pollingThreadPrintFeed ([&] () -> void {
      while (!stopPollingPrintFeed.load ()) {
        try {
          RssManager rssManager;
          RSSItem randomItem = rssManager.getRandomItem ();
          if (!randomItem.title_.empty ()) {
            int remainingItems = rssManager.getFeedQueueSize ();
            LOG_I_STREAM << rssManager.getItemTitle (randomItem) << " remain: " << remainingItems
                         << std::endl;

// // Discord Channel Output
#ifdef IS_RELEASED_DISCORD_BOT
            printStringToChannel (rssManager.getSimpleItemAsUrl (randomItem), channelRss, {},
                                  noEmbedded);
#endif

          } else {
            // printStringToChannel ("No items found in the feed queue.", channelRss, {}, noEmbedded);
            LOG_W_STREAM << "No items found in the feed queue." << std::endl;
          }
          isPollingPrintFeedRunning.store (true);
        } catch (const std::runtime_error& e) {
          LOG_E_STREAM << "Error: " << e.what () << std::endl;
          isPollingPrintFeedRunning.store (false);
        }
        std::this_thread::sleep_for (std::chrono::seconds (pollingPrintFeedIntervalInSec));
        // std::this_thread::sleep_for (std::chrono::milliseconds (40));
      }
    });
    pollingThreadPrintFeed.detach ();
  }
  return true;
}

// Polling fetch feed 12 hours
std::atomic<bool> isPollingFetchFeedRunning (false);
std::atomic<bool> stopPollingFetchFeed (false);

#ifdef IS_RELEASED_DISCORD_BOT
int pollingFetchFeedIntervalInSec = 60 * 60 * 2; // 2 hours
#else
int pollingFetchFeedIntervalInSec = 20; // test purpose
#endif

bool DiscordBot::startPollingFetchFeed () {
  {
    std::thread pollingThreadFetchFeed ([&] () -> void {
      while (!stopPollingFetchFeed.load ()) {
        try {
          RssManager rssManager;
          int result;
          result = rssManager.fetchFeed ("https://www.root.cz/rss/clanky/");
          result = rssManager.fetchFeed ("https://www.root.cz/rss/zpravicky/");
          result = rssManager.fetchFeed ("https://www.root.cz/rss/knihy/");
          result = rssManager.fetchFeed ("https://blog.root.cz/rss/");
          result = rssManager.fetchFeed ("https://www.root.cz/rss/skoleni");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/abc.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/diskuse.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/hardware.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/software.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/zpravicky.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/slovnik.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/kdojekdo.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/blog.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/ovladace.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/blogDigest.rss");
          result = rssManager.fetchFeed ("https://www.abclinuxu.cz/auto/faq.rss");

          LOG_I_STREAM << "Thread pollingThreadFetchFeed: Fetched " << rssManager.getItemCount ()
                       << " items from the feeds." << std::endl;
          isPollingPrintFeedRunning.store (true);
        } catch (const std::runtime_error& e) {
          LOG_E_STREAM << "Error: " << e.what () << std::endl;
          isPollingPrintFeedRunning.store (false);
        }
        std::this_thread::sleep_for (std::chrono::seconds (pollingFetchFeedIntervalInSec));
      }
    });
    pollingThreadFetchFeed.detach ();
  }
  return true;
}

/// @brief Initialize the Discord bot cluster.
/// This function reads the bot token from a file, initializes the bot cluster, sets up logging,
/// and registers event handlers for slash commands and the ready event.
/// @return Returns 0 on success, -1 on failure, and -2 if the bot is already initialized.
int DiscordBot::initCluster () {

  std::string token;
  if (this->getTokenFromFile (token) != 0) {
    LOG_E_STREAM << "Failed to read token from file: " << DISCORD_OAUTH_TOKEN_FILE << std::endl;
    return -1;
  }

  try {
    DiscordBot::bot_
        = std::make_unique<dpp::cluster> (token, dpp::i_default_intents | dpp::i_message_content);

    bot_->log (dpp::ll_debug, "Bot++");

    bot_->on_log ([&] (const dpp::log_t& log) {
      LOG_D_STREAM << "[" << dpp::utility::current_date_time () << "] "
                   << dpp::utility::loglevel (log.severity) << ": " << log.message << std::endl;
    });

    loadOnSlashCommands ();
    loadOnReadyCommands ();

    bot_->start (dpp::st_wait);
  } catch (const std::exception& e) {
    LOG_E_STREAM << "Exception during bot initialization: " << e.what () << std::endl;
    return -1;
  }
  return 0;
}

/// @brief Load on slash commands.
void DiscordBot::loadOnSlashCommands () {

  bot_->on_slashcommand ([&, this] (const dpp::slashcommand_t& event) {
    // root.cz
    if (event.command.get_command_name () == "rootclanky") {
      printFullFeedToChannel ("https://www.root.cz/rss/clanky/", channelRss, event, noEmbedded);
    }
    if (event.command.get_command_name () == "rootclanek") {
      printRandomFeedToChannel ("https://www.root.cz/rss/clanky/", channelRss, event, noEmbedded);
    }
    if (event.command.get_command_name () == "rootzpravicky") {
      printFullFeedToChannel ("https://www.root.cz/rss/zpravicky/", channelRss, event, noEmbedded);
    }
    if (event.command.get_command_name () == "rootzpravicka") {
      printRandomFeedToChannel ("https://www.root.cz/rss/zpravicky/", channelRss, event,
                                noEmbedded);
    }
    if (event.command.get_command_name () == "rootknihy") {
      printFullFeedToChannel ("https://www.root.cz/rss/knihy/", channelRss, event, noEmbedded);
    }
    if (event.command.get_command_name () == "rootkniha") {
      printRandomFeedToChannel ("https://www.root.cz/rss/knihy/", channelRss, event, noEmbedded);
    }
    if (event.command.get_command_name () == "rootblogy") {
      printFullFeedToChannel ("https://blog.root.cz/rss/", channelRss, event, noEmbedded);
    }
    if (event.command.get_command_name () == "rootblog") {
      printRandomFeedToChannel ("https://blog.root.cz/rss/", channelRss, event, noEmbedded);
    }
    if (event.command.get_command_name () == "rootskoleni") {
      printFullFeedToChannel ("https://www.root.cz/rss/skoleni", channelRss, event, noEmbedded);
    }

    // abclinuxu.cz
    if (event.command.get_command_name () == "abcclanky") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/abc.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abcotazky") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/diskuse.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abchw") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/hardware.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abcsw") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/software.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abczpravicky") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/zpravicky.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abcpojmy") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/slovnik.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abcosobnosti") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/kdojekdo.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abczapisky") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/blog.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abclzapisky") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/blogDigest.rss", channelRss, event,
                              noEmbedded);
    }
    if (event.command.get_command_name () == "abcfaq") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/faq.rss", channelRss, event, false);
    }
    if (event.command.get_command_name () == "abcovladace") {
      printFullFeedToChannel ("https://www.abclinuxu.cz/auto/ovladace.rss", channelRss, event,
                              noEmbedded);
    }

    // rss
    if (event.command.get_command_name () == "rss") {
      event.reply ("Bot++ podporuje RSS 1.0 (RDF) format a RSS 2.0 format.\n"
                   "Pro více informací navštivte: https://www.w3.org/2001/sw/Activity.html#RDF\n"
                   "Pro více informací navštivte: https://www.rssboard.org/rss-specification");
    }

    // ping pong
    if (event.command.get_command_name () == "ping") {
      event.reply ("Pong! 🏓");
    }
    // pong ping
    if (event.command.get_command_name () == "pong") {
      event.reply ("Ping! 🏓");
    }
    // gang
    if (event.command.get_command_name () == "gang") {
      dpp::message msg (event.command.channel_id, "Bang bang! 💥💥");
      event.reply (msg);
      bot_->message_create (msg);
    }
    // bot info
    if (event.command.get_command_name () == "bot") {
      dpp::embed embed
          = dpp::embed ()
                .set_color (dpp::colors::sti_blue)
                .set_title ("TuX++ "
                            + std::string (IBOT_VERSION + std::string (" 🐧 ") + DPP_VERSION_TEXT
                                           + " loaded"))
                .set_url ("https://github.com/tomasmark79/BotppFree")
                .set_author ("D🌀tName", "https://digitalspace.name",
                             "https://digitalspace.name/avatar/avatarpix.png")
                .set_description (this->getLinuxFastfetchCpp ().substr (0, 8192 - 2) + "\n")
                .set_thumbnail ("https://digitalspace.name/avatar/Linux-Logo-1996-present.png")
                .add_field (
                    "Další informace",
                    "Operační systém Linux používá Linux kernel, který vychází z myšlenek Unixu "
                    "a respektuje příslušné standardy POSIX a Single UNIX Specification.")
                //.add_field ("🩵🩵", "🩵🩵", true)
                //.add_field ("🩵🩵", "🩵🩵", true)
                .set_image ("https://digitalspace.name/avatar/Linux-Logo-1996-present.png")
                .set_footer (dpp::embed_footer ()
                                 .set_text ("Ve spolupráci s Delirium")
                                 .set_icon ("https://digitalspace.name/avatar/Delirium.png"))
                .set_timestamp (time (0));

      /* Create a message with the content as our new embed. */
      dpp::message msg (event.command.channel_id, embed);

      /* Reply to the user with the message, containing our embed. */
      event.reply (msg);
    }
  });
}

/// @brief Load commands to be executed when the bot is ready.
void DiscordBot::loadOnReadyCommands () {
  bot_->on_ready ([&] (const dpp::ready_t& event) {
    // clang-format off

    // random
    // bot_->global_command_create (dpp::slashcommand ("randomroot", "Get news!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("randomabc", "Get news!", bot_->me.id));

    // root.cz
    bot_->global_command_create (dpp::slashcommand ("rootclanky", "Aktuální články na Rootu!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("rootclanek", "Náhodný článek na Rootu!", bot_->me.id));
    
    bot_->global_command_create (dpp::slashcommand ("rootzpravicky", "Aktuální zprávičky na Rootu!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("rootzpravicka", "Náhodná zprávička na Rootu!", bot_->me.id));
    
    bot_->global_command_create (dpp::slashcommand ("rootknihy", "Knihovna na knihy.root.cz!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("rootkniha", "Náhodná kniha na knihy.root.cz!", bot_->me.id));
    
    bot_->global_command_create (dpp::slashcommand ("rootblogy", "Aktuální blogy na Rootu!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("rootblog", "Náhodný blog na Rootu!", bot_->me.id));

    bot_->global_command_create (dpp::slashcommand ("rootskoleni", "Připravovaná školení!", bot_->me.id));
    
    // abclinuxu.cz
    bot_->global_command_create (dpp::slashcommand ("abcclanky", "Články!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abcotazky", "Otázky v poradně!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abchw", "Hardwarové záznamy!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abcsw", "Softwarové záznamy!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abczpravicky", "Zprávičky!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abcpojmy", "Slovníkové pojmy!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abcosobnosti", "Osobnosti (Kdo je)!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abczapisky", "Blogové zápisky!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abclzapisky", "Linuxové blogové zápisky!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abcfaq", "Často kladené dotazy!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("abcovladace", "Ovladače!", bot_->me.id));


    // get-pollinginterval
    bot_->global_command_create (
        dpp::slashcommand ("get-pollinginterval",
                           "Get the current polling interval for RSS feeds.", bot_->me.id)
    );
    
    
    // rss
    bot_->global_command_create (dpp::slashcommand ("rss", "About RSS Support!", bot_->me.id));

    // ping pong
    bot_->global_command_create (dpp::slashcommand ("ping", "Ping pong!", bot_->me.id));

    // pong ping
    bot_->global_command_create (dpp::slashcommand ("pong", "Pong ping!", bot_->me.id));

    // gang
    bot_->global_command_create (dpp::slashcommand ("gang", "Will shoot!", bot_->me.id));

    // bot info
    bot_->global_command_create (dpp::slashcommand ("bot", "About Bot++!", bot_->me.id));

    // clang-format on

    // 5 seconds delay to ensure the commands are registered
    std::this_thread::sleep_for (std::chrono::seconds (5));
    startPollingFetchFeed ();
    startPollingPrintFeed ();
  });
}

/// @brief Get Linux system information using fastfetch command.
/// This function executes the `fastfetch` command with a specific configuration file and captures
/// the output. It returns the output as a string.
/// @note The command used is `fastfetch -c archey.jsonc --pipe --logo none`.
/// @return A string containing the output of the `fastfetch` command.
std::string DiscordBot::getLinuxFastfetchCpp () {
  constexpr size_t bufferSize = 2000;
  std::stringstream result;

  // Create unique_ptr with custom deleter for RAII
  std::unique_ptr<FILE, decltype (&pclose)> pipe (
      popen ("fastfetch -c archey.jsonc --pipe --logo none", "r"), pclose);
  if (!pipe)
    throw std::runtime_error ("Failed to run fastfetch command");

  std::array<char, bufferSize> buffer;
  while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr) {
    result << buffer.data ();
    if (result.str ().size () > bufferSize - 2)
      break;
  }

  return result.str ();
}

/// @brief  Print a string message to a specified Discord channel.
/// This function sends a message to a Discord channel, either as a reply to a slash command or as a
/// direct message. It checks the message length and channel ID before sending.
/// @param message
/// @param channelId
/// @param event
/// @param allowEmbedded
/// @return Returns 0 on success, -1 if the message is empty, -2 if the message exceeds the maximum length,
///         -3 if the channel ID is 0.
int DiscordBot::printStringToChannel (const std::string& message, dpp::snowflake channelId,
                                      const dpp::slashcommand_t& event, bool allowEmbedded) {
  if (message.empty ()) {
    LOG_W_STREAM << "Message is empty, nothing to send." << std::endl;
    return -1;
  }

  if (message.size () > DISCORD_MAX_MSG_LEN) {
    LOG_E_STREAM << "Message exceeds maximum length of " << DISCORD_MAX_MSG_LEN
                 << " characters. Message will not be sent." << std::endl;
    return -2;
  }

  if (channelId == 0) {
    LOG_W_STREAM << "Channel ID is 0, message will not be sent." << std::endl;
    return -3;
  }

  dpp::message msg (channelId, message);
  if (!allowEmbedded) {
    msg.set_flags (dpp::m_suppress_embeds); // Suppress embeds if allowEmbedded is false
  }
  if (event.command.id != 0) {
    event.reply (msg);
  } else {
    bot_->message_create (msg);
  }
  LOG_I_STREAM << "Message sent to channel " << channelId << ": " << message << std::endl;

  return 0;
}

/// @brief Send RSS feed via slash command reply
/// @param url RSS feed URL
/// @param event Slash command event
/// @param allowEmbedded Whether to allow Discord embeds
/// @param fullFeed If true, sends full feed; if false, sends random item
void DiscordBot::sendRssFeedViaReply (const std::string& url, const dpp::slashcommand_t& event,
                                      bool allowEmbedded, bool fullFeed) {
  event.reply ("This feature is not implemented yet. Please try again later.");
  // FeedFetcher feedFetcher;
  // std::string rssFeed = fullFeed ? feedFetcher.feedFromUrl (url) : feedFetcher.feedRandomFromUrl (url);
  // if (rssFeed.empty ()) {
  //   event.reply ("Failed to fetch RSS feed. Please try again later.");
  // } else {
  //   dpp::message msg (event.command.channel_id, rssFeed);
  //   if (!allowEmbedded) {
  //     msg.set_flags (dpp::m_suppress_embeds);
  //   }
  //   event.reply (msg);
  // }
}

/// @brief Send RSS feed via direct message to channel
/// @param url RSS feed URL
/// @param channelId Target channel ID
/// @param allowEmbedded Whether to allow Discord embeds
/// @param fullFeed If true, sends full feed; if false, sends random item
void DiscordBot::sendRssFeedToChannel (const std::string& url, dpp::snowflake channelId,
                                       bool allowEmbedded, bool fullFeed) {
  dpp::message msg (channelId, "This feature is not implemented yet. Please try again later.");
  // FeedFetcher feedFetcher;
  // std::string rssFeed = fullFeed ? feedFetcher.feedFromUrl (url) : feedFetcher.feedRandomFromUrl (url);
  // if (rssFeed.empty ()) {
  //   bot_->message_create (
  //       dpp::message (channelId, "Failed to fetch RSS feed. Please try again later."));
  // } else {
  //   dpp::message msg (channelId, rssFeed);
  //   if (!allowEmbedded) {
  //     msg.set_flags (dpp::m_suppress_embeds);
  //   }
  //   bot_->message_create (msg);
  // }
}

/// @brief Print the full RSS feed to a specified Discord channel.
void DiscordBot::printFullFeedToChannel (const std::string& url, dpp::snowflake channelId,
                                         const dpp::slashcommand_t& event, bool allowEmbedded) {
  sendRssFeedViaReply (url, event, allowEmbedded, true);
}

/// @brief Print a random item from the RSS feed to a specified Discord channel.
void DiscordBot::printRandomFeedToChannel (const std::string& url, dpp::snowflake channelId,
                                           const dpp::slashcommand_t& event, bool allowEmbedded) {
  sendRssFeedViaReply (url, event, allowEmbedded, false);
}

// /// @brief Print the full RSS feed to a specified Discord channel.
// /// This function fetches the RSS feed from the provided URL and sends it as a message to the specified channel.
// /// If the feed cannot be fetched, it sends an error message.
// /// @param url The URL of the RSS feed.
// /// @param channelId The ID of the Discord channel where the feed will be sent.
// void DiscordBot::printFullFeedToChannel (const std::string& url, dpp::snowflake channelId,
//                                          const dpp::slashcommand_t& event, bool allowEmbedded) {
//   RssReader rssReader;
//   std::string rssFeed = rssReader.feedFromUrl (url);
//   if (rssFeed.empty ()) {
//     bot_->message_create (
//         dpp::message (channelId, "Failed to fetch RSS feed. Please try again later."));
//   } else {
//     dpp::message msg (channelId, rssFeed);
//     if (!allowEmbedded) {
//       msg.set_flags (dpp::m_suppress_embeds); // Suppress embeds if allowEmbedded is false
//     }
//     bot_->message_create (msg);
//     event.reply (msg);
//   }
// }

// /// @brief Print a random item from the RSS feed to a specified Discord channel.
// /// This function fetches a random item from the RSS feed at the provided URL and sends it as a message
// /// to the specified channel. If the feed cannot be fetched, it sends an error message.
// /// @param url The URL of the RSS feed from which a random item will be fetched.
// /// @param channelId The ID of the Discord channel where the random item will be sent.
// void DiscordBot::printRandomFeedToChannel (const std::string& url, dpp::snowflake channelId,
//                                            const dpp::slashcommand_t& event, bool allowEmbedded) {
//   RssReader rssReader;
//   std::string rssFeed = rssReader.feedRandomFromUrl (url);
//   if (rssFeed.empty ()) {
//     bot_->message_create (
//         dpp::message (channelId, "Failed to fetch RSS feed. Please try again later."));
//   } else {
//     dpp::message msg (channelId, rssFeed);
//     bot_->message_create (msg);
//     if (!allowEmbedded) {
//       msg.set_flags (dpp::m_suppress_embeds); // Suppress embeds if allowEmbedded is false
//     }
//     event.reply (msg);
//   }
// }

int DiscordBot::getTokenFromFile (std::string& token) {
  std::ifstream tokenFile (DISCORD_OAUTH_TOKEN_FILE);
  if (!tokenFile.is_open ()) {
    LOG_E_STREAM << "Failed to open token file: " << DISCORD_OAUTH_TOKEN_FILE << std::endl;
    return -1;
  }
  std::getline (tokenFile, token);
  tokenFile.close ();
  if (token.empty ()) {
    LOG_E_STREAM << "Token file is empty or invalid." << std::endl;
    return -1;
  }
  return 0;
}