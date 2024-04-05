#include <iostream>

// Template function that takes a pointer as an argument
template <typename T> void displayValue(T *ptr) {
  std::cout << "Value at the pointer: " << *ptr << std::endl;
}

int main() {
  // Using the template function with an integer pointer
  int intValue = 10;
  int *intPtr = &intValue;
  displayValue(intPtr);

  // Using the template function with a double pointer
  double doubleValue = 3.14;
  double *doublePtr = &doubleValue;
  displayValue(doublePtr);

  return 0;
}
