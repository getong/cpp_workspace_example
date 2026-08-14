#include <iostream>

class Parent {
public:
  virtual ~Parent() = default;

  virtual void CallOut() { std::cout << "Parent CallOut()\n\n"; }
};

class Child : public Parent {
public:
  void CallOut() override { std::cout << "Child CallOut()\n\n"; }
};

class Grandchild : public Child {
public:
  void CallOut() override { std::cout << "Grandchild CallOut()\n\n"; }
};

int main() {
  Parent *p = new Parent;
  Parent *c = new Child;
  Parent *g = new Grandchild;
  Parent *ObjectArray[3];
  ObjectArray[0] = p;
  ObjectArray[1] = c;
  ObjectArray[2] = g;

  for (int i = 0; i < 3; i++) {
    std::cout << "i = " << i << std::endl;

    Parent *P = dynamic_cast<Parent *>(ObjectArray[i]);
    if (P) {
      P->CallOut();
    }

    Child *C = dynamic_cast<Child *>(ObjectArray[i]);
    if (C) {
      C->CallOut();
    }

    Grandchild *G = dynamic_cast<Grandchild *>(ObjectArray[i]);
    if (G) {
      G->CallOut();
    }
  }

  delete p;
  delete c;
  delete g;
  return 0;
}

// g++ -std=c++20 -Wall -Wextra -pedantic unreal_dynamic_cast.cpp
