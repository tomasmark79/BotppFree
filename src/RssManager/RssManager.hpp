#ifndef __RSSMANAGER_H__
#define __RSSMANAGER_H__

#include <tinyxml2.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

const int DISCORD_MAX_MSG_LEN = 2000; // Discord max characters per message

inline int getModernRandomNumber (int min, int max) {
  static std::random_device rd;                     // Obtain a random number from hardware
  static std::mt19937 eng (rd ());                  // Seed the generator
  std::uniform_int_distribution<> distr (min, max); // Define the range
  return distr (eng);                               // Generate the random number
}

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

// todo use external storage for seen hashes
struct SeenHashes {
  std::vector<std::string> hashes; // Store unique hashes of seen items
  void addHash (const std::string& hash) {
    hashes.push_back (hash);
  }
  bool contains (const std::string& hash) const {
    return std::find (hashes.begin (), hashes.end (), hash) != hashes.end ();
  }
};

struct RSSFeed {
  //std::string toString () const;
  std::string title;
  std::string description;
  std::string link;
  std::string pubDate;
  std::string language;
  std::string hash; // Unique hash for the feed

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
  const std::vector<RSSItem>& getItems () const {
    return items;
  }
  std::vector<RSSItem>& getItems () {
    return items;
  }
};

inline SeenHashes seenHashes; // Global instance to track seen hashes (future in external storage)
inline RSSFeed rssFeeds;      // Global instance to store RSS feeds

class FeedPrinter {
public:
  FeedPrinter () = default;
  ~FeedPrinter () = default;

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

class FeedPicker {
public:
  FeedPicker () = default;
  ~FeedPicker () = default;

  RSSItem pickRandomItem (const RSSFeed& feed) {
    if (feed.getItemCount () == 0) {
      return RSSItem (); // Return empty item if no items are available
    }
    size_t randomIndex = rand () % feed.getItemCount ();
    return feed.getItems ()[randomIndex];
  }

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

    return randomItem; // Return the picked item
  }

private:
  size_t randomIndex_ = 0;
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
  int fetchFeed (std::string url);

private:
  FeedParser feedParser;
};

class RssManager {
public:
  RssManager () = default;
  ~RssManager () = default;

  // Fetch RSS feed from URL
  int fetchFeed (const std::string& url) {
    FeedFetcher fetcher;
    return fetcher.fetchFeed (url);
  }

  // Get a random item from the fetched feed
  RSSItem getRandomItem () {
    FeedPicker picker;
    return picker.pickUpRandomItem ();
  }

  // Print the item details
  std::string printItem (const RSSItem& item) {
    FeedPrinter printer;
    return printer.printItem (item);
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