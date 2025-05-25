#include "DiscordBot.hpp"
#include <Logger/Logger.hpp>
#include <IBot/version.h>
#include <RssReader/RssReader.hpp>

#include <thread>
#include <atomic>

#define DISCORD_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.bot++.key"
const dpp::snowflake channelRss = 1375852042790244352;
const bool noEmbedded = false;

std::atomic<bool> isRootOnTheLine (false);
std::atomic<bool> isPollingRunning (false);
std::atomic<bool> stopPolling (false);
int pollingIntervalInSec = 60 * 60; // Default polling interval set to 45 minutes
// int pollingIntervalInSec = 10;

/// @brief Thread function to start polling for RSS feeds.
/// This function runs in a separate thread and continuously fetches RSS feeds at specified intervals.
/// It checks the stopPolling atomic variable to determine when to stop polling.
/// If an error occurs during polling, it logs the error and sets isPollingRunning to false.
/// @return Returns true if polling started successfully, false otherwise.
bool DiscordBot::startPolling () {
  {
    std::thread pollingThread ([&] () -> void {
      while (!stopPolling.load ()) {
        try {
          if (isRootOnTheLine.load ()) {
            sendRssFeedToChannel ("https://www.root.cz/rss/clanky/", channelRss, false, false);
            isRootOnTheLine.store (false);
          } else {
            sendRssFeedToChannel ("https://www.abclinuxu.cz/auto/abc.rss", channelRss, false, false);
            isRootOnTheLine.store (true);
          }
          isPollingRunning.store (true);
        } catch (const std::runtime_error& e) {
          LOG_E_STREAM << "Error: " << e.what () << std::endl;
          isPollingRunning.store (false);
        }
        std::this_thread::sleep_for (std::chrono::seconds (pollingIntervalInSec));
      }
    });
    pollingThread.detach ();
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
    // random
    // if (event.command.get_command_name () == "randomroot") {
    //   printRandomFeedToChannel ("https://www.root.cz/rss/clanky/", channelRss, event);
    // }
    // if (event.command.get_command_name () == "randomabc") {
    //   printRandomFeedToChannel ("https://www.abclinuxu.cz/auto/abc.rss", channelRss, event);
    // }
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

    // set-pollinginterval
    if (event.command.get_command_name () == "set-pollinginterval") {
      if (std::holds_alternative<int64_t> (event.get_parameter ("interval"))) {
        pollingIntervalInSec
            = static_cast<int> (std::get<int64_t> (event.get_parameter ("interval")));
        event.reply ("Polling interval set to " + std::to_string (pollingIntervalInSec)
                     + " seconds.");
      } else {
        event.reply ("Invalid interval parameter. Please provide an integer value.");
      }
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

    // set-pollinginterval
    bot_->global_command_create (
        dpp::slashcommand ("set-pollinginterval",
                           "Set the polling interval for RSS feeds in seconds (default: 1800 seconds).",
                           bot_->me.id)
            .add_option (
                dpp::command_option (dpp::co_integer, "seconds", "Polling interval in seconds", true)
            )
    );

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

    startPolling ();
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

/// @brief Send RSS feed via slash command reply
/// @param url RSS feed URL
/// @param event Slash command event
/// @param allowEmbedded Whether to allow Discord embeds
/// @param fullFeed If true, sends full feed; if false, sends random item
void DiscordBot::sendRssFeedViaReply (const std::string& url, const dpp::slashcommand_t& event,
                                      bool allowEmbedded, bool fullFeed) {
  RssReader rssReader;
  std::string rssFeed = fullFeed ? rssReader.feedFromUrl (url) : rssReader.feedRandomFromUrl (url);

  if (rssFeed.empty ()) {
    event.reply ("Failed to fetch RSS feed. Please try again later.");
  } else {
    dpp::message msg (event.command.channel_id, rssFeed);
    if (!allowEmbedded) {
      msg.set_flags (dpp::m_suppress_embeds);
    }
    event.reply (msg);
  }
}

/// @brief Send RSS feed via direct message to channel
/// @param url RSS feed URL
/// @param channelId Target channel ID
/// @param allowEmbedded Whether to allow Discord embeds
/// @param fullFeed If true, sends full feed; if false, sends random item
void DiscordBot::sendRssFeedToChannel (const std::string& url, dpp::snowflake channelId,
                                       bool allowEmbedded, bool fullFeed) {
  RssReader rssReader;
  std::string rssFeed = fullFeed ? rssReader.feedFromUrl (url) : rssReader.feedRandomFromUrl (url);

  if (rssFeed.empty ()) {
    bot_->message_create (
        dpp::message (channelId, "Failed to fetch RSS feed. Please try again later."));
  } else {
    dpp::message msg (channelId, rssFeed);
    if (!allowEmbedded) {
      msg.set_flags (dpp::m_suppress_embeds);
    }
    bot_->message_create (msg);
  }
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