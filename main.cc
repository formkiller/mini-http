#include "http/HttpServer.h"
#include "handler/StaticFileHandler.h"

#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

int main(int argc, char* argv[])
{
  // 用法：./http-server [port] [threads] [docRoot]
  // 默认：port=8080, threads=4, docRoot=./www

  int port = 8080;
  int numThreads = 4;
  std::string docRoot = "./www";

  if (argc >= 2) port = std::atoi(argv[1]);
  if (argc >= 3) numThreads = std::atoi(argv[2]);
  if (argc >= 4) docRoot = argv[3];

  LOG_INFO << "========================================";
  LOG_INFO << "  MiniHTTP Server (based on muduo)";
  LOG_INFO << "  Port:      " << port;
  LOG_INFO << "  Threads:   " << numThreads;
  LOG_INFO << "  DocRoot:   " << docRoot;
  LOG_INFO << "========================================";

  EventLoop loop;
  InetAddress listenAddr(port);
  HttpServer server(&loop, listenAddr, "MiniHTTP");

  // 多线程模式
  server.setThreadNum(numThreads);

  // === 注册路由 ===

  // 静态文件处理器
  StaticFileHandler fileHandler(docRoot);

  // 默认路由：兜底所有请求，交给静态文件处理器
  server.router().get("/", [&fileHandler](const HttpRequest& req) {
    return fileHandler.handleRequest(req);
  });

  // 健康检查
  server.router().get("/health", [](const HttpRequest& req) {
    HttpResponse resp(200, "OK");
    resp.setHeader("Content-Type", "application/json");
    resp.setBody("{\"status\": \"ok\"}\n");
    return resp;
  });

  // 关于页面
  server.router().get("/about", [](const HttpRequest& req) {
    HttpResponse resp(200, "OK");
    resp.setHeader("Content-Type", "text/html; charset=utf-8");
    std::string body = "<html><body>";
    body += "<h1>MiniHTTP</h1>";
    body += "<p>A lightweight HTTP/1.1 server built from scratch</p>";
    body += "<ul>";
    body += "<li>Custom HTTP parser (state machine + Buffer::findCRLF)</li>";
    body += "<li>Static file serving with MIME type detection</li>";
    body += "<li>Keep-Alive support (HTTP/1.1 pipeline)</li>";
    body += "<li>Multi-threaded (EventLoopThreadPool)</li>";
    body += "<li>Security: directory traversal protection</li>";
    body += "</ul>";
    body += "</body></html>";
    resp.setBody(body);
    return resp;
  });

  server.start();

  printf("\n>>> MiniHTTP server listening on http://localhost:%d/\n", port);
  printf(">>> Pres Ctrl-C to stop.\n\n");

  loop.loop();
}
