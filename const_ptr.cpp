#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
  int var{55}, varToAssign{250};
  int *ptr = &var;

  int *const &refToPtr = ptr;
  // can not be reassigned
  // refToPtr = &varToAssign;
  cout << "refToPtr is " << *refToPtr << endl;

  int *&refToPtr2 = ptr;
  // without const, can be reassigned
  refToPtr2 = &varToAssign;
  cout << "refToPtr2 is " << *refToPtr2 << endl;

  return 0;
}

// g++ -std=c++20 const_ptr.cpp
// clang++ -std=c++20 const_ptr.cpp