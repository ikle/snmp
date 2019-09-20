/*
 * Round-Robin Dababase Helpers
 *
 * Copyright (c) 2019 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef RRDB_H
#define RRDB_H  1

int rrdb_update (const char *root, const char *name, int gauge, long value);

#endif  /* RRDB_H */
