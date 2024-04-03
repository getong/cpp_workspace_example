#include <cstdlib>
#include <ctime>
#include <iostream>

using namespace std;

int main() {
  int randomNum{};

  srand(time(nullptr));
  for (int i = 0; i < 3; i++) {
    randomNum = rand();
    cout << randomNum << endl;
  }

  return 0;
}
// g++ -std=c++11 random_num.cpp -o a.out