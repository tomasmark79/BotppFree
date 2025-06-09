#include "DiscordBot.hpp"
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <IBot/version.h>
#include <RssManager/RssManager.hpp>
#include <thread>
#include <atomic>

#define PUBLIC_RELEASED_DISCORD_BOT

constexpr size_t DISCORD_MAX_MSG_LEN = 2000; // (as per Discord API docs)
#define DISCORD_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.bot++.key"
const dpp::snowflake channelRss = 1375852042790244352;
RssManager rss;

DiscordBot::DiscordBot () {
  rss.initialize ();
}

//              _ _
//             | | |
//  _ __   ___ | | | ___ _ __
// | '_ \ / _ \| | |/ _ \ '__|
// | |_) | (_) | | |  __/ |
// | .__/ \___/|_|_|\___|_|
// | |
// |_|
// Printing feed items
std::atomic<bool> isPollingPrintFeedRunning (false);
std::atomic<bool> stopPollingPrintFeed (false);
#ifdef PUBLIC_RELEASED_DISCORD_BOT
int pollingPrintFeedIntervalInSec = 60 * 23; // 23 minutes
#else
int pollingPrintFeedIntervalInSec = 4;
#endif
bool DiscordBot::startPollingPrintFeed () {
  std::thread pollingThreadPrintFeed ([&] () -> void {
    while (!stopPollingPrintFeed.load ()) {
      try {
        RSSItem item = rss.getRandomItem ();
        if (!item.title.empty ()) {
          printStringToChannel (item.toMarkdownLink (), channelRss, {}, item.embedded);
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
  return true;
}

//              _ _
//             | | |
//  _ __   ___ | | | ___ _ __
// | '_ \ / _ \| | |/ _ \ '__|
// | |_) | (_) | | |  __/ |
// | .__/ \___/|_|_|\___|_|
// | |
// |_|
// Fetching feeds
std::atomic<bool> isPollingFetchFeedRunning (false);
std::atomic<bool> stopPollingFetchFeed (false);
#ifdef PUBLIC_RELEASED_DISCORD_BOT
int pollingFetchFeedIntervalInSec = 60 * 60 * 2; // 2 hours
#else
int pollingFetchFeedIntervalInSec = 15;
#endif

bool DiscordBot::startPollingFetchFeed () {
  std::thread pollingThreadFetchFeed ([&] () -> void {
    while (!stopPollingFetchFeed.load ()) {
      try {
        rss.fetchAllFeeds ();
        isPollingFetchFeedRunning.store (true);
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
        isPollingFetchFeedRunning.store (false);
      }
      std::this_thread::sleep_for (std::chrono::seconds (pollingFetchFeedIntervalInSec));
    }
  });
  pollingThreadFetchFeed.detach ();
  return true;
}

//  _       _ _
// (_)     (_) |
//  _ _ __  _| |_
// | | '_ \| | __|
// | | | | | | |_
// |_|_| |_|_|\__|
// initialize the Discord bot cluster
int DiscordBot::initCluster () {

  // token management
  std::string token;
  if (this->getTokenFromFile (token) != 0) {
    LOG_E_STREAM << "Failed to read token from file: " << DISCORD_OAUTH_TOKEN_FILE << std::endl;
    return -1;
  }

  try {
    DiscordBot::bot_
        = std::make_unique<dpp::cluster> (token, dpp::i_default_intents | dpp::i_message_content);

    // onlog callback
    bot_->log (dpp::ll_info, "Bot++");
    bot_->on_log ([&] (const dpp::log_t& log) {
      LOG_D_STREAM << "[" << dpp::utility::current_date_time () << "] "
                   << dpp::utility::loglevel (log.severity) << ": " << log.message << std::endl;
    });

    // slash commands and ready commands
    loadOnSlashCommands ();
    loadOnReadyCommands ();

    // Set the bot's presence
    bot_->start (dpp::st_wait);
  } catch (const std::exception& e) {
    LOG_E_STREAM << "Exception during bot initialization: " << e.what () << std::endl;
    return -1;
  }
  return 0;
}

//      _           _
//     | |         | |
//  ___| | __ _ ___| |__
// / __| |/ _` / __| '_ \ 
// \__ \ | (_| \__ \ | | |
// |___/_|\__,_|___/_| |_|
// onSlashCommands
void DiscordBot::loadOnSlashCommands () {

  bot_->on_slashcommand ([&, this] (const dpp::slashcommand_t& event) {
    if (event.command.get_command_name () == "queue") {
      try {
        size_t itemCount = rss.getItemCount ();
        if (itemCount == 0) {
          event.reply ("No items in the RSS feed queue.");
          return;
        }
        std::string response
            = "RSS feed queue contains " + std::to_string (itemCount) + " items.\n";
        LOG_I_STREAM << response;
        event.reply (response);
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
      }
    }
    if (event.command.get_command_name () == "listsources") {
      std::string sources = rss.getSourcesAsList ();
      if (sources.empty ()) {
        event.reply ("No RSS sources found.");
      } else {
        dpp::message msg (event.command.channel_id, sources);
        event.reply (msg);
      }
      return;
    }
    if (event.command.get_command_name () == "getfeednow") {
      try {
        RSSItem item = rss.getRandomItem ();
        if (!item.title.empty ()) {
          printStringToChannel (item.toMarkdownLink (), channelRss, event, item.embedded);
        } else {
          LOG_W_STREAM << "No items found in the feed queue." << std::endl;
          event.reply ("No items found in the RSS feed queue.");
        }
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
      }
    }
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
    if (event.command.get_command_name () == "ping") {
      event.reply ("Pong! 🏓");
    }
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
      dpp::message msg (event.command.channel_id, embed);
      event.reply (msg);
    }
  });
}

//                     _
//                    | |
//  _ __ ___  __ _  __| |_   _
// | '__/ _ \/ _` |/ _` | | | |
// | | |  __/ (_| | (_| | |_| |
// |_|  \___|\__,_|\__,_|\__, |
//                        __/ |
//                       |___/
// onReadyHandlers
void DiscordBot::loadOnReadyCommands () {
  bot_->on_ready ([&] (const dpp::ready_t& event) {
    bot_->global_command_create (
        dpp::slashcommand ("queue", "Get queue of RSS items", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("getfeednow", "Get RSS feed now", bot_->me.id));
    bot_->global_command_create (
        dpp::slashcommand ("listsources", "List all RSS sources", bot_->me.id));
    bot_->global_command_create (
        dpp::slashcommand ("addsource", "Add a new RSS source", bot_->me.id)
            .add_option (dpp::command_option (dpp::co_string, "url", "URL of the RSS feed", true))
            .add_option (dpp::command_option (dpp::co_boolean, "embedded",
                                              "Whether the feed should be embedded in the message",
                                              false)));
    bot_->global_command_create (dpp::slashcommand ("ping", "Ping pong!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("bot", "About Bot++!", bot_->me.id));
    std::this_thread::sleep_for (std::chrono::seconds (5)); // delay to user readable debug output
    startPollingFetchFeed ();
    startPollingPrintFeed ();
  });
}

int DiscordBot::isValidMessageRequest (const std::string& message, dpp::snowflake channelId) {
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
  return 0;
}

int DiscordBot::printStringToChannel (const std::string& message, dpp::snowflake channelId,
                                      const dpp::slashcommand_t& event, bool allowEmbedded) {
  int validationResult = isValidMessageRequest (message, channelId);
  if (validationResult != 0) {
    return validationResult;
  }

#ifdef PUBLIC_RELEASED_DISCORD_BOT
  dpp::message msg (channelId, message);
  if (!allowEmbedded) {
    msg.set_flags (dpp::m_suppress_embeds); // Suppress embeds if allowEmbedded is false
  }
  if (event.command.id != 0) {
    event.reply (msg);
    LOG_I_STREAM << "Message replied to slash command in channel " << event.command.channel_id
                 << ": " << message << std::endl;
  } else {
    bot_->message_create (msg, [this, message,
                                channelId] (const dpp::confirmation_callback_t& callback) {
      if (callback.is_error ()) {
        LOG_E_STREAM << "Failed to create message: " << callback.get_error ().message << std::endl;
        return;
      }

      const auto& createdMessage = callback.get<dpp::message> ();
      LOG_I_STREAM << "Message created successfully with ID: " << createdMessage.id << std::endl;
      LOG_I_STREAM << "Message sent to channel " << channelId << ": " << message << std::endl;

      // Crosspost the message if it's in a news channel
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
#else
  LOG_D_STREAM << "Fake message sent: " << message.substr (1, 40) << "..." << std::endl;
#endif
  return 0;
}

std::string DiscordBot::checkThreadName (const std::string& threadName) {
  if (threadName.empty ()) {
    LOG_W_STREAM << "Thread name is empty, generating a default name." << std::endl;
    return "";
  }
  if (threadName.size () > 100) {
    LOG_E_STREAM << "Thread name exceeds maximum length of 100 characters. "
                    "Truncating to fit the limit."
                 << std::endl;
    return threadName.substr (0, 100);
  }
  return threadName;
}

int DiscordBot::printStringToChannelAsThread (const std::string& message, dpp::snowflake channelId,
                                              const std::string& threadName, bool allowEmbedded) {

  int validationResult = isValidMessageRequest (message, channelId);
  if (validationResult != 0) {
    return validationResult;
  }
  std::string finalThreadName = checkThreadName (threadName);

#ifdef PUBLIC_RELEASED_DISCORD_BOT
  dpp::message msg (channelId, message);
  if (!allowEmbedded) {
    msg.set_flags (dpp::m_suppress_embeds);
  }
  bot_->message_create (msg, [this, finalThreadName, channelId,
                              message] (const dpp::confirmation_callback_t& callback) {
    if (callback.is_error ()) {
      LOG_E_STREAM << "Failed to create message: " << callback.get_error ().message << std::endl;
      return;
    }
    const auto& createdMessage = callback.get<dpp::message> ();
    LOG_I_STREAM << "Message created successfully with ID: " << createdMessage.id << std::endl;
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
#else // TESTING_DISCORD_BOT
  LOG_D_STREAM << "Fake thread message sent: " << message.substr (1, 40) << "..." << std::endl;
#endif
  return 0;
}

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

std::string DiscordBot::getLinuxFastfetchCpp () {
  constexpr size_t bufferSize = 2000;
  std::stringstream result;
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