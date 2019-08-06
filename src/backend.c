// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>

#include <internal/backend.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/** \file */

/** A linked list of all backends */
static const struct tym_i_backend_entry* tym_i_backend_list;

/** The currently selected backend. */
const struct tym_i_backend* tym_i_backend;

/** Backend entry, internal reference for cleanup stuff later on */
static const struct tym_i_backend_entry* tym_i_backend_entry;

static const char* plugin_dir = PREFIX "/lib/libttymultiplex/backend";

bool tym_i_external_backend_normal_loading;

static struct tym_i_backend_entry* load_and_init_speciffic_external_backend(const char* path){
  tym_i_external_backend_normal_loading = true;
  const char* libname = strrchr(path, '/');
  if(libname){
    libname += 1;
  }else{
    libname = path;
  }
  size_t libnamelen = strlen(libname);
  if(libnamelen < 6 || !isdigit(libname[0]) || !isdigit(libname[1]) || libname[2] != '-' || strcmp(".so",libname+libnamelen-3))
    goto error;
  {
    char* tmp = calloc(1,libnamelen-5);
    memcpy(tmp, libname+3, libnamelen-6);
    libname = tmp;
  }
  void* lib = dlopen(path, RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND);
  if(!lib){
    fprintf(stderr,"dlopen(\"%s\") failed: %s\n", path, dlerror());
    goto error_after_calloc;
  }
  dlerror();
  struct tym_i_backend_entry** res = dlsym(lib,"tymb_backend_entry");
  if(!res || !*res){
    fprintf(stderr, "%s: dlsym failed: %s\n", path, dlerror());
    goto error_after_dlopen;
  }
  struct tym_i_backend_entry* backend_entry = *res;
  if(strcmp(backend_entry->name, libname))
    fprintf(stderr, "Warning: builting backend name (%s) mismatches library name (%s). Overriding backend name...", backend_entry->name, libname);
  backend_entry->name = libname;
  backend_entry->library = lib;
  if(!tym_i_backend_validate_prepare(backend_entry))
    goto error_after_dlopen;
  tym_i_backend_entry = backend_entry;
  tym_i_backend = &backend_entry->backend;
  if(tym_i_backend->init() != 0)
    goto error_after_backend_register;
  tym_i_external_backend_normal_loading = false;
  return backend_entry;
error_after_backend_register:
  tym_i_backend_entry = 0;
  tym_i_backend = 0;
error_after_dlopen:
  dlclose(lib);
error_after_calloc:
  free((char*)libname);
error:
  tym_i_external_backend_normal_loading = false;
  return 0;
}

static int backend_filename_filter(const struct dirent* entry){
  size_t len = strlen(entry->d_name);
  if(len < 3)
    return 0;
  if(!isdigit(entry->d_name[0]) || !isdigit(entry->d_name[1]) || entry->d_name[2] != '-')
    return false;
  return !strcmp(entry->d_name+(len-3),".so");
}

static struct tym_i_backend_entry* load_and_init_external_backend(const char* name){
  enum { path_size_max = 255 };
  if(!plugin_dir || !*plugin_dir){
    errno = EINVAL;
    return 0;
  }
  size_t plugin_dir_length = strlen(plugin_dir);
  if(plugin_dir_length >= path_size_max-3){
    errno = ENOBUFS;
    return 0;
  }
  char path[path_size_max+1] = {0};
  memcpy(path, plugin_dir, plugin_dir_length);
  if(path[plugin_dir_length-1] != '/')
    path[plugin_dir_length++] = '/';
  size_t name_length = 0;
  size_t expected_path_length = 0;
  if(name){
    name_length = strlen(name);
    expected_path_length = plugin_dir_length + name_length + 6;
    if(expected_path_length > path_size_max)
      return 0;
  }
  struct dirent **entries = 0;
  int n = scandir(plugin_dir, &entries, backend_filename_filter, alphasort);
  if(n < 0){
    perror("scandir");
    return 0;
  }
  if(!n){
    if(entries)
      free(entries);
    errno = ENOENT;
    return 0;
  }
  struct tym_i_backend_entry* entry = 0;
  int i;
  for(i=0; i<n; i++){
    size_t len = strlen(entries[i]->d_name);
    if(path_size_max - plugin_dir_length < len)
      continue;
    if(name){
      if(plugin_dir_length + len != expected_path_length)
        continue;
      if(memcmp(entries[i]->d_name+3, name, name_length))
        continue;
    }
    memcpy(path + plugin_dir_length, entries[i]->d_name, len+1);
    entry = load_and_init_speciffic_external_backend(path);
    if(entry || name)
      break;
  }
  for(int j=n; j--;)
    free(entries[j]);
  free(entries);
  if(!entry){
    errno = name ? ENOENT : ENOTSUP;
    return 0;
  }
  return entry;
}

/**
 * Validates a backend in tym_i_backend_list & makes some last preparations for final usage
 * 
 * \returns true if OK, false otherwise
 */
bool tym_i_backend_validate_prepare(struct tym_i_backend_entry* entry){
  bool bad = false;
  if(!entry->name || !*entry->name)
    return false;
  #define R(RET, ID, PARAMS, DOC) \
    if(!entry->backend.ID){ \
      fprintf(stderr,"Backend \"%s\" is missing required function \"%s\"\n",entry->name,#ID); \
      bad = true; \
    }
  #define O(RET, ID, PARAMS, DOC) \
    if(!entry->backend.ID) \
      entry->backend.ID = tym_i_ ## ID ## _default_proc;
  TYM_I_BACKEND_CALLBACKS
  #undef R
  #undef O
  return !bad;
}

/**
 * Validates & registes a backend in tym_i_backend_list
 * 
 * \returns 0 on failure and entry on success
 */
struct tym_i_backend_entry* tym_i_backend_register(struct tym_i_backend_entry* entry){
  if(!tym_i_backend_validate_prepare(entry)){
    fprintf(stderr,"Invalid backend \"%s\", not registring backend!\n",entry->name);
    return 0;
  }
  entry->next = tym_i_backend_list;
  tym_i_backend_list = entry;
  return entry;
}

/**
 * Choose and Initialise a backend. The backend is initialised in tym_init.
 * 
 * \param backend The name of the backend. If this is 0, every backend is tried until one works.
 */
int tym_i_backend_init(const char* backend){
  struct tym_i_backend_entry* res = load_and_init_external_backend(backend);
  if(res){
    return 0;
  }else if(backend){
    for(const struct tym_i_backend_entry* it=tym_i_backend_list; it; it=it->next){
      if(!strcmp(it->name, backend)){
        tym_i_backend_entry = it;
        tym_i_backend = &it->backend;
        errno = 0;
        return tym_i_backend->init();
      }
    }
    errno = ENOENT;
    return -1;
  }else{
    for(const struct tym_i_backend_entry* it=tym_i_backend_list; it; it=it->next){
      tym_i_backend_entry = it;
      tym_i_backend = &it->backend;
      errno = 0;
      if(tym_i_backend->init() == 0)
        return 0;
    }
    tym_i_backend_entry = 0;
    tym_i_backend = 0;
    errno = ENOTSUP;
    return -1;
  }
}

/**
 * Unload the backend which is currently in use / was started using tym_i_backend_init.
 */
void tym_i_backend_unload(void){
  if(!tym_i_backend_entry || !tym_i_backend)
    return;
  tym_i_backend->cleanup();
  if(tym_i_backend_entry->library){
    free((char*)tym_i_backend_entry->name);
    dlclose(tym_i_backend_entry->library);
  }
  tym_i_backend_entry = 0;
  tym_i_backend = 0;
}

