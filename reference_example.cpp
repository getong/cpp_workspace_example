#include <iostream>

using namespace std;

void Function(int &Ref) {
  cout << "Value of Ref inside the function before changin: " << Ref << endl;
  Ref = 180;
  cout << "Address of Ref inside the function: " << &Ref << endl;
  cout << "value of Ref inside the function: " << Ref << endl << endl;
}

int main(int argc, char *argv[]) {
  int NewVariable(120);
  cout << "Value of NewVariable before calling: " << NewVariable << endl;
  cout << "Address of NewVariable befor calling: " << &NewVariable << endl;

  Function(NewVariable);

  cout << "Value of NewVariable after calling: " << NewVariable << endl;
  cout << "Address of NewVariable after calling: " << &NewVariable << endl;
  return 0;
}

// clang++ -std=c++20 reference_example.cpp