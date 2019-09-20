/*
 * SNMP Query Sample
 *
 * Copyright (c) 2019 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <errno.h>
#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#define ARRAY_SIZE(a)  (sizeof (a) / sizeof ((a)[0]))

netsnmp_session *
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

netsnmp_variable_list *snmp_get_template (const char *names[])
{
	const char **p;
	oid OID[MAX_OID_LEN];
	size_t len;
	netsnmp_variable_list *vars;

	for (p = names, vars = NULL; *p != NULL; ++p) {
		len = ARRAY_SIZE (OID);

		if (!snmp_parse_oid (*p, OID, &len)) {
			snmp_perror (*p);	// remove it?!
			goto no_parse;
		}

		snmp_varlist_add_variable (&vars, OID, len, ASN_NULL, NULL, 0);
	}

	return vars;
no_parse:
	snmp_free_varbind (vars);
	return NULL;
}

static void show_vars_0 (netsnmp_variable_list *vars)
{
	for (; vars != NULL; vars = vars->next_variable)
		print_variable (vars->name, vars->name_length, vars);
}

static void show_vars_1 (netsnmp_variable_list *vars)
{
	int i;
	char *p;

	for (i = 1; vars != NULL; vars = vars->next_variable, ++i)
		switch (vars->type) {
		case ASN_OCTET_STR:
			p = malloc (vars->val_len + 1);

			memcpy (p, vars->val.string, vars->val_len);
			p[vars->val_len] = '\0';
			printf ("I: %d: %s (string)\n", i, p);
			free (p);
			break;
		case ASN_INTEGER:
			printf ("I: %d: %ld (integer)\n", i,
				*vars->val.integer);
			break;
		case ASN_COUNTER:
			printf ("I: %d: %lu (counter)\n", i,
				*vars->val.integer);
			break;
		case ASN_GAUGE:
			printf ("I: %d: %lu (gauge)\n", i,
				*vars->val.integer);
			break;
		case ASN_NULL:
			printf ("I: %d: null\n", i);
			break;
		default:
			printf ("I: %d: unknown (%02x)\n", i, vars->type);
		}
}

static const char *names[] = {
	"sysDescr.0",		/* system.sysDescr.0 or .1.3.6.1.2.1.1.1.0 */
	"IF-MIB::ifMtu.3",
	"IF-MIB::ifType.3",
	"LM-SENSORS-MIB::lmTempSensorsValue.1",
	"IPV6-MIB::ipv6IfEffectiveMtu.2",
	"IF-MIB::ifInOctets.3",
	"IF-MIB::ifOutOctets.3",
	NULL
};

#include <err.h>

int main (int argc, char *argv[])
{
	netsnmp_session *ss;
	netsnmp_variable_list *vars;

//	snmp_disable_log ();

	if ((ss = snmp_session_open ("snmp-get", "10.0.26.2", "public")) == NULL)
		err (1, "snmp session");

	if ((vars = snmp_get_template (names)) == NULL)
		err (1, "parse OIDs");

	if (netsnmp_query_get (vars, ss) != SNMP_ERR_NOERROR)
		err (1, "snmp query");

	show_vars_0 (vars);
	show_vars_1 (vars);

	snmp_free_varbind (vars);
	snmp_close (ss);
	return 0;
}
