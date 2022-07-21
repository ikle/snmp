/*
 * Round-Robin Database Helpers
 *
 * Copyright (c) 2019-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rrd.h>

#include "rrdb.h"

#define ARRAY_SIZE(a)  (sizeof (a) / sizeof ((a)[0]))

static const char *rra[] = {
	/* average */
	"RRA:AVERAGE:0.5:1:480",
	"RRA:AVERAGE:0.5:3:480",
	"RRA:AVERAGE:0.5:21:480",
	"RRA:AVERAGE:0.5:93:480",
	"RRA:AVERAGE:0.5:1098:480",
	/* minimum */
	"RRA:MIN:0.5:1:480",
	"RRA:MIN:0.5:3:480",
	"RRA:MIN:0.5:21:480",
	"RRA:MIN:0.5:93:480",
	"RRA:MIN:0.5:1098:480",
	/* maximum */
	"RRA:MAX:0.5:1:480",
	"RRA:MAX:0.5:3:480",
	"RRA:MAX:0.5:21:480",
	"RRA:MAX:0.5:93:480",
	"RRA:MAX:0.5:1098:480",
};

static int db_create (const char *path, int gauge)
{
	const int argc = 5 + ARRAY_SIZE (rra);
	const char *argv[argc + 1];
	int i;

	if (access (path, R_OK | W_OK) == 0)
		return 1;

	if (errno != ENOENT)
		return 0;

	argv[0] = "fake";
	argv[1] = path;
	argv[2] = "-s";
	argv[3] = "60";
	argv[4] = gauge ? "DS:x:GAUGE:120:0:U" : "DS:x:DERIVE:120:0:U";

	for (i = 0; i < ARRAY_SIZE (rra); ++i)
		argv[i + 5] = rra[i];

	argv[i + 5] = NULL;

	return rrd_create (argc, (char **) argv) == 0;
}

static int db_update (const char *path, int gauge, long value)
{
	static const char *fmt = "N:%ld";
	int len;
	char *data;

	const int argc = 3;
	const char *argv[argc + 1];
	int ret;

	if (!db_create (path, gauge))
		return 0;

	len = snprintf (NULL, 0, fmt, value) + 1;

	if ((data = malloc (len)) == NULL)
		return 0;

	sprintf (data, fmt, value);

	argv[0] = "fake";
	argv[1] = path;
	argv[2] = data;
	argv[3] = NULL;

	ret = rrd_update (argc, (char **) argv) == 0;
	free (data);
	return ret;
}

static char *db_path (const char *root, const char *name)
{
	static const char *fmt = "%s/%s.rrd";
	int len;
	char *path;

	len = snprintf (NULL, 0, fmt, root, name) + 1;

	if ((path = malloc (len)) == NULL)
		return NULL;

	sprintf (path, fmt, root, name);
	return path;
}

int rrdb_update (const char *root, const char *name, int gauge, long value)
{
	char *path;
	int ret;

	if ((path = db_path (root, name)) == NULL)
		return 0;

	ret = db_update (path, gauge, value);
	free (path);
	return ret;
}
