
#include <curl/curl.h>
#include <signal.h>

#include <algorithm>

#include "Http.h"
#include "log.h"

namespace {
const char *CA_FILE_PATH = "/home/zhengze/curl/cacert.pem";

struct ReadBuffer {
  explicit ReadBuffer(const std::string &d) : data{d}, offset{0} {}
  const std::string &data;
  size_t offset;
};

size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
  ReadBuffer &in = *reinterpret_cast<ReadBuffer *>(userdata);
  size_t ret = 0;
  if (in.offset < in.data.size()) {
    size_t need_copy = in.data.size() - in.offset;
    size_t copy_size = std::min(need_copy, size * nitems);

    auto begin = in.data.begin() + in.offset;
    std::copy(begin, begin + copy_size, buffer);
    in.offset += copy_size;
    ret = copy_size;
  }

  // 0:写完成
  return ret;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  Http::Response &out = *reinterpret_cast<Http::Response *>(userdata);
  size_t ret = 0;

  if (out.write_callback) {
    ret = out.write_callback(ptr, size * nmemb);
  } else {
    LOG_ERROR("curl function_write: No processing of received data has been "
              "carried out.");
  }

  return ret;
}

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal, curl_off_t ulnow) {
  Http::Progress &progress = *reinterpret_cast<Http::Progress *>(clientp);
  if (ultotal != 0) {
    const uint8_t progress_upload = static_cast<uint8_t>(ulnow * 100 / ultotal);
    if (progress.upload_progress_callback &&
        progress.progress_upload != progress_upload) {
      progress.progress_upload = progress_upload;
      progress.upload_progress_callback(progress_upload);
    }
  }
  if (dltotal != 0) {
    const uint8_t progress_download =
        static_cast<uint8_t>(dlnow * 100 / dltotal);
    if (progress.download_progress_callback &&
        progress.progress_download != progress_download) {
      progress.progress_download = progress_download;
      progress.download_progress_callback(progress_download);
    }
  }

  return 0;
}

class HttpGlobalHelper {
public:
  HttpGlobalHelper() {
    auto ret = curl_global_init(CURL_GLOBAL_ALL);
    if (ret != CURLE_OK) {
      LOG_ERROR("The curl initialization failed. Reason:%s.",
                curl_easy_strerror(ret));
      exit(EXIT_FAILURE);
    }

    // libcurl库在多线程环境下，会有问题。
    // 按 https://curl.se/libcurl/c/threadsafe.html 方式处理。
    signal(SIGPIPE, SIG_IGN);
  }

  ~HttpGlobalHelper() { curl_global_cleanup(); }
};
} // namespace

Http::Http() { static HttpGlobalHelper helper; }

Http::~Http() {}

bool Http::Get(const std::string &url, const std::string &in, Response &out,
               Progress &progress, const long timeout_ms) {
  bool result = false;
  auto curl = curl_easy_init();
  int ret = CURLE_OK;
  if (curl) {
    ret |= curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    ret |= curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // 线程安全方式

    ret |= curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    ret |= curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT,
                            10 * 1024L);                        // 最低10KB/s
    ret |= curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L); // 持续时间:30s

    // 设置SSL验证（强制HTTPS）
    ret |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L); // 验证证书
    ret |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L); // 严格验证主机名
    ret |= curl_easy_setopt(curl, CURLOPT_CAINFO,
                            CA_FILE_PATH); // 设置CA文件路径

    ReadBuffer buffer_in(in);
    if (!in.empty()) {
      // 上传数据
      ret |= curl_easy_setopt(curl, CURLOPT_READDATA, &buffer_in);
      ret |= curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
      // CURLOPT_UPLOAD_BUFFERSIZE
    }

    // 返回数据
    ret |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    ret |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    ret |= curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 512 * 1024L);

    // 上传/下载进度
    ret |= curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);
    ret |= curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    ret |= curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

    // 如果返回的 HTTP 代码等于或大于 400，则请求失败
    ret |= curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    if (ret == CURLE_OK) {
      ret = curl_easy_perform(curl);
      if (ret == CURLE_OK) {
        result = true;
      } else {
        LOG_WARN("The HTTP request execution failed. Reason:%s.",
                 curl_easy_strerror(CURLcode(ret)));
      }
      long httpCode = 0;
      ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      if (ret == CURLE_OK) {
        if (httpCode) {
          LOG_INFO("HTTP Code is %ld.", httpCode);
        }
      } else {
        LOG_WARN("Query failed for HTTP code.. Reason:%s.",
                 curl_easy_strerror(CURLcode(ret)));
      }

    } else {
      LOG_WARN("Configuration of HTTP request failed. Reason:%s.",
               curl_easy_strerror(CURLcode(ret)));
    }

    curl_easy_cleanup(curl);
  } else {
    LOG_WARN("The initialization of the HTTP request failed.");
  }

  return result;
}

bool Http::Post(const std::string &url, const std::string &in,
                const std::vector<std::string> &headers, Response &out,
                const long timeout_ms) {
  bool result = false;
  auto curl = curl_easy_init();
  int ret = CURLE_OK;
  if (curl) {
    ret |= curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    ret |= curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // 线程安全方式

    ret |= curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    ret |= curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT,
                            10 * 1024L);                        // 最低10KB/s
    ret |= curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L); // 持续时间:30s

    // 设置SSL验证（强制HTTPS）
    ret |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L); // 验证证书
    ret |= curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L); //
    ret |=
        curl_easy_setopt(curl, CURLOPT_CAINFO, CA_FILE_PATH); // 设置CA文件路径

    ReadBuffer buffer_in(in);
    if (!in.empty()) {
      ret |= curl_easy_setopt(curl, CURLOPT_POST, 1L);
      ret |= curl_easy_setopt(curl, CURLOPT_READDATA, &buffer_in);
      ret |= curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
      ret |= curl_easy_setopt(curl, CURLOPT_POSTFIELDS, nullptr);
    }

    struct curl_slist *pHeaders = nullptr;
    if (!headers.empty()) {
      for (const auto &header : headers) {
        pHeaders = curl_slist_append(pHeaders, header.c_str());
      }

      ret |= curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pHeaders);
    }

    ret |= curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
    ret |= curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    ret |= curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 64 * 1024L);

    // 如果返回的 HTTP 代码等于或大于 400，则请求失败
    ret |= curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    if (ret == CURLE_OK) {
      ret = curl_easy_perform(curl);
      if (ret == CURLE_OK) {
        result = true;
      } else {
        LOG_WARN("The HTTP request execution failed. Reason:%s.",
                 curl_easy_strerror(CURLcode(ret)));
      }
      long httpCode = 0;
      ret = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
      if (ret == CURLE_OK) {
        if (httpCode) {
          LOG_INFO("HTTP Code is %ld.", httpCode);
        }
      } else {
        LOG_WARN("Query failed for HTTP code.. Reason:%s.",
                 curl_easy_strerror(CURLcode(ret)));
      }

    } else {
      LOG_WARN("Configuration of HTTP request failed. Reason:%s.",
               curl_easy_strerror(CURLcode(ret)));
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(pHeaders);
  } else {
    LOG_WARN("The initialization of the HTTP request failed.");
  }

  return result;
}