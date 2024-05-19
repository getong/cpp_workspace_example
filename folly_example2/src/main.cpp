#include <fmt/format.h> // Include fmt library
#include <folly/FBString.h>
#include <folly/Uri.h>
#include <folly/executors/ThreadedExecutor.h>
#include <folly/futures/Future.h>
#include <iostream>
#include <utility>

static void print_uri(const folly::fbstring &address) {
  const folly::Uri uri(address);
  const auto authority =
      fmt::format("The authority from {} is {}", uri.fbstr(), uri.authority());
  std::cout << authority << std::endl;
}

int main() {
  folly::ThreadedExecutor executor;
  folly::Promise<folly::fbstring> promise;
  folly::Future<folly::fbstring> future =
      promise.getSemiFuture().via(&executor);
  folly::Future<folly::Unit> unit = std::move(future).thenValue(print_uri);
  promise.setValue("https://conan.io/");
  std::move(unit).get();
  return 0;
}
