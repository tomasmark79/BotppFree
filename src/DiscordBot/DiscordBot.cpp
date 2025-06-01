#include "DiscordBot.hpp"
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <IBot/version.h>
#include <RssManager/RssManager.hpp>
#include <thread>
#include <atomic>

// Discord message max length (as per Discord API docs)
constexpr size_t DISCORD_MAX_MSG_LEN = 2000;

#define IS_RELEASED_DISCORD_BOT
// #define TESTING_DISCORD_BOT

#define DISCORD_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.bot++.key"
const dpp::snowflake channelRss = 1375852042790244352;
RssManager rss;

DiscordBot::DiscordBot () {
  rss.initialize ();
}

// Polling print feed every 23 minutes
std::atomic<bool> isPollingPrintFeedRunning (false);
std::atomic<bool> stopPollingPrintFeed (false);
#ifdef IS_RELEASED_DISCORD_BOT
  #ifdef TESTING_DISCORD_BOT
int pollingPrintFeedIntervalInSec = 10; // 10 seconds
  #else
int pollingPrintFeedIntervalInSec = 60 * 23; // 23 minutes
  #endif
#else
int pollingPrintFeedIntervalInSec = 1; // test purpose
#endif
bool DiscordBot::startPollingPrintFeed () {
  {
    std::thread pollingThreadPrintFeed ([&] () -> void {
      while (!stopPollingPrintFeed.load ()) {
        try {
          RSSItem item = rss.getRandomItem ();
          if (!item.title.empty ()) {
            LOG_D_STREAM << "Random item: " << item.title << " emb: " << item.embedded << std::endl;
#ifdef IS_RELEASED_DISCORD_BOT
            printStringToChannel (item.toMarkdownLink (), channelRss, {}, item.embedded);
            // printStringToChannelAsThread (item.toMarkdownLink (), channelRss, item.title, false);
#endif
          } else {
            LOG_W_STREAM << "No items found in the feed queue." << std::endl;
          }
          isPollingPrintFeedRunning.store (true);
        } catch (const std::runtime_error& e) {
          LOG_E_STREAM << "Error: " << e.what () << std::endl;
          isPollingPrintFeedRunning.store (false);
        }
        std::this_thread::sleep_for (std::chrono::seconds (pollingPrintFeedIntervalInSec));
      }
    });
    pollingThreadPrintFeed.detach ();
  }
  return true;
}

// Polling fetch feeds
std::atomic<bool> isPollingFetchFeedRunning (false);
std::atomic<bool> stopPollingFetchFeed (false);

#ifdef IS_RELEASED_DISCORD_BOT
int pollingFetchFeedIntervalInSec = 60 * 60 * 2; // 2 hours
#else
int pollingFetchFeedIntervalInSec = 5; // test purpose
#endif

bool DiscordBot::startPollingFetchFeed () {
  {
    std::thread pollingThreadFetchFeed ([&] () -> void {
      while (!stopPollingFetchFeed.load ()) {
        try {

          rss.fetchAllFeeds ();

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

    // bot_->log (dpp::ll_debug, "Bot++");
    bot_->log (dpp::ll_info, "Bot++");

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
    // listsources
    if (event.command.get_command_name () == "listsources") {
      std::string sources = rss.getSourcesAsList ();
      if (sources.empty ()) {
        event.reply ("No RSS sources found.");
      } else {
        dpp::message msg (event.command.channel_id, sources);
        event.reply (msg);
        // bot_->message_create (msg);
      }
      return;
    }

    // getfeednow
    if (event.command.get_command_name () == "getfeednow") {
      try {
        RSSItem item = rss.getRandomItem ();
        if (!item.title.empty ()) {
          LOG_D_STREAM << "getfeednow Random item: " << item.title << " emb: " << item.embedded
                       << std::endl;
#ifdef IS_RELEASED_DISCORD_BOT
          printStringToChannel (item.toMarkdownLink (), channelRss, event, item.embedded);
#endif
        } else {
          LOG_W_STREAM << "No items found in the feed queue." << std::endl;
        }
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
      }
    }

    // addsource
    if (event.command.get_command_name () == "addsource") {
      std::string url = std::get<std::string> (event.get_parameter ("url"));

      bool embedded = false;
      auto embedded_param = event.get_parameter ("embedded");
      if (embedded_param.index () != 0) { // Check if parameter exists (not std::monostate)
        embedded = std::get<bool> (embedded_param);
      }

      if (url.empty ()) {
        event.reply ("Error: URL parameter is required.");
        return;
      }

      try {
        int result = rss.addUrl (url, embedded);
        if (result == -1) {
          LOG_W_STREAM << "URL already exists: " << url << std::endl;
          event.reply ("Warning: URL already exists.");
          return;
        }
        event.reply ("Source added successfully: " + url);
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error adding source: " << e.what () << std::endl;
        event.reply ("Error adding source: " + std::string (e.what ()));
        return;
      }

      rss.fetchAllFeeds ();

      return;
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
                .add_field ("Další informace",
                            "Operační systém Linux používá Linux kernel, který vychází z myšlenek "
                            "Unixu "
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

    // getfeednow
    bot_->global_command_create (dpp::slashcommand ("getfeednow", "Get RSS feed now",
                                                   bot_->me.id));
    // list sources - list all RSS sources
    bot_->global_command_create (dpp::slashcommand ("listsources", "List all RSS sources",
                                                   bot_->me.id));

    // add source - supported rss1.0 and rss2.0 and Atom feeds
    // example of use command: /addsource url:https://www.root.cz/rss/clanky/ embedded:true
    bot_->global_command_create (dpp::slashcommand ("addsource", "Add a new RSS source",
                                                   bot_->me.id)
                                     .add_option (dpp::command_option (
                                         dpp::co_string, "url", "URL of the RSS feed", true))
                                     .add_option (dpp::command_option (
                                         dpp::co_boolean, "embedded",
                                         "Whether the feed should be embedded in the message",
                                         false)));
                                         
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

#ifdef TESTING_DISCORD_BOT
  dpp::message msg (channelId, message + " test action");
#else
  dpp::message msg (channelId, message);
#endif

  if (!allowEmbedded) {
    msg.set_flags (dpp::m_suppress_embeds); // Suppress embeds if allowEmbedded is false
  }

  if (event.command.id != 0) {
    // If event is a slash command, reply to it
    event.reply (msg);
    LOG_I_STREAM << "Message replied to slash command in channel " << event.command.channel_id
                 << ": " << message << std::endl;
  } else {

    // If no event, send as a direct message to the channel
    // bot_->message_create (msg);
    // Nejdřív vytvořte zprávu
    bot_->message_create (msg, [this, message,
                                channelId] (const dpp::confirmation_callback_t& callback) {
      if (callback.is_error ()) {
        LOG_E_STREAM << "Failed to create message: " << callback.get_error ().message << std::endl;
        return;
      }

      const auto& createdMessage = callback.get<dpp::message> ();
      LOG_I_STREAM << "Message created successfully with ID: " << createdMessage.id << std::endl;
      LOG_I_STREAM << "Message sent to channel " << channelId << ": " << message << std::endl;

      // Pak ji crosspostněte (pouze pokud je kanál announcement kanál)
      bot_->message_crosspost (createdMessage.id, createdMessage.channel_id,
                               [this] (const dpp::confirmation_callback_t& callback) {
                                 if (callback.is_error ()) {
                                   LOG_E_STREAM << "Failed to crosspost message: "
                                                << callback.get_error ().message << std::endl;
                                 } else {
                                   LOG_I_STREAM << "Message crossposted successfully" << std::endl;
                                 }
                               });
    });
  }
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

// ...existing code...

/// @brief Print a string message to a specified Discord channel as a new thread.
/// This function sends a message to a Discord channel and creates a new thread for it.
/// It checks the message length and channel ID before sending.
/// @param message The message content
/// @param channelId The target channel ID
/// @param threadName The name for the new thread (optional, defaults to first 100 chars of message)
/// @param allowEmbedded Whether to allow Discord embeds
/// @return Returns 0 on success, -1 if the message is empty, -2 if the message exceeds the maximum length,
///         -3 if the channel ID is 0, -4 if thread creation fails
int DiscordBot::printStringToChannelAsThread (const std::string& message, dpp::snowflake channelId,
                                              const std::string& threadName, bool allowEmbedded) {
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

  // Generate thread name if not provided
  std::string finalThreadName = threadName;
  if (finalThreadName.empty ()) {
    finalThreadName = message.substr (0, 100); // Discord thread name limit is 100 chars
    if (message.size () > 100) {
      finalThreadName += "...";
    }
  }

#ifdef TESTING_DISCORD_BOT
  dpp::message msg (channelId, message + " test action");
#else
  dpp::message msg (channelId, message);
#endif

  if (!allowEmbedded) {
    msg.set_flags (dpp::m_suppress_embeds);
  }

  // First create the message
  bot_->message_create (msg, [this, finalThreadName, channelId,
                              message] (const dpp::confirmation_callback_t& callback) {
    if (callback.is_error ()) {
      LOG_E_STREAM << "Failed to create message: " << callback.get_error ().message << std::endl;
      return;
    }

    const auto& createdMessage = callback.get<dpp::message> ();
    LOG_I_STREAM << "Message created successfully with ID: " << createdMessage.id << std::endl;

    // Then create a thread from that message
    bot_->thread_create_with_message (
        finalThreadName, channelId, createdMessage.id, 60, // auto_archive_duration in minutes
        0, // rate_limit_per_user (0 = no rate limit)
        [this, channelId, message] (const dpp::confirmation_callback_t& thread_callback) {
          if (thread_callback.is_error ()) {
            LOG_E_STREAM << "Failed to create thread: " << thread_callback.get_error ().message
                         << std::endl;
            return;
          }

          const auto& createdThread = thread_callback.get<dpp::thread> ();
          LOG_I_STREAM << "Thread created successfully with ID: " << createdThread.id << std::endl;
          LOG_I_STREAM << "Message sent to channel " << channelId << " as thread: " << message
                       << std::endl;
        });
  });

  return 0;
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