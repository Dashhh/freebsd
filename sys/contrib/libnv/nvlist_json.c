/*-
 * Copyright (c) 2016 Adam Starak <starak.adam@gmail.com>
 * All rights reserved.
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

#ifdef _KERNEL

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/stdarg.h>

#else
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#define	_WITH_DPRINTF
#include <stdio.h>
#include <stdlib.h>
#endif

#include <sys/nv.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include "nv_impl.h"
#include "nvlist_impl.h"
#include "nvpair_impl.h"

#define INITIAL_SIZE 100

#define START_BRACE "{"
#define START_BRACKET "["
#define END_BRACE "}"
#define END_BRACKET "]"
#define APOSTROPHE "\'"
#define COLON ":"
#define QUOTE "\""
#define COMMA ","

static void
insert_char(int fd, char c)
{
	switch(c) {
	case '\"':
		dprintf(fd, "\\\"");
		break;
	case '\\':
		dprintf(fd, "\\\\");
		break;
	case '\b':
		dprintf(fd, "\\b");
		break;
	case '\f':
		dprintf(fd, "\\f");
		break;
	case '\n':
		dprintf(fd, "\\n");
		break;
	case '\r':
		dprintf(fd, "\\r");
		break;
	case '\t':
		dprintf(fd, "\\t");
		break;
	default:
		dprintf(fd, "%c", c);
		break;
	}
}

static void
insert_text(int fd, const char *text)
{

	dprintf(fd, "%s", text);
}

static void
insert_key(int fd, const char *key)
{
	size_t i;

	insert_text(fd, QUOTE);
	for (i = 0; i < strlen(key); i++)
		insert_char(fd, key[i]);	
	insert_text(fd, QUOTE);
	insert_text(fd, COLON);
}

static void
insert_null(int fd)
{
	dprintf(fd, "null");
}

static void
insert_bool(int fd, const bool value)
{

	if (value)
		insert_text(fd, "true");
	else
		insert_text(fd, "false");
}

static void
insert_number(int fd, uint64_t number)
{

	dprintf(fd, "%"PRIu64, number);
}

static void
insert_string(int fd, const char *string)
{
	unsigned i;

	insert_text(fd, QUOTE);
	for (i = 0; i < strlen(string); i++)
		insert_char(fd, string[i]);
	insert_text(fd, QUOTE);
}

static void
insert_bool_array(int fd, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const bool *bools;

	bools = nvpair_get_bool_array(nvp, &size);
	insert_text(fd, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_bool(fd, bools[i]);
		insert_text(fd, COMMA);
	}
	insert_bool(fd, bools[i]);
	insert_text(fd, END_BRACKET);
}

static void
insert_binary(int fd, const nvpair_t *nvp)
{
	const unsigned char *binary;
	unsigned int i;
	size_t size;

	binary = nvpair_get_binary(nvp, &size);
	for (i = 0; i < size; i++)
		dprintf(fd, "%02hhx", binary[i]);
}

static void
insert_number_array(int fd, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const uint64_t *numbers;

	numbers = nvpair_get_number_array(nvp, &size);
	insert_text(fd, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_number(fd, numbers[i]);
		insert_text(fd, COMMA);
	}
	insert_number(fd, numbers[i]);
	insert_text(fd, END_BRACKET);
}

static void
insert_string_array(int fd, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const char * const *strings;

	strings = nvpair_get_string_array(nvp, &size);
	insert_text(fd, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_string(fd, strings[i]);
		insert_text(fd, COMMA);
	}
	insert_string(fd, strings[i]);
	insert_text(fd, END_BRACKET);
}

static void
insert_nvlist(const nvlist_t *nvl, int fd)
{
	const nvlist_t *tmpnvl;
	nvpair_t *nvp, *tmpnvp;
	void *cookie;

	insert_text(fd, START_BRACE);
	nvp = nvlist_first_nvpair(nvl);
	if (nvp == NULL) {
		insert_text(fd, END_BRACE);
		return;
	}
	while (nvp != NULL) {
		if (nvlist_first_nvpair(nvl) != nvp)
			insert_text(fd, COMMA);
		insert_key(fd, nvpair_name(nvp));
		switch (nvpair_type(nvp)) {
		case NV_TYPE_NULL:
			insert_null(fd);
			break;
		case NV_TYPE_BOOL:
			insert_bool(fd, nvpair_get_bool(nvp));
			break;
		case NV_TYPE_NUMBER:
			insert_number(fd, nvpair_get_number(nvp));
			break;
		case NV_TYPE_STRING:
			insert_string(fd, nvpair_get_string(nvp));
			break;
		case NV_TYPE_NVLIST:
			insert_text(fd, START_BRACE);
			tmpnvl = nvpair_get_nvlist(nvp);
			tmpnvp = nvlist_first_nvpair(tmpnvl);
			if (tmpnvp != NULL) {
				nvl = tmpnvl;
				nvp = tmpnvp;
				continue;
			}
			break;
		case NV_TYPE_BINARY:
		    {
			insert_binary(fd, nvp);
			break;
		    }
		case NV_TYPE_BOOL_ARRAY:
		    {
			insert_bool_array(fd, nvp);
			break;
		    }
		case NV_TYPE_STRING_ARRAY:
		    {
			insert_string_array(fd, nvp);
			break;
		    }
		case NV_TYPE_NUMBER_ARRAY:
		    {
			insert_number_array(fd, nvp);
			break;
		    }
		case NV_TYPE_NVLIST_ARRAY:
		    {
			const nvlist_t * const *value;
			unsigned int ii;
			size_t nitems;

			insert_text(fd, START_BRACKET);
			value = nvpair_get_nvlist_array(nvp, &nitems);
			tmpnvl = NULL;
			tmpnvp = NULL;
			for (ii = 0; ii < nitems; ii++) {
				if (tmpnvl == NULL) {
					tmpnvp = nvlist_first_nvpair(value[ii]);
					if (tmpnvp != NULL)
						tmpnvl = value[ii];
					else
						ii == nitems-1 ? dprintf(fd, "{}") : dprintf(fd, "{},");
				}
			}
			if (tmpnvp != NULL) {
				nvl = tmpnvl;
				nvp = tmpnvp;
				insert_text(fd, START_BRACE);
				continue;
			}
			else
				insert_text(fd, END_BRACE);
			break;
		    }
		}
		while ((nvp = nvlist_next_nvpair(nvl, nvp)) == NULL) {
			do {
				cookie = NULL;
				nvl = nvlist_get_pararr(nvl, &cookie);
				if (nvl == NULL) {
					insert_text(fd, END_BRACE);
					return;
				}
				if (nvlist_in_array(nvl) && cookie == NULL) {
					nvp = nvlist_first_nvpair(nvl);
					dprintf(fd, "},{");
				} else {
					nvp = cookie;
					insert_text(fd, END_BRACE);
					if (nvpair_type(nvp) == NV_TYPE_NVLIST_ARRAY)
						insert_text(fd, END_BRACKET);
				}
			} while (nvp == NULL);
			if (nvlist_in_array(nvl) && cookie == NULL)
				break;
		}
	}
}

void
nvlist_to_json(const nvlist_t *nvl, int fd)
{

	if (nvlist_error(nvl) != 0) {
		ERRNO_SET(nvlist_error(nvl));
		return;
	}

	if (nvlist_ndescriptors(nvl) > 0) {
		ERRNO_SET(EOPNOTSUPP);
		return;
	}

	insert_nvlist(nvl, fd);
}
