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

#include <sys/types.h>
#include <sys/cnv.h>
#include <sys/nv.h>

#include <atf-c++.hpp>

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


ATF_INIT_TEST_CASES(tp)
{
	ATF_ADD_TEST_CASE(tp, cnvlist_get_bool__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_number__present);
	ATF_ADD_TEST_CASE(tp, cnvlist_get_string__present);
}
