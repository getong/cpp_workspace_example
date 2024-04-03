#include <iostream>
using namespace std;

void pause() {
#ifdef _WIN32
  // Pause execution waiting for user input
  // For Windows
  system("pause");
#else
  // For Unix-like systems
  system("read -p 'Press enter to continue...'");
#endif
}

int main(int argc, char *argv[]) {
  cout << "hello world!" << endl;

  cout << endl << "--- End of Program ---" << endl;
  pause();
  return 0;
}

// g++ -std=c++11 system_pause.cpp -o a.out