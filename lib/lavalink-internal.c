#include <stdlib.h>
#include <string.h>

#include <concord/discord.h>
#include <concord/websockets.h>

#include <coglink/lavalink-internal.h>
#include <coglink/lavalink.h>
#include <coglink/definitions.h>

size_t __coglink_WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t writeSize = size * nmemb;
  struct httpRequest *mem = (struct httpRequest *)userp;

  char *ptr = realloc(mem->body, mem->size + writeSize + 1);
  if (!ptr) {
    perror("[SYSTEM] Not enough memory to realloc.\n");
    return 1;
  }

  mem->body = ptr;
  memcpy(&(mem->body[mem->size]), contents, writeSize);
  mem->size += writeSize;
  mem->body[mem->size] = 0;

  return writeSize;
}

size_t __coglink_WriteMemoryCallbackNoSave(void *contents, size_t size, size_t nmemb, void *userp) {
  (void) contents; (void) size; (void) userp;
  return nmemb;
}

int __coglink_checkCurlCommand(struct lavaInfo *lavaInfo, CURL *curl, CURLcode cRes, char *pos, int additionalDebugging, int getResponse, struct httpRequest *res) {
  if (cRes != CURLE_OK) {
    if (lavaInfo->debugging->allDebugging || additionalDebugging || lavaInfo->debugging->curlErrorsDebugging) log_fatal("[coglink:libcurl] curl_easy_setopt [%s] failed: %s\n", pos, curl_easy_strerror(cRes));

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    if (getResponse) free((*res).body);

    return COGLINK_LIBCURL_FAILED_SETOPT;
  }
  return COGLINK_SUCCESS;
}

int __coglink_performRequest(struct lavaInfo *lavaInfo, int requestType, int additionalDebuggingSuccess, int additionalDebuggingError, char *path, int pathLength, char *body, long bodySize, struct httpRequest *res, int getResponse, CURL *reUsedCurl) {
  if (!reUsedCurl) curl_global_init(CURL_GLOBAL_ALL);

  CURL *curl = reUsedCurl;
  if (!reUsedCurl) curl = curl_easy_init();

  if (!curl) {
    if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->curlErrorsDebugging) log_fatal("[coglink:libcurl] Error while initializing libcurl.");

    curl_global_cleanup();

    return COGLINK_LIBCURL_FAILED_INITIALIZE;
  }

  char lavaURL[strnlen(lavaInfo->node->hostname, 128) + (lavaInfo->node->ssl ? 12 : 11) + pathLength];
  if (lavaInfo->node->ssl) snprintf(lavaURL, sizeof(lavaURL), "https://%s/v4%s", lavaInfo->node->hostname, path);
  else snprintf(lavaURL, sizeof(lavaURL), "http://%s/v3%s", lavaInfo->node->hostname, path);

  CURLcode cRes = curl_easy_setopt(curl, CURLOPT_URL, lavaURL);
  if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "1", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) return COGLINK_LIBCURL_FAILED_SETOPT;

  struct curl_slist *chunk = NULL;
    
  if (lavaInfo->node->password) {
    char AuthorizationH[256];
    snprintf(AuthorizationH, sizeof(AuthorizationH), "Authorization: %s", lavaInfo->node->password);
    chunk = curl_slist_append(chunk, AuthorizationH);

    if (lavaInfo->debugging->allDebugging || additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Authorization header set.");
  }

  if (body) {
    chunk = curl_slist_append(chunk, "Content-Type: application/json");
    if (lavaInfo->debugging->allDebugging || additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Content-Type header set.");
  }

  chunk = curl_slist_append(chunk, "Client-Name: Coglink");
  if (lavaInfo->debugging->allDebugging || additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Client-Name header set.");

  chunk = curl_slist_append(chunk, "User-Agent: libcurl");
  if (lavaInfo->debugging->allDebugging || additionalDebuggingSuccess || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] User-Agent header set.");

  cRes = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "2", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) {
    curl_slist_free_all(chunk);
    return COGLINK_LIBCURL_FAILED_SETOPT;
  }

  if (requestType == __COGLINK_DELETE_REQ || requestType == __COGLINK_PATCH_REQ) {
    if (requestType == __COGLINK_DELETE_REQ) cRes = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    else if (requestType == __COGLINK_PATCH_REQ) cRes = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "3", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  }

  if (getResponse) {
    (*res).body = malloc(1);
    (*res).size = 0;

    cRes = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __coglink_WriteMemoryCallback);

    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "4", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }

    cRes = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&(*res));
    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "5", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  } else {
    cRes = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __coglink_WriteMemoryCallbackNoSave);

    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "6", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  }

  if (body) {
    cRes = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "7", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }

    cRes = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bodySize);
    if (__coglink_checkCurlCommand(lavaInfo, curl, cRes, "8", additionalDebuggingError, getResponse, res) != COGLINK_SUCCESS) {
      curl_slist_free_all(chunk);
      return COGLINK_LIBCURL_FAILED_SETOPT;
    }
  }

  cRes = curl_easy_perform(curl);
  if (cRes != CURLE_OK) {
    if (lavaInfo->debugging->allDebugging || additionalDebuggingError || lavaInfo->debugging->curlErrorsDebugging) log_fatal("[coglink:libcurl] curl_easy_perform failed: %s\n", curl_easy_strerror(cRes));

    curl_easy_cleanup(curl);
    curl_slist_free_all(chunk);
    curl_global_cleanup();
    if (getResponse) free((*res).body);

    return COGLINK_LIBCURL_FAILED_PERFORM;
  }

  curl_easy_cleanup(curl);
  curl_slist_free_all(chunk);
  curl_global_cleanup();
  
  if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->curlSuccessDebugging) log_debug("[coglink:libcurl] Performed request successfully.");

  return COGLINK_SUCCESS;
}

int __coglink_checkParse(struct lavaInfo *lavaInfo, jsmnf_pair *field, char *fieldName) {
  if (!field) {
    if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->checkParseErrorsDebugging) log_error("[coglink:jsmn-find] Failed to find %s field.", fieldName);
    return COGLINK_JSMNF_ERROR_FIND;
  }
  if (lavaInfo->debugging->allDebugging || lavaInfo->debugging->checkParseSuccessDebugging) log_debug("[coglink:jsmn-find] Successfully found %s field.", fieldName);
  return COGLINK_PROCEED;
}

void __coglink_randomString(char *dest, size_t length) {
  char charset[] = "abcdefghijklmnopqrstuvwxyz";

  while(length-- > 0) {
    size_t pos = (double) rand() / RAND_MAX * (sizeof(charset) - 1);
    *dest++ = charset[pos];
  }
  *dest = '\0';
}

int __coglink_IOPoller(struct io_poller *io, CURLM *multi, void *user_data) {
  (void) io; (void) multi;
  struct lavaInfo *lavaInfo = user_data;
  return !ws_multi_socket_run(lavaInfo->ws, &lavaInfo->tstamp) ? COGLINK_WAIT : COGLINK_SUCCESS;;
}
