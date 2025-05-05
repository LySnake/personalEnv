#ifndef HTTP_H
#define HTTP_H

#include <functional>
#include <string>
#include <vector>

// HTTP/HTTPS请求相关，对curl库功能封装
using namespace std::placeholders;
class Http {
public:
  Http();
  ~Http();

  struct Response {
    // 写数据回调。
    // Return 返回实际转存的数据。如果返回值与count，则本次http请求失败。
    using WriteCallback =
        std::function<size_t(const char *data, const size_t count)>;

    static constexpr size_t DEF_BUFF_SIZE = 1024;
    Response() {
      resp.reserve(DEF_BUFF_SIZE);
      write_callback = std::bind(&Response::DefCallback, this, _1, _2);
    }
    // write_cb:下载数据，写回调
    explicit Response(WriteCallback &&write_cb)
        : write_callback{std::move(write_cb)} {}

    size_t DefCallback(const char *data, const size_t count) {
      resp.append(data, count);
      return count;
    }

    WriteCallback write_callback; // 数据返回，写回调
    std::string resp;             // 默认写数据buff
  };

  // 数据上传/下载进度
  class Progress {
  public:
    // progress: [0, 100]
    using ProgressCallback = std::function<void(const uint8_t progress)>;
    Progress(ProgressCallback &&upload_cb = ProgressCallback(),
             ProgressCallback &&download_cb = ProgressCallback())
        : upload_progress_callback{std::move(upload_cb)},
          download_progress_callback{std::move(download_cb)} {
      progress_upload = 0;
      progress_download = 0;
    }

    ProgressCallback upload_progress_callback;   // 数据上传，进度回调
    ProgressCallback download_progress_callback; // 数据上传，进度回调
    uint8_t progress_upload;                     // 上传进度，进度[0,100]
    uint8_t progress_download;                   // 下载进度，进度[0,100]
  };

  /****************************************************
   * 函数说明：发起HTTPS GET请求
   *
   * url:请求的url
   * in:需要发送的数据
   * out:获取数据的接收器
   * progress:上传/下载数据的进度条Callback
   * timeout_s: 允许整个传输作所花费的最长时间（以秒为单位）
   *
   * return: true:成功    false:失败
   *****************************************************/
  bool Get(const std::string &url, const std::string &in, Response &out,
           Progress &progress, const long timeout_ms = 10 * 1000);

  /****************************************************
   * 函数说明：发起HTTPS POST请求
   *
   * url:请求的url
   * in:需要发送的数据(数据类型不同，需要设置不同的header,比如发送json,需要设置
   *        "Content-Type:application/json")
   * headers:设置http post请求的headers
   * out:获取数据的接收器
   * timeout_s: 允许整个传输作所花费的最长时间（以秒为单位）
   *
   * return: true:成功    false:失败
   *****************************************************/
  bool Post(const std::string &url, const std::string &in,
            const std::vector<std::string> &headers, Response &out,
            const long timeout_ms = 10 * 1000);

private:
};

#endif // HTTP_H