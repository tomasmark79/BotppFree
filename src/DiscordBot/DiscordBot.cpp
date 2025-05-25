#include "DiscordBot.hpp"
#include <Logger/Logger.hpp>
#include <IBot/version.h>
#include <RssReader/RssReader.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#define DISCORD_OAUTH_TOKEN_FILE "/home/tomas/.tokens/.bot++.key"
const dpp::snowflake channelRss = 1375852042790244352;

std::string DiscordBot::getBotNameAndVersion () {
  return std::string ("Bot++ " + std::string (IBOT_VERSION) + " 👾 ") + DPP_VERSION_TEXT
         + " loaded.\nInitial development D🌀tName 2025\nCreated with 🩵 for all Linux enthusiast "
           "🐧";
}

bool DiscordBot::getToken (std::string& token, const std::filesystem::path& filePath) {
  std::ifstream file (filePath);
  if (!file.is_open ()) {
    LOG_E_STREAM << "Error: Could not open file " << filePath << std::endl;
    return false;
  }
  std::getline (file, token);
  file.close ();
  if (token.empty ()) {
    LOG_E_STREAM << "Error: Token is empty" << std::endl;
    return false;
  }
  return true;
}

int DiscordBot::initCluster () {
  std::string token;
  if (getToken (token, DISCORD_OAUTH_TOKEN_FILE)) {
    try {
      if (bot_) {
        LOG_E_STREAM << "Bot is already initialized!" << std::endl;
        return -2;
      }

      // RedHats childs needs for failed ssl contexts in
      // in /etc/ssl/openssl.cnf
      // https://github.com/openssl/openssl/discussions/23016
      // config_diagnostics = 1 to config_diagnostics = 0

      DiscordBot::bot_
          = std::make_unique<dpp::cluster> (token, dpp::i_default_intents | dpp::i_message_content);

      bot_->log (dpp::ll_debug, "Bot++");

      std::string message = "Initializing Discord bot";
      dpp::message msg (channelRss, message);
      bot_->message_create (msg);
      LOG_I_STREAM << message << std::endl;

      getBotNameAndVersion ();
      loadVariousBotCommands ();

      bot_->start (dpp::st_wait);

    } catch (const std::exception& e) {
      LOG_E_STREAM << "Exception during bot initialization: " << e.what () << std::endl;
      return -1;
    }
  }
  return 0;
}

void DiscordBot::loadVariousBotCommands () {

  bot_->on_log ([&] (const dpp::log_t& log) {
    LOG_D_STREAM << "[" << dpp::utility::current_date_time () << "] "
                 << dpp::utility::loglevel (log.severity) << ": " << log.message << std::endl;
  });

  bot_->on_slashcommand ([&, this] (const dpp::slashcommand_t& event) {
    if (event.command.get_command_name () == "randomroot") {
      RssReader rssReader;
      std::string rssFeed = rssReader.feedRandomFromUrl ("https://www.root.cz/rss/clanky/");
      if (rssFeed.empty ()) {
        event.reply ("Failed to fetch RSS feed. Please try again later.");
      } else {
        dpp::message msg (channelRss, rssFeed);
        event.reply (msg);
        bot_->message_create (msg);
      }
    }

    if (event.command.get_command_name () == "randomabc") {
      RssReader rssReader;
      std::string rssFeed = rssReader.feedRandomFromUrl ("www.abclinuxu.cz/auto/abc.rss", 1);
      if (rssFeed.empty ()) {
        event.reply ("Failed to fetch RSS feed. Please try again later.");
      } else {
        dpp::message msg (channelRss, rssFeed);
        event.reply (msg);
        bot_->message_create (msg);
      }
    }

    if (event.command.get_command_name () == "rss") {
      RssReader rssReader;
      std::string rssFeed = rssReader.feedFromUrl ("https://www.root.cz/rss/clanky/");
      if (rssFeed.empty ()) {
        event.reply ("Failed to fetch RSS feed. Please try again later.");
      } else {
        dpp::message msg (channelRss, rssFeed);
        event.reply (msg);
        bot_->message_create (msg);
      }
    }

    if (event.command.get_command_name () == "ping") {
      event.reply ("Pong! 🏓");
    }

    if (event.command.get_command_name () == "pong") {
      event.reply ("Ping! 🏓");
    }

    if (event.command.get_command_name () == "gang") {
      dpp::message msg (event.command.channel_id, "Bang bang! 💥💥");
      event.reply (msg);
      bot_->message_create (msg);
    }

    if (event.command.get_command_name () == "bot") {
      std::string botInfo = getBotNameAndVersion () + "\n"
                            + this->getLinuxFastfetchCpp ().substr (0, 8192 - 2) + "\n";
      dpp::message msg (channelRss, botInfo);
      event.reply (msg);
    }
  });

  bot_->on_ready ([&] (const dpp::ready_t& event) {
    bot_->global_command_create (dpp::slashcommand ("randomroot", "Get news!", bot_->me.id));
    
    
    bot_->global_command_create (dpp::slashcommand ("randomabc", "Get news!", bot_->me.id));

    // bot_->global_command_create (dpp::slashcommand ("abcclanky", "Články!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abcotazky", "Otázky v poradně!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abchw", "Hardwarové záznamy!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abcsw", "Softwarové záznamy!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abczpravicky", "Zprávičky!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abcpojmy", "Slovníkové pojmy!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abcosobnosti", "Osobnosti (Kdo je)!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abczapisky", "Blogové zápisky!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abclzapisky", "Linuxové blogové zápisky!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abcfaq", "Často kladené dotazy!", bot_->me.id));
    // bot_->global_command_create (dpp::slashcommand ("abcovladace", "Ovladače!", bot_->me.id));

// Články
//     www.abclinuxu.cz/auto/abc.rss
// Otázky v poradně (včetně počtu odpovědí)
//     www.abclinuxu.cz/auto/diskuse.rss
// Hardwarové záznamy
//     www.abclinuxu.cz/auto/hardware.rss
// Softwarové záznamy
//     www.abclinuxu.cz/auto/software.rss
// Zprávičky
//     www.abclinuxu.cz/auto/zpravicky.rss
// Slovníkové pojmy
//     www.abclinuxu.cz/auto/slovnik.rss
// Osobnosti (Kdo je)
//     www.abclinuxu.cz/auto/kdojekdo.rss
// Blogové zápisky
//     www.abclinuxu.cz/auto/blog.rss
// Linuxové blogové zápisky
//     www.abclinuxu.cz/auto/blogDigest.rss
// FAQ (často kladené dotazy)
//     www.abclinuxu.cz/auto/faq.rss
// Ankety
//     www.abclinuxu.cz/auto/ankety.rss
// Inzeráty v bazaru
//     www.abclinuxu.cz/auto/bazar.rss
// Ovladače
//     www.abclinuxu.cz/auto/ovladace.rss 
    
    bot_->global_command_create (dpp::slashcommand ("rss", "Get news!", bot_->me.id));

    /* ping */
    bot_->global_command_create (dpp::slashcommand ("ping", "Ping pong!", bot_->me.id));

    /* pong */
    bot_->global_command_create (dpp::slashcommand ("pong", "Pong ping!", bot_->me.id));

    /* gang */
    bot_->global_command_create (dpp::slashcommand ("gang", "Will shoot!", bot_->me.id));

    /* bot */
    bot_->global_command_create (dpp::slashcommand ("bot", "About Bot++!", bot_->me.id));
  });
}

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