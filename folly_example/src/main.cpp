#include <folly/FBString.h>
#include <iostream>

int main() {
  folly::fbstring str = "Hello, Folly!";
  std::cout << str << std::endl;
  return 0;
}
