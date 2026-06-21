#include "http/HttpServer.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpConnection.h"

#include <cstdio>

using namespace muduo;
using namespace muduo::net;

// ========== 构造函数 ==========
HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name)
  : server_(loop, listenAddr, name)
{
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

// ========== 连接建立 / 断开 ==========
void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    // 新连接——为它创建一个 HttpRequest 对象，挂在 context 上
    conn->setContext(HttpRequest());
    LOG_INFO << "HTTP connection UP from " << conn->peerAddress().toIpPort();
  }
  else
  {
    LOG_INFO << "HTTP connection DOWN from " << conn->peerAddress().toIpPort();
  }
}

// ========== 收到数据——核心处理流程 ==========
void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
  // ① 拿到这条连接的 HttpRequest 对象（挂在 context 上）
  HttpRequest* req = boost::any_cast<HttpRequest>(conn->getMutableContext());

  if (!req)
  {
    sendError(conn, 500, "Internal Server Error");
    conn->shutdown();
    return;
  }

  // ② 不停地尝试解析——Buffer 里可能一次来了多个请求（pipeline）
  while (req->parse(buf))
  {
    // ③ 解析完成一个完整请求 → 查路由表
    Router::Handler handler = router_.findHandler(*req);
    HttpResponse response(200, "OK");

    if (handler)
    {
      // 有匹配的路由 → 调用处理函数
      try
      {
        response = handler(*req);
      }
      catch (...)
      {
        response = HttpResponse(500, "Internal Server Error");
        response.setCloseConnection(true);
      }
    }
    else
    {
      // 404
      response = HttpResponse(404, "Not Found");
      std::string body = "<html><body><h1>404 Not Found</h1>";
      body += "<p>Path: " + req->path() + "</p></body></html>";
      response.setHeader("Content-Type", "text/html");
      response.setBody(body);
      response.setCloseConnection(true);
    }

    // ④ 判断是否需要关闭连接（HTTP/1.0 或 Connection: close）
    if (shouldCloseConnection(*req))
    {
      response.setCloseConnection(true);
    }

    // ⑤ HEAD 方法不返回 body
    if (req->method() == "HEAD")
    {
      response.setBody("");
    }

    // ⑥ 发送响应
    sendResponse(conn, response);

    // ⑦ 如果需要关闭 → shutdown
    if (response.closeConnection())
    {
      conn->shutdown();
      return;  // 不再解析后续请求
    }

    // ⑧ Keep-Alive：重置 request 对象，准备解析下一个请求
    req->reset();
  }
}

// ========== 发送响应（构造 HTTP 报文 → 写入 Buffer → 发送） ==========
void HttpServer::sendResponse(const TcpConnectionPtr& conn,
                              const HttpResponse& resp)
{
  Buffer buf;
  resp.appendToBuffer(&buf);
  conn->send(&buf);
}

// ========== 发送错误响应 ==========
void HttpServer::sendError(const TcpConnectionPtr& conn,
                           int statusCode,
                           const std::string& message)
{
  HttpResponse resp(statusCode, message);
  resp.setHeader("Content-Type", "text/plain");
  resp.setBody(message + "\n");
  resp.setCloseConnection(true);
  sendResponse(conn, resp);
}

// ========== 判断是否需要关闭连接 ==========
bool HttpServer::shouldCloseConnection(const HttpRequest& req) const
{
  // HTTP/1.0 默认关闭
  if (req.version() == "HTTP/1.0")
    return true;

  // 检查 Connection 头
  std::string connection = req.getHeader("Connection");
  if (!connection.empty())
  {
    // 大小写不敏感比较
    std::string lower;
    for (char c : connection) lower += std::tolower(c);
    if (lower.find("close") != std::string::npos)
      return true;
    if (lower.find("keep-alive") != std::string::npos)
      return false;
  }

  // HTTP/1.1 默认 Keep-Alive
  return false;
}
