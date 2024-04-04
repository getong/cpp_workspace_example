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

  void toString() const { cout << member << endl; }

  void setMember(int memberP) { member = memberP; }

private:
  int member;
};

int main(int argc, char *argv[]) {
  const int ARRAY_SIZE = 4;

  Entity *ptrArray = new Entity[ARRAY_SIZE]{Entity(3), Entity(5), Entity(9)};

  for (int i = 0; i < ARRAY_SIZE; i++) {
    if (&ptrArray[i] != nullptr) {

      cout << "address is " << &ptrArray[i] << " point to address is "
           << " variable is ";
      ptrArray[i].toString();
    }
  }

  delete[] ptrArray;
  ptrArray = nullptr;

  cout << endl << "--- End of Program ---" << endl;
  return 0;
}

// g++ -std=c++11 dyn_array_of_ptr.cpp