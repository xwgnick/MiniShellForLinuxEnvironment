#ifndef __HEAD_FUNCTIONS_H__
#define __HEAD_FUNCTIONS_H__
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string clear_escape(std::string & content) {
  std::size_t pos = 0;  // erase the "\"s in the content

  do {
    pos = content.find("\\", pos);   // find the position of "\"
    if (pos == std::string::npos) {  // if cannot find, break
      break;
    }
    if (content[pos + 1] == ' ') {  // check if it is a space after "\"
      content.erase(pos, 1);        // if it is, erase the "\"
    }
    pos++;
  } while (pos != std::string::npos);  // keep erasing the "\"s until reach the end of content
  return content;
}
#endif
