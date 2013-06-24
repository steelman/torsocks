/*
 * Copyright (C) 2000-2008 - Shaun Clowes <delius@progsoc.org> 
 * 				 2008-2011 - Robert Hogan <robert@roberthogan.net>
 * 				 	  2013 - David Goulet <dgoulet@ev0ke.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 only, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>

#include <common/log.h>

#include "torsocks.h"
/*
 * Torsocks call for gethostbyname(3).
 *
 * NOTE: This call is OBSOLETE in the glibc.
 */
LIBC_GETHOSTBYNAME_RET_TYPE tsocks_gethostbyname(LIBC_GETHOSTBYNAME_SIG)
{
	int ret;
	uint32_t ip;
	const char *ret_str;

	DBG("[gethostbyname] Requesting %s hostname", __name);

	if (!__name) {
		h_errno = HOST_NOT_FOUND;
		goto error;
	}

	/* Resolve the given hostname through Tor. */
	ret = tsocks_tor_resolve(__name, &ip);
	if (ret < 0) {
		goto error;
	}

	/* Reset static host entry of tsocks. */
	memset(&tsocks_he, 0, sizeof(tsocks_he));
	memset(tsocks_he_addr_list, 0, sizeof(tsocks_he_addr_list));
	memset(tsocks_he_addr, 0, sizeof(tsocks_he_addr));

	ret_str = inet_ntop(AF_INET, &ip, tsocks_he_addr, sizeof(tsocks_he_addr));
	if (!ret_str) {
		PERROR("inet_ntop");
		h_errno = NO_ADDRESS;
		goto error;
	}

	tsocks_he_addr_list[0] = tsocks_he_addr;
	tsocks_he_addr_list[1] = NULL;

	tsocks_he.h_name = (char *) __name;
	tsocks_he.h_aliases = NULL;
	tsocks_he.h_length = sizeof(in_addr_t);
	tsocks_he.h_addrtype = AF_INET;
	tsocks_he.h_addr_list = tsocks_he_addr_list;

	DBG("Hostname %s resolved to %s", __name, tsocks_he_addr);

	errno = 0;
	return &tsocks_he;

error:
	return NULL;
}

/*
 * Libc hijacked symbol gethostbyname(3).
 */
LIBC_GETHOSTBYNAME_DECL
{
	return tsocks_gethostbyname(LIBC_GETHOSTBYNAME_ARGS);
}

/*
 * Torsocks call for gethostbyname2(3).
 *
 * This call, like gethostbyname(), returns pointer to static data thus is
 * absolutely not reentrant.
 */
LIBC_GETHOSTBYNAME2_RET_TYPE tsocks_gethostbyname2(LIBC_GETHOSTBYNAME2_SIG)
{
	/*
	 * For now, there is no way of resolving a domain name to IPv6 through Tor
	 * so only accept INET request thus using the original gethostbyname().
	 */
	if (__af != AF_INET) {
		h_errno = HOST_NOT_FOUND;
		return NULL;
	}

	return tsocks_gethostbyname(__name);
}

/*
 * Libc hijacked symbol gethostbyname2(3).
 */
LIBC_GETHOSTBYNAME2_DECL
{
	return tsocks_gethostbyname2(LIBC_GETHOSTBYNAME2_ARGS);
}
