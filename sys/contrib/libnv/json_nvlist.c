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

#include <sys/nv.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nv_impl.h"
#include "nvlist_impl.h"
#include "nvpair_impl.h"


struct stack
{
	char	**key;
	unsigned *level;
};

struct machine
{
	int		 fd;			// provided file_descriptor
	size_t		 position;		// letters read

	char		 buffer;		// last letter read
	char		*key;			// key recognised by machine
	int		 type;			// NV_TYPE
	unsigned	 level;			// nest level

	struct stack	 stack;			// nvlist array name stack
	size_t		 stack_size;		// stack max size
	size_t		 stack_position;	// stack position
};

static void
report_wrong(struct machine *machine, const char *str)
{
	fprintf(stderr, "Wrong char at %zu %s\n", machine->position, str);
	exit(-1);
}

static char
nextc(struct machine *machine)
{
	char c;
	read(machine->fd, &c, 1);
	machine->position++;
	machine->buffer = c;
	return c;
}

static void
verifyc(struct machine *machine, char expected)
{
	if (nextc(machine) != expected)
		report_wrong(machine, "verifyc");
}

static void
verify_text(struct machine *machine, const char *text)
{
	unsigned i;

	for (i = 0; i < strlen(text); i++)
		verifyc(machine, text[i]);
}

static void
clear_white(struct machine *machine)
{
	while(isspace(machine->buffer))
		nextc(machine);
}

static bool
end(struct machine *machine)
{
	return 10 == machine->buffer;
}

static void
add_nvlist_array(struct machine *machine, nvlist_t **nvl)
{
	nvlist_t **child;

	if (machine->stack_position == machine->stack_size) {
		machine->stack_size *= 2;
		machine->stack.level =
			realloc(machine->stack.level, sizeof(machine->stack)*machine->stack_size);
		machine->stack.key =
			realloc(machine->stack.key, sizeof(machine->stack)*machine->stack_size);
	}

	machine->stack.key[machine->stack_position] = strdup(machine->key);
	machine->stack.level[machine->stack_position] = machine->level;
	machine->stack_position++;

	child = malloc(sizeof(*child));
	child[0] = nvlist_create(0);
	nvlist_move_nvlist_array(*nvl, machine->key, child, 1);
	*nvl = child[0];
	machine->level++;
}

static void
update_nvlist_array(struct machine *machine, nvlist_t **nvl)
{
	nvlist_t **children;
	size_t nitems;
	char *key;

	if (!machine->stack_position ||
	    machine->level != machine->stack.level[machine->stack_position-1])
	    return;

	key = machine->stack.key[machine->stack_position-1];
	children = nvlist_take_nvlist_array(*nvl, key, &nitems);
	children = realloc(children, sizeof(*children)*(nitems+1));
	children[nitems] = nvlist_create(0);
	nvlist_move_nvlist_array(*nvl, key, children, nitems+1);
	*nvl = children[nitems];

	nextc(machine);
	clear_white(machine);
	machine->level++;
}

static void
get_array(struct machine *machine, nvlist_t **nvl)
{

	switch(machine->buffer) {
	case 'f':
		machine->type = NV_TYPE_BOOL_ARRAY;
		break;
	case 't':
		machine->type = NV_TYPE_BOOL_ARRAY;
		break;
	case '\"':
		machine->type = NV_TYPE_STRING_ARRAY;
		break;
	case '{':
		if (machine->level == 0)
			machine->level++;
		nextc(machine);
		clear_white(machine);
		machine->type = NV_TYPE_NVLIST_ARRAY;
		add_nvlist_array(machine, nvl);
		break;
	default:
		if (isdigit(machine->buffer))
			machine->type = NV_TYPE_NUMBER_ARRAY;
		else
			machine->type = NV_TYPE_NONE;
		break;
	}
}

static int
get_type(struct machine *machine, nvlist_t **nvl)
{

	switch(machine->buffer) {
	case 'n':
		machine->type = NV_TYPE_NULL;
		break;
	case 'f':
		machine->type = NV_TYPE_BOOL;
		break;
	case 't':
		machine->type = NV_TYPE_BOOL;
		break;
	case '\"':
		machine->type = NV_TYPE_STRING;
		break;
	case '{':
		nextc(machine);
		clear_white(machine);
		machine->type = NV_TYPE_NVLIST;
		machine->level++;
		break;
	case '[':
		nextc(machine);
		clear_white(machine);
		get_array(machine, nvl);
		break;
	default:
		if (isdigit(machine->buffer))
			machine->type = NV_TYPE_NUMBER;
		else
			machine->type = NV_TYPE_NONE;
		break;
	}

	return machine->type;
}

static void
insert_null(struct machine *machine, nvlist_t *nvl)
{

	verify_text(machine, "ull");
	nvlist_add_null(nvl, machine->key);
	nextc(machine);
}

static bool
fetch_bool(struct machine *machine)
{
	if (machine->buffer == 'f') {
		verify_text(machine, "alse");
		nextc(machine);
		return 0;
	}
	else {
		verify_text(machine, "rue");
		nextc(machine);
		return 1;
	}
}

static void
insert_bool(struct machine *machine, nvlist_t *nvl)
{
	bool value;

	value = fetch_bool(machine);
	nvlist_add_bool(nvl, machine->key, value);
}

//TODO: special chars
static char *
fetch_string(struct machine *machine)
{
	char *value;
	size_t size;
	unsigned i;

	i = 0;
	size = 100;
	value = malloc(sizeof(*value)*size);

	nextc(machine);
	while (machine->buffer != 10 && machine->buffer != '\"') {
		value[i] = machine->buffer;
		i++;
		if (i+1 == size) {
			value = realloc(&value, sizeof(*value)*size*2);
			size *= 2;
		}
		nextc(machine);
	}
	value[i] = '\0';
	value = realloc(value, sizeof(*value)*(strlen(value)+1));

	if (machine->buffer == 10)
		report_wrong(machine, "fetch string");

	nextc(machine);

	return (value);
}

// TODO: ADD czy MOVE?
static void
insert_string(struct machine *machine, nvlist_t *nvl)
{
	char *value;

	value = fetch_string(machine);
	nvlist_move_string(nvl, machine->key, value);
}

static uint64_t
fetch_number(struct machine *machine)
{
	uint64_t value;

	value = 0;
	if (machine->buffer == '0') {
		nextc(machine);
		return (value);
	}

	value = machine->buffer - '0';
	while (isdigit(nextc(machine)))
		value = value * 10 + machine->buffer - '0';

	return (value);
}

// TODO: check overflow
static void
insert_number(struct machine *machine, nvlist_t *nvl)
{
	uint64_t value;

	value = fetch_number(machine);
	nvlist_add_number(nvl, machine->key, value);
}

static void
insert_bool_array(struct machine *machine, nvlist_t *nvl)
{
	bool *value;
	size_t size;
	unsigned i;

	i = 0;
	size = 20;
	value = malloc(sizeof(*value)*size);

	machine->type = NV_TYPE_BOOL;
	while (machine->buffer != ']' && machine->buffer != 10) {
		if (machine->type != NV_TYPE_BOOL)
			report_wrong(machine, "bool array");

		value[i] = fetch_bool(machine);

		i++;
		if (i == size) {
			value = realloc(value, sizeof(*value)*size*2);
			size *= 2;
		}
		clear_white(machine);
		if (machine->buffer == ',') {
			nextc(machine);
			clear_white(machine);
			get_type(machine, &nvl);
		}
		else if (machine->buffer != ']')
			report_wrong(machine, "bool array 2");
	}
	if (machine->buffer == 10)
		report_wrong(machine, "bool array 3");

	value = realloc(value, sizeof(*value)*i);
	nvlist_move_bool_array(nvl, machine->key, value, i);

	machine->type = NV_TYPE_BOOL_ARRAY;
	nextc(machine);
}

static void
insert_string_array(struct machine *machine, nvlist_t *nvl)
{
	char **value;
	size_t size;
	unsigned i;

	i = 0;
	size = 20;
	value = malloc(sizeof(*value)*size);

	machine->type = NV_TYPE_STRING;
	while (machine->buffer != ']' && machine->buffer != 10) {
		if (machine->type != NV_TYPE_STRING)
			report_wrong(machine, "string array");

		value[i] = fetch_string(machine);
		clear_white(machine);

		i++;
		if (i == size) {
			value = realloc(value, sizeof(*value)*size*2);
			size *= 2;
		}
		if (machine->buffer == ',') {
			nextc(machine);
			clear_white(machine);
			get_type(machine, &nvl);
		}
		else if (machine->buffer != ']')
			report_wrong(machine, "string array 2");
	}
	if (machine->buffer == 10)
		report_wrong(machine, "string array 3");

	value = realloc(value, sizeof(value)*i);
	nvlist_move_string_array(nvl, machine->key, value, i);

	nextc(machine);
	machine->type = NV_TYPE_STRING_ARRAY;
}

static void
insert_number_array(struct machine *machine, nvlist_t *nvl)
{
	uint64_t *value;
	size_t size;
	unsigned i;

	i = 0;
	size = 20;
	value = malloc(sizeof(*value)*size);

	machine->type = NV_TYPE_NUMBER;
	while (machine->buffer != ']' && machine->buffer != 10) {
		if (machine->type != NV_TYPE_NUMBER)
			report_wrong(machine, "number array");

		value[i] = fetch_number(machine);
		clear_white(machine);

		i++;
		if (i == size) {
			value = realloc(value, sizeof(*value)*size*2);
			size *= 2;
		}
		if (machine->buffer == ',') {
			nextc(machine);
			clear_white(machine);
			get_type(machine, &nvl);
		}
		else if (machine->buffer != ']')
			report_wrong(machine, "number array 2");
	}
	if (machine->buffer == 10)
		report_wrong(machine, "number array 3");

	value = realloc(value, sizeof(*value)*i);
	nvlist_move_number_array(nvl, machine->key, value, i);

	machine->type = NV_TYPE_NUMBER_ARRAY;
	nextc(machine);
}

static void
insert_nvlist(struct machine *machine, nvlist_t **nvl)
{
	nvlist_t *child;

	child = nvlist_create(0);
	nvlist_move_nvlist(*nvl, machine->key, child);
	*nvl = child;
}

static void
insert_type(struct machine *machine, nvlist_t **nvl)
{

	switch(machine->type) {
	case NV_TYPE_NULL:
		insert_null(machine, *nvl);
		break;
	case NV_TYPE_BOOL:
		insert_bool(machine, *nvl);
		break;
	case NV_TYPE_STRING:
		insert_string(machine, *nvl);
		break;
	case NV_TYPE_NUMBER:
		insert_number(machine, *nvl);
		break;
	case NV_TYPE_BOOL_ARRAY:
		insert_bool_array(machine, *nvl);
		break;
	case NV_TYPE_STRING_ARRAY:
		insert_string_array(machine, *nvl);
		break;
	case NV_TYPE_NUMBER_ARRAY:
		insert_number_array(machine, *nvl);
		break;
	case NV_TYPE_NVLIST:
		insert_nvlist(machine, nvl);
		break;
	}
}

static void
get_key(struct machine *machine)
{
	machine->key = fetch_string(machine);
}

static void
change_level(struct machine *machine, nvlist_t **nvl)
{
	void *cookie;

	cookie = NULL;
	machine->level--;

	if (machine->level > 0) {
		*nvl = nvpair_nvlist(nvlist_get_nvpair_parent(*nvl));
		nextc(machine);
		clear_white(machine);
		if (machine->buffer == ']' && machine->stack_position) {
			free(machine->stack.key[machine->stack_position-1]);
			machine->stack_position--;
			nextc(machine);
			if (end(machine)) {
				machine->level--;
				return;
			}
			else
				clear_white(machine);
		}
		if ( machine->buffer != ',' && machine->buffer != '}')
			report_wrong(machine, "change level");
		else if (machine->buffer == ',') {
			nextc(machine);
			clear_white(machine);
			update_nvlist_array(machine, nvl);
		}
	}
}

static void
parse_nvlist(struct machine *machine, nvlist_t **nvl)
{

	while (machine->level != 0) {
		while (machine->buffer != '}') {
			if (machine->buffer != '\"')
				report_wrong(machine, "parse nvlist");

			get_key(machine);
			clear_white(machine);
			if (machine->buffer != ':')
				report_wrong(machine, "parse nvlist 2");
			nextc(machine);
			clear_white(machine);

			get_type(machine, nvl);
			insert_type(machine, nvl);
			clear_white(machine);
			free(machine->key);

			if (machine->type == NV_TYPE_NVLIST || machine->type == NV_TYPE_NVLIST_ARRAY)
				continue;

			if (machine->buffer != ',' && machine->buffer != '}')
				report_wrong(machine, "parse nvlist 3");
			if (machine->buffer != '}') {
				nextc(machine);
				clear_white(machine);
			}

		}
		change_level(machine, nvl);
	}
	nextc(machine);
}

nvlist_t *
json_to_nvlist(int fd)
{
	struct machine machine;
	nvlist_t *nvl;

	machine.fd = fd;
	machine.position = 0;
	machine.level = 0;
	machine.key = strdup("");
	machine.stack.key = malloc(sizeof(*machine.stack.level)*20);
	machine.stack.level = malloc(sizeof(*machine.stack.level)*20);
	machine.stack_position = 0;
	machine.stack_size = 20;

	nextc(&machine);
	clear_white(&machine);
	nvl = nvlist_create(0);
	get_type(&machine, &nvl);

	switch(machine.type) {
	case NV_TYPE_NONE:
		free(machine.key);
		free(machine.stack.key);
		free(machine.stack.level);
		report_wrong(&machine, "to_nvlist typ non");
		break;
	case NV_TYPE_NVLIST:
		free(machine.key);
		parse_nvlist(&machine, &nvl);
		break;
	case NV_TYPE_NVLIST_ARRAY:
		free(machine.key);
		parse_nvlist(&machine, &nvl);
		break;
	default:
		insert_type(&machine, &nvl);
		if (!end(&machine))
			report_wrong(&machine, "pojedynczy to_nvlist");
		free(machine.key);
		break;
	}
	free(machine.stack.key);
	free(machine.stack.level);
	if (!end(&machine))
		report_wrong(&machine, "koniec");

	return nvl;
}
