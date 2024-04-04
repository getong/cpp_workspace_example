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

  Entity *ptrArray[ARRAY_SIZE]{};
  ptrArray[0] = new Entity[3]{Entity(3), Entity(5), Entity(9)};
  ptrArray[1] = new Entity[3]{};
  ptrArray[1][2].setMember(55);

  for (int i = 0; i < ARRAY_SIZE; i++) {
    delete[] ptrArray[i];
    ptrArray[i] = nullptr;
  }

  cout << endl << "--- End of Program ---" << endl;
  return 0;
}

// g++ -std=c++11 dyn_array_of_array_ptr.cpp