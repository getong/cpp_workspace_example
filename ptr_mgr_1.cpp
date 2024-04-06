#include <iostream>
using namespace std;

struct Entity {
  ~Entity() { cout << "Destructor invoked!" << endl; }
  int member{};
};

template <typename T> class PointerManager {
public:
  explicit PointerManager(T *ptrP = nullptr) { mPtr = ptrP; }

  // copy constructor
  PointerManager(const PointerManager &) = delete;

  ~PointerManager() { delete mPtr; }

  // deleted assignment operator
  PointerManager &operator=(const PointerManager &) = delete;

  // Overload dereference operator
  T &operator*() { return *mPtr; }
  // Overload arrow operator
  T *operator->() { return mPtr; }

private:
  T *mPtr;
};

int main(int argc, char *argv[]) {
  PointerManager<Entity> ptr1{new Entity{}};
  cout << "ptr1 is " << ptr1->member << endl;

  PointerManager<Entity> ptr2;
  // ptr2 = ptr1;
  // cout << "ptr2 is " << ptr2->member << endl;
  return 0;
}

// g++ -std=c++20 ptr_mgr_1.cpp
// clang++ -std=c++20 ptr_mgr_1.cpp