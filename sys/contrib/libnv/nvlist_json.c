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

struct json
{
	char *string;
	size_t position;
	size_t size;
};

static void
int_to_string(int number, char *space)
{
	memset(space, 0, sizeof(*space)*65);
	snprintf(space, 65, "%d", number);
}

static void
number_to_string(uint64_t number, char *space)
{
	memset(space, 0, sizeof(*space)*65);
	snprintf(space, 65, "%" PRIu64, number);
}

static char *
preprocess_string(const char *value)
{
	size_t size;
	char *result;

	size = strlen(value)+3;
	result = malloc(sizeof(*result)*size);
	result[0] = *QUOTE;
	strcpy(result+1, value);
	result[size-2] = *QUOTE;
	result[size-1] = *END;
	return result;
}

static void
expand_string(struct json *jstring, size_t size)
{
	if (jstring->size - jstring->position >= size)
		return;
	while (jstring->size - jstring->position < size) {
		jstring->string = realloc(jstring->string, sizeof(*jstring->string)*jstring->size*2);
		jstring->size *= 2;
	}
}

static void
insert_text(struct json *jstring, const char *text)
{
	expand_string(jstring, strlen(text));
	strcpy(jstring->string+jstring->position, text);
	jstring->position += strlen(text);
}


static void
single_insert(struct json *jstring, const char *name, const char *value, const char *type)
{
	insert_text(jstring, START_BRACE);
	insert_text(jstring, KEY);
	insert_text(jstring, QUOTE);
	insert_text(jstring, name);
	insert_text(jstring, QUOTE);
	insert_text(jstring, COMMA);
	insert_text(jstring, VALUE);
	insert_text(jstring, value);
	insert_text(jstring, COMMA);
	insert_text(jstring, TYPE);
	insert_text(jstring, type);
	insert_text(jstring, END_BRACE);
	insert_text(jstring, COMMA);
}

static void
parse_nvlist(const nvlist_t *nvl, struct json *jstring)
{
	void *cookie;
	int type;
	char number[65], stype[65];
	char *string;
	const char *name;
	cookie = NULL;

	insert_text(jstring, START_BRACKET);
	while (nvlist_next(nvl, &type, &cookie) != NULL) {
		name = nvpair_name(cookie);
		int_to_string(type, stype);
		switch (type) {
		case NV_TYPE_NULL:
			single_insert(jstring, name, "\"\"", stype);
			break;
		case NV_TYPE_BOOL:
			int_to_string(nvpair_get_bool(cookie), number);
			single_insert(jstring, name, number, stype);
			break;
		case NV_TYPE_NUMBER:
			number_to_string(nvpair_get_number(cookie), number);
			single_insert(jstring, name, number, stype);
			break;
		case NV_TYPE_STRING:
			string = preprocess_string(nvpair_get_string(cookie));
			single_insert(jstring, name, string, stype);
			free(string);
			break;
		default:
			break;
		}
		printf("dodano\n");
	}
	jstring->position--;
	insert_text(jstring, END_BRACKET);
	insert_text(jstring, END);
	printf("Wykorzystano %zu\n", jstring->position);
}

char *
nvlist_to_json(const nvlist_t *nvl)
{
	struct json jstring;

	jstring.position = 0;
	jstring.size = INITIAL_SIZE;
	jstring.string = malloc(sizeof(*jstring.string)*INITIAL_SIZE);

	if (nvl != NULL)
		parse_nvlist(nvl, &jstring);
	return jstring.string;
}
