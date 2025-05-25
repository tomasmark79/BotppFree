#ifndef DISCORDBOT_HPP
#define DISCORDBOT_HPP

#include <string>
#include <filesystem>
#include <dpp/dpp.h>
#include <memory>

// RedHats childs needs for failed ssl contexts in
// in /etc/ssl/openssl.cnf
// https://github.com/openssl/openssl/discussions/23016
// config_diagnostics = 1 to config_diagnostics = 0

class DiscordBot {
public:
  DiscordBot () = default;
  ~DiscordBot () = default;

  bool startPolling ();

  int initCluster ();
  void printFullFeedToChannel (const std::string& url, dpp::snowflake channelId,
                               const dpp::slashcommand_t& event, bool allowEmbedded = true);
  void printRandomFeedToChannel (const std::string& url, dpp::snowflake channelId,
                                 const dpp::slashcommand_t& event, bool allowEmbedded = true);


  void loadOnSlashCommands ();
  void loadOnReadyCommands ();

  // std::string getBotNameAndVersion ();
  std::string getLinuxFastfetchCpp ();

  int getTokenFromFile (std::string& token);

private:
  std::unique_ptr<dpp::cluster> bot_;
};

#endif