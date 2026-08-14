#include <iostream>
#include <string>
class A {
public:
  A();
  A(int i_data, float f_data);
  void F(std::string message);
  void SetIntData(int i_data) { int_data = i_data; }
  void SetFloatData(float f_data) { float_data = f_data; }
  void PrintData();

private:
  int int_data;
  float float_data;
};

A::A() : A(0, 0.0f) {}

A::A(int i_data, float f_data)
    : int_data(i_data), float_data(f_data) {}

void A::F(std::string message) { std::cout << message << '\n'; }

void A::PrintData() {
  std::cout << "int_data: " << int_data << '\n'
            << "float_data: " << float_data << '\n';
}

int main() {
  std::cout << "Creating an A object:\n\n";
  A a;
  a.SetIntData(9);
  a.SetFloatData(2.18f);
  a.PrintData();
  std::cout << "\n\nCreating a pointer to an A object:\n\n";

  A *A_ptr = &a;
  std::cout << "Calling F from the pointer:\n";
  (*A_ptr).F("Call F.");
  std::cout << "\nCalling methods using arrow notation:\n\n";
  A_ptr->SetIntData(3);
  A_ptr->SetFloatData(4.99f);
  A_ptr->PrintData();
  std::cout << "Creating an A object dynamically:\n\n";

  A *a_dyn = new A(42, 9.587f);
  a_dyn->PrintData();
  delete a_dyn;
  std::cout << std::endl;
}

// g++ -std=c++20 -Wall -Wextra -Wpedantic unreal_pointer.cpp