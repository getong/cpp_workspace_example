#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {

  void *p;

  double d = 10;
  p = &d;

  // Correct way to dereference a void pointer
  cout << "*p is " << *(static_cast<double *>(p)) << endl;
  return 0;
}

// g++ -std=c++20 ptr_to_void.cpp
// clang++ -std=c++20 ptr_to_void.cpp