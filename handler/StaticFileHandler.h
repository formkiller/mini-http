#ifndef HANDLER_STATICFILEHANDLER_H
#define HANDLER_STATICFILEHANDLER_H

#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

#include <map>
#include <string>

// 静态文件处理器
//
// 职责：
//   1. 根据请求 URL 路径，映射到本地文件系统（docRoot）
//   2. 安全检查：防止目录穿越攻击（过滤 /../ 和 /..\）
//   3. 根据文件扩展名确定 MIME 类型
//   4. 读取文件内容，构造 HTTP 响应（200/404/403/413）
//   5. 缓存小文件（可选）
//
// 用法：
//   StaticFileHandler handler("/var/www/html");
//   server.router().get("/*", std::bind(&StaticFileHandler::handleRequest, &handler, _1));

class StaticFileHandler
{
 public:
  // 构造函数：指定静态文件根目录
  explicit StaticFileHandler(const std::string& docRoot);

  // 处理请求，返回 HTTP 响应
  HttpResponse handleRequest(const HttpRequest& req) const;

 private:
  // 将 URL 路径映射为本地文件系统路径
  std::string urlToPath(const std::string& urlPath) const;

  // 根据文件扩展名获取 MIME 类型
  static std::string getMimeType(const std::string& path);

  // 初始化 MIME 类型映射表
  static std::map<std::string, std::string> initMimeTypes();

  std::string docRoot_;  // 静态文件根目录
  static const std::map<std::string, std::string> kMimeTypes;  // MIME 类型表
};

#endif  // HANDLER_STATICFILEHANDLER_H
