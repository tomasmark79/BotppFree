#ifndef RSSREADER_HPP
#define RSSREADER_HPP

#include <tinyxml2.h>
#include <string>
#include <vector>

class RssReader {

public:
  RssReader () = default;
  ~RssReader () = default;

  struct RSSItem {
    std::string title;
    std::string link;
    std::string description;
    std::string pubDate;
    std::string guid;

    RSSItem () = default;
    RSSItem (const std::string& t, const std::string& l, const std::string& d,
             const std::string& date = "", const std::string& id = "")
        : title (t), link (l), description (d), pubDate (date), guid (id) {
    }
  };

  struct RSSFeed {
    std::string title;
    std::string description;
    std::string link;
    std::vector<RSSItem> items;

    // Methods for manipulation
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

    std::string toString () const;
  };

  RSSFeed parseRSSToStruct (const std::string& xmlData);
  std::string feedFromUrl (std::string url, int rssType = 2);
  std::string feedRandomFromUrl (std::string url, int rssType = 2);

private:
  const int MAX_CHARS_PER_MSG = 2000; // Max characters per message
};

#endif