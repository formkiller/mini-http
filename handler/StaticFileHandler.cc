#include "handler/StaticFileHandler.h"

#include "muduo/base/Logging.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

using namespace muduo;

// ========== 初始化 MIME 类型表 ==========
std::map<std::string, std::string> StaticFileHandler::initMimeTypes()
{
  std::map<std::string, std::string> mime;
  mime[".html"]    = "text/html; charset=utf-8";
  mime[".htm"]     = "text/html; charset=utf-8";
  mime[".css"]     = "text/css; charset=utf-8";
  mime[".js"]      = "application/javascript; charset=utf-8";
  mime[".json"]    = "application/json; charset=utf-8";
  mime[".txt"]     = "text/plain; charset=utf-8";
  mime[".xml"]     = "application/xml; charset=utf-8";

  // 图片
  mime[".png"]     = "image/png";
  mime[".jpg"]     = "image/jpeg";
  mime[".jpeg"]    = "image/jpeg";
  mime[".gif"]     = "image/gif";
  mime[".svg"]     = "image/svg+xml";
  mime[".ico"]     = "image/x-icon";
  mime[".bmp"]     = "image/bmp";
  mime[".webp"]    = "image/webp";

  // 字体
  mime[".woff"]    = "font/woff";
  mime[".woff2"]   = "font/woff2";
  mime[".ttf"]     = "font/ttf";
  mime[".otf"]     = "font/otf";

  // 音视频
  mime[".mp3"]     = "audio/mpeg";
  mime[".mp4"]     = "video/mp4";
  mime[".webm"]    = "video/webm";
  mime[".ogg"]     = "audio/ogg";

  // 文档
  mime[".pdf"]     = "application/pdf";
  mime[".doc"]     = "application/msword";
  mime[".docx"]    = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";

  // 二进制
  mime[".zip"]     = "application/zip";
  mime[".gz"]      = "application/gzip";
  mime[".tar"]     = "application/x-tar";
  mime[".bz2"]     = "application/x-bzip2";

  // 默认
  mime[".bin"]     = "application/octet-stream";
  mime[".exe"]     = "application/octet-stream";

  return mime;
}

// 静态常量：MIME 类型表
const std::map<std::string, std::string> StaticFileHandler::kMimeTypes = initMimeTypes();

// ========== 构造函数 ==========
StaticFileHandler::StaticFileHandler(const std::string& docRoot)
  : docRoot_(docRoot)
{
  LOG_INFO << "StaticFileHandler docRoot = " << docRoot_;
}

// ========== 根据扩展名获取 MIME 类型 ==========
std::string StaticFileHandler::getMimeType(const std::string& path)
{
  // 找最后一个 '.'
  size_t dot = path.rfind('.');
  if (dot == std::string::npos)
    return "application/octet-stream";  // 无扩展名 → 默认二进制

  std::string ext = path.substr(dot);
  // 转小写
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

  auto it = kMimeTypes.find(ext);
  if (it != kMimeTypes.end())
    return it->second;

  return "application/octet-stream";
}

// ========== URL 路径 → 本地文件系统路径 ==========
std::string StaticFileHandler::urlToPath(const std::string& urlPath) const
{
  // ① URL 解码（简化版——只处理 %20 空格）
  std::string path = urlPath;
  size_t pct = path.find('%');
  while (pct != std::string::npos && pct + 2 < path.size())
  {
    std::string hex = path.substr(pct + 1, 2);
    char c = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
    path.replace(pct, 3, 1, c);
    pct = path.find('%', pct + 1);
  }

  // ② 安全：过滤目录穿越攻击（..）
  if (path.find("..") != std::string::npos)
    return "";  // 空字符串表示拒绝访问

  // ③ 去掉开头的 /
  if (!path.empty() && path[0] == '/')
    path = path.substr(1);

  // ④ 如果路径为空，默认 index.html
  if (path.empty())
    path = "index.html";

  // ⑤ 拼接完整路径
  return docRoot_ + "/" + path;
}

// ========== 处理 HTTP 请求 ==========
HttpResponse StaticFileHandler::handleRequest(const HttpRequest& req) const
{
  // ① URL → 文件路径
  std::string filePath = urlToPath(req.path());
  if (filePath.empty())
  {
    HttpResponse resp(403, "Forbidden");
    resp.setHeader("Content-Type", "text/plain");
    resp.setBody("403 Forbidden - Directory traversal not allowed\n");
    return resp;
  }

  LOG_INFO << "StaticFileHandler: " << req.path() << " → " << filePath;

  // ② 检查文件状态
  struct stat fileStat;
  if (::stat(filePath.c_str(), &fileStat) < 0)
  {
    // 文件不存在
    HttpResponse resp(404, "Not Found");
    std::string body = "<html><body><h1>404 Not Found</h1>";
    body += "<p>File not found: " + filePath + "</p></body></html>";
    resp.setHeader("Content-Type", "text/html; charset=utf-8");
    resp.setBody(body);
    return resp;
  }

  // ③ 检查是否为普通文件
  if (!S_ISREG(fileStat.st_mode))
  {
    HttpResponse resp(403, "Forbidden");
    resp.setHeader("Content-Type", "text/plain");
    resp.setBody("403 Forbidden\n");
    return resp;
  }

  // ④ 文件太大 → 拒绝（大于 100 MB）
  const off_t kMaxFileSize = 100 * 1024 * 1024;
  if (fileStat.st_size > kMaxFileSize)
  {
    HttpResponse resp(413, "Payload Too Large");
    resp.setHeader("Content-Type", "text/plain");
    resp.setBody("413 Payload Too Large - File exceeds 100 MB limit\n");
    return resp;
  }

  // ⑤ 读取文件内容
  std::ifstream file(filePath, std::ios::binary);
  if (!file.is_open())
  {
    HttpResponse resp(500, "Internal Server Error");
    resp.setHeader("Content-Type", "text/plain");
    resp.setBody("500 Internal Server Error - Cannot open file\n");
    return resp;
  }

  std::ostringstream ss;
  ss << file.rdbuf();
  file.close();

  // ⑥ 构造成功响应
  HttpResponse resp(200, "OK");
  resp.setHeader("Content-Type", getMimeType(filePath));

  // HEAD 方法不返回 body（由 HttpServer 层处理）
  resp.setBody(ss.str());

  return resp;
}
