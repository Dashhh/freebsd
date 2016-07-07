/*- * Copyright (c) 2016 Adam Starak <starak.adam@gmail.com>
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
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nv_impl.h"
#include "nvlist_impl.h"
#include "nvpair_impl.h"

#define DNDEBUG 0
#define CHECK(name, expect, effect) do {				\
	if (name == expect)						\
		effect;							\
} while (0)
#define CHECK_INEQUAL(name, expect, effect) do {			\
	if (name != expect)						\
		effect;							\
} while (0)
#define PERROR(text, pos) do {						\
	fprintf(stderr, "%s (%zu)\n", text, pos);			\
} while (0)

#define DEBUG(func, text) do {						\
	if (DNDEBUG)							\
		fprintf(stderr, "[%s] %s\n", func, text);		\
} while (0)
#define DEBUG_INSERT(key, type) do {					\
	if (DNDEBUG)							\
		fprintf(stderr, "Dodalem {\"%s\" : %s}\n", key, type);	\
} while (0)

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
	bool		 nv_array;		// true if json is an array of nvlists

	struct stack	 stack;			// nvlist array name stack
	size_t		 stack_size;		// stack max size
	size_t		 stack_position;	// stack position
};

static int
nextc(struct machine *machine)
{
	char c;
	int byte;

	byte = read(machine->fd, &c, 1);
	machine->position++;
	machine->buffer = c;
	if (byte == -1) {
		PERROR("Cannot read next byte", machine->position);
	}

	return byte;
}

static bool
verifyc(struct machine *machine, char expected)
{
	int value;

	value = nextc(machine);
	CHECK(value, -1, return false);
	if (value == 0) {
		PERROR("Expected character at", machine->position);
		return false;
	}
	CHECK(machine->buffer, expected, return true);
	PERROR("Expected other character at", machine->position);
	return false;
}

static bool
verify_text(struct machine *machine, const char *text)
{
	unsigned i;

	for (i = 0; i < strlen(text); i++)
		CHECK(verifyc(machine, text[i]), false, return false);
	return true;
}

static int
end(struct machine *machine)
{
	int value;
	value = nextc(machine);
	CHECK(value, -1, return -1);
	return value ? 0 : 1;
}

static int
clear_white(struct machine *machine)
{
	int value;

	while(isspace(machine->buffer)) {
		value = nextc(machine);
		CHECK(value, -1, return -1);
		CHECK(value, 0, return 0);
	}
	return 1;
}

static void
clear_machine(struct machine *machine)
{
	unsigned i;

	if (machine->stack.key != NULL) {
		for (i = 0; i < machine->stack_position; i++)
			if (machine->stack.key[i] != NULL)
				free(machine->stack.key[i]);
		free(machine->stack.key);
	}
	if (machine->stack.level != NULL)
		free(machine->stack.level);
	if (machine->key != NULL)
		free(machine->key);
}

static bool
add_nvlist_array(struct machine *machine, nvlist_t **nvl)
{
	nvlist_t **child;

	if (machine->stack_position == machine->stack_size) {
		machine->stack_size *= 2;
		machine->stack.level =
			realloc(machine->stack.level, sizeof(machine->stack)*machine->stack_size);
		CHECK(machine->stack.level, NULL, return false);

		machine->stack.key =
			realloc(machine->stack.key, sizeof(machine->stack)*machine->stack_size);
		CHECK(machine->stack.key, NULL, return false);
	}

	machine->stack.key[machine->stack_position] = strdup(machine->key);
	CHECK(machine->stack.key[machine->stack_position], NULL, return false);
	machine->stack.level[machine->stack_position] = machine->level;
	machine->stack_position++;

	child = malloc(sizeof(*child));
	if (child == NULL) {
		free(child);
		return false;
	}

	child[0] = nvlist_create(0);
	if (child[0] == NULL) {
		free(child);
		return false;
	}

	nvlist_move_nvlist_array(*nvl, machine->key, child, 1);

	*nvl = child[0];
	machine->level++;

	DEBUG("add_nvlist_array", "dodaje nvliste do arraya");
	return true;
}

static int
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
		CHECK(nextc(machine), -1, return -1);
		CHECK(clear_white(machine), -1, return -1);
		machine->type = NV_TYPE_NVLIST_ARRAY;
		add_nvlist_array(machine, nvl);
		break;
	default:
		if (isdigit(machine->buffer))
			machine->type = NV_TYPE_NUMBER_ARRAY;
		else {
			PERROR("Error checking array type", machine->position);
			machine->type = -1;
		}
		break;
	}
	return machine->type;
}

static int
get_type(struct machine *machine, nvlist_t **nvl)
{

	DEBUG("get_type", "sprawdzam typ");
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
		CHECK(nextc(machine), -1, return -1);
		CHECK(clear_white(machine), -1, return -1);
		machine->type = NV_TYPE_NVLIST;
		machine->level++;
		break;
	case '[':
		CHECK(nextc(machine), -1, return -1);
		CHECK(clear_white(machine), -1, return -1);
		CHECK(get_array(machine, nvl), -1, return -1);
		break;
	default:
		if (isdigit(machine->buffer))
			machine->type = NV_TYPE_NUMBER;
		else if (end(machine))
			machine->type = NV_TYPE_NONE;
		else {
			PERROR("Error checking array type", machine->position);
			return -1;
		}
		break;
	}

	DEBUG("get_type", nvpair_type_string(machine->type));

	return machine->type;
}

static bool
insert_null(struct machine *machine, nvlist_t *nvl)
{

	CHECK(verify_text(machine, "ull"), false, return false);

	nvlist_add_null(nvl, machine->key);

	DEBUG_INSERT(machine->key, "null");

	return nextc(machine) > -1 ? true : false;
}

static int
fetch_bool(struct machine *machine)
{
	if (machine->buffer == 'f') {
		CHECK(verify_text(machine, "alse"), false, return -1);
		CHECK(nextc(machine), -1, return -1);
		return false;
	}
	else {
		CHECK(verify_text(machine, "rue"), false, return -1);
		CHECK(nextc(machine), -1, return -1);
		return true;
	}
}

static bool
insert_bool(struct machine *machine, nvlist_t *nvl)
{
	int value;

	value = fetch_bool(machine);
	CHECK(value, -1, return 0);

	nvlist_add_bool(nvl, machine->key, value);

	if (DNDEBUG && value) {
		DEBUG_INSERT(machine->key, "true");
	}
	else if (DNDEBUG)
		DEBUG_INSERT(machine->key, "false");

	return true;
}

static char
insert_special(struct machine *machine)
{
	if (nextc(machine) != 1) {
		PERROR("Expected other character at", machine->position);
		return (-1);
	}
	switch(machine->buffer) {
	case '\"':
		return ('\"');
	case '\\':
		return ('\\');
	case 'b':
		return ('\b');
	case 'f':
		return ('\f');
	case 'n':
		return ('\n');
	case 'r':
		return ('\r');
	case 't':
		return ('\t');
	default:
		PERROR("Expected other character at", machine->position);
	}
	return (-1);
}

static char *
fetch_string(struct machine *machine)
{
	char *value;
	size_t size;
	unsigned i;

	i = 0;
	size = 100;
	value = malloc(sizeof(*value)*size);
	CHECK(value, NULL, goto failed);

	CHECK_INEQUAL(nextc(machine), 1, goto failed);
	while (machine->buffer != '\"') {
		value[i] = machine->buffer;
		if (value[i] == '\\') {
			value[i] = insert_special(machine);
			CHECK(value[i], -1, goto failed);
		}
		i++;
		if (i+1 == size) {
			value = realloc(&value, sizeof(*value)*size*2);
			CHECK(value, NULL, goto failed);
			size *= 2;
		}
		CHECK_INEQUAL(nextc(machine), 1, goto failed);
	}
	value[i] = '\0';
	value = realloc(value, sizeof(*value)*(strlen(value)+1));
	CHECK(value, NULL, goto failed);

	CHECK(nextc(machine), -1, goto failed);

	return (value);

failed:
	PERROR("Error parsing string", machine->position);
	if (value != NULL)
		free(value);
	return NULL;
}

static bool
insert_string(struct machine *machine, nvlist_t *nvl)
{
	char *value;

	value = fetch_string(machine);
	CHECK(value, NULL, return false);

	nvlist_move_string(nvl, machine->key, value);

	DEBUG_INSERT(machine->key, "string");
	return true;
}

static bool
overflow(uint64_t value, char digit)
{
	uint64_t max = ULLONG_MAX/10 - digit + '0';
	return value <= max ? false : true;
}

static uint64_t
fetch_number(struct machine *machine, uint64_t *result)
{
	uint64_t value;

	value = 0;
	if (machine->buffer == '0') {
		CHECK(nextc(machine), -1, return false);
		*result = 0;
		return true;
	}

	value = machine->buffer - '0';
	CHECK(nextc(machine), -1, return false);
	while (isdigit(machine->buffer)) {
		if (overflow(value, machine->buffer)) {
			PERROR("Overflow at", machine->position);
			return false;
		}
		value = value * 10 + machine->buffer - '0';
		CHECK(nextc(machine), -1, return false);
	}
	*result = value;

	return true;
}

static bool
insert_number(struct machine *machine, nvlist_t *nvl)
{
	uint64_t value;

	value = 0;
	if (!fetch_number(machine, &value)) {
		PERROR("Error fetching number", machine->position);
		return false;
	}
	nvlist_add_number(nvl, machine->key, value);
	DEBUG_INSERT(machine->key, "number");

	return true;
}

static bool
insert_bool_array(struct machine *machine, nvlist_t *nvl)
{
	bool *value;
	int tmp;
	size_t size;
	unsigned i;

	i = 0;
	size = 20;
	value = malloc(sizeof(*value)*size);
	CHECK(value, NULL, goto failed);

	machine->type = NV_TYPE_BOOL;
	while (machine->buffer != ']') {
		if (machine->type != NV_TYPE_BOOL) {
			PERROR("Wrong type in array, expected bool at", machine->position);
			goto failed;
		}

		tmp = fetch_bool(machine);
		CHECK(tmp, -1, goto failed);
		value[i] = tmp;

		i++;
		if (i == size) {
			value = realloc(value, sizeof(*value)*size*2);
			CHECK(value, NULL, goto failed);
			size *= 2;
		}
		CHECK(clear_white(machine), -1, goto failed);
		if (machine->buffer == ',') {
			CHECK(nextc(machine), -1, goto failed);
			CHECK(clear_white(machine), -1, goto failed);
			CHECK(get_type(machine, &nvl), -1, goto failed);
		}
		else if (machine->buffer != ']') {
			PERROR("Wrong char at", machine->position);
		}
	}

	value = realloc(value, sizeof(*value)*i);
	CHECK(value, NULL, goto failed);

	nvlist_move_bool_array(nvl, machine->key, value, i);
	DEBUG_INSERT(machine->key, "bool_array");

	machine->type = NV_TYPE_BOOL_ARRAY;
	CHECK(nextc(machine), -1, goto failed);
	return true;

failed:
	if (value != NULL)
		free(value);
	return false;
}

static bool
insert_string_array(struct machine *machine, nvlist_t *nvl)
{
	char **value;
	size_t size;
	unsigned i, j;

	i = 0;
	size = 20;
	value = malloc(sizeof(*value)*size);
	CHECK(value, NULL, goto failed);

	machine->type = NV_TYPE_STRING;
	while (machine->buffer != ']') {
		if (machine->type != NV_TYPE_STRING) {
			PERROR("Wrong type in array, expected bool at", machine->position);
			goto failed;
		}

		value[i] = fetch_string(machine);
		CHECK(value[i], NULL, goto failed);
		CHECK(clear_white(machine), -1, goto failed);

		i++;
		if (i == size) {
			value = realloc(value, sizeof(*value)*size*2);
			CHECK(value, NULL, goto failed);
			size *= 2;
		}
		if (machine->buffer == ',') {
			CHECK(nextc(machine), -1, goto failed);
			CHECK(clear_white(machine), -1, goto failed);
			CHECK(get_type(machine, &nvl), -1, goto failed);
		}
		else if (machine->buffer != ']') {
			PERROR("Wrong character at", machine->position);
			goto failed;
		}
	}

	value = realloc(value, sizeof(value)*i);
	CHECK(value, NULL, goto failed);

	nvlist_move_string_array(nvl, machine->key, value, i);
	DEBUG_INSERT(machine->key, "string_array");

	CHECK(nextc(machine), -1, goto failed);
	machine->type = NV_TYPE_STRING_ARRAY;

	return true;

failed:
	j = 0;
	if (value != NULL) {
		for (j = 0; j <= i; j++)
			if (value[j] != NULL)
				free(value[j]);
		free(value);
	}

	return false;
}

static bool
insert_number_array(struct machine *machine, nvlist_t *nvl)
{
	uint64_t *value;
	size_t size;
	unsigned i;

	i = 0;
	size = 20;
	value = malloc(sizeof(*value)*size);
	CHECK(value, NULL, goto failed);

	machine->type = NV_TYPE_NUMBER;
	while (machine->buffer != ']') {
		if (machine->type != NV_TYPE_NUMBER) {
			PERROR("Wrong type in array, expected bool at", machine->position);
			goto failed;
		}

		CHECK(fetch_number(machine, &value[i]), false, goto failed);
		CHECK(clear_white(machine), -1, goto failed);

		i++;
		if (i == size) {
			value = realloc(value, sizeof(*value)*size*2);
			CHECK(value, NULL, goto failed);
			size *= 2;
		}
		if (machine->buffer == ',') {
			CHECK(nextc(machine), -1, goto failed);
			CHECK(clear_white(machine), -1, goto failed);
			CHECK(get_type(machine, &nvl), -1, goto failed);
		}
		else if (machine->buffer != ']') {
			PERROR("Wrong character at", machine->position);
			goto failed;
		}
	}

	value = realloc(value, sizeof(*value)*i);
	CHECK(value, NULL, goto failed);

	nvlist_move_number_array(nvl, machine->key, value, i);

	machine->type = NV_TYPE_NUMBER_ARRAY;
	CHECK(nextc(machine), -1, goto failed);
	DEBUG_INSERT(machine->key, "number_array");
	return true;

failed:
	if (value != NULL)
		free(value);
	return false;
}

static bool
insert_nvlist(struct machine *machine, nvlist_t **nvl)
{
	nvlist_t *child;

	child = nvlist_create(0);

	CHECK(child, NULL, return false);

	nvlist_move_nvlist(*nvl, machine->key, child);

	*nvl = child;

	DEBUG_INSERT(machine->key, "nvlist");
	return true;
}

static bool
insert_type(struct machine *machine, nvlist_t **nvl)
{

	switch(machine->type) {
	case NV_TYPE_NULL:
		return insert_null(machine, *nvl) ? true : false;
		break;
	case NV_TYPE_BOOL:
		return insert_bool(machine, *nvl) ? true : false;
		break;
	case NV_TYPE_STRING:
		return insert_string(machine, *nvl) ? true : false;
		break;
	case NV_TYPE_NUMBER:
		return insert_number(machine, *nvl) ? true : false;
		break;
	case NV_TYPE_BOOL_ARRAY:
		return insert_bool_array(machine, *nvl) ? true : false;
		break;
	case NV_TYPE_STRING_ARRAY:
		return insert_string_array(machine, *nvl) ? true : false;
		break;
	case NV_TYPE_NUMBER_ARRAY:
		return insert_number_array(machine, *nvl) ? true : false;
		break;
	case NV_TYPE_NVLIST:
		return insert_nvlist(machine, nvl) ? true : false;
		break;
	}
	return true;
}

static bool
get_key(struct machine *machine)
{

	if (machine->key != NULL)
		free(machine->key);
	machine->key = fetch_string(machine);
	if (machine->key == NULL)
		return false;
	return true;
}

static bool
update_nvlist_array(struct machine *machine, nvlist_t **nvl)
{
	nvlist_t **children;
	size_t nitems, i;
	char *key;

	if (!machine->stack_position ||
	    machine->level != machine->stack.level[machine->stack_position-1])
	    return true;

	key = machine->stack.key[machine->stack_position-1];
	children = nvlist_take_nvlist_array(*nvl, key, &nitems);

	children = realloc(children, sizeof(*children)*(nitems+1));
	CHECK(children, NULL, goto failed);
	children[nitems] = nvlist_create(0);
	CHECK(children[nitems], NULL, goto failed);
	nvlist_move_nvlist_array(*nvl, key, children, nitems+1);
	*nvl = children[nitems];

	CHECK_INEQUAL(nextc(machine), 1, return false);
	CHECK(clear_white(machine), -1, return false);
	machine->level++;

	return true;

failed:
	if (children != NULL) {
		for (i = 0; i <= nitems; i++) {
			if (children[i] != NULL)
				nvlist_destroy(children[i]);
		}
		free(children);
	}
	return false;
}

static bool
change_level(struct machine *machine, nvlist_t **nvl)
{

	machine->level--;

	if (machine->level > 0) {
		*nvl = nvpair_nvlist(nvlist_get_nvpair_parent(*nvl));
		CHECK_INEQUAL(nextc(machine), 1, return false);
		CHECK(clear_white(machine), -1, return false);
		if (machine->buffer == ']' && machine->stack_position) {
			free(machine->stack.key[machine->stack_position-1]);
			machine->stack_position--;
			if (machine->nv_array && !machine->stack_position) {
				machine->level--;
				return false;
			}
			CHECK_INEQUAL(nextc(machine), 1, return false);
			CHECK(clear_white(machine), 0, return false);
		}
		if ( machine->buffer != ',' && machine->buffer != '}') {
			PERROR("Expected ',' or '}' at", machine->position);
			return false;
		}
		else if (machine->buffer == ',') {
			CHECK_INEQUAL(nextc(machine), 1, return false);
			CHECK(clear_white(machine), -1, return false);
			CHECK(update_nvlist_array(machine, nvl), false, return false);
		}
	}

	return true;

}

static bool
parse_nvlist(struct machine *machine, nvlist_t **nvl)
{

	while (machine->level != 0) {
		while (machine->buffer != '}') {
			if (machine->buffer != '\"') {
				PERROR("Error parsing key at", machine->position);
				goto failed;
			}

			CHECK(get_key(machine), false, goto failed);
			CHECK(clear_white(machine), -1, goto failed);
			if (machine->buffer != ':') {
				PERROR("Expected ':' at", machine->position);
			}
			CHECK_INEQUAL(nextc(machine), 1, goto failed);
			CHECK(clear_white(machine), -1, goto failed);

			CHECK(get_type(machine, nvl), -1, goto failed);
			CHECK(insert_type(machine, nvl), false, goto failed);

			if (machine->type == NV_TYPE_NVLIST || machine->type == NV_TYPE_NVLIST_ARRAY)
				continue;

			if (machine->buffer != ',' && machine->buffer != '}') {
				PERROR("Expected ',' or '}' at", machine->position);
				goto failed;
			}
			if (machine->buffer != '}') {
				CHECK_INEQUAL(nextc(machine), 1, goto failed);
				CHECK(clear_white(machine), -1, goto failed);
			}

		}
		change_level(machine, nvl);
	}
	CHECK(nextc(machine), -1, goto failed);
	return true;

failed:
	while (nvlist_get_parent(*nvl, NULL) != NULL)
		*nvl = nvpair_nvlist(nvlist_get_nvpair_parent(*nvl));
	return false;
}


nvlist_t *
json_to_nvlist(int fd)
{
	struct machine machine;
	int tmp;
	nvlist_t *nvl;

	nvl = nvlist_create(0);
	CHECK(nvl, NULL, goto failed);

	machine.fd = fd;
	machine.position = 0;
	machine.level = 0;
	machine.key = strdup("");
	machine.nv_array = false;

	machine.stack.key = malloc(sizeof(*machine.stack.level)*20);
	machine.stack_size = 20;
	CHECK(machine.stack.key, NULL, goto failed);

	machine.stack.level = malloc(sizeof(*machine.stack.level)*20);
	machine.stack_position = 0;
	CHECK(machine.stack.level, NULL, goto failed);


	CHECK(nextc(&machine), -1, goto failed);
	tmp = clear_white(&machine);
	if (tmp == -1)
		goto failed;
	else if (tmp == 0) {
		clear_machine(&machine);
		return nvl;
	}
	CHECK(get_type(&machine, &nvl), -1, goto failed);

	switch(machine.type) {
	case NV_TYPE_NONE:
		clear_machine(&machine);
		return nvl;
		break;
	case NV_TYPE_NVLIST:
		CHECK(parse_nvlist(&machine, &nvl), false, goto failed);
		break;
	case NV_TYPE_NVLIST_ARRAY:
		machine.nv_array = true;
		parse_nvlist(&machine, &nvl);
		break;
	default:
		CHECK(insert_type(&machine, &nvl), false, goto failed);
		break;
	}
	clear_white(&machine);
	if (!end(&machine)) {
		PERROR("ERROR at the end of parsing", machine.position);
		goto failed;
	}

	clear_machine(&machine);
	return nvl;

failed:
	if (nvl != NULL)
		nvlist_destroy(nvl);
	clear_machine(&machine);
	ERRNO_SET(EINVAL);
	return NULL;
}
