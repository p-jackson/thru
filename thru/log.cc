#include "log.h"

#include <iostream>

namespace thru {
  std::wostream& log() {
    return std::wcout;
  }
}