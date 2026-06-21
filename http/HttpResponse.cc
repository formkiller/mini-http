#include "http/HttpResponse.h"

#include "muduo/net/Buffer.h"

#include <cstdio>

using namespace muduo;
using namespace muduo::net;

// ========== 构造标准 HTTP 状态行 ==========
static std::string makeStatusLine(const std::string& version,
                                  int statusCode,
                                  const std::string& statusMessage)
{
  char buf[64];
  snprintf(buf, sizeof buf, "%s %d ", version.c_str(), statusCode);
  return std::string(buf) + statusMessage + "\r\n";
}

// ========== 将响应写入 Buffer ==========
void HttpResponse::appendToBuffer(Buffer* output) const
{
  // 1. 状态行
  output->append(makeStatusLine(version_, statusCode_, statusMessage_));

  // 2. 构造缺失的默认头
  bool hasServer = false;
  bool hasDate = false;
  bool hasContentType = false;

  for (const auto& h : headers_)
  {
    // 粗略检查（后续可优化）
    if (!hasServer && h.first.find("Server") != std::string::npos) hasServer = true;
    if (!hasDate && h.first.find("Date") != std::string::npos) hasDate = true;
    if (!hasContentType && h.first.find("Content-Type") != std::string::npos) hasContentType = true;
  }

  // 3. 写响应头 + 自动补头
  char buf[256];

  // Server
  if (!hasServer)
    output->append("Server: MiniHTTP/1.0 (muduo)\r\n");

  // Date（简化版——在真实项目中应解析 Timestamp）
  // 此处省略，curl 不强制检查 Date 头

  // Content-Type
  if (!hasContentType && !body_.empty())
    output->append("Content-Type: text/plain\r\n");

  // 4. 写出用户设置的头
  for (const auto& h : headers_)
  {
    output->append(h.first);
    output->append(": ");
    output->append(h.second);
    output->append("\r\n");
  }

  // 5. Content-Length
  snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
  output->append(buf);

  // 6. Connection
  output->append(closeConnection_ ? "Connection: close\r\n"
                                  : "Connection: Keep-Alive\r\n");

  // 7. 空行 + 响应体
  output->append("\r\n");
  output->append(body_);
}
