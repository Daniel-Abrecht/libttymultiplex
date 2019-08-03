#ifndef TERMINFO_HELPER_H
#define TERMINFO_HELPER_H

#include <sys/types.h>
#include <sys/stat.h>

int tym_i_open_terminfo(const char* name, mode_t mode);

#endif
