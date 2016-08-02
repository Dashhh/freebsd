/*-
 * Copyright (c) 2009-2013 The FreeBSD Foundation
 * Copyright (c) 2013-2015 Mariusz Zaborski <oshogbo@FreeBSD.org>
 * All rights reserved.
 *
 * This software was developed by Pawel Jakub Dawidek under sponsorship from
 * the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/endian.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#ifdef _KERNEL

#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/stdarg.h>

#else
#include <sys/socket.h>

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#define	_WITH_DPRINTF
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "msgio.h"
#endif

#ifdef HAVE_PJDLOG
#include <pjdlog.h>
#endif

#include <sys/nv.h>

#include "nv_impl.h"
#include "nvlist_impl.h"
#include "nvpair_impl.h"

static bool
nvlist_sysctl_format(const int *mib, size_t miblen, u_int *kind)
{
	u_int *buff;
	size_t size;

	if (sysctl(mib, miblen, NULL, &size, NULL, 0) < 0)
		return (false);

	buff = malloc(sizeof(*buff)*size);
	size = sizeof(*buff)*size;
	if (buff == NULL)
		return (false);

	if (sysctl(mib, miblen, buff, &size, NULL, 0) < 0) {
		free(buff);
		return (false);
	}

	*kind = buff[0];

	return (true);
}

static bool
nvlist_sysctl_get_binary(nvlist_t *old, const char *name, const int *mib,
    size_t miblen, size_t valsize)
{
	int st;
	size_t size;
	void *buff;

	buff = NULL;
	if ((st = sysctl(mib, miblen, NULL, &size, NULL, 0)) < 0)
		return (false);
	do {
		size += size/10;
		buff = realloc(buff, size);
		if (buff == NULL)
			goto fail;
		st = sysctl(mib, miblen, buff, &size, NULL, 0);
	} while (st == -1 && errno == ENOMEM);
	if (st == -1 || size % valsize != 0)
		goto fail;

	buff = realloc(buff, size);
	if (buff == NULL)
		goto fail;

	nvlist_move_binary(old, name, buff, size);

	return (true);
fail:
	if (buff != NULL)
		free(buff);
	return (false);
}

static bool
nvlist_sysctl_get_string(nvlist_t *old, const char *name, const int *mib,
    size_t miblen)
{
	int st;
	size_t size;
	char *buff;

	buff = NULL;
	if ((st = sysctl(mib, miblen, NULL, &size, NULL, 0)) < 0)
		return (false);
	do {
		size += size*sizeof(char)/10;
		buff = realloc(buff, size);
		if (buff == NULL)
			goto fail;
		st = sysctl(mib, miblen, buff, &size, NULL, 0);
	} while (st == -1 && errno == ENOMEM);
	if (st == -1 || size % sizeof(char) != 0)
		goto fail;

	buff = realloc(buff, size);
	if (buff == NULL)
		goto fail;

	nvlist_move_string(old, name, buff);

	return (true);
fail:
	if (buff != NULL)
		free(buff);
	return (false);
}

static bool
nvlist_sysctl_get_number(nvlist_t *old, const char *name, const int *mib,
    size_t miblen)
{
	int st;
	size_t size;
	uint64_t *buff;

	buff = NULL;
	if ((st = sysctl(mib, miblen, NULL, &size, NULL, 0)) < 0)
		return (false);
	do {
		size += size*sizeof(uint64_t)/10;
		buff = realloc(buff, size);
		if (buff == NULL)
			goto fail;
		st = sysctl(mib, miblen, buff, &size, NULL, 0);
	} while (st == -1 && errno == ENOMEM);
	if (st == -1 || size % sizeof(uint64_t) != 0)
		goto fail;

	buff = realloc(buff, size);
	if (buff == NULL)
		goto fail;

	if (size / sizeof(uint64_t) > 1) {
		nvlist_move_number_array(old, name, buff, 
		    size / sizeof(uint64_t));
	} else {
		nvlist_add_number(old, name, buff[0]);
		free(buff);
	}

	return (true);
fail:
	if (buff != NULL)
		free(buff);
	return (false);
}


int
nvlist_sysctl(const char *name, nvlist_t *old, nvlist_t *new)
{
	int mib[CTL_MAXNAME+2];
	size_t size, datalen;
	u_int kind;
	void *data;

	size = CTL_MAXNAME+2;
	mib[0] = 0;
	mib[1] = 4;
	if (sysctlnametomib(name, mib+2, &size))
		return (-1);
	size += 2;

	if (old != NULL) {
		if (!nvlist_sysctl_format(mib, size, &kind))
			return (-1);
		printf("%d\n", CTLTYPE & kind);
		switch(kind & CTLTYPE) {
		case CTLTYPE_NODE:
			break;
		case CTLTYPE_INT:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(int))) {
				return (-1);
			}
			break;
		case CTLTYPE_STRING:
			if (!nvlist_sysctl_get_string(old, name, mib+2,
			    size-2)) {
				return (-1);
			}
			break;
		case CTLTYPE_S64:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(int64_t))) {
				return (-1);
			}
			break;
		case CTLTYPE_OPAQUE:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, 1)) {
				return (-1);
			}
			break;
		case CTLTYPE_UINT:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(u_int))) {
				return (-1);
			}
			break;
		case CTLTYPE_LONG:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(long))) {
				return (-1);
			}
			break;
		case CTLTYPE_ULONG:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(unsigned long))) {
				return (-1);
			}
			break;
		case CTLTYPE_U64:
			if (!nvlist_sysctl_get_number(old, name, mib+2,
			    size-2)) {
				return (-1);
			}
			break;
		case CTLTYPE_U8:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(uint8_t))) {
				return (-1);
			}
			break;
		case CTLTYPE_U16:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(uint16_t))) {
				return (-1);
			}
			break;
		case CTLTYPE_S8:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(int8_t))) {
				return (-1);
			}
			break;
		case CTLTYPE_S16:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(int16_t))) {
				return (-1);
			}
			break;
		case CTLTYPE_S32:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(int32_t))) {
				return (-1);
			}
			break;
		case CTLTYPE_U32:
			if (!nvlist_sysctl_get_binary(old, name, mib+2,
			    size-2, sizeof(uint32_t))) {
				return (-1);
			}
			break;
		}
	}

	if (new != NULL) {
		if (!nvlist_exists_binary(new, ""))
			return (-1);

		data = nvlist_take_binary(new, "", &datalen);
		return (sysctl(mib+2, size-2, NULL, NULL, data, datalen));
	}

	return (0);
}
