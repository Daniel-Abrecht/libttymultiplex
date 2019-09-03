// Copyright (c) 2018 Daniel Abrecht
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef SUMMARY_H
#define SUMMARY_H

#ifdef __cplusplus
extern "C" {
#endif

enum sum_mode {
  SM_SILENT,
  SM_TOTAL_ONLY,
  SM_ALL_TREE,
  SM_ONE_LEVEL
};

extern enum sum_mode mode;

int add_entry(unsigned long n, unsigned long i, char* line);
int print_result(void);

#ifdef __cplusplus
}
#endif

#endif
