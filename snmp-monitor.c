/*
 * SNMP Monitor
 *
 * Copyright (c) 2019 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include "rrdb.h"

#define ARRAY_SIZE(a)  (sizeof (a) / sizeof ((a)[0]))

static netsnmp_session *
snmp_session_open (const char *domain, const char *peer, const char *community)
{
	netsnmp_session session;

	init_snmp (domain);

	snmp_sess_init (&session);
	session.peername = (char *) peer;

	session.version = SNMP_VERSION_1;

	session.community = (u_char *) community;
	session.community_len = strlen (community);

	return snmp_open (&session);
}

struct record {
	struct record *next;
	char *name;
	oid *OID;
	size_t OID_len;
	int gauge;
};

static struct record *get_record (int argc, char *argv[])
{
	struct record *o;
	oid OID[MAX_OID_LEN];
	size_t len = ARRAY_SIZE (OID);

	if (argc < 3) {
		errno = EINVAL;
		return NULL;
	}

	if ((o = malloc (sizeof (*o))) == NULL)
		return NULL;

	o->next = NULL;

	if ((o->name = strdup (argv[0])) == NULL)
		goto no_name;

	if (!snmp_parse_oid (argv[1], OID, &len)) {
		errno = ENOENT;
		goto no_parse;
	}

	if ((o->OID = malloc (sizeof (OID[0]) * len)) == NULL)
		goto no_oid;

	memcpy (o->OID, OID, sizeof (OID[0]) * len);
	o->OID_len = len;

	if (strcmp(argv[2], "gauge") == 0)
		o->gauge = 1;
	else if (strcmp(argv[2], "counter") == 0)
		o->gauge = 0;
	else {
		errno = EINVAL;
		goto no_type;
	}

	return o;
no_type:
no_oid:
no_parse:
	free (o->name);
no_name:
	free (o);
	return NULL;
}

static struct record *get_records (FILE *conf)
{
	char *line;
	size_t len, lineno;
	int argc;
	char **argv;
	struct record *head, **tail = &head;

	for (
		line = NULL, lineno = 0, head = NULL;
		getline (&line, &len, conf) > 0;
		++lineno
	) {
		if (!g_shell_parse_argv (line, &argc, &argv, NULL)) {
			warnx ("W: cannot parse config line %zu\n", lineno);
			continue;
		}

		if ((*tail = get_record (argc, argv)) == NULL)
			warn ("W: wrong config line %zu", lineno);
		else
			tail = &(*tail)->next;

		g_strfreev (argv);
	}

	if (!feof (conf))
		warn ("W: cannot read config file");

	free (line);
	return head;
}

static netsnmp_variable_list *snmp_get_template (const struct record *list)
{
	const struct record *o;
	netsnmp_variable_list *vars;

	for (o = list, vars = NULL; o != NULL; o = o->next)
		if (snmp_varlist_add_variable (&vars, o->OID, o->OID_len,
					       ASN_NULL, NULL, 0) == NULL)
			goto no_var;

	return vars;
no_var:
	snmp_free_varbind (vars);
	return NULL;
}

static void
show_vars (const struct record *o, netsnmp_variable_list *v)
{
	for (; o != NULL; o = o->next) {
		if (snmp_oid_compare (o->OID, o->OID_len,
				      v->name, v->name_length) != 0)
			continue;

		switch (v->type) {
		case ASN_INTEGER:
		case ASN_COUNTER:
		case ASN_GAUGE:
			printf ("%s: %lu\n", o->name, *v->val.integer);
			rrdb_update ("db", o->name, o->gauge, *v->val.integer);
			break;
		}

		v = v->next_variable;
	}
}

int main (int argc, char *argv[])
{
	netsnmp_session *ss;
	struct record *list;
	netsnmp_variable_list *vars;

//	snmp_disable_log ();

	if ((ss = snmp_session_open ("snmp-mon", "10.0.26.2", "public")) == NULL)
		err (1, "E: snmp session");

	if ((list = get_records (stdin)) == NULL)
		errx (1, "I: nothing to do");

	if ((vars = snmp_get_template (list)) == NULL)
		err (1, "parse OIDs");

	if (netsnmp_query_get (vars, ss) != SNMP_ERR_NOERROR)
		err (1, "snmp query");

	show_vars (list, vars);

	snmp_free_varbind (vars);
	snmp_close (ss);
	return 0;
}