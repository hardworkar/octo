#include <iostream>

#include "application.h"
#include "globals.h"

int main() {
  std::cout << "hello from octo version: " << OCTO_VERSION_MAJOR << "."
            << OCTO_VERSION_MINOR << "\n";
  Application app;
  app.run();
}
