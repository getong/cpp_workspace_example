#include <iostream>
using namespace std;

class Entity {
public:
  //! Copy constructor
  Entity(int memberP = 0) : member(memberP) {
    cout << "Entity Constructor Invoked. Member is: " << member << endl;
  }

  //! Destructor
  ~Entity() {
    cout << "Entity Constructor Invoked. Member is: " << member << endl;
  };

  void info() { toString(); }

  void toString() const { cout << member << endl; }

  void setMember(int memberP) { member = memberP; }

private:
  int member;
};

int main(int argc, char *argv[]) {
  const int ARRAY_SIZE = 3;

  int x{3}, y{5}, z{9};

  cout << "x pointer address is " << &x << endl;
  cout << "y pointer address is " << &y << endl;
  cout << "z pointer address is " << &z << endl;
  int *ptrArray[ARRAY_SIZE]{&x, &y, &z};

  for (int i = 0; i < ARRAY_SIZE; i++) {
    cout << "address is " << &ptrArray[i] << " point to address is "
         << ptrArray[i] << " variable is " << *ptrArray[i] << endl;
    ;
  }

  Entity entity1, entity2{5};
  Entity *ptrArray2[ARRAY_SIZE]{&entity1, &entity2};

  for (int i = 0; i < ARRAY_SIZE; i++) {
    if (ptrArray2[i] != nullptr) {
      cout << "address is " << &ptrArray2[i] << " point to address is "
           << ptrArray2[i] << " variable is ";
      ptrArray2[i]->info();
    }
  }

  cout << endl << "--- End of Program ---" << endl;
  return 0;
}

// g++ -std=c++11 array_of_ptr.cpp