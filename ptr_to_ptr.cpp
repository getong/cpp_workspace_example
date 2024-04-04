#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
  int var = 55;
  int *ptr = &var;

  cout << "Variable..." << endl;
  cout << &var << '\t' << var << endl;
  *ptr -= 5;

  cout << "\nPointer to Variable..." << endl;
  cout << &ptr << '\t' << ptr << '\t' << *ptr << endl;

  cout << endl << "Pointer to Pointer ..." << endl;
  int **ptrToPtr = &ptr;
  **ptrToPtr -= 5;
  cout << &ptrToPtr << '\t' << ptrToPtr << '\t' << *ptrToPtr << " variable is "
       << **ptrToPtr << endl;

  cout << endl << "Pointer to Pointer to Pointer..." << endl;
  int ***ptrToPtrToPtr = &ptrToPtr;
  ***ptrToPtrToPtr -= 5;
  cout << &ptrToPtrToPtr << '\t' << ptrToPtrToPtr << '\t' << *ptrToPtrToPtr
       << '\t' << **ptrToPtrToPtr << " variable is " << ***ptrToPtrToPtr
       << endl;

  cout << endl << "--- End of Program ---" << endl;

  return 0;
}

// g++ -std=c++11 ptr_to_ptr.cpp