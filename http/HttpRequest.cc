#include "http/HttpRequest.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Buffer.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

using namespace muduo;
using namespace muduo::net;

// ========== 工具函数：大小写不敏感的字符串比较 ==========
static bool iequal(const std::string& a, const std::string& b)
{
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (std::tolower(a[i]) != std::tolower(b[i]))
      return false;
  return true;
}

// ========== 工具函数：去掉字符串首尾空白 ==========
static std::string trim(const std::string& s)
{
  size_t start = 0;
  while (start < s.size() && std::isspace(s[start])) ++start;
  size_t end = s.size();
  while (end > start && std::isspace(s[end - 1])) --end;
  return s.substr(start, end - start);
}

// ========== 解析请求行：GET /index.html HTTP/1.1\r\n ==========
bool HttpRequest::processRequestLine(const char* begin, const char* end)
{
  // 请求行格式：METHOD SP URL SP VERSION CR LF
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  if (space == end)
    return false;  // 没有空格，格式错误

  method_.assign(start, space);

  // 跳过连续空格
  start = space + 1;
  space = std::find(start, end, ' ');
  if (space == end)
    return false;

  path_.assign(start, space);

  // 版本号
  start = space + 1;
  version_.assign(start, end);

  return true;
}

// ========== 查询请求头（大小写不敏感） ==========
std::string HttpRequest::getHeader(const std::string& field) const
{
  std::string result;
  for (const auto& h : headers_)
  {
    if (iequal(h.first, field))
    {
      result = h.second;
      break;
    }
  }
  return result;
}

// ========== 核心：从 Buffer 解析 HTTP 请求 ==========
bool HttpRequest::parse(Buffer* buf)
{
  bool hasMore = true;
  while (hasMore)
  {
    switch (state_)
    {
      // === 状态 1：等请求行 ===
      case kExpectRequestLine:
      {
        const char* crlf = buf->findCRLF();
        if (crlf)
        {
          // 解析 "GET /index.html HTTP/1.1\r\n"
          bool ok = processRequestLine(buf->peek(), crlf);
          if (ok)
          {
            buf->retrieveUntil(crlf + 2);  // 跳过 \r\n
            state_ = kExpectHeaders;
          }
          else
          {
            return false;  // 请求行格式错误
          }
        }
        else
        {
          hasMore = false;  // 数据还没到齐，等下次
        }
        break;
      }

      // === 状态 2：等请求头 ===
      case kExpectHeaders:
      {
        const char* crlf = buf->findCRLF();
        if (crlf)
        {
          const char* colon = std::find(buf->peek(), crlf, ':');
          if (colon != crlf)
          {
            // 普通请求头：Host: localhost:8080\r\n
            std::string field(buf->peek(), colon);
            std::string value = trim(std::string(colon + 1, crlf));
            headers_[field] = value;
          }
          else
          {
            // 空行 \r\n —— 请求头结束
            // 检查是否有 Content-Length → 决定是否进 ExpectBody
            std::string contentLength = getHeader("Content-Length");
            if (!contentLength.empty())
            {
              state_ = kExpectBody;
            }
            else
            {
              state_ = kGotCompleteRequest;
            }
          }
          buf->retrieveUntil(crlf + 2);
        }
        else
        {
          hasMore = false;  // 数据没到齐
        }
        break;
      }

      // === 状态 3：等请求体 ===
      case kExpectBody:
      {
        int contentLength = std::atoi(getHeader("Content-Length").c_str());
        if (contentLength <= 0)
        {
          state_ = kGotCompleteRequest;
          break;
        }

        if (buf->readableBytes() >= static_cast<size_t>(contentLength))
        {
          // 请求体已全部到达
          body_.assign(buf->peek(), contentLength);
          buf->retrieve(contentLength);
          state_ = kGotCompleteRequest;
        }
        else
        {
          hasMore = false;  // 请求体还没收全
        }
        break;
      }

      // === 状态 4：解析完毕 ===
      case kGotCompleteRequest:
      {
        hasMore = false;
        break;
      }
    }
  }

  return state_ == kGotCompleteRequest;
}
