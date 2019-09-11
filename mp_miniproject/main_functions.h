#ifndef __MAIN_FUNCTIONS_H__
#define __MAIN_FUNCTIONS_H__
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
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

#include "shell.h"

std::string get_dollar_replacement(std::string & content, std::vector<shellvariable *> & sv) {
  std::size_t len = 0;
  std::string::iterator it = content.begin();
  // find the length of the substr form beginning to the first illegal character
  while (it != content.end()) {
    int c = *it;                      // get the int value of every character in this content
    if ((isalnum(c)) || (c == 95)) {  // check if it is letter,number or "_"(its value = 95)
      ++it;
      len++;  // if this charcter is legal, then length ++
    }
    else {
      break;  // find the first illegal character, break
    }
  }
  bool has_var = false;                      // set a flag to illustrate if this variable exists
  std::string var = content.substr(0, len);  // get the variable name by using "substr"
  std::string left("");                      // "left" is the content left behind the variable name
  //if the variable's length is not equal the the whole content's length, which means there is something characters the user want to attach the the value of this variable
  if (content.size() != len) {
    left = content.substr(
        len,
        content.size() -
            len);  // then get the left content by using substr form index "len" with a length of "size of this content" - "variable name length"
  }
  std::vector<shellvariable *>::iterator it_sv = sv.begin();
  while (it_sv != sv.end()) {                 // traversal the shell variable vetctor
    if ((*(*it_sv)).get_variable() == var) {  // find the variable which has the name we want
      has_var = true;                         // set the has_var flag
      var = (*(*it_sv)).get_value();          // replace "var" with its value
      break;
    }
    ++it_sv;
  }
  if (has_var == true) {  // if we find the variable we want
    var += left;          // attach the left content to its current value
  }
  else {
    var = "\0";  // if we cannot find the variable we want, ruturn a null string
  }
  return var;
}
bool check_format_set_and_dollar(std::string & between_set_dollar) {
  /* this function is used in find_dollar_replace() to check when when have both "set" and "$" in our function, it is legal or not. The format of content between set and dollar should be the combination of "spaces" + "characters" + "spaces" . Because "$" can only apperas in the value of the variable,like "set a $b"*/
  std::size_t checking_format = between_set_dollar.find_first_not_of(
      " ");  // set check_fomat fisrt to find the postion of characters after the fisrt space
  if (checking_format ==
      std::string::
          npos) {  // if cannot find, then the format is not illegal, print error message and then return false
    std::cout << "cannot have $ in the variable name" << std::endl;
    return false;
  }
  else {  // if we can find a charcter after the first space, then we have to make sure there are spaces behind the characters
    checking_format =
        between_set_dollar.find(" ", checking_format);  // try to find spaces behind the characters
    if (checking_format == std::string::npos) {         // cannot find, then the format is illegal
      std::cout << "cannot have $ in the variable name" << std::endl;
      return false;
    }
  }
  return true;
}
int find_dollar_replace(std::string & content, std::vector<shellvariable *> & sv) {
  // find three special cmmands, which require specific consideration when they have "$" in their command lines
  std::size_t found_set = content.find("set");
  std::size_t found_inc = content.find("inc");
  std::size_t found_export = content.find("export");
  bool has_dollar = false;  // set a flag to illustrate whether we can find a "$" or not
  std::size_t head =
      0;  // head and tail are head and tail index of the substrings we want. Initialize them to 0
  std::size_t tail = 0;
  do {
    tail = head;                      // update head to tail's position
    tail = content.find("$");         // use tail to find "$"
    if (tail != std::string::npos) {  // if tail can find a "$"
      has_dollar = true;              // set the flag properly
      // if we have "set", "inc" or "export" in our command, we need special consideration("$" is an illegal charcter, and should do exist in the variabls's name)
      if (found_set != std::string::npos || found_inc != std::string::npos ||
          found_export != std::string::npos) {
        // check the character after set is " " or not to make sure it is purely "set"
        if (content[found_set + 3] == ' ') {
          // if it is, then get the content between "set" and "$"
          std::string between_set_dollar = content.substr(found_set + 3, tail - 1 - found_set - 2);
          // check the format of "between_set_dollar" is legal or not
          if (check_format_set_and_dollar(between_set_dollar) == false) {
            return 0;
          }
        }
        // if we find inc or export in the content, we should directoly report a mistake. Because export and inc can only take variable names, they are not supposed to have "$"s in them
        if (content[found_inc + 3] == ' ' || content[found_export + 3] == ' ') {
          std::cout << "cannot have $ in the variable name" << std::endl;
          return 0;
        }
      }
      // if the content do not contain the three special commands
      head = tail;                         // fisrt update head to the postion of tail
      tail = content.find("$", tail + 1);  // search the next "$"
      if (tail != std::string::npos) {     // if found the second "$"
        std::string need_replace =
            content.substr(head + 1, tail - head - 1);  // get the content that need to be replaced
        std::string replacement =
            get_dollar_replacement(need_replace, sv);     // get the replacement of the content
        content.replace(head, tail - head, replacement);  // replace the content
      }
      else {  // if no second "$", then break out the loop
        break;
      }
    }
    else {  // if there's no "$" in the first place, break out
      break;
    }
  } while (tail != std::string::npos);  // do the things until cnanot find another "$"
  if (has_dollar ==
      true) {  // if there are dollar in the content. In this case,it means it has only one "$" in it
    std::string need_replace = content.substr(
        head + 1,
        content.size() - head -
            1);  // get the relate content that need to be replaced. Its size should be handled properly
    std::string replacement = get_dollar_replacement(need_replace, sv);  // get the repalcement
    content.replace(head, content.size() - head, replacement);           // replace it
  }
  return 1;  // success
}
bool are_all_numbers(std::string & content) {
  std::string ::iterator it = content.begin();
  //traversal every characters in content and see if they are numbers
  while (it != content.end()) {
    int c = *it;
    if (isdigit(c)) {
      ++it;
    }
    else {
      return false;  // find something that is not a number
    }
  }
  return true;  //everything are numbers
}
void new_inc_var(std::string & inc_varname,
                 std::vector<shellvariable *> & sv,
                 bool * need_find,
                 bool * found,
                 env_variable & ece551path) {
  //make a format "set varname 1"
  std::string format("set ");
  format += inc_varname;
  format += " 1";
  //use the format to new a shellvariable and push it into the sv vector
  shellvariable * newsvar = new shellvariable(format, ece551path, need_find, found);
  sv.push_back(newsvar);
}
void do_exist_inc(std::vector<shellvariable *>::iterator it) {
  //get the value of the existing variable
  std::string value = (*(*it)).get_value();
  //check if they are all numbers or not
  if (are_all_numbers(value)) {
    (*(*it)).inc_value();  // if they are numbers, then ++ it
  }
  else {  // if not, set the value to be "1"
    std::string nvalue("1");
    (*(*it)).set_newvalue(nvalue);
  }
}
void do_inc(input_inf & input,
            std::vector<shellvariable *> & sv,
            bool * need_find,
            bool * found,
            env_variable & ece551path) {
  // get the variable name we want to inc. It should be the second element in the input.argv vector
  std::string inc_varname((input.get_argv())[1]);
  std::vector<shellvariable *>::iterator it = sv.begin();
  //traverse the shell variable vetcotor to see if there is a existing variable has the same name with the variable we want to inc
  while (it != sv.end()) {
    if (inc_varname == (*(*it)).get_variable()) {
      do_exist_inc(it);  // when find an existing vairable, do_exist_inc()
      return;
    }
    it++;
  }
  new_inc_var(inc_varname,
              sv,
              need_find,
              found,
              ece551path);  // cannot find the variable in the existing list, the do new_inc_var()
}
std::string get_kvpair_key(std::string kvpair) {
  //find "=" and then get the content before it
  std::size_t equal = kvpair.find("=");
  return kvpair.substr(0, equal);
}
void envcheck_exist_erase(std::string & kv, std::vector<std::string> & k_v_pair) {
  std::vector<std::string>::iterator it = k_v_pair.begin();
  while (it != k_v_pair.end()) {
    //traverse the whole k_v_pair vector, see if their has one element has the key is the varible's name we want to export
    if (get_kvpair_key(kv) == get_kvpair_key(*it)) {  // if find one
      k_v_pair.erase(it);                             // erase the old element
      it = k_v_pair.begin();                          // push the new element into it
      continue;
    }
    ++it;
  }
}
void svacheck_exist_erase(shellvariable * newshellvar, std::vector<shellvariable *> & oldshellvar) {
  //this function works the same as  envcheck_exist_erase(std::string & kv, std::vector<std::string> & k_v_pair) fuction
  std::vector<shellvariable *>::iterator it = oldshellvar.begin();
  while (it != oldshellvar.end()) {
    if ((*newshellvar).get_variable() == (*(*it)).get_variable()) {
      delete *it;
      oldshellvar.erase(it);
      it = oldshellvar.begin();
      continue;
    }
    ++it;
  }
}
void export_env(char ** arg,
                std::vector<shellvariable *> & sv,
                std::vector<std::string> & k_v_pair) {
  // skip the fisrt arugument, which is "export". We are going to traverse every element in the arg array, and export them respectly into environment
  arg++;
  while (*arg != NULL) {  // if we haven't reached the end of the array
    for (std::vector<shellvariable *>::iterator it = sv.begin(); it != sv.end(); ++it) {
      //traverse the sv vector and try to find the variable which is the same as the arguments in export command
      if ((*it)->get_variable() ==
          "ECE551PATH") {  // check if the variable in the sv is ECE551PATH, then do not have to export
        continue;
      }
      if (*arg ==
          (*it)->get_variable()) {  // find the vairables that existing in the shell variable vector
        // get its key/value information
        std::string key = (*it)->get_variable();
        std::string value = (*it)->get_value();
        // make a new env_variable based on these information
        const char * env_name = key.c_str();
        char * env_content = const_cast<char *>(value.c_str());
        env_variable new_env(
            env_name, env_content);  // get the key and value and construct a env_variable object
        std::string k_v =
            new_env.get_key_value_pair();  // get the key_value pari from the env_variable object
        envcheck_exist_erase(k_v, k_v_pair);
        k_v_pair.push_back(k_v);  // store into the vecotr
      }
    }
    arg++;
  }
}
int do_redirect(input_inf & input, std::string & redirect_filename) {
  if (input.get_redirect_type() != 0) {  // it is a ">" or "2>"
    int fd = open(redirect_filename.c_str(),
                  O_RDWR | O_CREAT | O_TRUNC,
                  S_IRWXU);  // open the file. If it not exists, then it will creat it
    if (fd == -1) {          // if open failed
      std::cout << "cannot open " << redirect_filename << std::endl;
      return -1;
    }
    int replaced = dup2(fd, input.get_redirect_type());  // duplicate the file discriptors
    if (replaced == -1) {                                // if dup2 failed
      std::cout << "cannot replace " << redirect_filename << std::endl;
      return -1;
    }
  }
  else {                                                        // it is a "<"
    int fd = open(redirect_filename.c_str(), O_RDWR, S_IRWXU);  // opent the file
    if (fd == -1) {                                             // if open failed
      std::cout << "cannot open " << redirect_filename << std::endl;
      return -1;
    }
    int replaced = dup2(fd, 0);  // duplicate the file discriptors
    if (replaced == -1) {        // if dup2 failed
      std::cout << "cannot replace " << redirect_filename << std::endl;
      return -1;
    }
  }
  return 0;
}
void run_child(std::string pathname, char ** argv, char ** env) {  // run the child process
  int exe_erro;
  exe_erro = execve(pathname.c_str(), argv, env);  // use execve to run the child process
  if (exe_erro < 0) {                              // see if the evecve failed
    std::cerr << "Problematic program execution" << std::endl;
  }
}
void check_wstatus(int wstatus) {
  if (WIFEXITED(wstatus)) {  // if the program was not killed by some signal
    std::cout << "Program exited with status " << WEXITSTATUS(wstatus) << std::endl;
  }
  else if (WIFSIGNALED(wstatus)) {  // if it was killed by something
    std::cout << " Program was killed by signal " << WTERMSIG(wstatus) << std::endl;
  }
}
bool has_special_command(std::string & content) {
  //try to find these three commands
  std::size_t found_set = content.find("set");
  std::size_t found_inc = content.find("inc");
  std::size_t found_export = content.find("export");
  if (found_set != std::string::npos || found_inc != std::string::npos ||
      found_export != std::string::npos) {
    return true;  // if found any of these
  }
  return false;  // nothing was found
}
int determine_pipe(std::string & content, std::vector<std::string> & programs) {
  int program_number = 1;  // initialize the program number to
  // if it has special commands, They should see pipe "|" as normal arguments. For example the user can "set a |"
  if (has_special_command(content)) {
    programs.push_back(content);  // push the content into the vector fo programs
    return program_number;        // return immediately
  }
  std::size_t start = 0;  // set start and end to extract every seprate command lines out of pipe
  std::size_t end = content.find("|");
  if (content.size() == end + 1) {  // check if "|" is the last character in the command line
    std::cout << "you should put something behind |" << std::endl;
    return -1;
  }
  else if (content.find_first_not_of(" ", end + 1) ==
           std::string::npos) {  // check if "|   " is at the end of the command line
    std::cout << "you should put something behind |" << std::endl;
    return -1;
  }
  while (end != std::string::npos) {
    //traverse the command line and find all the "|" in it. Seperate all the subcommand lines into a vector "programs"
    program_number++;  // every time finds a "|", it means there are one more program
    std::string program_inf = content.substr(start, end - start);     // take the subcommand out
    if ((program_inf.find_first_not_of(" ")) == std::string::npos) {  // check if it is invalid
      std::cout << "invalid input in pipe" << std::endl;
      return -1;
    }
    programs.push_back(program_inf);  // push this subcommand into the vector
    start = end + 1;
    end = content.find("|", start);  // search the next "|"
  }
  std::string program_inf =
      content.substr(start, content.size() - start);  // take the last subcommand out
  programs.push_back(program_inf);
  return program_number;
}
void set_ece551path(env_variable & ece551path,
                    bool * need_find,
                    bool * found,
                    std::vector<shellvariable *> & sv) {
  //construct the proper command to set a variable
  std::string set_ece551 = "set ";
  set_ece551 += "ECE551PATH ";
  set_ece551 += ece551path.get_env_content();
  shellvariable * newsvar = new shellvariable(set_ece551,
                                              ece551path,
                                              need_find,
                                              found);  // put the input information into a class

  sv.push_back(newsvar);  // push it into the shell variable vector
}
void clean_before_continue(input_inf * newinput,
                           std::vector<std::string>::iterator * get_inp_inf,
                           int * program_number) {
  delete newinput;   // delete the new space
  (*get_inp_inf)++;  // ++ the iterator to get anoter program
  *program_number = *program_number - 1;
}
void clean_ForkStuff_before_continue(std::vector<input_inf *> & inps, std::vector<char **> & envs) {
  std::vector<input_inf *>::iterator it_inps = inps.begin();
  std::vector<char **>::iterator it_envs = envs.begin();
  while (it_inps != inps.end()) {
    delete *it_inps;
    ++it_inps;
  }
  while (it_envs != envs.end()) {
    delete[] * it_envs;
    ++it_envs;
  }
}
#endif
