#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {
  int Value{15};

  int &rValue{Value};

  cout << "Value:  " << Value << endl;
  cout << "rValue: " << rValue << endl;

  cout << "Address Value:  " << &Value << endl;
  cout << "Address rValue: " << &rValue << endl << endl;
  rValue = 20;
  cout << "after changed" << endl;
  cout << "Value:  " << Value << endl;
  cout << "rValue: " << rValue << endl;

  cout << "Address Value:  " << &Value << endl;
  cout << "Address rValue: " << &rValue << endl << endl;

  int NewValue{70};
  rValue = NewValue;
  cout << "Value:  " << Value << endl;
  cout << "rValue: " << rValue << endl;
  cout << "NewValue: " << NewValue << endl;

  cout << "Address Value:  " << &Value << endl;
  cout << "Address rValue: " << &rValue << endl;
  cout << "Address NewValue: " << &NewValue << endl << endl;

  std::vector<double> Numbers{1.2, 2.3, 7.9, 4.5};
  for (double &Num : Numbers) {
    Num *= 2;
  }

  for (double Num : Numbers) {
    cout << Num << endl;
  }
  return 0;
}

// clang++ -std=c++20 reference_address.cpp