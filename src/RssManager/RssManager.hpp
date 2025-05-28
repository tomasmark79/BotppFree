#ifndef __RSSMANAGER_H__
#define __RSSMANAGER_H__

#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <nlohmann/json.hpp>
#include <tinyxml2.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <filesystem>
#include <fstream>

const int DISCORD_MAX_MSG_LEN = 2000; // Discord max characters per message

inline int getModernRandomNumber (int min, int max) {
  static std::random_device rd;                     // Obtain a random number from hardware
  static std::mt19937 eng (rd ());                  // Seed the generator
  std::uniform_int_distribution<> distr (min, max); // Define the range
  return distr (eng);                               // Generate the random number
}

struct UrlsStruct {
  std::string url;
  bool embed = false; // Default to false
  std::string category;

  UrlsStruct () = default;
  UrlsStruct (const std::string& u, bool e = false, const std::string& c = "")
      : url (u), embed (e), category (c) {
  }
};

struct VolatileHashes {
  std::vector<std::string> hashes; // Store unique hashes of seen items
  void addHash (const std::string& hash) {
    hashes.push_back (hash);
  }
  bool contains (const std::string& hash) const {
    return std::find (hashes.begin (), hashes.end (), hash) != hashes.end ();
  }
};

struct SeenHashes {
  std::vector<std::string> hashes; // Store unique hashes of seen items
  void addHash (const std::string& hash) {
    hashes.push_back (hash);
  }
  bool contains (const std::string& hash) const {
    return std::find (hashes.begin (), hashes.end (), hash) != hashes.end ();
  }
};

struct RSSItem {
  std::string title_;
  std::string link_;
  std::string description_;
  std::string pubDate_;
  std::string language_;
  std::string hash_;

  RSSItem () = default;
  RSSItem (const std::string& t, const std::string& l, const std::string& d,
           const std::string& date = "", const std::string& lang = "", const std::string& hash = "")
      : title_ (t), link_ (l), description_ (d), pubDate_ (date), language_ (lang), hash_ (hash) {
  }
};

struct RSSFeed {
  std::string title;
  std::string description;
  std::string link;
  std::string pubDate;
  std::string language;
  std::string hash;
  std::vector<RSSItem> items;

  void addItem (const RSSItem& item) {
    items.push_back (item);
  }

  size_t getItemCount () const {
    return items.size ();
  }

  std::string getTitle () const {
    return title;
  }
  std::string getDescription () const {
    return description;
  }
  std::string getLink () const {
    return link;
  }
  std::string getPubDate () const {
    return pubDate;
  }
  std::string getLanguage () const {
    return language;
  }
  std::string getHash () const {
    return hash;
  }
  const std::vector<RSSItem>& getItems () const {
    return items;
  }
  std::vector<RSSItem>& getItems () {
    return items;
  }
};

inline RSSFeed rssFeeds;                // Global instance to store RSS feeds
inline std::vector<UrlsStruct> rssUrls; // Global vector to store RSS URLs
inline VolatileHashes volatileHashes;   // Global instance for volatile hashes
inline SeenHashes seenHashes;           // Global instance

class FeedPrinter {
public:
  FeedPrinter () = default;
  ~FeedPrinter () = default;

  // Returns a simple markdown link format for the item
  // Example: [Title](Link)
  // This is useful for creating a simple clickable link in markdown format
  std::string getSimpleItemAsUrl (const RSSItem& item) {
    std::string result = "[" + item.title_ + "](" + item.link_ + ")";
    result += "\n";
    return result;
  }

  std::string getItemTitle (const RSSItem& item) {
    return item.title_;
  }

  std::string printItem (const RSSItem& item) {
    std::string result = "Title: " + item.title_ + "\n";
    result += "Link: " + item.link_ + "\n";
    result += "Description: " + item.description_ + "\n";
    if (!item.pubDate_.empty ()) {
      result += "Published: " + item.pubDate_ + "\n";
    }
    if (!item.language_.empty ()) {
      result += "Language: " + item.language_ + "\n";
    }
    if (!item.hash_.empty ()) {
      result += "Hash: " + item.hash_ + "\n";
    }
    return result;
  }

  std::string printItemShort (const RSSItem& item) {
    std::string result = "Titulek: " + item.title_ + "\n";
    result += "Odkaz: " + item.link_ + "\n";
    result += "Popis: " + item.description_ + "\n";
    if (!item.pubDate_.empty ()) {
      result += "Publikováno: " + item.pubDate_ + "\n";
    }
    return result;
  }

  std::string printFeed (const RSSFeed& feed) {
    std::string result = "RSS Feed: " + feed.title + "\n";
    result += "Description: " + feed.description + "\n";
    result += "Link: " + feed.link + "\n\n";

    for (const auto& item : feed.items) {
      result += printItem (item) + "\n";
      result += "------------------------\n";
    }
    return result;
  }
};

class FeedParser {
public:
  FeedParser () = default;
  ~FeedParser () = default;
  RSSFeed& parseRSSToDataStructure (const std::string& xmlData);

private:
};

class FeedFetcher {
public:
  FeedFetcher () = default;
  ~FeedFetcher () = default;
  int fetchFeed (std::string url, bool embed);

private:
  FeedParser feedParser;
};

class FeedStorage {
public:
  FeedStorage () = default;
  ~FeedStorage () = default;

  std::filesystem::path getPathRssUrl () const {
    return AssetContext::getAssetsPath () / "rssUrls.json";
  }
  std::filesystem::path getPathSeenHashes () const {
    return AssetContext::getAssetsPath () / "seenHashes.json";
  }

  int createUrlListFile () {
    if (!std::filesystem::exists (getPathRssUrl ())) {
      // clang-format off
      nlohmann::json defaultRssUrls = {
        { "https://www.root.cz/rss/clanky/",              { { "embedded", true }, { "category", "root.cz" } } },
        { "https://www.root.cz/rss/zpravicky/",           { { "embedded", true }, { "category", "root.cz" } } },
        { "https://www.root.cz/rss/knihy/",               { { "embedded", true }, { "category", "root.cz" } } },
        { "https://www.root.cz/rss/skoleni",              { { "embedded", true }, { "category", "root.cz" } } },
        { "https://www.abclinuxu.cz/auto/abc.rss",        { { "embedded", false }, { "category", "abclinuxu.cz" } } },
        { "https://www.abclinuxu.cz/auto/zpravicky.rss",  { { "embedded", false }, { "category", "abclinuxu.cz" } } }
      };
      // clang-format on
      std::ofstream file (getPathRssUrl ());
      if (file.is_open ()) {
        file << defaultRssUrls.dump (4); // Pretty print with 4 spaces
        file.close ();
      } else {
        return -1;
      }
    }
    return 0;
  }

  // Initialize with an example array of strings
  int createSeenHashesFile () {
    if (!std::filesystem::exists (getPathSeenHashes ())) {
      // Create a default array with at least two string values
      nlohmann::json defaultSeenHashes = nlohmann::json::array ({ "0123456789", "1234567890" });
      // Write the default seen hashes to the file
      std::ofstream file (getPathSeenHashes ());
      if (file.is_open ()) {
        file << defaultSeenHashes.dump (4); // Pretty print with 4 spaces
        file.close ();
      } else {
        return -1;
      }
    }
    return 0;
  }

  int addSeenHashStringToSeenHashJsonFile (std::string hash) {
    std::ifstream file (getPathSeenHashes ());
    if (!file.is_open ()) {
      return -1; // Error opening the file
    }

    nlohmann::json jsonData;
    file >> jsonData;
    file.close ();

    // Add the new hash to the seen hashes array
    jsonData.push_back (hash);

    // Write back to the file
    std::ofstream outFile (getPathSeenHashes ());
    if (!outFile.is_open ()) {
      return -1; // Error opening the file for writing
    }
    outFile << jsonData.dump (4); // Pretty print with 4 spaces
    outFile.close ();
    return 0;
  }

  std::vector<UrlsStruct> fillUrlListStructFromFile () {
    std::vector<UrlsStruct> urls;
    std::ifstream file (getPathRssUrl ());
    if (file.is_open ()) {
      nlohmann::json jsonData;
      file >> jsonData;
      for (const auto& item : jsonData.items ()) {
        UrlsStruct urlStruct (item.key (), item.value ().value ("embedded", false),
                              item.value ().value ("category", ""));
        urls.push_back (urlStruct);
      }
      file.close ();
    } else {
      throw std::runtime_error ("Failed to open RSS URLs file." + getPathRssUrl ().string ());
    }
    return urls;
  }

  SeenHashes fillSeenHashesStructFromFile () {
    SeenHashes seenHashes;
    std::ifstream file (getPathSeenHashes ());
    if (file.is_open ()) {
      nlohmann::json jsonData;
      file >> jsonData;
      for (const auto& hash : jsonData) {
        if (!hash.is_string ()) {
          throw std::runtime_error ("Invalid hash format in seen hashes file.");
        }
        seenHashes.addHash (hash.get<std::string> ());
      }
      file.close ();
    } else {
      throw std::runtime_error ("Failed to open Seen Hashes file."
                                + getPathSeenHashes ().string ());
    }
    return seenHashes;
  }

  VolatileHashes copySeenHashesToVolatileHashes () {
    VolatileHashes volatileHashes;
    for (const auto& hash : seenHashes.hashes) {
      volatileHashes.addHash (hash);
    }
    return volatileHashes;
  }

  // Add an item to the feed storage
  void addItem (const RSSItem& item) {
    rssFeeds.addItem (item);
  }

  // Get the current RSS feed
  const RSSFeed& getFeed () const {
    return rssFeeds;
  }

  // Get the item count in the feed
  int getItemCount () const {
    return rssFeeds.getItemCount ();
  }
};

class FeedPicker {
public:
  FeedPicker () = default;
  ~FeedPicker () = default;

  /// @brief Picks a random item from the RSS feed.
  /// If no items are available, returns an empty RSSItem.

  /// No duplicates are allowed; once an item is picked, it is removed from the feed.

  /// @return A random RSSItem from the feed, or an empty RSSItem if no items are available.
  RSSItem pickUpRandomItem () {
    if (rssFeeds.getItems ().empty ()) {
      return RSSItem (); // Return empty item if no items are available
    }

    // Ensure randomIndex_ is within the bounds of the items vector
    if (randomIndex_ >= rssFeeds.getItemCount ()) {
      randomIndex_ = 0; // Reset to 0 if out of bounds
    }

    randomIndex_ = getModernRandomNumber (0, rssFeeds.getItemCount () - 1);
    RSSItem randomItem = rssFeeds.getItems ()[randomIndex_];

    // Remove the item from the feed
    rssFeeds.getItems ().erase (rssFeeds.getItems ().begin () + randomIndex_);

    // Store the hash of the picked item in seenHashes
    FeedStorage feedStorage;
    feedStorage.addSeenHashStringToSeenHashJsonFile (randomItem.hash_);

    // Check if the item is embedded

    return randomItem; // Return the picked item
  }

private:
  size_t randomIndex_ = 0;
};

class RssManager {
public:
  RssManager () = default;
  ~RssManager () = default;

  // create exteranl files
  int createFiles () {
    FeedStorage storage;
    int result = storage.createUrlListFile ();
    if (result != 0) {
      return result; // Error creating RSS URLs file
    }
    result = storage.createSeenHashesFile ();
    if (result != 0) {
      return result; // Error creating seen hashes file
    }
    return 0;
  }

  int loadUrlListFromFile () {
    FeedStorage storage;
    try {
      rssUrls = storage.fillUrlListStructFromFile ();
      for (const auto& urlStruct : rssUrls) {
        LOG_I_STREAM << "Loaded RSS URL: " << urlStruct.url << ", Embed: " << urlStruct.embed
                     << ", Category: " << urlStruct.category << std::endl;
      }
    } catch (const std::runtime_error& e) {
      LOG_E_STREAM << "Error loading RSS URLs: " << e.what () << std::endl;
      return -1;
    }
    return 0;
  }

  int loadSeenHashesFromFile () {
    FeedStorage storage;
    try {
      seenHashes = storage.fillSeenHashesStructFromFile ();
      LOG_I_STREAM << "Loaded " << seenHashes.hashes.size () << " seen hashes." << std::endl;
    } catch (const std::runtime_error& e) {
      LOG_E_STREAM << "Error loading seen hashes: " << e.what () << std::endl;
      return -1;
    }
    return 0;
  }

  int loadVolatileHashes () {
    FeedStorage storage;
    try {
      volatileHashes = storage.copySeenHashesToVolatileHashes ();
      LOG_I_STREAM << "Loaded " << volatileHashes.hashes.size () << " volatile hashes."
                   << std::endl;
    } catch (const std::runtime_error& e) {
      LOG_E_STREAM << "Error loading volatile hashes: " << e.what () << std::endl;
      return -1;
    }
    return 0;
  }

  int fetchFeeds () {
    FeedFetcher fetcher;
    int totalFetchedItems = 0;
    for (const auto& urlStruct : rssUrls) {
      int fetchedItems = fetcher.fetchFeed (urlStruct.url, urlStruct.embed);
      if (fetchedItems < 0) {
        LOG_E_STREAM << "Failed to fetch feed from URL: " << urlStruct.url << std::endl;
        continue; // Skip this URL on error
      }
      totalFetchedItems += fetchedItems;
      LOG_I_STREAM << "Total fetched " << fetchedItems << std::endl;
    }
    return totalFetchedItems;
  }

  // Fetch RSS feed from URL
  int fetchFeed (const std::string& url) {
    FeedFetcher fetcher;
    return fetcher.fetchFeed (url, false); // Default to not embedding
  }

  // Fetch RSS feed from URL
  int fetchFeed (const std::string& url, bool embed) {
    FeedFetcher fetcher;
    return fetcher.fetchFeed (url, embed);
  }

  // get item count
  int getItemCount () const {
    return rssFeeds.getItemCount ();
  }

  // Get a random item from the fetched feed
  RSSItem getRandomItem () {
    FeedPicker picker;
    return picker.pickUpRandomItem ();
  }

  std::string getSimpleItemAsUrl (const RSSItem& item) {
    FeedPrinter printer;
    return printer.getSimpleItemAsUrl (item);
  }

  // Print the item details
  std::string getItemTitle (const RSSItem& item) {
    FeedPrinter printer;
    return printer.getItemTitle (item);
  }

  std::string printItemShort (const RSSItem& item) {
    FeedPrinter printer;
    return printer.printItemShort (item);
  }

  // Get the size of the feed queue
  size_t getFeedQueueSize () const {
    return rssFeeds.getItemCount ();
  }
};

#endif // __RSSMANAGER_H__