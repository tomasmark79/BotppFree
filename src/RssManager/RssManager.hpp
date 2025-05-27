#ifndef __RSSMANAGER_H__
#define __RSSMANAGER_H__

#include <tinyxml2.h>
#include <string>
#include <vector>
#include <algorithm>

const int DISCORD_MAX_MSG_LEN = 2000; // Discord max characters per message

struct RSSItem {
  std::string title_;
  std::string link_;
  std::string description_;
  std::string pubDate_;
  std::string language_;
  std::string hash_;

  RSSItem () = default;
  RSSItem (const std::string& t, const std::string& l, const std::string& d,
           const std::string& date = "", const std::string& lang="" , const std::string& hash="")
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
  std::string toString () const;
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
};

class FeedParser {
public:
  FeedParser () = default;
  ~FeedParser () = default;
  RSSFeed parseRSSToDataStructure (const std::string& xmlData);

private:
};

class FeedFetcher {
public:
  FeedFetcher () = default;
  ~FeedFetcher () = default;
  std::string feedFromUrl (std::string url, int rssType = 2);
  std::string feedRandomFromUrl (std::string url, int rssType = 2);

private:
  FeedParser feedParser;
};

#endif // __RSSMANAGER_H__