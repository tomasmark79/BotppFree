#ifndef __RSSMANAGER_H__
#define __RSSMANAGER_H__

#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <nlohmann/json.hpp>
#include <tinyxml2.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <random>
#include <filesystem>

struct RSSUrl {
  std::string url;
  bool embedded;

  RSSUrl () : url (""), embedded (false) {
  }
  RSSUrl (const std::string& u, bool e = false) : url (u), embedded (e) {
  }
};

struct RSSItem {
  std::string title;
  std::string link;
  std::string description;
  std::string pubDate;
  std::string hash;
  bool embedded; // Whether this item should use embedded format

  RSSItem () : embedded (false) {
  }
  RSSItem (const std::string& t, const std::string& l, const std::string& d,
           const std::string& date = "", bool e = false)
      : title (t), link (l), description (d), pubDate (date), embedded (e) {
    generateHash ();
  }

  void generateHash () {
    std::hash<std::string> hasher;
    hash = std::to_string (hasher (title + link + description));
  }

  std::string toMarkdownLink () const {
    return "[" + title + "](" + link + ")";
  }
};

struct RSSFeed {
  std::string title;
  std::string description;
  std::string link;
  std::vector<RSSItem> items;

  void addItem (const RSSItem& item) {
    items.push_back (item);
  }

  size_t size () const {
    return items.size ();
  }

  void clear () {
    items.clear ();
  }
};

class RssManager {
public:
  RssManager ();
  ~RssManager () = default;

  // Main operations
  int initialize ();
  int fetchAllFeeds ();
  int fetchFeed (const std::string& url, bool embedded = false);

  // Item operations
  RSSItem getRandomItem ();
  RSSItem getRandomItem (bool embedded); // Get item with specific embedded preference
  size_t getItemCount () const {
    return feed_.size ();
  }
  size_t getItemCount (bool embedded) const; // Count items with specific embedded flag

  // Utility
  std::string getItemAsMarkdown (const RSSItem& item) const {
    return item.toMarkdownLink ();
  }
  std::string getSourcesAsList ();
  int addUrl (const std::string& url, bool embedded);
private:
  RSSFeed feed_;
  std::vector<RSSUrl> urls_;
  std::unordered_set<std::string> seenHashes_;
  std::mt19937 rng_;

  // File operations
  int saveUrls ();
  int loadUrls ();
  int loadSeenHashes ();
  int saveSeenHash (const std::string& hash);
  int saveAllSeenHashes (); // Save all hashes at once
  bool hasFileChanged (const std::filesystem::path& path,
                       std::filesystem::file_time_type& lastModified);
  void checkAndReloadFiles ();

  // RSS parsing
  RSSFeed parseRSS (const std::string& xmlData, bool embedded);
  std::string downloadFeed (const std::string& url);

  // Paths
  std::filesystem::path getUrlsPath () const {
    return AssetContext::getAssetsPath () / "rssUrls.json";
  }

  std::filesystem::path getHashesPath () const {
    return AssetContext::getAssetsPath () / "seenHashes.json";
  }

  // Add file timestamp tracking
  std::filesystem::file_time_type urlsLastModified_;
  std::filesystem::file_time_type hashesLastModified_;
};

#endif // __RSSMANAGER_H__