// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <summary.h>

int fork_exec(char* argv[]){
  int ret = fork();
  if(ret == -1){
    perror("fork failed");
    return -1;
  }
  if(ret == 0){
    execvp(argv[0], argv);
    exit(1);
  }
  return ret;
}

int main(int argc, char* argv[]){
  if(argc < 3){
    fprintf(stderr, "Usage: test-summary what command [args]\n");
    return 1;
  }
  char* sfd = getenv("SUMMARY_FD");
  long l = -1;
  int fd = -1;
  if( sfd == 0 || (fd=(int)(l=strtol(sfd,&sfd,10)))<0 || l != (long)fd || fcntl(fd, F_SETFD, FD_CLOEXEC) == -1 ){
    l  = -1;
    fd = -1;
  }
  mode = fd == -1 ? SM_ONE_LEVEL : SM_TOTAL_ONLY;
  unsetenv("SUMMARY_FD");
  int io[2];
  pipe(io);
  if(fcntl(io[0], F_SETFD, FD_CLOEXEC) == -1){
    perror("fcntl failed");
    return 1;
  }
  char cfd[64];
  snprintf(cfd, sizeof(cfd), "%d", io[1]);
  if(setenv("SUMMARY_FD", cfd, true) == -1){
    perror("setenv failed");
    return 1;
  }
  pid_t pid = fork_exec(argv+2);
  if(pid == -1)
    return 1;
  close(io[1]);
  FILE* f = fdopen(io[0], "r");
  if(!f)
    return 1;
  int ret = 0;
  char line[512];
  while(!feof(f)){
    long unsigned n=0, i=0;
    bool skipline = false;
    if(fscanf(f, "%lu %lu ", &n, &i) != 2 || n < i)
      skipline = true;
    bool first = true;
    bool output = true;
    while(fgets(line, sizeof(line), f)){
      bool newline = false;
      size_t m = strlen(line);
      if(!m) continue;
      if(line[m-1] == '\n'){
        newline = true;
        line[m-1] = 0;
        if(skipline)
          break;
      }
      if(skipline)
        continue;
      if(first){
        if(n != i)
          ret = 2;
        add_entry(n, i, line);
        if(fd != -1)
          dprintf(fd, "%lu %lu %s ", n, i, argv[1]);
        first = false;
        output = true;
      }
      if(fd != -1)
        dprintf(fd, "%s", line);
      if(newline)
        break;
    }
    if(output && fd != -1)
      dprintf(fd,"\n");
  }
  int wstatus = 0;
  do {
    int w = waitpid(pid, &wstatus, 0);
    if(w == -1){
      perror("waitpid failed");
      exit(1);
    }
  } while(!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
  if(WEXITSTATUS(wstatus) != 0)
    ret = WEXITSTATUS(wstatus);
  print_result();
  return ret;
}
