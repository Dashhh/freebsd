/*-
 * Copyright (c) 2013 The FreeBSD Foundation
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/stdarg.h>

#else
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#endif

#include <sys/nv.h>

#include <inttypes.h>
#include <string.h>

#include "nv_impl.h"
#include "nvlist_impl.h"
#include "nvpair_impl.h"

#define INITIAL_SIZE 100

#define BOOL_SCHEMA strlen("{key:\"\",value:,type:}")

#define KEY "key:"
#define VALUE "value:"
#define TYPE "type:"

#define START_BRACE "{"
#define START_BRACKET "["
#define END_BRACE "}"
#define END_BRACKET "]"
#define QUOTE "\""
#define COMMA ","
#define END "\0"


struct string
{
	char *ptr;
	size_t position;
	size_t size;
};

static void insert_nvlist(struct string *jstring, const nvlist_t *nvl);

static void
init_string(struct string *string)
{
	string->position = 0;
	string->size = INITIAL_SIZE;
	string->ptr = malloc(sizeof(*(string->ptr))*INITIAL_SIZE);
}

static void
expand_string(struct string *jstring, size_t size)
{
	if (jstring->size - jstring->position >= size)
		return;
	while (jstring->size - jstring->position < size) {
		jstring->ptr = realloc(jstring->ptr, sizeof(*jstring->ptr)*jstring->size*2);
		jstring->size *= 2;
	}
}

static void
insert_text(struct string *jstring, const char *text)
{
	expand_string(jstring, strlen(text));
	strcpy(jstring->ptr+jstring->position, text);
	jstring->position += strlen(text);
}

static size_t
number_size(uint64_t number)
{
	size_t size;

	size = number == 0 ? 2 : 1;
	while (number != 0) {
		size++;
		number /= 10;
	}
	return size;
}

static void
insert_type(struct string *jstring, const char *type)
{
	insert_text(jstring, START_BRACE);
	insert_text(jstring, TYPE);
	insert_text(jstring, QUOTE);
	insert_text(jstring, type);
	insert_text(jstring, QUOTE);
	insert_text(jstring, COMMA);
	insert_text(jstring, VALUE);
}

static void
insert_key(struct string *jstring, const char *key)
{
	insert_text(jstring, COMMA);
	insert_text(jstring, KEY);
	insert_text(jstring, QUOTE);
	insert_text(jstring, key);
	insert_text(jstring, QUOTE);
	insert_text(jstring, END_BRACE);
	insert_text(jstring, COMMA);

}

static void
insert_null(struct string *jstring)
{
	insert_text(jstring, QUOTE);
	insert_text(jstring, QUOTE);
}

static void
insert_bool(struct string *jstring, const bool value)
{
	if (value)
		insert_text(jstring, "1");
	else
		insert_text(jstring, "0");
}

// Co gdy asprintf sie nie powiedzie?
static void
insert_number(struct string *jstring, uint64_t number)
{
	size_t size;

	size = number_size(number);
	expand_string(jstring, size);
	snprintf(jstring->ptr+jstring->position, size, "%"PRIu64, number);
	jstring->position += size-1;
}

static void
insert_string(struct string *jstring, const char *string)
{
	size_t size;

	size = strlen(string)+1;
	insert_text(jstring, QUOTE);
	expand_string(jstring, size);
	snprintf(jstring->ptr+jstring->position, size, "%s", string);
	jstring->position += size-1;
	insert_text(jstring, QUOTE);
}

static void
insert_descriptor(struct string *jstring, int fd)
{
	insert_number(jstring, fd);
}

static void
insert_bool_array(struct string *jstring, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const bool *bools;

	bools = nvpair_get_bool_array(nvp, &size);
	insert_text(jstring, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_bool(jstring, bools[i]);
		insert_text(jstring, COMMA);
	}
	insert_bool(jstring, bools[i]);
	insert_text(jstring, END_BRACKET);
}

static void
insert_number_array(struct string *jstring, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const uint64_t *numbers;

	numbers = nvpair_get_number_array(nvp, &size);
	insert_text(jstring, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_number(jstring, numbers[i]);
		insert_text(jstring, COMMA);
	}
	insert_number(jstring, numbers[i]);
	insert_text(jstring, END_BRACKET);
}

static void
insert_string_array(struct string *jstring, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const char * const *strings;

	strings = nvpair_get_string_array(nvp, &size);
	insert_text(jstring, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_string(jstring, strings[i]);
		insert_text(jstring, COMMA);
	}
	insert_string(jstring, strings[i]);
	insert_text(jstring, END_BRACKET);
}

static void
insert_descriptor_array(struct string *jstring, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const int *descriptors;

	descriptors = nvpair_get_descriptor_array(nvp, &size);
	insert_text(jstring, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_descriptor(jstring, descriptors[i]);
		insert_text(jstring, COMMA);
	}
	insert_descriptor(jstring, descriptors[i]);
	insert_text(jstring, END_BRACKET);
}

static void
insert_nvlist_array(struct string *jstring, const nvpair_t *nvp)
{
	size_t size;
	unsigned i;
	const nvlist_t * const *nvlists;

	nvlists = nvpair_get_nvlist_array(nvp, &size);
	insert_text(jstring, START_BRACKET);
	for (i = 0; i < size-1; i++) {
		insert_nvlist(jstring, nvlists[i]);
		insert_text(jstring, COMMA);
	}
	insert_nvlist(jstring, nvlists[i]);
	insert_text(jstring, END_BRACKET);
}

static void
insert_nvlist(struct string *jstring, const nvlist_t *nvl)
{
	void *cookie;
	int type;
	cookie = NULL;

	insert_text(jstring, START_BRACKET);
	while (nvlist_next(nvl, &type, &cookie) != NULL) {
		insert_type(jstring, nvpair_type_string(type));
		switch (type) {
		case NV_TYPE_NULL:
			insert_null(jstring);
			break;
		case NV_TYPE_BOOL:
			insert_bool(jstring, nvpair_get_bool(cookie));
			break;
		case NV_TYPE_NUMBER:
			insert_number(jstring, nvpair_get_number(cookie));
			break;
		case NV_TYPE_STRING:
			insert_string(jstring, nvpair_get_string(cookie));
			break;
		case NV_TYPE_NVLIST:
			insert_nvlist(jstring, nvpair_get_nvlist(cookie));
			break;
		case NV_TYPE_DESCRIPTOR:
			insert_descriptor(jstring, nvpair_get_descriptor(cookie));
			break;
		case NV_TYPE_BOOL_ARRAY:
			insert_bool_array(jstring, cookie);
			break;
		case NV_TYPE_NUMBER_ARRAY:
			insert_number_array(jstring, cookie);
			break;
		case NV_TYPE_STRING_ARRAY:
			insert_string_array(jstring, cookie);
			break;
		case NV_TYPE_NVLIST_ARRAY:
			insert_nvlist_array(jstring, cookie);
			break;
		case NV_TYPE_DESCRIPTOR_ARRAY:
			insert_descriptor_array(jstring, cookie);
			break;
		default:
			break;
		}
		insert_key(jstring, nvpair_name(cookie));
		printf("%s\n\n", jstring->ptr);
	}
	if (jstring->ptr[jstring->position-1] == *COMMA)
		jstring->position--;
	insert_text(jstring, END_BRACKET);
	insert_text(jstring, END);
}

char *
nvlist_to_json(const nvlist_t *nvl)
{
	struct string jstring;

	init_string(&jstring);
	insert_nvlist(&jstring, nvl);

	return (jstring.ptr);
}
