#include <iostream>
using namespace std;

template <typename T> class PointerManager {
public:
  PointerManager() {
    member = new T {};
  }
  ~PointerManager() {
    cout << "Destructor invoked!" << endl;
    delete member;
  }

private:
  T *member;
};

int main(int argc, char *argv[]) {
  PointerManager<int> pointer1;
  return 0;
}

// g++ -std=c++11 ptr_mgr_basic.cpp