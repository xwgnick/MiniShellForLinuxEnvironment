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

#include "main_functions.h"
#include "shell.h"
//find the value of variables coming with $. If it exsits, return the value attached with other charcaters the user wants; If it do not exists, return a null string
std::string get_dollar_replacement(std::string & content, std::vector<shellvariable *> & sv);
// find the $ in the content, and then replace its following information with proper information. return 0 if failed, return 1 if succeed
int find_dollar_replace(std::string & content, std::vector<shellvariable *> & sv);
// check if content are all numbers
bool are_all_numbers(std::string & content);
// when inc a non-existing variable, set it with value "1"
void new_inc_var(std::string & inc_varname,
                 std::vector<shellvariable *> & sv,
                 bool * need_find,
                 bool * found,
                 env_variable & ece551path);
// inc an existing variable
void do_exist_inc(std::vector<shellvariable *>::iterator it);
// do_inc() is used to do the "inc" command
void do_inc(input_inf & input,
            std::vector<shellvariable *> & sv,
            bool * need_find,
            bool * found,
            env_variable & ece551path);
//get the key of an element in the kv_pair vector
std::string get_kvpair_key(std::string kvpair);
// check if the variable exists in the environment vector. If it does,erase it and update it
void envcheck_exist_erase(std::string & kv, std::vector<std::string> & k_v_pair);
// check if the variable exists in the shell variables vector. If it does,erase it and update it
void svacheck_exist_erase(shellvariable * newshellvar, std::vector<shellvariable *> & oldshellvar);
// export environement variables into the envrionemnt. If the variables exsitsin the environement, then it will update it. If it not exists in the environmen//t, then it will make a new environment variable. If the user wants to export a variable that is not existing in the shell variable vector, then this functi//on will do nothing
void export_env(char ** arg,
                std::vector<shellvariable *> & sv,
                std::vector<std::string> & k_v_pair);
// Do the redirection. This funciton will return 0 if succed, 1 if failed
int do_redirect(input_inf & input, std::string & redirect_filename);
// in this function run the execve function
void run_child(std::string pathname, char ** argv, char ** env);
// this function is used to check status and print proper inforamtion
void check_wstatus(int wstatus);
// this function is to check if the content contains "set", "export","inc"
bool has_special_command(std::string & content);
// this function is used to determine how many program exists when pipe existing
int determine_pipe(std::string & content, std::vector<std::string> & programs);
// this function is to set the variable ece551path into the shell variable vecotr
void set_ece551path(env_variable & ece551path,
                    bool * need_find,
                    bool * found,
                    std::vector<shellvariable *> & sv);
void clean_before_continue(input_inf * newinput,
                           std::vector<std::string>::iterator get_inp_inf,
                           int * program_number);
void clean_ForkStuff_before_continue(std::vector<input_inf *> & inps, std::vector<char **> & envs);
int main() {
  std::vector<shellvariable *> sv;  // this vector is to store all the shell variables
  std::vector<std::string>
      k_v_pair;  // this vector is to store all the environment waiting to be exported
  bool update_ece551path = false;  // set a flag to see have we updated the ECE551PATH or not
  while (1) {
    char * current_dir_name = get_current_dir_name();      // get the current working directory
    std::cout << "myShell:" << current_dir_name << " $ ";  // printout the prompt
    free(current_dir_name);                                // free the current_dir_name
    std::vector<std::string>
        programs;                   // make a vector to contain all the subcommands in the pipe line
    std::vector<input_inf *> inps;  // make vector to contain input information
    std::vector<std::string>
        redirect_filenames;             // make a vector to contain filenames using in redirection
    std::vector<std::string> commands;  // a vector to contain commands
    std::vector<char **>
        args;  // a vetor to contain arguments that will be used in the execve' second argument
    std::vector<char **>
        envs;  // a vetor to contain envrioment variables that will be used in the execve's third argument
    env_variable path(
        "PATH");  // get the information of the environment variable "PATH", put then into a new "env_variable" class
    env_variable ece551path(
        "ECE551PATH",
        path.get_env_content());  // set a new env_variable class ece551path with the information from path
    if (update_ece551path ==
        true) {  // if ece551path was updated somewhere, we need to put it into the
      std::vector<shellvariable *>::iterator it = sv.begin();
      while (it != sv.end()) {
        //traverse the whole shell variable vector and update the value of ECE551PATH
        if ((*(*it)).get_variable() == "ECE551PATH") {
          ece551path.change_env_content((*(*it)).get_value());
        }
        ++it;
      }
    }
    bool need_find =
        false;  // set a bool to see if the command need to be find in the environment paths
    bool found = false;                // if it needs to be found, see if it is found
    if (update_ece551path == false) {  // if ece551path never been updated
      set_ece551path(
          ece551path, &need_find, &found, sv);  // set ece551path into the shell variable vector
    }
    std::string temp;         // a string for information reading from stdin
    getline(std::cin, temp);  // read information from stdin until EOF
    if (!std::cin) {          // check for eof
      if (std::cin.eof())
        // when it exits, it should delete all the space
        for (std::vector<shellvariable *>::iterator it = sv.begin(); it != sv.end(); ++it) {
          delete *it;
        }
      break;
    }
    else if (temp == "exit") {  // if user input exit, then exit
      // when it exits, it should delete all the space
      for (std::vector<shellvariable *>::iterator it = sv.begin(); it != sv.end(); ++it) {
        delete *it;
      }
      break;
    }
    else if ((temp.find_first_not_of(" ")) == std::string::npos) {
      // if the user put enter or bunch of spaces, it should print the prompt again
      continue;
    }
    else {
      int find_dollar_err =
          find_dollar_replace(temp, sv);  // find "$" in temp and replace all of them
      if (find_dollar_err == 0) {         // if meets special commands
        continue;
      }
      int program_number =
          determine_pipe(temp, programs);  // determine how many programs in the pipe
      if (program_number == -1) {          // if determine fails
        continue;
      }
      //traverse all the programs in the pipe line
      std::vector<std::string>::iterator get_inp_inf = programs.begin();
      while (get_inp_inf != programs.end()) {
        input_inf * newinput =
            new input_inf(*get_inp_inf,
                          ece551path,
                          &need_find,
                          &found);  // put the input information into a input_inf class
        std::string redirect_filename =
            (*newinput).get_redirect_filename();  // if it needs to redirect, get the redirect name
        if (redirect_filename.find_first_not_of(" ") ==
            std::string::npos) {  // check if the redirect name illegal
          std::cout << "please use standard filename for redirection" << std::endl;
          clean_before_continue(
              newinput, &get_inp_inf, &program_number);  // clean all the space before continue
          continue;
        }
        if ((*newinput).check_if_shellset()) {  // if it is a "set" command
          shellvariable * newsvar;
          newsvar =
              new shellvariable(*get_inp_inf,
                                ece551path,
                                &need_find,
                                &found);  // put the input information into a shellvariable class
          if ((*newsvar).get_variable() == "ECE551PATH") {  // if the user is upading "ECE551PATH"
            update_ece551path = true;                       // set the flag properly
          }
          svacheck_exist_erase(newsvar,
                               sv);  // if there already exists this variable, then erase it
          sv.push_back(newsvar);     // push the variable into it
          clean_before_continue(
              newinput, &get_inp_inf, &program_number);  // clean all the space befor continue
          continue;
        }
        if ((*newinput).check_if_inc()) {                           // if it is "inc" command
          do_inc((*newinput), sv, &need_find, &found, ece551path);  // do inc stuff
          clean_before_continue(
              newinput, &get_inp_inf, &program_number);  // clean all the space before continue
          continue;
        }
        std::string command = (*newinput).pathname();  // get the suitable commandname
        if (!(*newinput).check_if_export()) {
          if ((need_find == true) &&
              (found ==
               false)) {  // if the command need to be found in the environment path and it is not found
            std::cout << "Command " << command << " not found" << std::endl;
            need_find = false;
            found = false;
            clean_before_continue(
                newinput, &get_inp_inf, &program_number);  // clean all the space befor continue
            continue;
          }
        }
        char ** arg = (*newinput).get_argv();  // get the arguments the child program needed
        if ((*newinput).check_if_export()) {   // if the command is export
          export_env(arg, sv, k_v_pair);
          clean_before_continue(
              newinput, &get_inp_inf, &program_number);  // clean all the space befor continue
          continue;                                      // after exportion, it should continue
        }
        if (command == "cd") {  // if the command line is cd
          if (arg[1] ==
              NULL) {  // if cd has no arguments, the directory should be change to the rootxs
            chdir("/");
            clean_before_continue(
                newinput, &get_inp_inf, &program_number);  // clean all the space befor continue
            continue;
          }
          if (arg[2] != NULL) {  // if user put more than two arguments for cd command
            std::cerr << "-bash: cd: too many arguments" << std::endl;
            clean_before_continue(
                newinput, &get_inp_inf, &program_number);  // clean all the space befor continue
            continue;
          }
          const char * todir = arg[1];  // get the taget directory
          int result = 0;
          result = chdir(todir);  // change working directory
          if (result == -1) {     // if changing the derectroy failed
            std::cerr << "-bash: cd: " << arg[1] << ": No such file or directory" << std::endl;
          }
          clean_before_continue(
              newinput, &get_inp_inf, &program_number);  // clean all the space befor continue
          continue;
        }
        if ((*newinput).inputhasslash() == false) {
          // if the input information has slash, but it is not a existing program, my shell should print an another error message
          if ((*newinput).check_before_exe() == false) {
            clean_before_continue(
                newinput, &get_inp_inf, &program_number);  // clean all the space befor continue
            continue;
          }
        }
        char ** env = new char *[k_v_pair.size() + 2];  // make new spaces for environment variables
        char * newpath_ch = const_cast<char *>(ece551path.get_key_value_pair().c_str());
        env[0] = newpath_ch;  // put key value pair in to the fisrt place of the env array
        size_t i = 1;
        std::vector<std::string>::iterator it_kv = k_v_pair.begin();
        while (it_kv != k_v_pair.end()) {  // put all the rest envrinoment variables into the array
          char * kv_ch = const_cast<char *>((*it_kv).c_str());
          env[i] = kv_ch;
          i++;
          ++it_kv;
        }
        env[k_v_pair.size() + 1] = NULL;  // add a NULL to the end of it
        // after preparing all the aruguments excve neead, my shell should store them properly in the vectors list as follows
        inps.push_back(newinput);
        redirect_filenames.push_back(redirect_filename);
        commands.push_back(command);
        args.push_back(arg);
        envs.push_back(env);
        ++get_inp_inf;  // get another input information
      }
      if (program_number == 0) {  // if the input has no "|" and continued before
        continue;
      }
      else {
        std::vector<int>
            c_pids;  // make a vector to store all the c_pids that will be returned by fork()
        pid_t c_pid;
        int i;
        int fd_total[(program_number - 1) * 2];  // set an array as pipe material
        int * temp = &fd_total[0];  // set a pointer to point every beginning of the pipe
        int p = 0;
        while (p <= (program_number - 1) * 2 + 1) {
          //traverse the array, and pipe every two elements together
          int pipe_err = pipe(temp);
          if (pipe_err == -1) {  // check if pipe failed
            std::cout << "pipe failed" << std::endl;
            clean_ForkStuff_before_continue(inps, envs);  // clean all the spaces before continue
            continue;
          }
          temp += 2;  // move 2 space after for temp
          p += 2;
        }
        for (i = 0; i < program_number; i++) {  // use a loop to fork proper times
          c_pid = fork();                       // use fork to duplicate the running process
          c_pids.push_back(c_pid);              // put all the c_pids into the vector
          if (c_pid == -1) {                    // see if fork failed
            std::cerr << "fork failed" << std::endl;
            clean_ForkStuff_before_continue(inps, envs);  // clean all the spaces before continue
            continue;
          }
          else if (c_pid == 0) {  // see if this process is a child process
            if (i != 0) {
              //    close(fd_total[i * 2 - 1]);
              dup2(fd_total[i * 2 - 2], 0);
            }
            if (i != program_number - 1) {
              // close(fd_total[i * 2]);
              dup2(fd_total[i * 2 + 1], 1);
            }
            //take all the arugunments excve need from the vectors
            std::string redirect_filename = redirect_filenames[i];
            std::string command = commands[i];
            input_inf * newinput = inps[i];
            char ** arg = args[i];
            char ** env = envs[i];
            if (redirect_filename != "no") {  // see if needs redirection
              int redirect_err = do_redirect(
                  (*newinput), redirect_filename);  // if it should rediect, then do_redirect
              if (redirect_err == -1) {             // if redirection failed
                // clean all the spaces before continue
                clean_ForkStuff_before_continue(inps, envs);
                for (std::vector<shellvariable *>::iterator it = sv.begin(); it != sv.end(); ++it) {
                  delete *it;
                }
                return EXIT_FAILURE;  // it should return in failure status
              }
            }
            for (int i = 0; i < (program_number - 1) * 2; i++) {  // close all the pipe ends
              close(fd_total[i]);
            }
            run_child(command, arg, env);  // run the child process
            return EXIT_FAILURE;           // if execve failed
          }
        }
        for (int i = 0; i < (program_number - 1) * 2;
             i++) {  // close all the pipe ends in the father process
          close(fd_total[i]);
        }
        for (int j = 0; j < program_number; j++) {  // wait all the child processes by their c_pids
          int wstatus;  // a place to hold the status change of the child process
          int w;
          w = waitpid(
              c_pids[j],
              &wstatus,
              WUNTRACED | WCONTINUED);  // use waitpid to wait for the change of child process
          //delete the spaces before another loop
          delete inps[j];
          delete[] envs[j];
          if (w == -1) {  // see if waitpid failed
            std::cerr << "waitpid failed!" << std::endl;
            exit(EXIT_FAILURE);
          }
          check_wstatus(wstatus);  // check the status of the child process
        }
      }
    }
  }
}
