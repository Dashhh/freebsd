/*-
 * Copyright (c) 2016 Adam Starak <starak.adam@gmail.com>
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
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/cnv.h>
#include <sys/nv.h>
#include <sys/types.h>

#include <atf-c++.hpp>
#include <fcntl.h>
#include <errno.h>

#define	fd_is_valid(fd)	(fcntl((fd), F_GETFL) != -1 || errno != EBADF)

/* ATF cnvlist_get tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_bool);
ATF_TEST_CASE_BODY(cnvlist_get_bool)
{
	nvlist_t *nvl;
	const char *key;
	bool value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = true;
	nvlist_add_bool(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	ATF_REQUIRE_EQ(cnvlist_get_bool(cookie), value);

	nvlist_destroy(nvl);
}
ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_number);
ATF_TEST_CASE_BODY(cnvlist_get_number)
{
	nvlist_t *nvl;
	const char *key;
	uint64_t value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = 420;
	nvlist_add_number(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	ATF_REQUIRE_EQ(cnvlist_get_number(cookie), value);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_string);
ATF_TEST_CASE_BODY(cnvlist_get_string)
{
	nvlist_t *nvl;
	const char *key;
	const char *value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = "text";
	nvlist_add_string(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	ATF_REQUIRE_EQ(strcmp(cnvlist_get_string(cookie), value), 0);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_nvlist);
ATF_TEST_CASE_BODY(cnvlist_get_nvlist)
{
	nvlist_t *nvl;
	const char *key;
	nvlist_t *value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	value = nvlist_create(0);

	cookie = NULL;
	nvlist_add_nvlist(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	ATF_REQUIRE_EQ(cnvlist_get_nvlist(cookie), nvlist_get_nvlist(nvl, key));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_descriptor);
ATF_TEST_CASE_BODY(cnvlist_get_descriptor)
{
	nvlist_t *nvl;
	const char *key;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_descriptor(nvl, key, STDERR_FILENO);
	nvlist_next(nvl, &type, &cookie);

	ATF_REQUIRE_EQ(fd_is_valid(cnvlist_get_descriptor(cookie)), 1);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_binary);
ATF_TEST_CASE_BODY(cnvlist_get_binary)
{
	nvlist_t *nvl;
	const char *key;
	void *in_binary;
	const void *out_binary;
	void *cookie;
	int type;
	size_t in_size, out_size;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	in_size = 13;
	in_binary = malloc(in_size);
	memset(in_binary, 0xa5, in_size);

	nvlist_add_binary(nvl, key, in_binary, in_size);
	nvlist_next(nvl, &type, &cookie);

	out_binary = cnvlist_get_binary(cookie, &out_size);
	ATF_REQUIRE_EQ(out_size, in_size);
	ATF_REQUIRE_EQ(memcmp(in_binary, out_binary, out_size), 0);

	nvlist_destroy(nvl);
}

/* ATF cnvlist_get array tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_bool_array);
ATF_TEST_CASE_BODY(cnvlist_get_bool_array)
{
	nvlist_t *nvl;
	bool in_array[16];
	const bool *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = (i % 2 == 0);

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_bool_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_get_bool_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 16);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_number_array);
ATF_TEST_CASE_BODY(cnvlist_get_number_array)
{
	nvlist_t *nvl;
	uint64_t in_array[16];
	const uint64_t *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = i;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_number_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_get_number_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 16);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_string_array);
ATF_TEST_CASE_BODY(cnvlist_get_string_array)
{
	nvlist_t *nvl;
	const char *in_array[4] = {"inequality", "sucks", ".", ""};
	const char * const *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_string_array(nvl, key, in_array, 4);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_get_string_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 4);
	for (i = 0; i < 4; i++)
		ATF_REQUIRE_EQ(strcmp(out_array[i], in_array[i]), 0);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_nvlist_array);
ATF_TEST_CASE_BODY(cnvlist_get_nvlist_array)
{
	nvlist_t *nvl;
	nvlist_t *in_array[6];
	const nvlist_t * const *out_array;
	const nvlist_t * const *out_result;
	void *cookie;
	const char *key;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);
	for (i = 0; i < 6; i++)
		in_array[i] = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_nvlist_array(nvl, key, in_array, 6);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_get_nvlist_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 6);
	out_result = nvlist_get_nvlist_array(nvl, key, &nitems);
	for (i = 0; i < 6; i++)
		ATF_REQUIRE_EQ(out_array[i], out_result[i]);

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_descriptor_array);
ATF_TEST_CASE_BODY(cnvlist_get_descriptor_array)
{

	nvlist_t *nvl;
	int *in_array, type, i, count;
	const int *out_array;
	void *cookie;
	const char *key;
	size_t nitems;

	nvl = nvlist_create(0);
	cookie = NULL;
	key = "name";
	count = 50;

	in_array = (int*)malloc(sizeof(*in_array) * count);
	ATF_REQUIRE(in_array != NULL);
	for (i = 0; i < count; i++) {
		in_array[i] = dup(STDERR_FILENO);
		ATF_REQUIRE(fd_is_valid(in_array[i]));
	}

	nvlist_add_descriptor_array(nvl, key, in_array, count);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_get_descriptor_array(cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, count);
	for (i = 0; i < count; i++)
		ATF_REQUIRE_EQ(fd_is_valid(out_array[i]), 1);

	nvlist_destroy(nvl);
}

/* ATF cnvlist_take tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_bool);
ATF_TEST_CASE_BODY(cnvlist_take_bool)
{
	nvlist_t *nvl;
	const char *key;
	bool value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = true;
	nvlist_add_bool(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	ATF_REQUIRE_EQ(cnvlist_take_bool(nvl, cookie), value);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_number);
ATF_TEST_CASE_BODY(cnvlist_take_number)
{
	nvlist_t *nvl;
	const char *key;
	uint64_t value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = 69;
	nvlist_add_number(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	ATF_REQUIRE_EQ(cnvlist_take_number(nvl, cookie), value);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_string);
ATF_TEST_CASE_BODY(cnvlist_take_string)
{
	nvlist_t *nvl;
	const char *key;
	const char *value;
	char *out_string;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = "text";
	nvlist_add_string(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	out_string = cnvlist_take_string(nvl, cookie);
	ATF_REQUIRE_EQ(strcmp(out_string, value), 0);
	ATF_REQUIRE(nvlist_empty(nvl));

	free(out_string);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_nvlist);
ATF_TEST_CASE_BODY(cnvlist_take_nvlist)
{
	nvlist_t *nvl;
	const char *key;
	nvlist_t *value, *out_value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	value = nvlist_create(0);

	cookie = NULL;
	nvlist_move_nvlist(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	out_value = cnvlist_take_nvlist(nvl, cookie);
	ATF_REQUIRE_EQ(out_value, value);
	ATF_REQUIRE(nvlist_empty(nvl));

	free(out_value);
	nvlist_destroy(nvl);
}

/* ATF cnvlist_take array tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_bool_array);
ATF_TEST_CASE_BODY(cnvlist_take_bool_array)
{
	nvlist_t *nvl;
	bool in_array[16];
	bool *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = (i % 2 == 0);

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_bool_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_take_bool_array(nvl, cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 16);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);
	ATF_REQUIRE(nvlist_empty(nvl));

	free(out_array);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_number_array);
ATF_TEST_CASE_BODY(cnvlist_take_number_array)
{
	nvlist_t *nvl;
	uint64_t in_array[16];
	const uint64_t *out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = i;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_number_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_take_number_array(nvl, cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 16);
	for (i = 0; i < 16; i++)
		ATF_REQUIRE_EQ(out_array[i], in_array[i]);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_string_array);
ATF_TEST_CASE_BODY(cnvlist_take_string_array)
{
	nvlist_t *nvl;
	const char *in_array[4] = {"inequality", "sucks", ".", ""};
	char **out_array;
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_string_array(nvl, key, in_array, 4);
	nvlist_next(nvl, &type, &cookie);

	out_array = cnvlist_take_string_array(nvl, cookie, &nitems);
	ATF_REQUIRE_EQ(nitems, 4);
	for (i = 0; i < 4; i++)
		ATF_REQUIRE_EQ(strcmp(out_array[i], in_array[i]), 0);
	ATF_REQUIRE(nvlist_empty(nvl));

	free(out_array);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_nvlist_array);
ATF_TEST_CASE_BODY(cnvlist_take_nvlist_array)
{
	nvlist_t *testnvl[8];
	nvlist_t **result;
	nvlist_t *nvl;
	void *cookie;
	size_t num_items;
	unsigned int i;
	int type;
	const char *somestr[8] = { "a", "b", "c", "d", "e", "f", "g", "h" };
	const char *key;

	for (i = 0; i < 8; i++) {
		testnvl[i] = nvlist_create(0);
		nvlist_add_string(testnvl[i], "nvl/string", somestr[i]);
	}

	key = "nvl/nvlist";
	nvl = nvlist_create(0);
	cookie = NULL;

	nvlist_add_nvlist_array(nvl, key, (const nvlist_t * const *)testnvl, 8);
	nvlist_next(nvl, &type, &cookie);

	result = cnvlist_take_nvlist_array(nvl, cookie, &num_items);
	ATF_REQUIRE(result != NULL);
	ATF_REQUIRE_EQ(num_items, 8);
	for (i = 0; i < num_items; i++) {
		ATF_REQUIRE_EQ(nvlist_error(result[i]), 0);
		ATF_REQUIRE(nvlist_get_array_next(result[i]) == NULL);
	}

	ATF_REQUIRE(!nvlist_exists_string_array(nvl, key));
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_error(nvl), 0);

	for (i = 0; i < 8; i++) {
		nvlist_destroy(result[i]);
		nvlist_destroy(testnvl[i]);
	}

	free(result);
	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_take_binary);
ATF_TEST_CASE_BODY(cnvlist_take_binary)
{
	nvlist_t *nvl;
	const char *key;
	void *in_binary;
	void *out_binary;
	void *cookie;
	int type;
	size_t in_size, out_size;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	in_size = 13;
	in_binary = malloc(in_size);
	memset(in_binary, 0xa5, in_size);

	nvlist_add_binary(nvl, key, in_binary, in_size);
	nvlist_next(nvl, &type, &cookie);

	out_binary = cnvlist_take_binary(nvl, cookie, &out_size);
	ATF_REQUIRE_EQ(out_size, in_size);
	ATF_REQUIRE_EQ(memcmp(in_binary, out_binary, out_size), 0);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

/* ATF cnvlist_free tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_bool);
ATF_TEST_CASE_BODY(cnvlist_free_bool)
{
	nvlist_t *nvl;
	const char *key;
	bool value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = true;
	nvlist_add_bool(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_bool(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_number);
ATF_TEST_CASE_BODY(cnvlist_free_number)
{
	nvlist_t *nvl;
	const char *key;
	uint64_t value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = 69;
	nvlist_add_number(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_number(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_string);
ATF_TEST_CASE_BODY(cnvlist_free_string)
{
	nvlist_t *nvl;
	const char *key;
	const char *value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	value = "text";
	nvlist_add_string(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_string(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_nvlist);
ATF_TEST_CASE_BODY(cnvlist_free_nvlist)
{
	nvlist_t *nvl;
	const char *key;
	nvlist_t *value;
	void *cookie;
	int type;

	nvl = nvlist_create(0);
	value = nvlist_create(0);

	cookie = NULL;
	nvlist_move_nvlist(nvl, key, value);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_nvlist(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_binary);
ATF_TEST_CASE_BODY(cnvlist_free_binary)
{
	nvlist_t *nvl;
	const char *key;
	void *in_binary;
	void *cookie;
	int type;
	size_t in_size;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	in_size = 13;
	in_binary = malloc(in_size);
	memset(in_binary, 0xa5, in_size);

	nvlist_add_binary(nvl, key, in_binary, in_size);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_binary(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

/* ATF cnvlist_take array tests */

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_bool_array);
ATF_TEST_CASE_BODY(cnvlist_free_bool_array)
{
	nvlist_t *nvl;
	bool in_array[16];
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = (i % 2 == 0);

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_bool_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_bool_array(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_number_array);
ATF_TEST_CASE_BODY(cnvlist_free_number_array)
{
	nvlist_t *nvl;
	uint64_t in_array[16];
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	for (i = 0; i < 16; i++)
		in_array[i] = i;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_number_array(nvl, key, in_array, 16);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_number_array(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_string_array);
ATF_TEST_CASE_BODY(cnvlist_free_string_array)
{
	nvlist_t *nvl;
	const char *in_array[4] = {"inequality", "sucks", ".", ""};
	const char *key;
	void *cookie;
	int type, i;
	size_t nitems;

	nvl = nvlist_create(0);

	cookie = NULL;
	key = "name";
	nvlist_add_string_array(nvl, key, in_array, 4);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_string_array(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_free_nvlist_array);
ATF_TEST_CASE_BODY(cnvlist_free_nvlist_array)
{
	nvlist_t *testnvl[8];
	nvlist_t *nvl;
	const char *key;
	void *cookie;
	int type;
	unsigned int i;

	for (i = 0; i < 8; i++)
		testnvl[i] = nvlist_create(0);

	key = "nvl/nvlist";
	nvl = nvlist_create(0);
	cookie = NULL;

	nvlist_add_nvlist_array(nvl, key, (const nvlist_t * const *)testnvl, 8);
	nvlist_next(nvl, &type, &cookie);

	cnvlist_free_nvlist_array(nvl, cookie);
	ATF_REQUIRE(nvlist_empty(nvl));

	nvlist_destroy(nvl);
}

ATF_INIT_TEST_CASES(tp)
{
	ATF_ADD_TEST_CASE(tp, cnvlist_get_bool);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_bool_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_number);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_string);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_nvlist);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_descriptor);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_binary);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_number_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_string_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_nvlist_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_descriptor_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_bool);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_number);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_string);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_nvlist);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_binary);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_bool_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_number_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_string_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_take_nvlist_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_bool);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_number);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_string);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_nvlist);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_binary);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_bool_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_number_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_string_array);
	ATF_ADD_TEST_CASE(tp, cnvlist_free_nvlist_array);
}
