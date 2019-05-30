// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <internal/backend.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/** \file */

/** A linked list of all backends */
static const struct tym_i_backend_entry* tym_i_backend_list;

/** The currently selected backend. */
const struct tym_i_backend* tym_i_backend;

/** Validates & registes a backend in tym_i_backend_list */
void tym_i_backend_register(struct tym_i_backend_entry* entry){
  bool bad = false;
  if(!entry->name || !*entry->name)
    return;
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
  if(bad){
    fprintf(stderr,"Refusing to register incomplete backend \"%s\"\n",entry->name);
    return;
  }
  entry->next = tym_i_backend_list;
  tym_i_backend_list = entry;
}

/**
 * Choose and Initialise a backend. The backend is initialised in tym_init.
 * 
 * \param backend The name of the backend. If this is 0, every backend is tried until one works.
 */
int tym_i_backend_init(const char* backend){
  if(backend){
    for(const struct tym_i_backend_entry* it=tym_i_backend_list; it; it=it->next){
      if(!strcmp(it->name, backend)){
        tym_i_backend = &it->backend;
        errno = 0;
        return tym_i_backend->init();
      }
    }
    errno = ENOENT;
    return -1;
  }else{
    for(const struct tym_i_backend_entry* it=tym_i_backend_list; it; it=it->next){
      tym_i_backend = &it->backend;
      errno = 0;
      if(tym_i_backend->init() == 0)
        return 0;
    }
    tym_i_backend = 0;
    errno = ENOTSUP;
    return -1;
  }
}
