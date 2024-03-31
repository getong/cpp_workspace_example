#include<iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

int main() {
  int randomNum{};

  for (int i = 0; i < 3; i++ ){
    randomNum = rand();
    cout << randomNum << endl;
  }

  return 0;
}
// g++ -std=c++11 random_num.cpp -o random_num