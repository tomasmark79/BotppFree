// MIT License
// Copyright (c) 2024-2025 Tomáš Mark
#include "SunrisetWorker.hpp"
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <ctime>

#if defined(PLATFORM_WEB)
  #include <emscripten/emscripten.h>
#endif

SunrisetWorker::SunrisetWorker () {
  AssetContext::clearAssetsPath ();
}

SunrisetWorker::SunrisetWorker (const std::filesystem::path& assetsPath, Params& params)
    : lat_ (params.lat.second), lon_ (params.lon.second),
      utcOffsetMinutes_ (params.utcOffsetMinutes.second),
      riseOffsetMinutes_ (params.riseOffsetMinutes.second),
      setOffsetMinutes_ (params.setOffsetMinutes.second), clear_ (params.clear.second),
      params_ (params) {

  // AssetContext::clearAssetsPath ();

  if (!assetsPath.empty ()) {
    AssetContext::setAssetsPath (assetsPath);
    LOG_D_STREAM << "Assets path given to the library\n"
                 << "╰➤ " << AssetContext::getAssetsPath () << std::endl;
    configPath_ = (AssetContext::getAssetsPath () / "config.json").string ();

    if (loadConfig () == 0) {
      LOG_I_STREAM << "Config file loaded: " << configPath_ << std::endl;
    } else {
      LOG_W_STREAM << "Config file not found or invalid, using default values." << std::endl;
      // If the config file is not found, we can create a new one with default values
      lat_ = 0;
      lon_ = 0;
      utcOffsetMinutes_ = 0;
      riseOffsetMinutes_ = 0;
      setOffsetMinutes_ = 0;
      saveConfig ();
    }

    if (params_.lat.first) {
      if (lat_ > 90.0 || lat_ < -90.0) {
        LOG_E_STREAM << "Latitude out of range: " << lat_ << std::endl;
        lat_ = 0.0;
      }
      LOG_I_STREAM << "Latitude set to: " << lat_ << std::endl;
      saveConfig ();
    }

    if (params_.lon.first) {
      if (lon_ > 180.0 || lon_ < -180.0) {
        LOG_E_STREAM << "Longitude out of range: " << lon_ << std::endl;
        lon_ = 0.0;
      }
      LOG_I_STREAM << "Longitude set to: " << lon_ << std::endl;
      saveConfig ();
    }

    if (utcOffsetMinutes_ > 720 || utcOffsetMinutes_ < -720) {
      LOG_E_STREAM << "UTC offset out of range: " << utcOffsetMinutes_ << std::endl;
      utcOffsetMinutes_ = 0;
    } else {
      saveConfig ();
    }

    if (riseOffsetMinutes_ > 720 || riseOffsetMinutes_ < -720) {
      LOG_E_STREAM << "Sunrise offset out of range: " << riseOffsetMinutes_ << std::endl;
      riseOffsetMinutes_ = 0;
    } else {
      saveConfig ();
    }

    if (params_.setOffsetMinutes.first) {
      if (setOffsetMinutes_ > 720 || setOffsetMinutes_ < -720) {
        LOG_E_STREAM << "Sunset offset out of range: " << setOffsetMinutes_ << std::endl;
        setOffsetMinutes_ = 0;
      }
      saveConfig ();
    }

    if (params_.clear.first && params_.clear.second) {
      lat_ = 0;
      lon_ = 0;
      utcOffsetMinutes_ = 0;
      riseOffsetMinutes_ = 0;
      setOffsetMinutes_ = 0;
      saveConfig ();
    }

    // Print the current settings
    auto now = std::chrono::system_clock::now ();               // Get current time
    auto now_time = std::chrono::system_clock::to_time_t (now); // Convert to time_t

#ifdef _WIN32
    localtime_s (&now_tm_, &now_time); // Convert to local time
#else
    localtime_r (&now_time, &now_tm_); // Convert to local time
#endif

    year_ = now_tm_.tm_year + 1900;
    month_ = now_tm_.tm_mon + 1;
    day_ = now_tm_.tm_mday;

    rise_ = 0.0;
    set_ = 0.0;

    sun_rise_set (year_, month_, day_, lon_, lat_, &rise_, &set_);

    std::string riseTime = to24Time (rise_);
    std::string setTime = to24Time (set_);

    auto normalizeTime = [] (double t) {
      while (t < 0)
        t += 24.0;
      while (t >= 24)
        t -= 24.0;
      return t;
    };

    rise_ = normalizeTime (rise_ + utcOffsetMinutes_ / 60.0);
    set_ = normalizeTime (set_ + utcOffsetMinutes_ / 60.0);

    riseTime = to24Time (rise_);
    setTime = to24Time (set_);

    // Get current time as a double (hours + minutes/60 + seconds/3600)
    double cT = now_tm_.tm_hour + now_tm_.tm_min / 60.0 + now_tm_.tm_sec / 3600.0;
    double riseOffMin = riseOffsetMinutes_ / 60.0;
    double setOffMin = setOffsetMinutes_ / 60.0;

    riseTimeWithOffset_ = to24Time (rise_ + riseOffMin);
    setTimeWithOffset_ = to24Time (set_ + setOffMin);

    ss_ << "🌅 Sunrise: " << riseTime << "  ➔  🌆 Sunset: " << setTime; // << std::endl; 
    
    // << "═══════════════ Sunrise & Sunset ═══════════════" << std::endl
        // << "📅 " << std::put_time (&now_tm_, "%d.%m.%Y %H:%M:%S") << std::endl
        // << "📍 Location: " << std::fixed << std::setprecision (4) << lat_ << "°N, " << lon_ << "°E"
        // << std::endl
        // << "🌐 UTC offset: " << utcOffsetMinutes_ / 60 << " hours" << std::endl
        // << std::endl
        
    // << std::endl;
    // << "User offset adjustments:" << std::endl
    // << "   Sunrise: " << (riseOffsetMinutes_ >= 0 ? "+" : "") << riseOffsetMinutes_ << " min"
    // << "   Sunset: " << (setOffsetMinutes_ >= 0 ? "+" : "") << setOffsetMinutes_ << " min"
    // << std::endl
    // << std::endl
    // << "Theme trigger times:" << std::endl
    // << "   Light theme at: " << riseTimeWithOffset_ << " ☀️" << std::endl
    // << "   Dark theme at: " << setTimeWithOffset_ << " 🌚" << std::endl
    // << "═══════════════════════════════════════════════════════════════" << std::endl;

    // Check if the sun is above or below the horizon
    if ((rise_) < (set_)) {
      if ((rise_ + riseOffMin) < cT && cT < (set_ + setOffMin)) {
        LOG_I_STREAM << "Current time is between sunrise and sunset" << " -> Applying light theme"
                     << std::endl;
        // switchLightThemeGNome (true);
        LOG_I_STREAM << "╰➤ Light theme applied" << std::endl;
      } else {
        LOG_I_STREAM << "Current time is outside of sunrise and sunset"
                     << " -> Applying dark theme" << std::endl;
        // switchLightThemeGNome (false);
        LOG_I_STREAM << "╰➤ Dark theme applied" << std::endl;
      }
    } else {
      // If the sun never sets or rises, we need to handle it differently
      if (rise_ > set_) {
        // if the sun never sets - polar night
        LOG_I_STREAM << "Sun never sets" << std::endl;
        // switchLightThemeGNome (true);
        LOG_I_STREAM << "╰➤ Light theme applied" << std::endl;
      }
      // if the sun never rises - polar day
      else if (rise_ < set_) {
        LOG_I_STREAM << "Sun never rises" << std::endl;
        // switchLightThemeGNome (false);
        LOG_I_STREAM << "╰➤ Dark theme applied" << std::endl;
      }
    }
  }
}

int SunrisetWorker::loadConfig () {
  std::ifstream configFile (configPath_);
  if (!configFile.is_open ()) {
    LOG_E_STREAM << "Failed to open config file: " << configPath_ << std::endl;
    return -1;
  }

  nlohmann::json configJson;
  try {
    configFile >> configJson;
  } catch (const nlohmann::json::parse_error& e) {
    LOG_E_STREAM << "Failed to parse config file: " << e.what () << std::endl;
    return -1;
  }

  // load only if not set by params
  if (!params_.lat.first)
    lat_ = configJson.value ("lat", 50.0755);
  if (!params_.lon.first)
    lon_ = configJson.value ("lon", 14.4378);
  if (!params_.utcOffsetMinutes.first)
    utcOffsetMinutes_ = configJson.value ("utcOffsetMinutes", 0);
  if (!params_.riseOffsetMinutes.first)
    riseOffsetMinutes_ = configJson.value ("riseOffsetMinutes", 0);
  if (!params_.setOffsetMinutes.first)
    setOffsetMinutes_ = configJson.value ("setOffsetMinutes", 0);

  return 0;
}

int SunrisetWorker::saveConfig () {
  nlohmann::json configJson;

  // Save Lon Lat
  configJson["lat"] = lat_;
  configJson["lon"] = lon_;
  configJson["utcOffsetMinutes"] = utcOffsetMinutes_;
  configJson["riseOffsetMinutes"] = riseOffsetMinutes_;
  configJson["setOffsetMinutes"] = setOffsetMinutes_;

  // Save to file
  std::ofstream configFile (configPath_);
  if (!configFile.is_open ()) {
    LOG_E_STREAM << "Failed to open config file for writing: " << configPath_ << std::endl;
    return -1;
  }

  try {
    configFile << configJson.dump (4); // Pretty print with 4 spaces
  } catch (const std::exception& e) {
    LOG_E_STREAM << "Failed to write config file: " << e.what () << std::endl;
    return -1;
  }

  return 0;
}

std::string SunrisetWorker::getSetTime () const {
  LOG_I_STREAM << "getSetTime called, returning: " << ss_.str () << std::endl;
  return ss_.str ();
}

SunrisetWorker::~SunrisetWorker () {
}