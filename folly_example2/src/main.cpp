#include <fmt/format.h> // Include fmt library
#include <folly/FBString.h>
#include <folly/Uri.h>
#include <folly/executors/ThreadedExecutor.h>
#include <folly/futures/Future.h>
#include <iostream>
#include <utility>

using namespace folly;
using namespace std;

static void print_uri(const fbstring &address) {
  const Uri uri(address);
  const auto authority =
      fmt::format("The authority from {} is {}", uri.fbstr(), uri.authority());
  cout << authority << endl;
}

int main() {
  ThreadedExecutor executor;
  Promise<fbstring> promise;
  Future<fbstring> future = promise.getSemiFuture().via(&executor);
  Future<Unit> unit = std::move(future).thenValue(print_uri);
  promise.setValue("https://conan.io/");
  std::move(unit).get();
  return 0;
}
