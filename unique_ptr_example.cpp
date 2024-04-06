#include <iostream>
#include <memory>
using namespace std;

struct Entity {
  Entity(int num = 0) : member(num) { cout << "Constructor invoked!" << endl; }
  ~Entity() { cout << "Destructor invoked!" << endl; }
  int member{};
};

int main(int argc, char *argv[]) {

  unique_ptr<int> sPtr{new int{55}};
  cout << *sPtr << endl;
  *sPtr = 150;
  cout << *sPtr << endl;

  unique_ptr<Entity> entityPtr{new Entity{55}};
  cout << entityPtr->member << endl;
  entityPtr->member = 150;
  cout << entityPtr->member << endl;

  entityPtr.reset();
  if (entityPtr) {
    entityPtr->member = 150;
    cout << entityPtr->member << endl;
  }

  return 0;
}

// g++ -std=c++20 unique_ptr_example.cpp
// clang++ -std=c++20 unique_ptr_example.cpp