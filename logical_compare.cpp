#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
  int x = 50;

  if (x > 5 && x < 10) {
    cout << x << " is an integer between 5 and 10." << endl;
  } else {
    cout << x << " is not an integer between 5 and 10." << endl;
  }

  if (x > 5 and x < 10) {
    cout << x << " is an integer between 5 and 10." << endl;
  } else {
    cout << x << " is not an integer between 5 and 10." << endl;
  }

  // if (5 < x < 10) {
  //   cout << x << " is an integer between 5 and 10." << endl;
  // } else {
  //   cout << x << " is not an integer between 5 and 10." << endl;
  // }

  return 0;
}

// g++ -std=c++20 logical_compare.cpp
// clang++ -std=c++20 logical_compare.cpp