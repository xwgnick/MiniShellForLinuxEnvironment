
#ifndef __SHELL_H__
#define __SHELL_H__
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

#include "head_functions.h"
std::string clear_escape(std::string & content);  // this function is used to clear escapes
class env_variable                                // make a class for environment variables
{
  //make field for variables' name and content. Choose type that can be used in execve function. Also a field for its key_value pair
  const char * env_name;
  char * env_content;
  std::string key_value_pair;

 public:
  env_variable() : env_name(NULL), env_content(NULL) {}
  env_variable(const char * n) :
      env_name(
          n) {  //given the name of the environment variable, the constructor should fill its content in it
    env_content = getenv(env_name);
    if (env_content == NULL) {  // if cannot get env_content from a environment variable
      throw std::invalid_argument("cannot set new environment variable!");
    }
    compute_key_value_pair();  // set the key_value_pair field
  }
  env_variable(const char * env_n, char * env_c) : env_name(env_n), env_content(env_c) {
    // given the name and content of a environment variable, this constructor should set a new environment variable
    compute_key_value_pair();
  }
  const char * get_env_name() { return env_name; }
  char * get_env_content() { return env_content; }
  void
  compute_key_value_pair() {  // get the key_value pair of the environment variable for the third argument of execve function
    //use string stream to append strings together
    std::stringstream ss;
    ss << env_name << "=" << env_content;
    key_value_pair = ss.str();
  }
  std::string & get_key_value_pair() { return key_value_pair; }
  void change_env_content(
      std::string &
          new_content) {  // this function can be used to change the env_variables' content
    env_content = const_cast<char *>(new_content.c_str());
    compute_key_value_pair();  // update the field
  }
};
class input_inf  // a class for input information
{  // make fields include the input information and the environment variable should be used
 protected:
  std::string inp_inf;  // inp_inf is containning the input information
  env_variable env;     // the environment variable the programs will used
  bool *
      p_need_find;  // these two are the address of two bool variables to instruct whether the command name need to be found in the ece551path and whether it is found or not
  bool * p_found;
  std::string * p_command;
  std::string command;  // this field is used to contain the command name
  char ** argv;         // this field is used to contain the arguments the child process needs
  std::vector<std::string>
      parsed_inp;  // this vector contains the parsed format of the input information. Its first element is the command name, and its second to the last elements are the arguments that the child process needs
  std::string redirect_filename;  // this sting contains the redirection filename
  int redirect_type;              // redirect_type:0 for stdin; 1 for stdout, 2 for stderr

 public:
  void set_command_argv()
  /*This function's goal is to put the parsed content into proper fields, including command and argv*/
  {
    std::vector<std::string>::iterator it = parsed_inp.begin();
    p_command = &(*it);  //command name should be the first element of the vector
    command = *p_command;
    argv =
        new char *[parsed_inp.size() +
                   1];  //new spaces for argv, the length shuold be vector's length + 1 because there is a null at the end.
    size_t i = 0;
    while (it != parsed_inp.end()) {  // put all the arguments into the argv field
      char * arg_content = const_cast<char *>((*it).c_str());
      argv[i] = arg_content;
      i++;
      ++it;
    }
    argv[parsed_inp.size()] = NULL;  // put null at the end of argv
  }
  void parse_inp() {
    /*
      This function is to parse the input information. The input information from the user will be parsed into a vector, where the fisrt element is the proper command name, and the second to the last elements are the arguments that the child process will use. Then, the fisrt element will be put into the "command" field, the second to the last element will be put into a "argv" field with the last element setted to be NULL. This will be passed to execve function in the main function.
     */
    // set a flag to see is there a "\" in the input information, which will lead to special parse rules
    bool need_escape = false;
    // set a flag to see if there are more than one space(" ") behind the "\"
    bool space_after_escape = false;
    // the head character's index of the special arguments(those who has "\" in them)
    std::size_t special_arg_head = 0;
    std::size_t head = 0;   // the head character's index of the part of content we need
    std::size_t tail = 0;   // the tail character's index of the part of content we need
    std::size_t len = 0;    // length of the content we need
    std::size_t start = 0;  // set a start index to indicate where should start find
    do {
      if (space_after_escape ==
          false) {  // if the character behind "\" is a charcter, then should continue finding
        head = inp_inf.find_first_not_of(
            " ", start);  // find the fisrt character in the inp_inf which is not " "
        if (head ==
            std::string::
                npos)  // this case is prepared for the arguments with " " at the end of them
        {
          if (need_escape == true &&
              space_after_escape == false)  //when "\ " is the last element in the arguments
          {
            len = inp_inf.size() -
                  special_arg_head;  // compute the proper length of the substring we want
            std::string result = inp_inf.substr(special_arg_head, len);  // take the substring out
            result = clear_escape(result);  // clear esacpe symbols in result
            parsed_inp.push_back(result);   // push this content into the vector
            space_after_escape = false;     // set flags back to false
            need_escape = false;
          }
          break;
        }
        start = head;  // move start to this new postition to find things behind
        tail = inp_inf.find(
            " ",
            start);  //find the first character which is a " ", and set tail to that position. The content between head and tail is the content we need
        start = tail;
        if (need_escape == false) {         // if there's no "\"s in the content
          if (tail != std::string::npos) {  // still not reach the end of the input information
            len = tail - head;              // get the len of the content we want
          }
          else {
            // if we are at the end of the input information,
            // the length of the content we want should be like this
            len = inp_inf.size() - head;
          }
        }
        else {  // if there are "\"s in the content
          if (tail != std::string::npos) {
            len = tail - special_arg_head;  // compute the length of this special content
          }
          else {
            len = inp_inf.size() - special_arg_head;
          }
        }
        if (tail != std::string::npos) {    // not at the end of the input information
          if (inp_inf[tail - 1] == '\\') {  // if the character before tail is a "\"
            if (need_escape ==
                false) {  // if the flag is not set to be ture, then set it, and move the special_arg_head to be the index of this special content's head
              special_arg_head = head;
              need_escape = true;
            }
            if (inp_inf[tail + 1] ==
                ' ') {  // see if there's more than one " "s behind "\", then set the proper flag and compute the proper length
              space_after_escape = true;
              len = tail - special_arg_head + 1;
            }

            continue;
          }
        }
      }
      if (need_escape ==
          false) {  // if the content doesn't has "\" in it, then use substring function to put it into the vector
        std::string result = inp_inf.substr(head, len);
        parsed_inp.push_back(result);
      }
      else {  // if the content has "\" in it, use "special_arg_head" as the begining of the substring
        std::string result = inp_inf.substr(special_arg_head, len);
        result = clear_escape(result);  // erase escape symbols from result
        parsed_inp.push_back(result);   // push this content into the vector
        space_after_escape = false;     // set flags back to false
        need_escape = false;
      }

    } while (
        tail !=
        std::string::npos);  // keep doing parsing until reaching the end of the input information
    do_redirect();           // do the redirection work
    set_command_argv();      // set the command and argv fields properly
  }

  input_inf(std::string i, env_variable & e, bool * pnf, bool * f) :
      inp_inf(i),
      env(e),
      p_need_find(pnf),
      p_found(f),
      p_command(NULL),
      redirect_filename("no"),
      redirect_type(0) {
    parse_inp();  // parse the input inforamation and construct all the left fields
  }               // constructor

  ~input_inf() { delete[] argv; }  // destructor

  bool hasslash() {  // see if there is a slash in the input information
    std::size_t found = command.find("/");
    if (found == std::string::npos) {
      return true;  // return true if do not has slash
    }
    else {
      return false;  // return false if it has slash
    }
  }
  std::vector<std::string> split_input(
      std::string
          s) {  // this function is to split the environment variable by its ":"s. Put the splited result into a vector
    std::vector<std::string> splited;  // initialize a vector to store result
    std::size_t start_ind = 0;  // initialize two index to illustrate which sustring we want to take
    std::size_t goal_ind = 0;
    do {
      goal_ind = s.find(":", start_ind);    // set goal_ind to where ":" is
      if (goal_ind == std::string::npos) {  // if goal comes to the end of the string
        //compute the proper length and take the proper substring
        int len = s.size() - start_ind;
        std ::string b = s.substr(start_ind, len);
        splited.push_back(b);  // push the result into the string
        break;
      }
      // if goal is not at the end
      //compute the proper length and take the proper substring
      int len = goal_ind - start_ind;
      std ::string b = s.substr(start_ind, len);
      start_ind = goal_ind + 1;  // set the start behind goal
      splited.push_back(b);
    } while (goal_ind !=
             std::string::npos);  // keep finding until goal reaching the end of the line
    return splited;
  }

  bool inputhasslash() {  // see if there is a slash in the input information
    std::size_t found = inp_inf.find("/");
    if (found == std::string::npos) {
      return true;  // if no "/", return true
    }
    else {
      return false;
    }
  }

  std::string & pathname() {  // make a suitable pathname to use
    command = *p_command;
    if ((command != "") && (command != "cd") &&
        hasslash()) {  // if the input is "" or "cd" or has slashes in it, it should directly return the input information, otherwise, do the rest of this function
      *p_need_find = true;
      std::string env_content_string(env.get_env_content());
      std::vector<std::string> splited = split_input(
          env_content_string);  // split the environement variable's content by ":"s and put them into a vector. The result will be used as directories for search.
      for (std::vector<std::string>::iterator it = splited.begin(); it != splited.end();
           ++it) {  // traveser the vecotr, search every directory in it.
        DIR * dir;
        struct dirent * entry;
        const char * dir_name = (*it).c_str();
        //open dirctory
        dir = opendir(dir_name);
        //check if nothing wrong with dirctory name
        if (!dir) {
          throw std::invalid_argument("Directory was not found!");
        }
        else {
          //read directory
          while ((entry = readdir(dir)) != NULL) {  //search every file in this directory
            if (entry->d_name ==
                command) {  // if found something has the same name with the input, then return a newly constructed pathname to the main function
              *p_found = true;
              command =
                  std::string(dir_name) + "/" +
                  std::string(
                      entry
                          ->d_name);  // put the directories' name and a "/" before the input information to make a new pathname, and store it into the field of "command".
            }
          }
        }
        closedir(dir);
      }
    }
    return command;  // return the suitable command name
  }
  bool check_before_exe() {
    //this function is used to consider the case when the command has a / in it but is not a valid directory or program
    //find the last "/" in the input information, and then extract the pure command out of it
    std::size_t found = command.find_last_of("/");
    std::string pure_command = command.substr(found + 1, inp_inf.size() - found);
    std ::string dir_name_string = command.substr(
        0, found + 1);  // get the directory which should be all the things before the pure command
    const char * dir_name = dir_name_string.c_str();
    DIR * dir;
    struct dirent * entry;
    dir = opendir(dir_name);  // open directory
    if (!dir) {               // check if openning fails
      throw std::invalid_argument("Directory was not found!");
    }
    else {
      //read directory
      while ((entry = readdir(dir)) != NULL) {  //search every file in this directory
        if (entry->d_name ==
            pure_command) {  // if find the command, then close the directory and return true
          closedir(dir);
          return true;
        }
      }
    }
    // if cannot find the command
    closedir(dir);
    std::cout << "-bash: " << command << ": No such file or directory"
              << std::endl;  // print out a more proper error message
    return false;
  }
  char ** get_argv() { return argv; }  // get the arv field
  bool
  check_if_shellset() {  // check if the input information is a "set" command and in the right format
    if (command == "set") {
      std::size_t start = 0;
      std::size_t space1 = inp_inf.find(" ", start);  // check if there is a space behind set
      if (space1 != std::string::npos) {
        start = space1 + 1;
        std::size_t space2 =
            inp_inf.find(" ", start);  // check if there is a space behind the variable name
        if (space2 != std::string::npos) {
          std::string shell_var_name = inp_inf.substr(
              space1 + 1,
              space2 - space1 - 1);  // take the variable name out, and check it's format
          if (check_setvar_format(shell_var_name)) {  // check the variable name's format
            return true;
          }
        }
      }
    }
    return false;
  }
  bool check_setvar_format(std::string & content) {  // check the shell variable's name's format
    if (content == "") {                             // if it is an empty string
      return false;
    }
    std::string::iterator it = content.begin();
    while (
        it !=
        content
            .end()) {  // check every charactor and see if them are letter, numbers or underscores
      int c = *it;
      if ((isalnum(c)) || (c == 95)) {
        ++it;
      }
      else {
        return false;  // if one of them doesn't fit in the checking, return false
      }
    }
    return true;  // all of the characters fit in foramat checking
  }
  bool check_if_export() {
    if (command == "export") {  // check if the command is export
      if (argv[1] == NULL) {    // check if there's no argument for export
        std ::cout << "too few aruguments for export" << std::endl;
      }
      else {
        return true;
      }
    }
    return false;
  }
  bool check_if_inc() {
    if (command != "inc") {  // check if the command is inc
      return false;
    }
    else {                           // if the command is inc
      if (parsed_inp.size() == 1) {  // use the size to check if too few arguments
        std::cout << "too few arguments for inc" << std::endl;
        return false;
      }
      else if (parsed_inp.size() > 2) {  // check if too more arguments
        std::cout << "too many arguments for inc" << std::endl;
        return false;
      }
      else {  // check if inc's variable has a proper format
        std::vector<std::string>::iterator it = parsed_inp.begin();
        ++it;
        if (check_setvar_format(*it) == false) {
          return false;
        }
      }
    }
    return true;
  }
  // this function is to check is there existing a given symbol and return its position by iterator
  std::vector<std::string>::iterator check_symbol(std::string symbol, bool * special_fault) {
    std::vector<std::string>::iterator it = parsed_inp.begin();
    while (it != parsed_inp.end()) {  // traverse the string and get the postion of the symbol
      std::size_t found = (*it).find(symbol);  // try to find the symbos's postion
      if (found != std::string::npos) {        // if find successfully
        if (symbol.size() ==
            (*it)
                .size()) {  // check if there are other charcters in this string. For example "><" can be a confusion when we want to find "<" or ">"
          return it;        // return it if the symbol is clean
        }
        else {
          if (*it ==
              "2>") {  // if the symbol is not clean, check if it is a "2>", this should not be an error case
            it = parsed_inp.end();
            continue;
          }
          else {                    // if the symbol is not clean and is not "2>"
            *special_fault = true;  // set the special_fault flage to illustrate a special error
            return it;
          }
        }
      }
      ++it;
    }
    return it;
  }
  std::vector<std::string> & get_parsed_inp() { return parsed_inp; }
  //this function will set the redirect filename and type properly. If no redirection should be done, it will set redirectfilename to "\0"
  void do_redirect() {
    bool special_fault = false;  // set a flag to illustrate if there existing special error or not
    // find the position of ">","<" and "2>"
    std::vector<std::string>::iterator it_out = check_symbol(">", &special_fault);
    std::vector<std::string>::iterator it_in = check_symbol("<", &special_fault);
    std::vector<std::string>::iterator it_err = check_symbol("2>", &special_fault);
    std::vector<std::string>::iterator
        it;                       // set this iterator to show the position of redirection symbols
    if (special_fault == true) {  // check the flag, print the error message
      std::cout << "please use standard format for redirection command" << std::endl;
      redirect_filename = "\0";
      return;
    }
    if (it_out != parsed_inp.end() && it_in == parsed_inp.end() &&
        it_err == parsed_inp.end()) {  // find ">"
      redirect_type = 1;               // set the type and iterator
      it = it_out;                     // set the iterator
    }
    else if (it_out == parsed_inp.end() && it_in != parsed_inp.end() &&
             it_err == parsed_inp.end()) {  // find "<"
      redirect_type = 0;                    // set the type
      it = it_in;
    }
    else if (it_out == parsed_inp.end() && it_in == parsed_inp.end() &&
             it_err != parsed_inp.end()) {  // find "2>"
      redirect_type = 2;                    // set the type and iterator
      it = it_err;
    }
    else if (
        it_out == parsed_inp.end() && it_in == parsed_inp.end() &&
        it_err ==
            parsed_inp
                .end()) {  // cannot find anything means this funciton does not need to do anything, just return is ok
      return;
    }
    else {  // all other cases should be error
      std::cout << "please use standard format for redirection command" << std::endl;
      redirect_filename = "\0";
      return;
    }
    std::vector<std::string>::iterator it_move = parsed_inp.begin();
    int len1 = 0;
    int len2 = 0;
    if (it != parsed_inp.end()) {
      //travese the parsed_inp vector
      while (it_move != it) {  // compute the length before the redirect symbol
        len1++;
        ++it_move;
      }
      while (it_move != parsed_inp.end()) {  // compute the length after the redirect
        len2++;
        ++it_move;
      }

      if (len1 == 0 || len2 == 1) {  // if nothing before or behind the redirection symbol
        std::cout << "please use standard format for redirection command" << std::endl;
        redirect_filename = "\0";
      }
      else {
        parsed_inp.erase(it);     // erase the symbol from the vector
        redirect_filename = *it;  // get the redirect filename
        if (redirect_filename == ">" || redirect_filename == "<" ||
            redirect_filename ==
                "2>") {  //if the redirect filename is one of the redirect symbols,it should give an error message
          redirect_filename = "\0";
        }
        while (it != parsed_inp.end()) {  // erase all the things behind the redirection symbol
          parsed_inp.erase(it);
        }
      }
    }
  }
  std::string & get_redirect_filename() { return redirect_filename; }
  int get_redirect_type() { return redirect_type; }
};

class shellvariable : public input_inf  // make a child class of input_inf to be shell variables
{
 private:
  std::string variable;  // it has unique field of variable and value
  std::string value;

 public:
  shellvariable(std::string i, env_variable & e, bool * pnf, bool * f) : input_inf(i, e, pnf, f) {
    std::string var_temp(argv[1]);  // set the variable with the second argument in arg
    variable = var_temp;
    set_value();  // set variables's value
  }
  void set_value() {
    size_t space2 = inp_inf.find(" ");
    space2 = inp_inf.find(" ", space2 + 1);  // find the second space in a "set" command
    std::string v_temp = inp_inf.substr(
        space2 + 1, inp_inf.size() - space2 - 1);  // extrat the string to be the value
    value = v_temp;
  }
  std::string & get_variable() { return variable; }
  std::string & get_value() { return value; }
  void inc_value() {
    //this function will ++ the value of this variable if it is a number
    int value_n = atoi(value.c_str());  // using atoi to tranfer string to a number
    value_n++;
    std::stringstream ss;  // using stringstream to set the new number properly
    ss << value_n;
    value = ss.str();
  }
  void set_newvalue(std::string & nvalue) { value = nvalue; }  // change the value of a variable
};
#endif
