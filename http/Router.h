#ifndef HTTP_ROUTER_H
#define HTTP_ROUTER_H

#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

#include <functional>
#include <map>
#include <string>

// URL 路由器——把 HTTP 请求路径映射到处理函数
//
// 用法：
//   Router router;
//   router.get("/", [](const HttpRequest& req) -> HttpResponse {
//     return HttpResponse(200, "OK").setBody("<h1>Home</h1>");
//   });
//
//   auto handler = router.findHandler(req);
//   if (handler)
//     HttpResponse resp = handler(req);

class Router
{
 public:
  // 处理函数签名：收到请求 → 返回响应
  typedef std::function<HttpResponse(const HttpRequest&)> Handler;

  // 注册 GET 路由
  void get(const std::string& path, Handler handler)
  {
    getHandlers_[path] = std::move(handler);
  }

  // 注册 POST 路由
  void post(const std::string& path, Handler handler)
  {
    postHandlers_[path] = std::move(handler);
  }

  // 注册兜底处理器（当没有精确匹配时调用）
  void setDefaultHandler(Handler handler)
  {
    defaultHandler_ = std::move(handler);
  }

  // 查找匹配的处理器
  // 返回空 Handler（nullptr）表示 404
  Handler findHandler(const HttpRequest& req) const
  {
    const auto& handlers = (req.method() == "GET" || req.method() == "HEAD")
                               ? getHandlers_
                               : postHandlers_;

    auto it = handlers.find(req.path());
    if (it != handlers.end())
      return it->second;

    return defaultHandler_;  // 兜底处理器
  }

 private:
  std::map<std::string, Handler> getHandlers_;
  std::map<std::string, Handler> postHandlers_;
  Handler defaultHandler_;
};

#endif  // HTTP_ROUTER_H
