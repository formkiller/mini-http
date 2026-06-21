#ifndef HTTP_HTTPSERVER_H
#define HTTP_HTTPSERVER_H

#include "muduo/net/TcpServer.h"
#include "http/Router.h"

#include <functional>
#include <string>

namespace muduo
{
namespace net
{
class EventLoop;
class InetAddress;
}
}

// HTTP 服务器主框架
//
// 职责：
//   1. 持有 TcpServer（接收连接）
//   2. 持有 Router（路由表）
//   3. 为每条连接维护一个 HttpRequest 对象
//   4. 在 onMessage 回调中：解析 → 路由 → 构建响应 → 发送
//
// 用法：
//   EventLoop loop;
//   InetAddress addr(8080);
//   HttpServer server(&loop, addr, "MiniHTTP");
//   server.router().get("/", handler);
//   server.setThreadNum(4);  // 多线程
//   server.start();
//   loop.loop();

class HttpServer
{
 public:
  HttpServer(muduo::net::EventLoop* loop,
             const muduo::net::InetAddress& listenAddr,
             const std::string& name);

  // 设置 IO 线程数（0 = 单线程）
  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  // 启动服务器
  void start()
  {
    server_.start();
  }

  // 获取路由器引用，用于注册路由
  // 例：server.router().get("/", handler);
  Router& router() { return router_; }

 private:
  // TcpServer 的回调
  void onConnection(const muduo::net::TcpConnectionPtr& conn);
  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp receiveTime);

  // 根据请求方式决定是否关闭连接
  bool shouldCloseConnection(const HttpRequest& req) const;

  // 向客户端发送 HTTP 响应
  void sendResponse(const muduo::net::TcpConnectionPtr& conn,
                    const HttpResponse& resp);

  // 发送错误响应
  void sendError(const muduo::net::TcpConnectionPtr& conn,
                 int statusCode, const std::string& message);

  muduo::net::TcpServer server_;
  Router router_;
};

#endif  // HTTP_HTTPSERVER_H
