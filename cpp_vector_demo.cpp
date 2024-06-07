#include <vector>
using std::vector;
#include <algorithm>
using std::count;
#include <iostream>
using namespace std;

int main() {
  vector<int> v{2, 7, 1, 6, 2, -2, 4, 0, 2};

  auto it = v.begin();
  auto element = *it;

  it++;
  element = *it;

  // count how many entries have the target value (2)
  int twos = 0;
  int const target = 2;
  for (size_t i = 0; i < v.size(); i++) {
    if (v[i] == target) {
      twos++;
    }
  }

  twos = count(v.begin(), v.end(), target);
  cout << "twos: " << twos << endl;
  twos = count(begin(v), end(v), target);
  cout << "twos: " << twos << endl;

  twos = std::ranges::count(v, target);
  cout << "twos: " << twos << endl;
}

// clang++ -std=c++20 cpp_vector_demo.cpp
// g++ -std=c++20 cpp_vector_demo.cpp