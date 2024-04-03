#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
  // reference
  int x{5};
  int &demoRef = x;
  cout << "demoRef is " << demoRef << endl;

  demoRef = 15;
  cout << "demoRef is " << demoRef << endl;
  cout << "demoRef address is " << &demoRef << endl;

  int y{6};
  int *ptr = &y;
  cout << "ptr is " << *ptr << endl;
  *ptr = 15;

  cout << "ptr is " << *ptr << endl;
  cout << "ptr address is " << &ptr << endl;

  return 0;
}

// g++ -std=c++11 reference_and_pointer.cpp