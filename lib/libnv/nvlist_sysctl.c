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

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/nv.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "msgio.h"

#ifdef HAVE_PJDLOG
#include <pjdlog.h>
#endif

static bool
nvlist_sysctl_null_format(int *mib, const char *name, size_t *miblen,
    u_int *kind)
{
	u_int *buff;
	size_t size;

	mib[0] = 0;
	mib[1] = 4;
	if (sysctlnametomib(name, mib+2, miblen) < 0)
		return (false);
	*miblen += 2;

	if (sysctl(mib, *miblen, NULL, &size, NULL, 0) < 0)
		return (false);

	buff = malloc(sizeof(*buff)*size);
	size = sizeof(*buff)*size;
	if (buff == NULL)
		return (false);

	if (sysctl(mib, *miblen, buff, &size, NULL, 0) < 0) {
		free(buff);
		return (false);
	}

	*kind = buff[0] & CTLTYPE;
	free(buff);

	return (true);
}

static bool
nvlist_sysctl_number_format(nvlist_t *old, int *mib, const char *name,
    size_t *miblen, u_int *kind)
{
	const uint64_t *arr;
	u_int *buff;
	size_t size;

	mib[0] = 0;
	mib[1] = 4;

	arr = nvlist_get_number_array(old, name, miblen);

	memcpy(&mib[2], arr, sizeof(*mib)*(*miblen));
	*miblen += 2;

	if (sysctl(mib, *miblen, NULL, &size, NULL, 0) < 0)
		return (false);

	buff = malloc(sizeof(*buff)*size);
	size = sizeof(*buff)*size;
	if (buff == NULL)
		return (false);

	if (sysctl(mib, *miblen, buff, &size, NULL, 0) < 0) {
		free(buff);
		return (false);
	}

	*kind = buff[0] & CTLTYPE;
	free(buff);

	return (true);
}

static bool
nvlist_sysctl_get_binary(nvlist_t *oldp, const char *name, const int *mib,
    size_t miblen)
{
	int st;
	size_t size;
	void *buff;

	buff = NULL;
	st = sysctl(mib, miblen, NULL, &size, NULL, 0);
	do {
		size += size/10;
		buff = realloc(buff, size);
		if (buff == NULL)
			goto fail;
		st = sysctl(mib, miblen, buff, &size, NULL, 0);
	} while (st < 0 && errno == ENOMEM);
	if (st < 0)
		goto fail;

	buff = realloc(buff, size);
	if (buff == NULL)
		goto fail;

	nvlist_add_binary(oldp, name, buff, size);

	return (true);
fail:
	if (buff != NULL)
		free(buff);
	return (false);
}

static bool
nvlist_sysctl_get_string(nvlist_t *oldp, const char *name, const int *mib,
    size_t miblen)
{
	int st;
	size_t size;
	char *buff;

	buff = NULL;
	st = sysctl(mib, miblen, NULL, &size, NULL, 0);
	do {
		size += size/10;
		buff = realloc(buff, size);
		if (buff == NULL)
			goto fail;
		st = sysctl(mib, miblen, buff, &size, NULL, 0);
	} while (st < 0 && errno == ENOMEM);
	if (st < 0)
		goto fail;

	buff = realloc(buff, size);
	if (buff == NULL)
		goto fail;

	nvlist_move_string(oldp, name, buff);

	return (true);
fail:
	if (buff != NULL)
		free(buff);
	return (false);
}

static bool
nvlist_sysctl_get_number(nvlist_t *oldp, const char *name, const int *mib,
    size_t miblen, size_t sizeval)
{
	int st;
	size_t size, nitems;
	uint64_t number;
	void *buff;

	buff = NULL;
	st = sysctl(mib, miblen, NULL, &size, NULL, 0);
	do {
		size += size/10;
		buff = realloc(buff, size);
		if (buff == NULL)
			goto fail;
		st = sysctl(mib, miblen, buff, &size, NULL, 0);
	} while (st < 0 && errno == ENOMEM);
	if (st < 0 || size % sizeval != 0)
		goto fail;

	buff = realloc(buff, size);
	if (buff == NULL)
		goto fail;

	nitems = size/sizeval;
	if (nitems > 1) {
		nvlist_move_number_array(oldp, name, buff, nitems);
	} else {
		memcpy(&number, buff, size);
		free(buff);
		nvlist_add_number(oldp, name, number);
	}

	return (true);
fail:
	if (buff != NULL)
		free(buff);
	return (false);
}

#define	NVLIST_SYSCTL_SET_NUMBER(type, fname)				\
static bool								\
nvlist_sysctl_set_##fname(nvlist_t *nvl, const char *name, int *mib,	\
    size_t miblen)							\
{									\
	uint64_t number;						\
	void *val;							\
	bool res;							\
									\
	val = malloc(sizeof(type));					\
	if (val == NULL)						\
		return (false);						\
	number = nvlist_get_number(nvl, name);				\
	memcpy(val, &number, sizeof(type));				\
									\
	res = sysctl(mib, miblen, NULL, NULL, val, sizeof(type)) == 0;	\
	free(val);							\
									\
	return (res);							\
}

NVLIST_SYSCTL_SET_NUMBER(int, int)
NVLIST_SYSCTL_SET_NUMBER(int64_t, s64)
NVLIST_SYSCTL_SET_NUMBER(u_int, uint)
NVLIST_SYSCTL_SET_NUMBER(long, long)
NVLIST_SYSCTL_SET_NUMBER(unsigned long, ulong)
NVLIST_SYSCTL_SET_NUMBER(uint64_t, u64)
NVLIST_SYSCTL_SET_NUMBER(uint8_t, u8)
NVLIST_SYSCTL_SET_NUMBER(uint16_t, u16)
NVLIST_SYSCTL_SET_NUMBER(int8_t, s8)
NVLIST_SYSCTL_SET_NUMBER(int16_t, s16)
NVLIST_SYSCTL_SET_NUMBER(int32_t, s32)
NVLIST_SYSCTL_SET_NUMBER(uint32_t, u32)

#undef NVLIST_SYSCTL_SET_NUMBER

static bool
nvlist_sysctl_set_number(nvlist_t *newp, const char *name)
{
	int mib[CTL_MAXNAME+2];
	u_int kind;
	size_t size;

	size = CTL_MAXNAME+2;
	if (!nvlist_sysctl_null_format(mib, name, &size, &kind))
		return (false);

	switch(kind) {
	case CTLTYPE_INT:
		if (!nvlist_sysctl_set_int(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S64:
		if (!nvlist_sysctl_set_s64(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_UINT:
		if (!nvlist_sysctl_set_uint(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_LONG:
		if (!nvlist_sysctl_set_long(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_ULONG:
		if (!nvlist_sysctl_set_ulong(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U64:
		if (!nvlist_sysctl_set_u64(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U8:
		if (!nvlist_sysctl_set_u8(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U16:
		if (!nvlist_sysctl_set_u16(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S8:
		if (!nvlist_sysctl_set_s8(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S16:
		if (!nvlist_sysctl_set_s16(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S32:
		if (!nvlist_sysctl_set_s32(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U32:
		if (!nvlist_sysctl_set_u32(newp, name, mib+2, size-2))
			return (false);
		break;
	}

	return (true);
}

#define	NVLIST_SYSCTL_SET_NUMBER_ARRAY(type, fname)			\
static bool								\
nvlist_sysctl_set_##fname##_array(nvlist_t *nvl, const char *name,	\
	int *mib, size_t miblen)					\
{									\
	const uint64_t *number;						\
	size_t size;							\
	void *val;							\
	bool res;							\
									\
	number = nvlist_get_number_array(nvl, name, &size);		\
	size *= sizeof(type);						\
	val = malloc(size);						\
	if (val == NULL)						\
		return (false);						\
	memcpy(val, number, size);					\
									\
	res = sysctl(mib, miblen, NULL, NULL, val, size) == 0;		\
	free(val);							\
									\
	return (res);							\
}

NVLIST_SYSCTL_SET_NUMBER_ARRAY(int, int)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(int64_t, s64)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(u_int, uint)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(long, long)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(unsigned long, ulong)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(uint64_t, u64)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(uint8_t, u8)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(uint16_t, u16)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(int8_t, s8)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(int16_t, s16)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(int32_t, s32)
NVLIST_SYSCTL_SET_NUMBER_ARRAY(uint32_t, u32)

#undef NVLIST_SYSCTL_SET_NUMBER

static bool
nvlist_sysctl_set_number_array(nvlist_t *newp, const char *name)
{
	int mib[CTL_MAXNAME+2];
	u_int kind;
	size_t size;

	size = CTL_MAXNAME+2;
	if (!nvlist_sysctl_null_format(mib, name, &size, &kind))
		return (false);

	switch(kind) {
	case CTLTYPE_INT:
		if (!nvlist_sysctl_set_int_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S64:
		if (!nvlist_sysctl_set_s64_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_UINT:
		if (!nvlist_sysctl_set_uint_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_LONG:
		if (!nvlist_sysctl_set_long_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_ULONG:
		if (!nvlist_sysctl_set_ulong_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U64:
		if (!nvlist_sysctl_set_u64_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U8:
		if (!nvlist_sysctl_set_u8_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U16:
		if (!nvlist_sysctl_set_u16_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S8:
		if (!nvlist_sysctl_set_s8_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S16:
		if (!nvlist_sysctl_set_s16_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_S32:
		if (!nvlist_sysctl_set_s32_array(newp, name, mib+2, size-2))
			return (false);
		break;
	case CTLTYPE_U32:
		if (!nvlist_sysctl_set_u32_array(newp, name, mib+2, size-2))
			return (false);
		break;
	}

	return (true);
}


static bool
nvlist_sysctl_set_string(nvlist_t *nvl, const char *name)
{
	const char *string;
	int mib[CTL_MAXNAME];
	size_t miblen;
	bool res;

	miblen = CTL_MAXNAME;
	if (sysctlnametomib(name, mib, &miblen) < 0)
		return (false);
	string = nvlist_get_string(nvl, name);

	res = sysctl(mib, miblen, NULL, NULL, string, strlen(string)) == 0;

	return (res);
}

static bool
nvlist_sysctl_set_binary(nvlist_t *nvl, const char *name)
{
	size_t size;
	const void *val;
	bool res;

	val = nvlist_get_binary(nvl, name, &size);

	res = sysctlbyname(name, NULL, NULL, val, size) == 0;

	return (res);
}

static bool
nvlist_sysctl_old(nvlist_t *oldp)
{
	int mib[CTL_MAXNAME+2], type;
	size_t size;
	u_int kind;
	const char *name;
	void *cookie;
	nvlist_t *values;

	if (oldp == NULL)
		return (true);

	values = nvlist_create(0);
	cookie = NULL;
	size = CTL_MAXNAME+2;

	while ((name = nvlist_next(oldp, &type, &cookie)) != NULL) {
		if (type != NV_TYPE_NULL && type != NV_TYPE_NUMBER_ARRAY)
			goto fail;

		if (type == NV_TYPE_NULL &&
		    !nvlist_sysctl_null_format(mib, name, &size, &kind))
			goto fail;

		if (type == NV_TYPE_NUMBER_ARRAY &&
		    !nvlist_sysctl_number_format(oldp, mib, name, &size, &kind))
			goto fail;

		switch(kind & CTLTYPE) {
		case CTLTYPE_NODE:
			if (!nvlist_sysctl_get_binary(values, name, mib+2,
			    size-2)) {
				goto fail;
			}
			break;
		case CTLTYPE_INT:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(int))) {
				goto fail;
			}
			break;
		case CTLTYPE_STRING:
			if (!nvlist_sysctl_get_string(values, name, mib+2,
			    size-2)) {
				goto fail;
			}
			break;
		case CTLTYPE_S64:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(int64_t))) {
				goto fail;
			}
			break;
		case CTLTYPE_OPAQUE:
			if (!nvlist_sysctl_get_binary(values, name, mib+2,
			    size-2)) {
				goto fail;
			}
			break;
		case CTLTYPE_UINT:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(u_int))) {
				goto fail;
			}
			break;
		case CTLTYPE_LONG:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(long))) {
				goto fail;
			}
			break;
		case CTLTYPE_ULONG:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(unsigned long))) {
				goto fail;
			}
			break;
		case CTLTYPE_U64:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(uint64_t))) {
				goto fail;
			}
			break;
		case CTLTYPE_U8:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(uint8_t))) {
				goto fail;
			}
			break;
		case CTLTYPE_U16:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(uint16_t))) {
				goto fail;
			}
			break;
		case CTLTYPE_S8:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(int8_t))) {
				goto fail;
			}
			break;
		case CTLTYPE_S16:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(int16_t))) {
				goto fail;
			}
			break;
		case CTLTYPE_S32:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(int32_t))) {
				goto fail;
			}
			break;
		case CTLTYPE_U32:
			if (!nvlist_sysctl_get_number(values, name, mib+2,
			    size-2, sizeof(uint32_t))) {
				goto fail;
			}
			break;
		}
	}

	nvlist_move_nvlist(oldp, "", values);

	return (true);
fail:
	if (values != NULL)
		nvlist_destroy(values);

	return (false);
}

static bool
nvlist_sysctl_new(nvlist_t *newp)
{
	int type;
	const char *newname;
	void *cookie;

	if (newp == NULL)
		return (true);

	cookie = NULL;
	while ((newname = nvlist_next(newp, &type, &cookie)) != NULL) {
		switch(type) {
		case NV_TYPE_NUMBER:
			if (!nvlist_sysctl_set_number(newp, newname))
				return (false);
			break;
		case NV_TYPE_NUMBER_ARRAY:
			if (!nvlist_sysctl_set_number_array(newp,
			    newname))
				return (false);
			break;
		case NV_TYPE_STRING:
			if (!nvlist_sysctl_set_string(newp, newname))
				return (false);
			break;
		case NV_TYPE_BINARY:
			if (!nvlist_sysctl_set_binary(newp, newname))
				return (false);
			break;
		default:
			return (false);
		}
	}

	return (true);
}

int
nvlist_sysctl(nvlist_t *oldp, nvlist_t *newp)
{

	if (nvlist_sysctl_old(oldp) && nvlist_sysctl_new(newp))
		return (0);

	if (nvlist_exists_nvlist(oldp, ""))
		nvlist_free_nvlist(oldp, "");

	return (-1);
}
