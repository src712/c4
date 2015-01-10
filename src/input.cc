#include "input.h"

#include <cstring>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

namespace c4 {

input::input(const char* name) : name_(name) { 
  if(strcmp(name_, "-") == 0) {
    name_ = "<stdin>";
    // read from std::cin
    std::cin.unsetf(std::ios_base::skipws);
    std::copy(std::istream_iterator<char>(std::cin), std::istream_iterator<char>(),
              std::back_inserter(input_));
    std::cin.setf(std::ios_base::skipws);
    begin_ = input_.data();
    end_ = input_.data() + input_.size();
  } else {
    int fd = open(name, O_RDONLY);
    if(fd == -1)
      throw input_error("Could not open file.");

    struct stat sb;
    if(fstat(fd, &sb) == -1) {
      close(fd);
      throw input_error("Could not stat file.");
    }

    if(sb.st_size != 0) {
      begin_ = static_cast<const char*>
        (mmap(nullptr, sb.st_size,
              PROT_READ, MAP_PRIVATE,
              fd, 0));
      if(begin_ == MAP_FAILED) {
        close(fd);
        throw input_error("mmap failed.");
      }

      end_ = begin_ + sb.st_size;
    } else {
      begin_ = end_ = nullptr;
    }

    close(fd);
  }
}

input::~input() noexcept {
  if(strcmp(name_, "<stdin>") != 0 && std::distance(begin_, end_) != 0) {
    if(-1 == munmap(const_cast<char*>(begin_), std::distance(begin_, end_))) {
      std::abort();
    }
  }
}

}
