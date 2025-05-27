#include "RssManager.hpp"
#include <Logger/Logger.hpp>
#include <BufferQueue/BufferQueue.hpp>
#include <random>
#include <curl/curl.h>

SeenHashes seenHashes; // Global instance to track seen hashes

size_t WriteCallback (void* contents, size_t size, size_t nmemb, void* userp) {
  ((std::string*)userp)->append ((char*)contents, size * nmemb);
  return size * nmemb;
}

RSSFeed FeedParser::parseRSSToDataStructure (const std::string& xmlData) {
  LOG_D_STREAM << "called: parseRSSToDataStructure" << std::endl;

  RSSFeed localFeed;
  tinyxml2::XMLDocument doc;
  doc.Parse (xmlData.c_str ());

  tinyxml2::XMLElement* rssElement = doc.FirstChildElement ("rss");
  tinyxml2::XMLElement* rdfElement = doc.FirstChildElement ("rdf:RDF");
  tinyxml2::XMLElement* channel = nullptr;
  tinyxml2::XMLElement* firstItem = nullptr;

  bool isRSS2 = false;

  if (rssElement) {
    // RSS 2.0 format
    isRSS2 = true;
    channel = rssElement->FirstChildElement ("channel");
    if (channel) {
      firstItem = channel->FirstChildElement ("item");
    }
  } else if (rdfElement) {
    // RSS 1.0 (RDF) format
    channel = rdfElement->FirstChildElement ("channel");
    firstItem = rdfElement->FirstChildElement ("item");
  } else {
    LOG_E_STREAM << "Error: No supported RSS root element found." << std::endl;
    return localFeed;
  }

  if (!channel) {
    LOG_E_STREAM << "Error: RSS feed is not valid - no channel found." << std::endl;
    return localFeed;
  }

  // Parse channel info (same for both formats)
  if (auto titleElement = channel->FirstChildElement ("title")) {
    localFeed.title = titleElement->GetText () ? titleElement->GetText () : "";
  }
  if (auto descElement = channel->FirstChildElement ("description")) {
    localFeed.description = descElement->GetText () ? descElement->GetText () : "";
  }
  if (auto linkElement = channel->FirstChildElement ("link")) {
    localFeed.link = linkElement->GetText () ? linkElement->GetText () : "";
  }
  if (auto languageElement = channel->FirstChildElement ("language")) {
    localFeed.language = languageElement->GetText () ? languageElement->GetText () : "";
  }
  if (auto pubDateElement = channel->FirstChildElement ("pubDate")) {
    localFeed.pubDate = pubDateElement->GetText () ? pubDateElement->GetText () : "";
  }

  // Parse items
  tinyxml2::XMLElement* item = firstItem;

  while (item) {
    RSSItem rssItem;

    if (auto titleElement = item->FirstChildElement ("title")) {
      rssItem.title_ = titleElement->GetText () ? titleElement->GetText () : "";
    }
    if (auto linkElement = item->FirstChildElement ("link")) {
      rssItem.link_ = linkElement->GetText () ? linkElement->GetText () : "";
    }
    if (auto descElement = item->FirstChildElement ("description")) {
      rssItem.description_ = descElement->GetText () ? descElement->GetText () : "";
    }
    if (auto pubDateElement = item->FirstChildElement ("pubDate")) {
      rssItem.pubDate_ = pubDateElement->GetText () ? pubDateElement->GetText () : "";
    }
    if (auto languageElement = item->FirstChildElement ("language")) {
      rssItem.language_ = languageElement->GetText () ? languageElement->GetText () : "";
    }

    // hash generation
    std::hash<std::string> hasher;
    rssItem.hash_ = std::to_string (hasher (rssItem.title_ + rssItem.link_ + rssItem.description_));

    // avoid adding duplicate items
    if (seenHashes.contains (rssItem.hash_)) {
      LOG_W_STREAM << "Skipping duplicate item with hash: " << rssItem.hash_ << std::endl;
    } else {
      // Add item to feed if it has a title and link
      if (!rssItem.title_.empty () && !rssItem.link_.empty ()) {
        localFeed.addItem (rssItem);
        seenHashes.addHash (rssItem.hash_); // Add hash to seen hashes
      } else {
        LOG_W_STREAM << "Skipping item with missing title or link." << std::endl;
      }
    }
    item = item->NextSiblingElement ("item");
  }

  LOG_I_STREAM << "Parsed " << localFeed.getItemCount () << " " << (isRSS2 ? "RSS2" : "RDF RSS")
               << " items" << std::endl;
  return localFeed;
}

// Helper method to convert RSSFeed back to string if needed
std::string RSSFeed::toString () const {
  std::string result = "RSS Feed: " + title + "\n";
  result += "Description: " + description + "\n";
  result += "Link: " + link + "\n\n";

  for (const auto& item : items) {
    result += "Title: " + item.title_ + "\n";
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
    result += "------------------------\n";
  }
  return result;
}

std::string FeedFetcher::feedFromUrl (std::string url, int rssType) {
  std::string msg = "";
  std::string limitedMsg = "";
  CURL* curl;
  CURLcode res;
  std::string rawRssBuffer;
  curl = curl_easy_init ();
  if (curl) {
    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L); /* temporary - todo cert */
    curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &rawRssBuffer);
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L); // -L

    res = curl_easy_perform (curl);

    if (res != CURLE_OK) {
      LOG_E_STREAM << "curl_easy_perform() failed: " << curl_easy_strerror (res) << std::endl;
    } else {
      // LOG_D_STREAM << "Downloaded content:\n" << rawRssBuffer << std::endl;

      // Parse the RSS feed
      RSSFeed localFeed = feedParser.parseRSSToDataStructure (rawRssBuffer);

      for (const auto& item : localFeed.getItems ()) {
        LOG_I_STREAM << "Title: " << item.title_ << std::endl;
        LOG_I_STREAM << "Link: " << item.link_ << std::endl;
        LOG_I_STREAM << "Description: " << item.description_ << std::endl;
        LOG_I_STREAM << "Published: " << item.pubDate_ << std::endl;
        LOG_I_STREAM << "Language: " << item.language_ << std::endl;
        LOG_I_STREAM << "Hash: " << item.hash_ << std::endl;

        msg = "[" + item.title_ + "](" + item.link_ + ")\n";
        if (msg.size () + limitedMsg.size () < DISCORD_MAX_MSG_LEN) {
          limitedMsg += msg;
        }
      }
      int len = limitedMsg.length ();
      return limitedMsg;
    }
    curl_easy_cleanup (curl);
  }
  return "Error: Could not get the RSS feed!";
}

std::string FeedFetcher::feedRandomFromUrl (std::string url, int rssType) {
  std::string msg = "";
  CURL* curl;
  CURLcode res;
  std::string rawRssBuffer;
  curl = curl_easy_init ();
  if (curl) {
    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L); /* temporary - todo cert */
    curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &rawRssBuffer);
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L); // -L

    res = curl_easy_perform (curl);

    if (res != CURLE_OK) {
      LOG_E_STREAM << "curl_easy_perform() failed: " << curl_easy_strerror (res) << std::endl;
    } else {
      LOG_D_STREAM << "Downloaded content:\n" << rawRssBuffer << std::endl;
      RSSFeed localFeed = feedParser.parseRSSToDataStructure (rawRssBuffer);

      // get one random item from feed
      if (localFeed.getItemCount () > 0) {

        std::random_device rd;
        std::mt19937 gen (rd ());
        std::uniform_int_distribution<size_t> dis (0, localFeed.getItemCount () - 1);
        size_t randomIndex = dis (gen);

        const RSSItem& item = localFeed.getItems ()[randomIndex];

        LOG_I_STREAM << "Random item: " << item.title_ << std::endl;
        LOG_I_STREAM << "Link: " << item.link_ << std::endl;
        msg = "[" + item.title_ + "](" + item.link_ + ")\n";
      } else {
        LOG_E_STREAM << "No items found in the RSS feed." << std::endl;
      }
      return msg;
    }
    curl_easy_cleanup (curl);
  }
  return "Error: Could not get the RSS feed!";
}