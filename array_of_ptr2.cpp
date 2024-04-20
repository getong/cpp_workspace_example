#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  int Arr[5]{1, 2, 3, 4, 5};
  int *ArrPtr{Arr};

  cout << "the third element is " << Arr[2] << endl;
  cout << "the third element by pointer is " << *(ArrPtr + 2) << endl;
  cout << "the third element address is " << &Arr[2] << endl;
  cout << "the third element by pointer address is " << ArrPtr + 2 << endl;

  int *Array[3];
  int Num1{15};
  int Num2{25};
  int Num3{34};

  Array[0] = &Num1;
  Array[1] = &Num2;
  Array[2] = &Num3;

  // Address
  cout << "Addresses: " << endl;
  cout << Array[0] << endl;
  cout << Array[1] << endl;
  cout << Array[2] << endl << endl;

  // Values
  cout << "Values: " << endl;
  cout << *Array[0] << endl;
  cout << *Array[1] << endl;
  cout << *Array[2] << endl << endl;

  return 0;
}

// clang++ -std=c++20 array_of_ptr2.cpp