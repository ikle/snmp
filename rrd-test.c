/*
 * Round-Robin Dababase Test
 *
 * Copyright (c) 2019 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <err.h>

#include "rrdb.h"

int main (int argc, char *argv[])
{
	const char *name = "test.rrd";

	if (!rrdb_update (".", name, 1, 37000))
		err (1, name);

	return 0;
}
