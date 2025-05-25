#ifndef DISCORDBOT_HPP
#define DISCORDBOT_HPP

#include <string>
#include <filesystem>
#include <dpp/dpp.h>
#include <memory>

class DiscordBot {
public:
  DiscordBot () = default;
  ~DiscordBot () = default;

  bool getToken (std::string& token, const std::filesystem::path& filePath);
  
  int initCluster ();
  void loadVariousBotCommands ();
  std::string getBotNameAndVersion ();
  std::string getLinuxFastfetchCpp ();

private:
  std::unique_ptr<dpp::cluster> bot_;
};

#endif