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
    cout << "Entity Deconstructor Invoked. Member is: " << member << endl;
  };

  void toString() const { cout << member << endl; }

  void setMember(int memberP) { member = memberP; }

private:
  int member;
};

int main(int argc, char *argv[]) {
  const int ROWS = 2;
  const int COLUMNS = 3;

  Entity **ppTable = new Entity*[ROWS]{};

  for (int currentRow = 0; currentRow < ROWS; currentRow++) {
    ppTable[currentRow] = new Entity[COLUMNS];
  }

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      ppTable[i][j].setMember(10 + i + j);
    }
  }

  for (int i = 0; i < ROWS; i++) {
    delete []ppTable[i];
  }
  delete []ppTable;

  cout << endl << "--- End of Program ---" << endl;
  return 0;
}

// g++ -std=c++11 dyn_array_of_dyn_array_ptr.cpp