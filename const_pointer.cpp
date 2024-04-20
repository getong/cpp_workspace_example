#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  int Num{5};
  int Num2{9};

  const int *NumPtr{&Num};

  cout << NumPtr << endl;
  // *NumPtr = 15;
  NumPtr = &Num2;

  cout << NumPtr << endl;

  int Number1{14};
  int Number2{11};

  int *const NumberPtr{&Number1};
  cout << *NumberPtr << endl;
  *NumberPtr = 20;
  cout << *NumberPtr << endl;
  cout << Number1 << endl;
  // NumberPtr = &Number2;

  int Number3{50};
  int Number4{19};

  const int *const NumberPtr1{&Number3};

  cout << NumberPtr1 << endl;
  cout << *NumberPtr1 << endl << endl;
  // NumberPtr1 = Number4;
  // *NumberPtr = 31；
  return 0;
}

// clang++ -std=c++20 const_pointer.cpp