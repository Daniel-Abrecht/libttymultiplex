// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <internal/terminfo_helper.h>

static int open_helper(const char*const* path, mode_t mode){
  int lfd = AT_FDCWD;
  for(const char*const* it=path; *it; it++){
    const char* dir = *it;
    if(it != path)
    while(*dir && *dir == '/')
      dir++;
    if(!*dir)
      continue;
    int nlfd = openat(lfd, dir, mode);
    if(lfd != AT_FDCWD)
      close(lfd);
    lfd = nlfd;
    if(lfd == -1)
      return -1;
  }
  if(lfd == AT_FDCWD){
    errno = EINVAL;
    return -1;
  }
  struct stat st;
  if(fstat(lfd, &st) == -1){
    close(lfd);
    return -1;
  }
  if(!S_ISREG(st.st_mode)){
    close(lfd);
    return -1;
  }
  return lfd;
}

/**
 * Returns a file descriptor for the terminfo file or -1.
 * 
 * \param name The name of the terminfo file
 */
int tym_i_open_terminfo(const char* name, mode_t mode){
  if(!name || !*name){
    errno = ENOENT;
    return -1;
  }
  if(getenv("TERMINFO"))
    return open_helper((const char*[]){getenv("TERMINFO"),(char[]){name[0],0},name,0}, mode);
  int ret = -1;
  if(getenv("HOME")){
    ret = open_helper((const char*[]){getenv("HOME"),".terminfo",(char[]){name[0],0},name,0}, mode);
    if(ret != -1)
      return ret;
  }
  char* dirs = getenv("TERMINFO_DIRS");
  if(dirs)
    dirs = strdup(dirs);
  if(dirs){
    const char* dir;
    while((dir = strtok(dirs,":"))){
      if(!*dir)
        dir = "/etc/terminfo";
      ret = open_helper((const char*[]){dir,(char[]){name[0],0},name,0}, mode);
      if(ret != -1){
        free(dirs);
        return ret;
      }
    }
    free(dirs);
  }
  for(const char* dirs[]={"/lib/terminfo/","/usr/share/terminfo","/etc/terminfo", 0}, **it=dirs; *it; it++){
    ret = open_helper((const char*[]){*it,(char[]){name[0],0},name,0}, mode);
    if(ret != -1)
      return ret;
  }
  errno = ENOENT;
  return -1;
}
