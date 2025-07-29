#include "GoogleGemini.hpp"

static size_t WriteCallback (void* contents, size_t size, size_t nmemb, void* userp) {
  auto* s = static_cast<std::string*> (userp);
  s->append (static_cast<char*> (contents), size * nmemb);
  return size * nmemb;
}

GoogleGemini::GoogleGemini () {
}

GoogleGemini::~GoogleGemini () {
}

std::string GoogleGemini::generateContentGemini (const std::string& apiKey,
                                                 const std::string& model,
                                                 const std::string& prompt) {
  CURL* curl = curl_easy_init ();
  if (!curl)
    return "";

  nlohmann::json body
      = { { "contents",
            nlohmann::json::array (
                { { { "parts", nlohmann::json::array ({ { { "text", prompt } } }) } } }) } };
  std::string requestBody = body.dump ();
  LOG_I_STREAM << "Request body: " << requestBody << std::endl;

  std::string response;
  struct curl_slist* headers = nullptr;
  headers = curl_slist_append (headers, "Content-Type: application/json");
  headers = curl_slist_append (headers, ("X-goog-api-key: " + apiKey).c_str ());

  curl_easy_setopt (
      curl, CURLOPT_URL,
      ("https://generativelanguage.googleapis.com/v1beta/models/" + model + ":generateContent")
          .c_str ());
  curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt (curl, CURLOPT_POSTFIELDS, requestBody.c_str ());
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0L);

  // Set User-Agent - many sites require this
  curl_easy_setopt (curl, CURLOPT_USERAGENT,
                    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
                    "Chrome/120.0.0.0 Safari/537.36");

  // Set timeout options
  curl_easy_setopt (curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 10L);

  // Accept any encoding
  curl_easy_setopt (curl, CURLOPT_ENCODING, "");

  CURLcode res = curl_easy_perform (curl);
  curl_slist_free_all (headers);
  curl_easy_cleanup (curl);

  if (res != CURLE_OK) {
    std::cerr << "CURL error: " << curl_easy_strerror (res) << std::endl;
    return "";
  }

  // parse odpovědi a vytáhneme první kandidátní text
  try {
    auto j = nlohmann::json::parse (response);
    if (j.contains ("candidates") && !j["candidates"].empty ()) {
      auto& candidate = j["candidates"][0];
      if (candidate.contains ("content") && candidate["content"].contains ("parts")
          && !candidate["content"]["parts"].empty ()) {
        return candidate["content"]["parts"][0]["text"].get<std::string> ();
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "JSON parse error: " << e.what () << std::endl;
  }
  return "";
}