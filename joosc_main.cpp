#include "joosc.h"
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char** argv) {
  const int ERROR = 42;

  if (argc < 2) {
    cerr << "usage: joosc <filename>..." << endl;
    return ERROR;
  }

  vector<string> files;
  for (int i = 1; i < argc; ++i) {
    files.emplace_back(argv[i]);
  }

  bool success = CompilerMain(CompilerStage::ALL, files, &cout, &cerr);
  int retcode = success ? 0 : ERROR;
  return retcode;
}
