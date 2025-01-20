#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>     // fork(), execvp()
#include <stdio.h>      // printf(), scanf(), setbuf(), perror()
#include <stdlib.h>     // malloc()
#include <sys/types.h>  // pid_t 
#include <sys/wait.h>   // waitpid()
#include <signal.h>     // kill(), SIGTERM, SIGKILL, SIGSTOP, SIGCONT
#include <errno.h>      // errno
#include "linked_list.h"

Node* head = NULL;


void func_BG(char **cmd){
  	pid_t pid = fork();
    if (pid < 0) { //Fork fails
      printf("Fork Failed\n");
      exit(-1);
    } else if (pid == 0) { //Child process
      char* command = cmd[1];
      execvp(command, &cmd[1]);
    } else { //Parent process
      printf("Fork Successful! Started background process: %d\n", pid);
      char* path = cmd[1];
      head = add_newNode(head, pid, path); //Adds pid and process name to linked list
    }

}


void func_BGlist(char **cmd){ 
  printList(head); //Prints out the list of processes
}


void func_BGkill(char * str_pid){
  pid_t pid = atoi(str_pid); //Converts string to int

  if (!pidExist(head, pid)) {
    printf("Error: Process %d does not exist\n", pid);
    return;
  }

  int error_no = kill(pid, SIGTERM); //Sends TERM signal to process
  if (error_no == 0) {
    head = deleteNode(head, pid);
    sleep(1);
    printf("Successfully killed process\n");
  } else {
    printf("Failed to perform bgkill\n");
  }
}


void func_BGstop(char * str_pid){
  pid_t pid = atoi(str_pid);
  
  if (!pidExist(head, pid)) {
    printf("Error: Process %d does not exist\n", pid);
    return;
  }

  int error_no = kill(pid, SIGSTOP); //Sends STOP signal to process
  if (!error_no) {
    sleep(1);
    printf("Successfully stopped process");
  } else {
    printf("Failed to perform bgstop\n");
  }
}


void func_BGstart(char * str_pid){
	//Your code here
  pid_t pid = atoi(str_pid);
  
  if (!pidExist(head, pid)) {
    printf("Error: Process %d does not exist\n", pid);
    return;
  }

  int error_no = kill(pid, SIGCONT); //Sends CONT signal to process
  if (!error_no) {
    sleep(1);
    printf("Successfully started process");
  } else {
    printf("Failed to perform bgstart\n");
  }
}


void func_pstat(char * str_pid){
	//Your code here
  pid_t pid = atoi(str_pid);

  if (!pidExist(head, pid)) {
    printf("Error: Process %d does not exist\n", pid);
    return;
  }

  //Code to get data from status (Contains ctxt switches information)
  char statusPath[50];
  char statusLine[150][150];
  sprintf(statusPath, "/proc/%d/status", pid);
  FILE* statusFile = fopen(statusPath, "r");
  if (statusFile == NULL) {
    printf("Error: Could not read file\n");
    return;
  }
  int i = 0;
  while(fgets(statusLine[i], 150, statusFile) != NULL) {
    i++;
  }
  fclose(statusFile);

//Code to get data from stat (Easier to use data; outputs just numbers)
  char statPath[100];
  char statLine[1000];
  char* statContents[100];
  sprintf(statPath, "/proc/%d/stat", pid);
  FILE* statFile = fopen(statPath, "r");
  if (statFile == NULL) {
    printf("Error: Could not read file\n");
    return;
  }
  int j = 0;
  while(fgets(statLine, sizeof(statLine), statFile) != NULL) {
    char* token = strtok(statLine, " "); //Code to split values from one line of values
    statContents[j] = token;
    while (token != NULL) {
      statContents[j] = token;
      token = strtok(NULL, " ");
      j++;
    }
  }
  fclose(statFile);
  
  char* end;
	long unsigned int utime = strtoul(statContents[13], &end, 10) / sysconf(_SC_CLK_TCK);
	long unsigned int stime = strtoul(statContents[14], &end, 10) / sysconf(_SC_CLK_TCK);
  
  printf("comm: %s\n", statContents[1]);
  printf("state: %s\n", statContents[2]);
  printf("utime: %lu\n", utime);
  printf("stime: %lu\n", stime);
  printf("rss: %s\n", statContents[23]);
  printf("%s", statusLine[55]);
  printf("%s", statusLine[56]);
}

 
int main(){
    char user_input_str[50];
    while (true) {
      printf("Pman: > ");
      fgets(user_input_str, 50, stdin);
      printf("User input: %s \n", user_input_str);
      char * ptr = strtok(user_input_str, " \n");
      if(ptr == NULL){
        continue;
      }
      char * lst[50];
      int index = 0;
      lst[index] = ptr;
      index++;
      while(ptr != NULL){ 
        ptr = strtok(NULL, " \n");
        lst[index]=ptr;
        index++;
      }
      if (strcmp("bg",lst[0]) == 0){
        func_BG(lst);
      } else if (strcmp("bglist",lst[0]) == 0) {
        func_BGlist(lst);
      } else if (strcmp("bgkill",lst[0]) == 0) {
        func_BGkill(lst[1]);
      } else if (strcmp("bgstop",lst[0]) == 0) {
        func_BGstop(lst[1]);
      } else if (strcmp("bgstart",lst[0]) == 0) {
        func_BGstart(lst[1]);
      } else if (strcmp("pstat",lst[0]) == 0) {
        func_pstat(lst[1]);
      } else if (strcmp("q",lst[0]) == 0) {
        printf("Bye Bye \n");
        exit(0);
      } else {
        printf("Invalid input\n");
      }
    }

  return 0;
}
