#include "RssManager.hpp"
#include <Logger/Logger.hpp>
#include <random>
#include <curl/curl.h>

// callback function for curl to write data into a string
size_t WriteCallback (void* contents, size_t size, size_t nmemb, void* userp) {
  ((std::string*)userp)->append ((char*)contents, size * nmemb);
  return size * nmemb;
}

/// @brief Parses an RSS feed from XML data and populates the RSSFeed data structure.
/// @param xmlData The XML data of the RSS feed as a string.
/// @return A reference to the populated RSSFeed data structure.
RSSFeed& FeedParser::parseRSSToDataStructure (const std::string& xmlData) {
  LOG_D_STREAM << "called: parseRSSToDataStructure" << std::endl;

  std::vector<std::string> skippedItems;

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
    return rssFeeds;
  }

  if (!channel) {
    LOG_E_STREAM << "Error: RSS feed is not valid - no channel found." << std::endl;
    return rssFeeds;
  }

  // Parse channel info (same for both formats)
  if (auto titleElement = channel->FirstChildElement ("title")) {
    rssFeeds.title = titleElement->GetText () ? titleElement->GetText () : "";
  }
  if (auto descElement = channel->FirstChildElement ("description")) {
    rssFeeds.description = descElement->GetText () ? descElement->GetText () : "";
  }
  if (auto linkElement = channel->FirstChildElement ("link")) {
    rssFeeds.link = linkElement->GetText () ? linkElement->GetText () : "";
  }
  if (auto languageElement = channel->FirstChildElement ("language")) {
    rssFeeds.language = languageElement->GetText () ? languageElement->GetText () : "";
  }
  if (auto pubDateElement = channel->FirstChildElement ("pubDate")) {
    rssFeeds.pubDate = pubDateElement->GetText () ? pubDateElement->GetText () : "";
  }

  // Parse items
  tinyxml2::XMLElement* item = firstItem;

  while (item) {
    RSSItem localRssItem;

    if (auto titleElement = item->FirstChildElement ("title")) {
      localRssItem.title_ = titleElement->GetText () ? titleElement->GetText () : "";
    }
    if (auto linkElement = item->FirstChildElement ("link")) {
      localRssItem.link_ = linkElement->GetText () ? linkElement->GetText () : "";
    }
    if (auto descElement = item->FirstChildElement ("description")) {
      localRssItem.description_ = descElement->GetText () ? descElement->GetText () : "";
    }
    if (auto pubDateElement = item->FirstChildElement ("pubDate")) {
      localRssItem.pubDate_ = pubDateElement->GetText () ? pubDateElement->GetText () : "";
    }
    if (auto languageElement = item->FirstChildElement ("language")) {
      localRssItem.language_ = languageElement->GetText () ? languageElement->GetText () : "";
    }

    // hash generation
    std::hash<std::string> hasher;
    localRssItem.hash_ = std::to_string (
        hasher (localRssItem.title_ + localRssItem.link_ + localRssItem.description_));

    // avoid adding duplicate items
    if (seenHashes.contains (localRssItem.hash_)) {
      skippedItems.push_back (localRssItem.hash_);

    } else {
      // Add item to feed if it has a title and link
      if (!localRssItem.title_.empty () && !localRssItem.link_.empty ()) {
        rssFeeds.addItem (localRssItem);
        seenHashes.addHash (localRssItem.hash_); // Add hash to seen hashes
      } else {
        LOG_W_STREAM << "Skipping item with missing title or link." << std::endl;
      }
    }
    item = item->NextSiblingElement ("item");
  }

  LOG_I_STREAM << "Parsed " << rssFeeds.getItemCount () << " " << (isRSS2 ? "RSS2" : "RDF RSS")
               << " items" << std::endl;

  if (!skippedItems.empty ()) {
    LOG_W_STREAM << "Skipped " << skippedItems.size () << " duplicate items: ";
    for (const auto& hash : skippedItems) {
      LOG_W_STREAM << hash << " ";
    }
    LOG_W_STREAM << std::endl;
  }

  return rssFeeds;
}

/// @brief Fetches an RSS feed from a given URL and populates the RSSFeed data structure.
/// @param url The URL of the RSS feed to fetch.
/// @return Count of items fetched from the RSS feed, or -1 on error.
int FeedFetcher::fetchFeed (std::string url) {
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
      rssFeeds = feedParser.parseRSSToDataStructure (rawRssBuffer);
      if (rssFeeds.getItemCount () > 0) {
        LOG_D_STREAM << "Fetched " << rssFeeds.getItemCount () << " items from the feed."
                     << std::endl;
      } else {
        LOG_E_STREAM << "No items found in the RSS feed." << std::endl;
        curl_easy_cleanup (curl);
        return 0; // No items found
      }
    }
    curl_easy_cleanup (curl);
    return rssFeeds.getItemCount ();
  }
  curl_easy_cleanup (curl);
  return -1;
}

