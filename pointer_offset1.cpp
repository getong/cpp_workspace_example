#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
  int demoArray[]{1, 2, 3};
  int *ptr1{demoArray};

  cout << &demoArray[1] << endl;
  cout << (demoArray + 1) << endl;
  cout << (ptr1 + 1) << endl;

  cout << *&demoArray[1] << endl;
  cout << *(demoArray + 1) << endl;
  cout << *(ptr1 + 1) << endl;
  return 0;
}

// g++ -std=c++11 pointer_offset1.cpp -o a.out