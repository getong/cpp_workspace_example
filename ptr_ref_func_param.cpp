#include <iostream>
using namespace std;

// passing a pointer by value, change its address not work
void modifyPointer(int *ptrP, int *addressToAssign) { ptrP = addressToAssign; }

// passing a pointer to a pointer, change its address works
void modifyPointer1(int **ptrP, int *addressToAssign) {
  *ptrP = addressToAssign;
}

// passing to a reference to a pointer, change its address works
void modifyPointer2(int *&ptrP, int *addressToAssign) {
  ptrP = addressToAssign;
}

// change it own value, but not change its address
void modifyPointer3(int *ptrP, int *addressToAssign) { ptrP = addressToAssign; }

int main(int argc, char *argv[]) {
  int var{55}, varToAssign{250};

  int *ptr = &var;

  cout << "         Before passing:\t " << *ptr << " var is " << var
       << " varToAssign is " << varToAssign << endl;

  modifyPointer(ptr, &varToAssign);

  cout << "unchange, After passing:\t " << *ptr << " var is " << var
       << " varToAssign is " << varToAssign << endl;

  modifyPointer1(&ptr, &varToAssign);
  cout << "  change, After passing:\t " << *ptr << " var is " << var
       << " varToAssign is " << varToAssign << endl;

  modifyPointer2(ptr, &varToAssign);
  cout << "  change, After passing:\t " << *ptr << " var is " << var
       << " varToAssign is " << varToAssign << endl;

  modifyPointer3(ptr, &varToAssign);
  cout << "  change, After passing:\t " << *ptr << " var is " << var
       << " varToAssign is " << varToAssign << endl;

  cout << "\n --- End of Program ---" << endl;
  return 0;
}

// g++ -std=c++11 ptr_ref_func_param.cpp