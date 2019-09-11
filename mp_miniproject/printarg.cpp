#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
int main(int argc, char ** argv) {
  /*  if (argc < 2) {
    std::string stdinput;

    getline(std::cin, stdinput);
    std::cout << "stdin is " << stdinput << std::endl;
    }*/

  for (int i = 1; i <= argc - 1; i++) {
    std::cout << "argv[" << i << "] is " << argv[i] << std::endl;
    std::string str1(argv[i]);
    std::string str2 = str1;
    std::cout << str2 << std::endl;
  }
  std::string stdinput;
  getline(std::cin, stdinput);
  std::cout << "stdin is " << stdinput << std::endl;
}
