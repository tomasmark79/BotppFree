#include "RssReader.hpp"
#include <Logger/Logger.hpp>
#include <random>
#include <curl/curl.h>

size_t WriteCallback (void* contents, size_t size, size_t nmemb, void* userp) {
  ((std::string*)userp)->append ((char*)contents, size * nmemb);
  return size * nmemb;
}

RssReader::RSSFeed RssReader::parseRSSToStruct (const std::string& xmlData) {
  LOG_D_STREAM << "Parsing RSS feed to structure..." << std::endl;

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

  // Parse items
  tinyxml2::XMLElement* item = firstItem;
  while (item) {
    RSSItem rssItem;

    if (auto titleElement = item->FirstChildElement ("title")) {
      rssItem.title = titleElement->GetText () ? titleElement->GetText () : "";
    }
    if (auto linkElement = item->FirstChildElement ("link")) {
      rssItem.link = linkElement->GetText () ? linkElement->GetText () : "";
    }
    if (auto descElement = item->FirstChildElement ("description")) {
      rssItem.description = descElement->GetText () ? descElement->GetText () : "";
    }
    if (auto pubDateElement = item->FirstChildElement ("pubDate")) {
      rssItem.pubDate = pubDateElement->GetText () ? pubDateElement->GetText () : "";
    }
    if (auto guidElement = item->FirstChildElement ("guid")) {
      rssItem.guid = guidElement->GetText () ? guidElement->GetText () : "";
    }

    if (!rssItem.title.empty () && !rssItem.link.empty ()) {
      localFeed.addItem (rssItem);
      LOG_I_STREAM << "Added item: " << rssItem.title << std::endl;
    }

    item = item->NextSiblingElement ("item");
  }

  LOG_I_STREAM << "Parsed " << localFeed.getItemCount () << " " << (isRSS2 ? "RSS2" : "RDF RSS")
               << " items" << std::endl;
  return localFeed;
}

// Helper method to convert RSSFeed back to string if needed
std::string RssReader::RSSFeed::toString () const {
  std::string result = "RSS Feed: " + title + "\n";
  result += "Description: " + description + "\n";
  result += "Link: " + link + "\n\n";

  for (const auto& item : items) {
    result += "Title: " + item.title + "\n";
    result += "Link: " + item.link + "\n";
    result += "Description: " + item.description + "\n";
    if (!item.pubDate.empty ()) {
      result += "Published: " + item.pubDate + "\n";
    }
    result += "------------------------\n";
  }
  return result;
}

std::string RssReader::feedFromUrl (std::string url, int rssType) {
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
      LOG_D_STREAM << "Downloaded content:\n" << rawRssBuffer << std::endl;
      RSSFeed localFeed = parseRSSToStruct (rawRssBuffer);
      for (const auto& item : localFeed.getItems ()) {
        LOG_I_STREAM << "Title: " << item.title << std::endl;
        LOG_I_STREAM << "Link: " << item.link << std::endl;
        msg = "[" + item.title + "](" + item.link + ")\n";
        if (msg.size () + limitedMsg.size () < MAX_CHARS_PER_MSG) {
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

std::string RssReader::feedRandomFromUrl (std::string url, int rssType) {
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
      RSSFeed localFeed = parseRSSToStruct (rawRssBuffer);

      // get one random item from feed
      if (localFeed.getItemCount () > 0) {

        std::random_device rd;
        std::mt19937 gen (rd ());
        std::uniform_int_distribution<size_t> dis (0, localFeed.getItemCount () - 1);
        size_t randomIndex = dis (gen);

        const RSSItem& item = localFeed.getItems ()[randomIndex];
        
        LOG_I_STREAM << "Random item: " << item.title << std::endl;
        LOG_I_STREAM << "Link: " << item.link << std::endl;
        msg = "[" + item.title + "](" + item.link + ")\n";
      } else {
        LOG_E_STREAM << "No items found in the RSS feed." << std::endl;
      }
      return msg;
    }
    curl_easy_cleanup (curl);
  }
  return "Error: Could not get the RSS feed!";
}