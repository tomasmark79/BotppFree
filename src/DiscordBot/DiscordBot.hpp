#ifndef DISCORDBOT_HPP
#define DISCORDBOT_HPP

#include <string>
#include <filesystem>
#include <dpp/dpp.h>
#include <memory>

// curl https temporary fix
// RedHats childs needs for failed ssl contexts in
// in /etc/ssl/openssl.cnf
// https://github.com/openssl/openssl/discussions/23016
// config_diagnostics = 1 to config_diagnostics = 0

class DiscordBot {
  std::filesystem::path rssUrls_;
  std::filesystem::path seenHashes_;

public:
  DiscordBot ();
  ~DiscordBot () = default;

  /**
   * @brief Initialize the Discord bot cluster.
   * @return 0 on success, -1 on failure.
   */
  int initCluster ();

  /**
   * @brief Get the bot instance.
   * @return A reference to the bot instance.
   */
  dpp::cluster& getBot () {
    return *bot_;
  }

  /**
   * @brief Get the bot instance (const version).
   * @return A const reference to the bot instance.
   */
  const dpp::cluster& getBot () const {
    return *bot_;
  }

private:
  std::string getLinuxFastfetchCpp ();
  std::unique_ptr<dpp::cluster> bot_;
  int getTokenFromFile (std::string& token);

  bool startPollingPrintFeed ();
  bool startPollingFetchFeed ();

  int isValidMessageRequest (const std::string& message, dpp::snowflake channelId);

  int printStringToChannelAsThread (const std::string& message, dpp::snowflake channelId,
                                    const std::string& threadName = "", bool allowEmbedded = true);
  std::string checkThreadName (const std::string& threadName);
  int printStringToChannel (const std::string& str, dpp::snowflake channelId,
                            const dpp::slashcommand_t& event, bool allowEmbedded);

  void addSource (const std::string& url, bool embedded);

  void loadOnSlashCommands ();
  int getQueueSize ();
  void loadOnReadyCommands ();
};

#endif