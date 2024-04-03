/* boost_lexical_cast_example.cpp  */
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
using namespace std;

int main(void) {
  try {
    string str;
    cout << "Please input first number: ";
    cin >> str;
    int n1 = boost::lexical_cast<int>(str);
    cout << "Please input second number: ";
    cin >> str;

    int n2 = boost::lexical_cast<int>(str);
    cout << "The sum of the two numbers is ";
    cout << n1 + n2 << "\n";
    }
  catch (const boost::bad_lexical_cast &e) {
    cerr << e.what() << "\n";
    return 1;
  }
}

// g++ -Wall -std=c++11 -lboost_system boost_lexical_cast_example.cpp -o a.out