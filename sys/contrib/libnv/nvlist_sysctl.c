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
nvlist_sysctl_long(nvlist_t *old, const char *name, const int *mib,
    size_t miblen)
{
	int st;
	size_t size;
	long *buff;

	buff = NULL;
	if ((st = sysctl(mib, miblen, NULL, &size, NULL, 0)) < 0)
		return (false);
	do {
		size *= sizeof(long);
		buff = realloc(buff, size);
		if (buff == NULL)
			goto fail;
		st = sysctl(mib, miblen, buff, &size, NULL, 0);
	} while (st == -1 && errno == ENOMEM);
	if (st == -1 || size % sizeof(long) != 0)
		goto fail;

	buff = realloc(buff, size);
	if (buff == NULL)
		goto fail;

	if (size / sizeof(long) > 1){
		nvlist_move_number_array(old, name, buff, size / sizeof(long));
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
	if (sysctlnametomib(name, mib+2, &size)) {
		perror("tomib\n");
		return (-1);
	}
	size += 2;

	if (old != NULL) {
		if (!nvlist_sysctl_format(mib, size, &kind)) {
			perror("format\n");
			return (-1);
		}

		switch(kind & CTLTYPE) {
		case CTLTYPE_LONG:
			if (!nvlist_sysctl_long(old, name, mib+2, size-2)) {
				perror("sysctl\n");
				return (-1);
			}
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
