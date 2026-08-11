#include "designated_init.hpp"

#include <iostream>
#include <string>

namespace modules::designated_init
{

namespace
{

constexpr int default_port = 8080;
constexpr int default_timeout_ms = 3000;

// 指定初始化器只对“聚合体”生效：没有用户声明的构造函数、
// 没有私有非静态成员、没有虚函数的类型。
struct server_config
{
  std::string host = "localhost";
  int port = default_port;
  bool use_tls = false;
  int timeout_ms = default_timeout_ms;
};

void print(const server_config& config)
{
  std::cout << (config.use_tls ? "https://" : "http://") << config.host << ':'
            << config.port << " (timeout " << config.timeout_ms << "ms)\n";
}

}  // namespace

void run()
{
  // 全部使用默认值。
  print(server_config {});

  // 只覆盖关心的字段，其余保持默认；成员名即文档，可读性远好于位置参数。
  // 注意：必须按成员声明顺序书写（可以跳过，但不能乱序）。
  print(server_config {.host = "example.com", .use_tls = true});

  print(server_config {.port = 9090, .timeout_ms = 500});
}

}  // namespace modules::designated_init
