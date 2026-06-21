#ifndef HTTP_HTTPREQUEST_H
#define HTTP_HTTPREQUEST_H

#include <map>
#include <string>

namespace muduo
{
namespace net
{
class Buffer;
}
}

// HTTP 请求解析器——自己逐字节解析，不依赖第三方库
//
// 状态机：
//   kExpectRequestLine  → 解析 "GET /index.html HTTP/1.1\r\n"
//   kExpectHeaders      → 解析 "Host: localhost\r\n" 直到遇到空行 \r\n
//   kExpectBody         → 解析请求体（根据 Content-Length）
//   kGotCompleteRequest → 解析完毕，可以处理了
//
// 支持：
//   - GET / HEAD / POST
//   - 请求头跨 Buffer 分片（半包自动处理）
//   - Keep-Alive 多请求复用（reset() 重置状态）
//   - Content-Length 读取请求体

class HttpRequest
{
 public:
  enum ParseState
  {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotCompleteRequest,
  };

  HttpRequest()
    : state_(kExpectRequestLine)
  {
  }

  // 从 Buffer 中解析数据
  // 返回 true 表示一个完整的请求已经解析完毕
  // 返回 false 表示数据不完整，等更多数据来了再调
  //
  // 用法：
  //   HttpRequest req;
  //   if (req.parse(&inputBuffer)) {
  //     // req.method(), req.path() ... 都已就绪
  //     handleRequest(req);
  //     req.reset();  // 准备解析下一个请求（Keep-Alive）
  //   }
  bool parse(muduo::net::Buffer* buf);

  // 重置状态，用于 Keep-Alive 下解析同一个连接上的下一个请求
  void reset()
  {
    state_ = kExpectRequestLine;
    method_.clear();
    path_.clear();
    version_.clear();
    headers_.clear();
    body_.clear();
  }

  // === 访问器 ===
  const std::string& method()  const { return method_; }
  const std::string& path()    const { return path_; }
  const std::string& version() const { return version_; }
  const std::string& body()    const { return body_; }

  // 查询请求头（不区分大小写）
  std::string getHeader(const std::string& field) const;

  // 获取所有请求头
  const std::map<std::string, std::string>& headers() const { return headers_; }

  ParseState state() const { return state_; }

 private:
  // 解析请求行："GET /index.html HTTP/1.1\r\n"
  bool processRequestLine(const char* begin, const char* end);

  ParseState state_;
  std::string method_;
  std::string path_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

#endif  // HTTP_HTTPREQUEST_H
