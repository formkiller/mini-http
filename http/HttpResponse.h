#ifndef HTTP_HTTPRESPONSE_H
#define HTTP_HTTPRESPONSE_H

#include <map>
#include <string>

namespace muduo
{
namespace net
{
class Buffer;
}
}

// HTTP 响应构造器——纯数据组装，不涉及网络 I/O
//
// 用法：
//   HttpResponse resp(200, "OK");
//   resp.setHeader("Content-Type", "text/html");
//   resp.setBody("<h1>Hello World</h1>");
//   resp.appendToBuffer(&outputBuffer);  // 把响应写入发送缓冲区

class HttpResponse
{
 public:
  // 构造函数：状态码 + 状态消息 + HTTP 版本
  HttpResponse(int statusCode, const std::string& statusMessage,
               const std::string& version = "HTTP/1.1")
    : version_(version),
      statusCode_(statusCode),
      statusMessage_(statusMessage),
      closeConnection_(false)
  {
  }

  // 设置响应头
  void setHeader(const std::string& field, const std::string& value)
  {
    headers_[field] = value;
  }

  // 设置响应体（小内容——字符串方式）
  void setBody(const std::string& body)
  {
    body_ = body;
  }

  // 关闭连接（用于错误响应或 Connection: close）
  void setCloseConnection(bool on) { closeConnection_ = on; }
  bool closeConnection() const { return closeConnection_; }

  // HEAD 模式：只返回头，不返回 body
  void setHeadOnly(bool on) { headOnly_ = on; }

  // 将完整的 HTTP 响应写入 Buffer
  // 包括：状态行 + 响应头 + 空行 + 响应体（headOnly_ 时跳过 body）
  void appendToBuffer(muduo::net::Buffer* output) const;

 private:
  std::string version_;
  int statusCode_;
  std::string statusMessage_;
  std::map<std::string, std::string> headers_;
  std::string body_;
  bool closeConnection_;
  bool headOnly_ = false;
};

#endif  // HTTP_HTTPRESPONSE_H
