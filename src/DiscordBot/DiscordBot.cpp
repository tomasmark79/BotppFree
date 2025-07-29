#include "DiscordBot.hpp"
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <IBot/version.h>
#include <RssManager/RssManager.hpp>
#include <Llm/GoogleGemini.hpp>
#include <thread>
#include <atomic>

#define IS_TOMAS_MARK_BOT
#define IS_RSS_MODULE_ACTIVE
#define PUBLIC_RELEASED_DISCORD_BOT

const int START_SLOW_MODE_AT_HOUR = 22;           // Start slow mode at 22:00
const int STOP_SLOW_MODE_AT_HOUR = 7;             // Stop slow mode at 07:00
const int SLOW_MODE_POLLING_INTERVAL = 60 * 180;  // 180 minutes (3 hours)
const int NORMAL_MODE_POLLING_INTERVAL = 60 * 23; // 23 minutes
const int ULTRA_FAST_POLLING_INTERVAL = 30;       // 30 seconds
const int FEED_FETCH_INTERVAL = 60 * 60 * 2;      // 2 hours

const std::string NO_ITEMS_IN_QUEUE = "No items in the RSS feed queue.";
const std::string ALL_FEEDS_REFETCHED = "All RSS feeds have been refetched successfully.";
constexpr size_t DISCORD_MAX_MSG_LEN = 2000; // (as per Discord API docs)

#ifdef IS_TOMAS_MARK_BOT
  // DigitalSpace
  #define CREDITS "[DotName](https://digitalspace.name/) for bot hosting"
  #define DISCORD_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.botpp-supervisor.key"
  #define GEMINI_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.gemini"
  #define GEMINI_MODEL "gemini-2.0-flash" // Use Gemini 2.0 Flash model
uint64_t defaultChannelRss = 1398904149856223262;
#else
  // Linux CZ/SK feed channel
  #define CREDITS "[Delirium](https://robctl.dev/) for bot hosting"
  #define DISCORD_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.bot++.key"
  #define GEMINI_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.gemini"
  #define GEMINI_MODEL "gemini-2.0-flash" // Use Gemini 2.0 Flash model
uint64_t defaultChannelRss = 1375852042790244352;
#endif

RssManager rss;

const std::string botCommandsHelp = R"(
`/bot` - display this information
`/env` - display environment information
`/queue` - number of feeds in queue
`/refetch` - fetch new feeds from web
`/getfeednow` - print feed right now
`/listsources` - list RSS sources
`/addsource` - add RSS source [RSS 1.0, 2.0, Atom]
`/addsource url:https://www.root.cz/rss/clanky/ embedded:true`
`/runterminalcommand` `fortune` `df -h` `free -h` `cat /etc/os-release`
`/heygoogle` prompt: `What's the weather like today?` - ask Google Gemini AI

)";

const std::string botDescription
    = R"(written in C++ using the [DotNameCpp](https://github.com/tomasmark79/DotNameCppFree) project template and utilizes the [DPP](https://github.com/brainboxdotcc/DPP) library for Discord API access.)";

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
bool DiscordBot::startPollingPrintFeed () {
  std::thread pollingThreadPrintFeed ([&] () -> void {
    while (!stopPollingPrintFeed.load ()) {
      try {
        RSSItem item = rss.getRandomItem ();
        if (!item.title.empty ()) {
          // Want answer in the channel received by rss, or default channel if not specified
          printStringToChannel (item.toMarkdownLink (),
                                item.discordChannelId > 0 ? item.discordChannelId
                                                          : defaultChannelRss,
                                {}, item.embedded);
        } else {
          LOG_W_STREAM << "No items found in the feed queue." << std::endl;
        }
        isPollingPrintFeedRunning.store (true);
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
        isPollingPrintFeedRunning.store (false);
      }

      int printFeedInterval = 0;
      std::time_t now = std::time (nullptr);
      std::tm* localTime = std::localtime (&now);

#ifndef PUBLIC_RELEASED_DISCORD_BOT
      // In development mode, use ultra fast polling
      printFeedInterval = ULTRA_FAST_POLLING_INTERVAL;
#else
      if ((localTime->tm_hour >= START_SLOW_MODE_AT_HOUR)
          || (localTime->tm_hour < STOP_SLOW_MODE_AT_HOUR)) {
        printFeedInterval = SLOW_MODE_POLLING_INTERVAL;
      } else {
        printFeedInterval = NORMAL_MODE_POLLING_INTERVAL;
      }
#endif

      std::this_thread::sleep_for (std::chrono::seconds (printFeedInterval));
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
bool DiscordBot::startPollingFetchFeed () {
  std::thread pollingThreadFetchFeed ([&] () -> void {
    while (!stopPollingFetchFeed.load ()) {
      try {
#ifdef IS_RSS_MODULE_ACTIVE
        rss.fetchAllFeeds ();
#endif
        isPollingFetchFeedRunning.store (true);
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
        isPollingFetchFeedRunning.store (false);
      }
      std::this_thread::sleep_for (std::chrono::seconds (FEED_FETCH_INTERVAL));
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
  GoogleGemini gemini;

  bot_->on_slashcommand ([&, this] (const dpp::slashcommand_t& event) {
    if (event.command.get_command_name () == "heygoogle") {

      auto prompt_param = event.get_parameter ("prompt");
      if (prompt_param.index () == 0) {
        event.reply ("Error: Prompt parameter is required.");
        return;
      }

      std::string apiKey;
      std::string prompt = std::get<std::string> (event.get_parameter ("prompt"));
      LOG_I_STREAM << "/HeyGoogle " << prompt << std::endl;
      event.reply ("/HeyGoogle " + prompt);

      if (prompt.empty ()) {
        event.reply ("Error: Prompt parameter is required.");
        return;
      }

      try {
        if (getGoogleGeminiTokenFromFile (apiKey) != 0) {
          LOG_E_STREAM << "Failed to read Google Gemini API key from file: "
                       << GEMINI_OAUTH_TOKEN_FILE << std::endl;
          event.reply ("Error: Failed to read Google Gemini API key.");
          return;
        }

        std::string response
            = gemini.generateContentGemini (apiKey, GEMINI_MODEL, prompt + "Maximum 1900 znaků");
        if (response.empty ()) {
          event.reply ("Error: No response from Google Gemini.");
          return;
        }

        std::string truncatedResponse = response.substr (0, DISCORD_MAX_MSG_LEN - 100);
        dpp::message msg (event.command.channel_id, truncatedResponse);
        LOG_I_STREAM << "Response from Google Gemini: " << truncatedResponse << std::endl;
        bot_->message_create (msg);

      } catch (const std::exception& e) {
        LOG_E_STREAM << "Exception while processing /heygoogle command: " << e.what () << std::endl;
        event.reply ("Error: " + std::string (e.what ()));
      }
      return;
    }

    if (event.command.get_command_name () == "refetch") {
      try {
        event.reply ("Refetching all RSS feeds...");
        rss.fetchAllFeeds ();
        size_t itemCount = rss.getItemCount ();
        if (itemCount == 0) {
          LOG_W_STREAM << NO_ITEMS_IN_QUEUE << std::endl;
          event.reply (NO_ITEMS_IN_QUEUE);
          return;
        }
        std::string response = "All RSS feeds have been refetched successfully. Queue contains "
                               + std::to_string (itemCount) + " items.\n";
        dpp::message msg (event.command.channel_id, response);
        bot_->message_create (msg);
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
        event.reply ("Error refetching feeds: " + std::string (e.what ()));
      }
      return;
    }
    if (event.command.get_command_name () == "queue") {
      try {
        size_t itemCount = rss.getItemCount ();
        if (itemCount == 0) {
          event.reply (NO_ITEMS_IN_QUEUE);
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
          // Want answer in the same channel
          printStringToChannel (item.toMarkdownLink (), event.command.channel_id, event,
                                item.embedded);
        } else {
          LOG_W_STREAM << NO_ITEMS_IN_QUEUE << std::endl;
          event.reply (NO_ITEMS_IN_QUEUE);
        }
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error: " << e.what () << std::endl;
      }
    }
    if (event.command.get_command_name () == "addsource") {
      auto url_param = event.get_parameter ("url");
      if (url_param.index () == 0) { // std::monostate means parameter doesn't exist
        event.reply ("Error: URL parameter is required.");
        return;
      }
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
        // Add the URL to the RSS manager - use the channel ID from the command
        int result = rss.addUrl (url, embedded, event.command.channel_id);
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
    if (event.command.get_command_name () == "runterminalcommand") {

      auto command_param = event.get_parameter ("command");
      if (command_param.index () == 0) {
        event.reply ("Error: Command parameter is required.");
        return;
      }
      std::string command = std::get<std::string> (event.get_parameter ("command"));

      // allowed commands
      if (command != "fortune" && command != "df -h" && command != "free -h"
          && command != "cat /etc/os-release" && command != "fastfetch --logo none") {
        event.reply ("Error: Command not allowed.");
        return;
      }
      if (command.empty ()) {
        event.reply ("Error: Command parameter is required.");
        return;
      }
      try {
        std::string output = runTerminalCommand (command);
        if (output.empty ()) {
          output = "Command executed successfully, but no output was returned.";
        } else {
          output = "```txt\n" + output + "\n```";
        }
        LOG_I_STREAM << "Command output: " << output << std::endl;
        printStringToChannel (output, event.command.channel_id, event, false);
      } catch (const std::runtime_error& e) {
        LOG_E_STREAM << "Error executing command: " << e.what () << std::endl;
        event.reply ("Error executing command: " + std::string (e.what ()));
      }
      return;
    }
    if (event.command.get_command_name () == "ping") {
      event.reply ("Pong! 🏓");
    }
    if (event.command.get_command_name () == "bot") {
      dpp::embed embed
          = dpp::embed ()
                .set_author ("With 🩵 by D🌀tName (c) 2025", "https://digitalspace.name",
                             "https://digitalspace.name/avatar/avatarpix.png")
                .set_color (dpp::colors::sti_blue)
                .set_title ("BotppFree")
                .set_url ("https://github.com/tomasmark79/BotppFree")
                .set_description (botDescription + "\n")
                .set_thumbnail ("https://digitalspace.name/avatar/Linux-Logo-1996-present.png")
                .add_field ("commands:", botCommandsHelp, true)
                .add_field ("build info:",
                            "version " + std::string (IBOT_VERSION) + "\nDPP version "
                                + DPP_VERSION_TEXT + "\nC++ version "
                                + std::to_string (__cplusplus),
                            false)
                .add_field ("credits:", CREDITS, false)
                .set_image ("https://digitalspace.name/avatar/tuxik.png");

      dpp::message msg (event.command.channel_id, embed);
      event.reply (msg);
    }
    if (event.command.get_command_name () == "env") {
      std::string envInfo = getLinuxFastfetchCpp ();
      if (envInfo.empty ()) {
        envInfo = "No environment information available.";
      } else {
        envInfo = "```txt\n" + envInfo + "\n```";
      }
      LOG_I_STREAM << "Environment information: " << envInfo << std::endl;
      printStringToChannel (envInfo, event.command.channel_id, event, false);
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
        dpp::slashcommand ("heygoogle", "Ask Google Gemini AI", bot_->me.id)
            .add_option (dpp::command_option (dpp::co_string, "prompt",
                                              "The prompt to send to Google Gemini", true)));
    bot_->global_command_create (
        dpp::slashcommand ("refetch", "Refetch all RSS feeds", bot_->me.id));
    bot_->global_command_create (
        dpp::slashcommand ("queue", "Get queue of RSS items", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("getfeednow", "Get RSS feed now", bot_->me.id));
    bot_->global_command_create (
        dpp::slashcommand ("listsources", "List all RSS sources", bot_->me.id));

    bot_->global_command_create (
        dpp::slashcommand ("addsource",
                           "Add a new RSS source + <channel_id> where command was invoked",
                           bot_->me.id)
            .add_option (dpp::command_option (dpp::co_string, "url", "URL of the RSS feed", true))
            .add_option (dpp::command_option (dpp::co_boolean, "embedded",
                                              "Whether the feed should be embedded in the message",
                                              false)));
    bot_->global_command_create (
        dpp::slashcommand ("runterminalcommand", "Run a terminal command and return the output",
                           bot_->me.id)
            .add_option (dpp::command_option (dpp::co_string, "command",
                                              "The terminal command to run", true)));

    bot_->global_command_create (dpp::slashcommand ("ping", "Ping pong!", bot_->me.id));
    bot_->global_command_create (dpp::slashcommand ("bot", "About Bot++", bot_->me.id));
    bot_->global_command_create (
        dpp::slashcommand ("env", "Display environment information", bot_->me.id));
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

  // shortening the message if it exceeds Discord's maximum length
  std::string truncatedMessage = message;
  if (truncatedMessage.size () > DISCORD_MAX_MSG_LEN) {
    truncatedMessage = truncatedMessage.substr (0, DISCORD_MAX_MSG_LEN - 3) + "...";
  }

  dpp::message msg (channelId, truncatedMessage);
  if (!allowEmbedded) {
    msg.set_flags (dpp::m_suppress_embeds); // Suppress embeds if allowEmbedded is false
  }
  if (event.command.id != 0) {
    event.reply (msg);
    LOG_I_STREAM << "Message replied to slash command in channel " << event.command.channel_id
                 << ": " << truncatedMessage << std::endl;
  } else {
    bot_->message_create (msg, [this, truncatedMessage,
                                channelId] (const dpp::confirmation_callback_t& callback) {
      if (callback.is_error ()) {
        LOG_E_STREAM << "Failed to create message: " << callback.get_error ().message << std::endl;
        return;
      }

      const auto& createdMessage = callback.get<dpp::message> ();
      LOG_I_STREAM << "Message created successfully with ID: " << createdMessage.id << std::endl;
      LOG_I_STREAM << "Message sent to channel " << channelId << ": " << truncatedMessage
                   << std::endl;

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
  // shortening the message if it exceeds Discord's maximum length
  std::string truncatedMessage = message;
  if (truncatedMessage.size () > DISCORD_MAX_MSG_LEN) {
    truncatedMessage = truncatedMessage.substr (0, DISCORD_MAX_MSG_LEN - 3) + "...";
  }

  dpp::message msg (channelId, truncatedMessage);
  if (!allowEmbedded) {
    msg.set_flags (dpp::m_suppress_embeds);
  }
  bot_->message_create (msg, [this, finalThreadName, channelId,
                              truncatedMessage] (const dpp::confirmation_callback_t& callback) {
    if (callback.is_error ()) {
      LOG_E_STREAM << "Failed to create message: " << callback.get_error ().message << std::endl;
      return;
    }
    const auto& createdMessage = callback.get<dpp::message> ();
    LOG_I_STREAM << "Message created successfully with ID: " << createdMessage.id << std::endl;
    bot_->thread_create_with_message (
        finalThreadName, channelId, createdMessage.id, 60, // auto_archive_duration in minutes
        0, // rate_limit_per_user (0 = no rate limit)
        [this, channelId, truncatedMessage] (const dpp::confirmation_callback_t& thread_callback) {
          if (thread_callback.is_error ()) {
            LOG_E_STREAM << "Failed to create thread: " << thread_callback.get_error ().message
                         << std::endl;
            return;
          }
          const auto& createdThread = thread_callback.get<dpp::thread> ();
          LOG_I_STREAM << "Thread created successfully with ID: " << createdThread.id << std::endl;
          LOG_I_STREAM << "Message sent to channel " << channelId
                       << " as thread: " << truncatedMessage << std::endl;
        });
  });
#else // TESTING_DISCORD_BOT
  LOG_D_STREAM << "Fake thread message sent: " << message.substr (1, 40) << "..." << std::endl;
#endif
  return 0;
}

int DiscordBot::getGoogleGeminiTokenFromFile (std::string& token) {
  std::ifstream tokenFile (GEMINI_OAUTH_TOKEN_FILE);
  if (!tokenFile.is_open ()) {
    LOG_E_STREAM << "Failed to open token file: " << GEMINI_OAUTH_TOKEN_FILE << std::endl;
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
  constexpr size_t bufferSize = DISCORD_MAX_MSG_LEN;
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

/// @brief  Run a terminal command and return the output.
/// @param command
/// @return
std::string DiscordBot::runTerminalCommand (const std::string& command) {
  std::string result;
  std::array<char, 128> buffer;
  std::unique_ptr<FILE, decltype (&pclose)> pipe (popen (command.c_str (), "r"), pclose);
  if (!pipe) {
    LOG_E_STREAM << "Failed to run command: " << command << std::endl;
    return result;
  }
  while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr) {
    result += buffer.data ();
  }
  return result;
}
