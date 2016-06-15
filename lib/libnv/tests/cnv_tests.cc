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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_bool__present);
ATF_TEST_CASE_BODY(cnvlist_get_bool__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_number__present);
ATF_TEST_CASE_BODY(cnvlist_get_number__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_string__present);
ATF_TEST_CASE_BODY(cnvlist_get_string__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_nvlist__present);
ATF_TEST_CASE_BODY(cnvlist_get_nvlist__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_descriptor__present);
ATF_TEST_CASE_BODY(cnvlist_get_descriptor__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_bool_array__present);
ATF_TEST_CASE_BODY(cnvlist_get_bool_array__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_number_array__present);
ATF_TEST_CASE_BODY(cnvlist_get_number_array__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_string_array__present);
ATF_TEST_CASE_BODY(cnvlist_get_string_array__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_nvlist_array__present);
ATF_TEST_CASE_BODY(cnvlist_get_nvlist_array__present)
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

ATF_TEST_CASE_WITHOUT_HEAD(cnvlist_get_descriptor_array__present);
ATF_TEST_CASE_BODY(cnvlist_get_descriptor_array__present)
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

ATF_INIT_TEST_CASES(tp)
{
	ATF_ADD_TEST_CASE(tp, cnvlist_get_bool__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_number__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_string__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_nvlist__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_descriptor__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_bool_array__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_number_array__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_string_array__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_nvlist_array__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_descriptor_array__present);
}
