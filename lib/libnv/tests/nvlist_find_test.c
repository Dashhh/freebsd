/*-
 * Copyright (c) 2016 Adam Starak
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
 *
 * $FreeBSD$
 */

#include <sys/nv.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int ntest = 1;

#define	CHECK(expr)	do {						\
	if ((expr))							\
		printf("ok # %d %s:%u\n", ntest, __FILE__, __LINE__);	\
	else								\
		printf("not ok # %d %s:%u\n", ntest, __FILE__, __LINE__);\
	ntest++;							\
} while (0)

int
main(void)
{
	nvlist_t *nvl;
	size_t ii;

	nvl = nvlist_create(NV_FLAG_NO_UNIQUE);
	CHECK(!nvlist_exists(nvl, "nothing"));

	nvlist_add_bool(nvl, "bool", true);
	CHECK(nvlist_get_bool(nvl, "bool") == true);
	nvlist_add_bool(nvl, "bool", false);
	CHECK(nvlist_get_bool(nvl, "bool") == true);

	nvlist_add_null(nvl, "null");
	CHECK(nvlist_exists_null(nvl, "null"));
	CHECK(nvlist_get_bool(nvl, "bool") == true);

	nvlist_add_string(nvl, "string", "a");
	nvlist_add_string(nvl, "string", "b");
	nvlist_add_string(nvl, "string", "c");
	nvlist_add_string(nvl, "string", "d");
	CHECK(nvlist_get_bool(nvl, "bool") == true);

	nvlist_take_bool(nvl, "bool");
	CHECK(nvlist_get_bool(nvl, "bool") == false);

	nvlist_add_number(nvl, "string", 156);
	CHECK(nvlist_get_number(nvl, "string") == 156);
	CHECK(strcmp(nvlist_get_string(nvl, "string"), "a") == 0);

	for (ii = 0; ii < 50; ii++)
		nvlist_add_number(nvl, "string", ii*2);
	CHECK(nvlist_exists(nvl, "string"));
	CHECK(nvlist_get_number(nvl, "string") == 156);
	CHECK(strcmp(nvlist_get_string(nvl, "string"), "a") == 0);
	CHECK(nvlist_exists_null(nvl, "null"));

	CHECK(nvlist_get_bool(nvl, "bool") == false);
	for (ii = 0; ii < 50; ii++)
		nvlist_add_bool(nvl, "bool", (ii + 1) % 2);
	CHECK(nvlist_get_bool(nvl, "bool") == false);

	nvlist_destroy(nvl);
}
