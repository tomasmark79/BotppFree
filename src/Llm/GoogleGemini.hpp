#ifndef GOOGLEGEMINI_HPP
#define GOOGLEGEMINI_HPP

#include <curl/curl.h>
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <nlohmann/json.hpp>
#include <string>

class GoogleGemini {
public:
  GoogleGemini ();
  ~GoogleGemini ();

public:
  std::string generateContentGemini (const std::string& apiKey, const std::string& model,
                                     const std::string& prompt);
};

#endif