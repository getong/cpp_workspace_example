#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
  int var{55};
  int *ptr = &var;

  // Pointer to Pointer -> ptrToPtr stores the address of ptr
  int **ptrToPtr = &ptr;

  // Reference to Pointer -> refToPtr is an Alias of ptr
  int *&refToPtr = ptr;

  // refToPtr can be add number, but is wrong
  // refToPtr += 5;
  // cout << "refToPtr is " << refToPtr << endl;

  *refToPtr += 5;
  cout << "refToPtr is " << *refToPtr << endl;
  cout << "var is " << var << endl;
  return 0;
}

// g++ -std=c++20 ptr_to_ref.cpp
// clang++ -std=c++20 ptr_to_ref.cpp
