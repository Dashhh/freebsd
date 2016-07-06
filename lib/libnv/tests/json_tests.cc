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

#include <sys/types.h>
#include <sys/nv.h>

#include <atf-c++.hpp>
#include <unistd.h>


ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__empty);
ATF_TEST_CASE_BODY(json_nvlist__empty)
{
	nvlist_t *nvl;
	FILE *f;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_empty", "r");
	ATF_REQUIRE(f != NULL);
	nvl = json_to_nvlist(fileno(f));

	fclose(f);
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE(!nvlist_error(nvl));
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__empty_ws);
ATF_TEST_CASE_BODY(json_nvlist__empty_ws)
{
	nvlist_t *nvl;
	FILE *f;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_empty", "r");
	ATF_REQUIRE(f != NULL);
	nvl = json_to_nvlist(fileno(f));

	fclose(f);
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE(!nvlist_error(nvl));
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__non_object);
ATF_TEST_CASE_BODY(json_nvlist__non_object)
{
	nvlist_t *nvl;
	FILE *f[8];
	void *cookie;
	unsigned i;
	size_t nitems;
	const bool barray[3] = {true, true, false};
	const char *sarray[3] = {"str", "pyrydy", "manesa"};
	const uint64_t narray[3] = {123523, 31252, 12390};

	cookie = NULL;
	f[0] = fopen("json/test_true", "r");
	f[1] = fopen("json/test_false", "r");
	f[2] = fopen("json/test_number", "r");
	f[3] = fopen("json/test_string", "r");
	f[4] = fopen("json/test_bool_array", "r");
	f[5] = fopen("json/test_number_array", "r");
	f[6] = fopen("json/test_string_array", "r");
	f[7] = fopen("json/test_null", "r");

	for (i = 0; i < 8; i++)
		ATF_REQUIRE(f[i] != NULL);

	nvl = json_to_nvlist(fileno(f[0]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, ""), true);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[0]);

	nvl = json_to_nvlist(fileno(f[1]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, ""), false);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[1]);

	nvl = json_to_nvlist(fileno(f[2]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_get_number(nvl, ""), 422);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[2]);

	nvl = json_to_nvlist(fileno(f[3]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string(nvl, ""), "manamana"), 0);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[3]);

	nvl = json_to_nvlist(fileno(f[4]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "", &nitems)[i], barray[i]);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[4]);

	nvl = json_to_nvlist(fileno(f[5]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number_array(nvl, "", &nitems)[i], narray[i]);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[5]);

	nvl = json_to_nvlist(fileno(f[6]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "", &nitems)[i], sarray[i]), 0);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[6]);

	nvl = json_to_nvlist(fileno(f[7]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE(nvlist_exists_null(nvl, ""));
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[7]);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__non_object_ws);
ATF_TEST_CASE_BODY(json_nvlist__non_object_ws)
{
	nvlist_t *nvl;
	FILE *f[8];
	unsigned i;
	size_t nitems;
	const bool barray[3] = {true, true, false};
	const char *sarray[3] = {"str", "pyrydy", "manesa"};
	const uint64_t narray[3] = {123523, 31252, 12390};

	f[0] = fopen("json/test_true_ws", "r");
	f[1] = fopen("json/test_false_ws", "r");
	f[2] = fopen("json/test_number_ws", "r");
	f[3] = fopen("json/test_string_ws", "r");
	f[4] = fopen("json/test_bool_array_ws", "r");
	f[5] = fopen("json/test_number_array_ws", "r");
	f[6] = fopen("json/test_string_array_ws", "r");
	f[7] = fopen("json/test_null_ws", "r");

	for (i = 0; i < 8; i++)
		ATF_REQUIRE(f[i] != NULL);

	nvl = json_to_nvlist(fileno(f[0]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, ""), true);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[0]);

	nvl = json_to_nvlist(fileno(f[1]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, ""), false);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[1]);

	nvl = json_to_nvlist(fileno(f[2]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(nvlist_get_number(nvl, ""), 422);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[2]);

	nvl = json_to_nvlist(fileno(f[3]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string(nvl, ""), "manamana"), 0);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[3]);

	nvl = json_to_nvlist(fileno(f[4]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "", &nitems)[i], barray[i]);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[4]);

	nvl = json_to_nvlist(fileno(f[5]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number_array(nvl, "", &nitems)[i], narray[i]);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[5]);

	nvl = json_to_nvlist(fileno(f[6]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "", &nitems)[i], sarray[i]), 0);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[6]);

	nvl = json_to_nvlist(fileno(f[7]));
	ATF_REQUIRE(!nvlist_empty(nvl));
	ATF_REQUIRE(nvlist_exists_null(nvl, ""));
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f[7]);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__object_empty);
ATF_TEST_CASE_BODY(json_nvlist__object_empty)
{
	nvlist_t *nvl;
	FILE *f;

	f = fopen("json/test_object_empty", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__object_empty_ws);
ATF_TEST_CASE_BODY(json_nvlist__object_empty_ws)
{
	nvlist_t *nvl;
	FILE *f;

	f = fopen("json/test_object_empty_ws", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__object);
ATF_TEST_CASE_BODY(json_nvlist__object)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	void *cookie;
	unsigned i;
	size_t nitems;
	const bool barray[3] = {false, true, false};
	const char *sarray[3] = {"parza", "pyrydy", "marep"};
	const uint64_t narray[3] = {0, 15, 27};

	cookie = NULL;
	f = fopen("json/test_object", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "mada"), 0);
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, "mada"), true);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "wada"), 0);
	ATF_REQUIRE_EQ(nvlist_get_number(nvl, "wada"), 150);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "lada"), 0);
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string(nvl, "lada"), "strod"), 0);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "hada"), 0);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "hada", &nitems)[i], barray[i]);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "dada"), 0);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number_array(nvl, "dada", &nitems)[i], narray[i]);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "bada"), 0);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "bada", &nitems)[i], sarray[i]), 0);

	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__object_ws);
ATF_TEST_CASE_BODY(json_nvlist__object_ws)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	void *cookie;
	unsigned i;
	size_t nitems;
	const bool barray[3] = {false, true, false};
	const char *sarray[3] = {"parza", "pyrydy", "marep"};
	const uint64_t narray[3] = {0, 15, 27};

	cookie = NULL;
	f = fopen("json/test_object", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "mada"), 0);
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, "mada"), true);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "wada"), 0);
	ATF_REQUIRE_EQ(nvlist_get_number(nvl, "wada"), 150);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "lada"), 0);
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string(nvl, "lada"), "strod"), 0);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "hada"), 0);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "hada", &nitems)[i], barray[i]);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "dada"), 0);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number_array(nvl, "dada", &nitems)[i], narray[i]);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "bada"), 0);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "bada", &nitems)[i], sarray[i]), 0);

	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);
	ATF_REQUIRE(!nvlist_error(nvl));
	nvlist_destroy(nvl);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nested_object_empty);
ATF_TEST_CASE_BODY(json_nvlist__nested_object_empty)
{
	nvlist_t *nvl, *nest;
	FILE *f;
	int type;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nested_object_empty", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "a"), 0);
	ATF_REQUIRE(nvlist_exists_nvlist(nvl, "a"));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);

	nest = nvlist_take_nvlist(nvl, "a");
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE(nvlist_empty(nest));

	nvlist_destroy(nvl);
	nvlist_destroy(nest);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nested_object_empty_ws);
ATF_TEST_CASE_BODY(json_nvlist__nested_object_empty_ws)
{
	nvlist_t *nvl, *nest;
	FILE *f;
	int type;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nested_object_empty_ws", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "a"), 0);
	ATF_REQUIRE(nvlist_exists_nvlist(nvl, "a"));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);

	nest = nvlist_take_nvlist(nvl, "a");
	ATF_REQUIRE(nvlist_empty(nvl));
	ATF_REQUIRE(nvlist_empty(nest));

	nvlist_destroy(nvl);
	nvlist_destroy(nest);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nested_object);
ATF_TEST_CASE_BODY(json_nvlist__nested_object)
{
	nvlist_t *nvl, *nest;
	FILE *f;
	int type;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nested_object", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "a"), 0);
	ATF_REQUIRE(nvlist_exists_nvlist(nvl, "a"));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);

	nest = nvlist_take_nvlist(nvl, "a");

	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nest, &type, &cookie), "b"), 0);
	ATF_REQUIRE_EQ(nvlist_get_number(nest, "b"), 125);
	ATF_REQUIRE_EQ(nvlist_next(nest, &type, &cookie), NULL);

	nvlist_destroy(nvl);
	nvlist_destroy(nest);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nested_object_ws);
ATF_TEST_CASE_BODY(json_nvlist__nested_object_ws)
{
	nvlist_t *nvl, *nest;
	FILE *f;
	int type;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nested_object_ws", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "a"), 0);
	ATF_REQUIRE(nvlist_exists_nvlist(nvl, "a"));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);

	nest = nvlist_take_nvlist(nvl, "a");
	ATF_REQUIRE(nvlist_empty(nvl));

	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nest, &type, &cookie), "b"), 0);
	ATF_REQUIRE_EQ(nvlist_get_number(nest, "b"), 125);
	ATF_REQUIRE_EQ(nvlist_next(nest, &type, &cookie), NULL);

	nvlist_destroy(nvl);
	nvlist_destroy(nest);
	fclose(f);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nvlist_array_empty);
ATF_TEST_CASE_BODY(json_nvlist__nvlist_array_empty)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nvlist_array_empty", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), ""), 0);
	ATF_REQUIRE(nvlist_exists_nvlist_array(nvl, ""));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE(nvlist_empty(nvlist_get_nvlist_array(nvl, "", &nitems)[i]));
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nvlist_array_empty_ws);
ATF_TEST_CASE_BODY(json_nvlist__nvlist_array_empty_ws)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nvlist_array_empty_ws", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), ""), 0);
	ATF_REQUIRE(nvlist_exists_nvlist_array(nvl, ""));
	for (i = 0; i < 3; i++)
		ATF_REQUIRE(nvlist_empty(nvlist_get_nvlist_array(nvl, "", &nitems)[i]));
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nvlist_array);
ATF_TEST_CASE_BODY(json_nvlist__nvlist_array)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nvlist_array", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), ""), 0);
	ATF_REQUIRE(nvlist_exists_nvlist_array(nvl, ""));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number(nvlist_get_nvlist_array(nvl, "", &nitems)[i], "a"), 120);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nvlist_array_ws);
ATF_TEST_CASE_BODY(json_nvlist__nvlist_array_ws)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nvlist_array_ws", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), ""), 0);
	ATF_REQUIRE(nvlist_exists_nvlist_array(nvl, ""));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number(nvlist_get_nvlist_array(nvl, "", &nitems)[i], "a"), 120);
}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nested_nvlist_array);
ATF_TEST_CASE_BODY(json_nvlist__nested_nvlist_array)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nested_nvlist_array", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "b"), 0);
	ATF_REQUIRE(nvlist_exists_nvlist_array(nvl, "b"));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number(nvlist_get_nvlist_array(nvl, "b", &nitems)[i], "a"), 120);

}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__nested_nvlist_array_ws);
ATF_TEST_CASE_BODY(json_nvlist__nested_nvlist_array_ws)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_nested_nvlist_array_ws", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "b"), 0);
	ATF_REQUIRE(nvlist_exists_nvlist_array(nvl, "b"));
	ATF_REQUIRE_EQ(nvlist_next(nvl, &type, &cookie), NULL);
	for (i = 0; i < 3; i++)
		ATF_REQUIRE_EQ(nvlist_get_number(nvlist_get_nvlist_array(nvl, "b", &nitems)[i], "a"), 120);

}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__random_nvlist);
ATF_TEST_CASE_BODY(json_nvlist__random_nvlist)
{
	nvlist_t *nvl, *nest;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie, *nest_cookie;

	cookie = NULL;
	f = fopen("json/test_random_nvlist", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "asd"), 0);
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, "asd"), true);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "bs"), 0);
	ATF_REQUIRE(nvlist_exists_null(nvl, "bs"));

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "mas"), 0);
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "mas", &nitems)[0], "asd"), 0);
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "mas", &nitems)[1], "314w"), 0);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "pas"), 0);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "tras"), 0);
	for (i = 0; i < 2; i++) {
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "tras", &nitems)[2*i], true);
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "tras", &nitems)[2*i+1], false);
	}

	nest = nvlist_take_nvlist(nvl, "pas");
	nest_cookie = NULL;

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nest, &type, &nest_cookie), "asd"), 0);
	ATF_REQUIRE_EQ(nvlist_get_bool(nvlist_get_nvlist_array(nest, "asd", &nitems)[0], "klas"), true);
	ATF_REQUIRE(nvlist_exists_null(nvlist_get_nvlist_array(nest, "asd", &nitems)[0], ""));
	ATF_REQUIRE(nvlist_empty(nvlist_get_nvlist_array(nest, "asd", &nitems)[1]));

}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__random_nvlist_ws);
ATF_TEST_CASE_BODY(json_nvlist__random_nvlist_ws)
{
	nvlist_t *nvl, *nest;
	FILE *f;
	int type;
	unsigned i;
	size_t nitems;
	void *cookie, *nest_cookie;

	cookie = NULL;
	f = fopen("json/test_random_nvlist_ws", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	cookie = NULL;

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "asd"), 0);
	ATF_REQUIRE_EQ(nvlist_get_bool(nvl, "asd"), true);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "bs"), 0);
	ATF_REQUIRE(nvlist_exists_null(nvl, "bs"));

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "mas"), 0);
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "mas", &nitems)[0], "asd"), 0);
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string_array(nvl, "mas", &nitems)[1], "314w"), 0);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "pas"), 0);

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), "tras"), 0);
	for (i = 0; i < 2; i++) {
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "tras", &nitems)[2*i], true);
		ATF_REQUIRE_EQ(nvlist_get_bool_array(nvl, "tras", &nitems)[2*i+1], false);
	}

	nest = nvlist_take_nvlist(nvl, "pas");
	nest_cookie = NULL;

	ATF_REQUIRE_EQ(strcmp(nvlist_next(nest, &type, &nest_cookie), "asd"), 0);
	ATF_REQUIRE_EQ(nvlist_get_bool(nvlist_get_nvlist_array(nest, "asd", &nitems)[0], "klas"), true);
	ATF_REQUIRE(nvlist_exists_null(nvlist_get_nvlist_array(nest, "asd", &nitems)[0], ""));
	ATF_REQUIRE(nvlist_empty(nvlist_get_nvlist_array(nest, "asd", &nitems)[1]));

	nvlist_destroy(nvl);
	nvlist_destroy(nest);

}

ATF_TEST_CASE_WITHOUT_HEAD(json_nvlist__string_special);
ATF_TEST_CASE_BODY(json_nvlist__string_special)
{
	nvlist_t *nvl;
	FILE *f;
	int type;
	void *cookie;

	cookie = NULL;
	f = fopen("json/test_string_special", "r");
	ATF_REQUIRE(f != NULL);

	nvl = json_to_nvlist(fileno(f));
	ATF_REQUIRE_EQ(strcmp(nvlist_next(nvl, &type, &cookie), ""), 0);
	ATF_REQUIRE_EQ(strcmp(nvlist_get_string(nvl, ""), "\"\\\b\f\n\r\t"), 0);
	nvlist_destroy(nvl);
}

ATF_INIT_TEST_CASES(tp)
{
	ATF_ADD_TEST_CASE(tp, json_nvlist__empty);
	ATF_ADD_TEST_CASE(tp, json_nvlist__empty_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__non_object);
	ATF_ADD_TEST_CASE(tp, json_nvlist__non_object_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__object_empty);
	ATF_ADD_TEST_CASE(tp, json_nvlist__object_empty_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__object);
	ATF_ADD_TEST_CASE(tp, json_nvlist__object_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nested_object_empty);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nested_object_empty_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nested_object);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nested_object_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nvlist_array_empty);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nvlist_array_empty_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nvlist_array);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nvlist_array_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nested_nvlist_array);
	ATF_ADD_TEST_CASE(tp, json_nvlist__nested_nvlist_array_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__random_nvlist);
	ATF_ADD_TEST_CASE(tp, json_nvlist__random_nvlist_ws);
	ATF_ADD_TEST_CASE(tp, json_nvlist__string_special);
}
