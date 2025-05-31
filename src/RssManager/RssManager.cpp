#include "RssManager.hpp"
#include <Logger/Logger.hpp>
#include <curl/curl.h>
#include <fstream>
#include <random>

// CURL callback
size_t WriteCallback (void* contents, size_t size, size_t nmemb, void* userp) {
  ((std::string*)userp)->append ((char*)contents, size * nmemb);
  return size * nmemb;
}

RssManager::RssManager () : rng_ (std::random_device{}()) {
}

int RssManager::initialize () {
  // Create default files if they don't exist
  if (!std::filesystem::exists (getUrlsPath ())) {
    nlohmann::json defaultUrls = nlohmann::json::array (
        { { { "url", "https://www.root.cz/rss/clanky/" }, { "embedded", true } },
          { { "url", "https://www.lupa.cz/rss/clanky/" }, { "embedded", true } },
          { { "url", "https://www.root.cz/rss/zpravicky/" }, { "embedded", true } },
          { { "url", "https://www.root.cz/rss/skoleni" }, { "embedded", true } },
          { { "url", "https://www.abclinuxu.cz/auto/zpravicky.rss" }, { "embedded", false } },
          { { "url", "https://9to5linux.com/feed/atom" }, { "embedded", true } },
          { { "url", "https://www.abclinuxu.cz/auto/abc.rss" }, { "embedded", false } } });

    std::ofstream file (getUrlsPath ());
    if (!file.is_open ())
      return -1;
    file << defaultUrls.dump (4);
  }

  if (!std::filesystem::exists (getHashesPath ())) {
    nlohmann::json defaultHashes = nlohmann::json::array ();
    std::ofstream file (getHashesPath ());
    if (!file.is_open ())
      return -1;
    file << defaultHashes.dump (4);
  }

  // Initialize timestamps after loading
  if (std::filesystem::exists (getUrlsPath ())) {
    urlsLastModified_ = std::filesystem::last_write_time (getUrlsPath ());
  }
  if (std::filesystem::exists (getHashesPath ())) {
    hashesLastModified_ = std::filesystem::last_write_time (getHashesPath ());
  }

  return loadUrls () == 0 && loadSeenHashes () == 0 ? 0 : -1;
}

int RssManager::loadUrls () {
  std::ifstream file (getUrlsPath ());
  if (!file.is_open ())
    return -1;

  nlohmann::json jsonData;
  file >> jsonData;

  urls_.clear ();
  for (const auto& item : jsonData) {
    if (item.is_object () && item.contains ("url")) {
      std::string url = item["url"].get<std::string> ();
      bool embedded = item.contains ("embedded") ? item["embedded"].get<bool> () : false;
      urls_.emplace_back (url, embedded);
    } else if (item.is_string ()) {
      // Backwards compatibility - treat strings as non-embedded
      urls_.emplace_back (item.get<std::string> (), false);
    }
  }

  LOG_I_STREAM << "Loaded " << urls_.size () << " RSS URLs." << std::endl;
  return 0;
}

int RssManager::loadSeenHashes () {
  std::ifstream file (getHashesPath ());
  if (!file.is_open ())
    return -1;

  nlohmann::json jsonData;
  file >> jsonData;

  seenHashes_.clear ();
  for (const auto& hash : jsonData) {
    if (hash.is_string ()) {
      seenHashes_.insert (hash.get<std::string> ());
    }
  }

  LOG_I_STREAM << "Loaded " << seenHashes_.size () << " seen hashes." << std::endl;
  return 0;
}

int RssManager::saveSeenHash (const std::string& hash) {
  seenHashes_.insert (hash);

  nlohmann::json jsonData = nlohmann::json::array ();
  for (const auto& h : seenHashes_) {
    jsonData.push_back (h);
  }

  std::ofstream file (getHashesPath ());
  if (!file.is_open ())
    return -1;
  file << jsonData.dump (4);
  return 0;
}

std::string RssManager::downloadFeed (const std::string& url) {
  CURL* curl = curl_easy_init ();
  if (!curl)
    return "";

  std::string buffer;
  curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &buffer);
  curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L);

  CURLcode res = curl_easy_perform (curl);
  curl_easy_cleanup (curl);

  if (res != CURLE_OK) {
    LOG_E_STREAM << "CURL error: " << curl_easy_strerror (res) << std::endl;
    return "";
  }

  return buffer;
}

RSSFeed RssManager::parseRSS (const std::string& xmlData, bool embedded) {
  RSSFeed feed;
  tinyxml2::XMLDocument doc;
  doc.Parse (xmlData.c_str ());

  tinyxml2::XMLElement* channel = nullptr;
  tinyxml2::XMLElement* firstItem = nullptr;
  bool isAtom = false;

  // Try RSS 2.0 format
  if (auto rssElement = doc.FirstChildElement ("rss")) {
    channel = rssElement->FirstChildElement ("channel");
    if (channel) {
      firstItem = channel->FirstChildElement ("item");
    }
  }
  // Try RSS 1.0 format
  else if (auto rdfElement = doc.FirstChildElement ("rdf:RDF")) {
    channel = rdfElement->FirstChildElement ("channel");
    firstItem = rdfElement->FirstChildElement ("item");
  }
  // Try Atom format
  else if (auto feedElement = doc.FirstChildElement ("feed")) {
    channel = feedElement;
    firstItem = feedElement->FirstChildElement ("entry");
    isAtom = true;
  }

  if (!channel) {
    LOG_E_STREAM << "No valid RSS/Atom channel found." << std::endl;
    return feed;
  }

  // Parse channel info
  if (isAtom) {
    // Atom feed info
    if (auto titleEl = channel->FirstChildElement ("title")) {
      feed.title = titleEl->GetText () ? titleEl->GetText () : "";
    }
    if (auto subtitleEl = channel->FirstChildElement ("subtitle")) {
      feed.description = subtitleEl->GetText () ? subtitleEl->GetText () : "";
    }
    if (auto linkEl = channel->FirstChildElement ("link")) {
      const char* href = linkEl->Attribute ("href");
      feed.link = href ? href : "";
    }
  } else {
    // RSS feed info
    if (auto titleEl = channel->FirstChildElement ("title")) {
      feed.title = titleEl->GetText () ? titleEl->GetText () : "";
    }
    if (auto descEl = channel->FirstChildElement ("description")) {
      feed.description = descEl->GetText () ? descEl->GetText () : "";
    }
    if (auto linkEl = channel->FirstChildElement ("link")) {
      feed.link = linkEl->GetText () ? linkEl->GetText () : "";
    }
  }

  // Parse items
  int newItems = 0;
  const char* itemTag = isAtom ? "entry" : "item";

  for (auto item = firstItem; item; item = item->NextSiblingElement (itemTag)) {
    RSSItem rssItem;
    rssItem.embedded = embedded;

    if (isAtom) {
      // Parse Atom entry
      if (auto titleEl = item->FirstChildElement ("title")) {
        rssItem.title = titleEl->GetText () ? titleEl->GetText () : "";
      }
      if (auto linkEl = item->FirstChildElement ("link")) {
        const char* href = linkEl->Attribute ("href");
        rssItem.link = href ? href : "";
      }
      if (auto summaryEl = item->FirstChildElement ("summary")) {
        rssItem.description = summaryEl->GetText () ? summaryEl->GetText () : "";
      } else if (auto contentEl = item->FirstChildElement ("content")) {
        rssItem.description = contentEl->GetText () ? contentEl->GetText () : "";
      }
      if (auto updatedEl = item->FirstChildElement ("updated")) {
        rssItem.pubDate = updatedEl->GetText () ? updatedEl->GetText () : "";
      } else if (auto publishedEl = item->FirstChildElement ("published")) {
        rssItem.pubDate = publishedEl->GetText () ? publishedEl->GetText () : "";
      }
    } else {
      // Parse RSS item (existing code)
      if (auto titleEl = item->FirstChildElement ("title")) {
        rssItem.title = titleEl->GetText () ? titleEl->GetText () : "";
      }
      if (auto linkEl = item->FirstChildElement ("link")) {
        rssItem.link = linkEl->GetText () ? linkEl->GetText () : "";
      }
      if (auto descEl = item->FirstChildElement ("description")) {
        rssItem.description = descEl->GetText () ? descEl->GetText () : "";
      }
      if (auto dateEl = item->FirstChildElement ("pubDate")) {
        rssItem.pubDate = dateEl->GetText () ? dateEl->GetText () : "";
      }
    }

    if (rssItem.title.empty () || rssItem.link.empty ())
      continue;

    rssItem.generateHash ();

    // Skip if already seen
    if (seenHashes_.find (rssItem.hash) != seenHashes_.end ()) {
      LOG_D_STREAM << "Skipping duplicate item: " << rssItem.title << std::endl;
      continue;
    }
    feed.addItem (rssItem);
    newItems++;
  }

  LOG_I_STREAM << "Parsed " << newItems << " new items from " << (isAtom ? "Atom" : "RSS")
               << " feed (embedded: " << (embedded ? "true" : "false") << ")." << std::endl;
  return feed;
}

int RssManager::fetchFeed (const std::string& url, bool embedded) {
  LOG_D_STREAM << "Fetching feed: " << url << " (embedded: " << (embedded ? "true" : "false") << ")"
               << std::endl;

  std::string xmlData = downloadFeed (url);
  if (xmlData.empty ())
    return -1;

  RSSFeed newFeed = parseRSS (xmlData, embedded);

  // Create set of current hashes for fast lookup
  std::unordered_set<std::string> currentHashes;
  for (const auto& item : feed_.items) {
    currentHashes.insert (item.hash);
  }

  // Add new items to main feed, but check for duplicates
  int addedItems = 0;
  for (const auto& item : newFeed.items) {
    if (currentHashes.find (item.hash) == currentHashes.end ()) {
      feed_.addItem (item);
      currentHashes.insert (item.hash); // Update set for next iterations
      addedItems++;
    }
  }

  LOG_D_STREAM << "Added " << addedItems << " new items to feed" << std::endl;
  return addedItems;
}

int RssManager::fetchAllFeeds () {
  checkAndReloadFiles ();

  int totalItems = 0;

  for (const auto& rssUrl : urls_) {
    int items = fetchFeed (rssUrl.url, rssUrl.embedded);
    if (items > 0) {
      totalItems += items;
    }
  }

  LOG_I_STREAM << "Total fetched items: " << totalItems << std::endl;
  return totalItems;
}

size_t RssManager::getItemCount (bool embedded) const {
  size_t count = 0;
  for (const auto& item : feed_.items) {
    if (item.embedded == embedded) {
      count++;
    }
  }
  return count;
}

RSSItem RssManager::getRandomItem () {
  if (feed_.items.empty ()) {
    return RSSItem ();
  }

  std::uniform_int_distribution<size_t> dist (0, feed_.items.size () - 1);
  size_t index = dist (rng_);

  RSSItem item = feed_.items[index];

  // Save hash immediately to prevent re-processing
  saveSeenHash (item.hash);

  // Remove item from feed
  feed_.items.erase (feed_.items.begin () + index);

  return item;
}

RSSItem RssManager::getRandomItem (bool embedded) {
  // Find items with matching embedded flag
  std::vector<size_t> matchingIndices;
  for (size_t i = 0; i < feed_.items.size (); ++i) {
    if (feed_.items[i].embedded == embedded) {
      matchingIndices.push_back (i);
    }
  }

  if (matchingIndices.empty ()) {
    return RSSItem ();
  }

  // Pick random item from matching ones
  std::uniform_int_distribution<size_t> dist (0, matchingIndices.size () - 1);
  size_t randomIndex = dist (rng_);
  size_t actualIndex = matchingIndices[randomIndex];

  RSSItem item = feed_.items[actualIndex];

  // Save hash immediately to prevent re-processing
  saveSeenHash (item.hash);

  // Remove item from feed
  feed_.items.erase (feed_.items.begin () + actualIndex);

  return item;
}

// Add method to save all hashes at once (call this periodically or at shutdown)
int RssManager::saveAllSeenHashes () {
  nlohmann::json jsonData = nlohmann::json::array ();
  for (const auto& h : seenHashes_) {
    jsonData.push_back (h);
  }

  std::ofstream file (getHashesPath ());
  if (!file.is_open ())
    return -1;
  file << jsonData.dump (4);
  return 0;
}

bool RssManager::hasFileChanged (const std::filesystem::path& path,
                                 std::filesystem::file_time_type& lastModified) {
  if (!std::filesystem::exists (path)) {
    return false;
  }

  auto currentModified = std::filesystem::last_write_time (path);
  if (currentModified != lastModified) {
    lastModified = currentModified;
    return true;
  }
  return false;
}

void RssManager::checkAndReloadFiles () {
  bool urlsChanged = hasFileChanged (getUrlsPath (), urlsLastModified_);
  bool hashesChanged = hasFileChanged (getHashesPath (), hashesLastModified_);

  if (urlsChanged) {
    LOG_I_STREAM << "URLs file changed, reloading..." << std::endl;
    loadUrls ();
  }

  if (hashesChanged) {
    LOG_I_STREAM << "Hashes file changed, reloading..." << std::endl;
    loadSeenHashes ();
  }
}