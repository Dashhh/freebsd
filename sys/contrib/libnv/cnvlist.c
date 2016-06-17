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

#include <sys/cnv.h>
#include <sys/nv.h>

#include "nv_impl.h"
#include "nvlist_impl.h"
#include "nvpair_impl.h"

#define CNVLIST_GET(ftype, type, nvtype)                                \
ftype                                                                   \
cnvlist_get_##type(void *cookiep)                                       \
{                                                                       \
        if (nvpair_type(cookiep) != NV_TYPE_##nvtype) {                 \
		const char *name = nvpair_name(cookiep);		\
		nvlist_report_missing(NV_TYPE_##nvtype,	name);		\
	}								\
        return (nvpair_get_##type(cookiep));				\
}

CNVLIST_GET(bool, bool, BOOL)
CNVLIST_GET(uint64_t, number, NUMBER)
CNVLIST_GET(const char *, string, STRING)
CNVLIST_GET(const nvlist_t *, nvlist, NVLIST)
#ifndef _KERNEL
CNVLIST_GET(int, descriptor, DESCRIPTOR)
#endif

#undef CNVLIST_GET

#define CNVLIST_GET_ARRAY(ftype, type, nvtype)                          \
ftype                                                                   \
cnvlist_get_##type(void *cookiep, size_t *nitemsp)                      \
{                                                                       \
        if (nvpair_type(cookiep) != NV_TYPE_##nvtype) {                 \
		const char *name = nvpair_name(cookiep);		\
		nvlist_report_missing(NV_TYPE_##nvtype,	name);		\
	}								\
        return (nvpair_get_##type(cookiep, nitemsp));			\
}

CNVLIST_GET_ARRAY(const bool *, bool_array, BOOL_ARRAY)
CNVLIST_GET_ARRAY(const uint64_t *, number_array, NUMBER_ARRAY)
CNVLIST_GET_ARRAY(const char * const *, string_array, STRING_ARRAY)
CNVLIST_GET_ARRAY(const nvlist_t * const *, nvlist_array, NVLIST_ARRAY)
#ifndef _KERNEL
CNVLIST_GET_ARRAY(const int *, descriptor_array, DESCRIPTOR_ARRAY)
#endif

#undef  CNVLIST_GET_ARRAY

const void *
cnvlist_get_binary(void *cookiep, size_t *sizep)
{
        if (nvpair_type(cookiep) != NV_TYPE_BINARY) {
		const char *name = nvpair_name(cookiep);
		nvlist_report_missing(NV_TYPE_BINARY, name);
	}
        return (nvpair_get_binary(cookiep, sizep));
}

#define CNVLIST_TAKE(ftype, type, nvtype)                               \
ftype                                                                   \
cnvlist_take_##type(nvlist_t *nvl, void *cookiep)                       \
{                                                                       \
        if (nvpair_type(cookiep) != NV_TYPE_##nvtype) {                 \
		const char *name = nvpair_name(cookiep);		\
		nvlist_report_missing(NV_TYPE_##nvtype,	name);		\
	}								\
        ftype value;							\
        value = (ftype)(intptr_t)nvpair_get_##type(cookiep);		\
	nvlist_remove_nvpair(nvl, cookiep);				\
	nvpair_free_structure(cookiep);					\
	return (value);							\
}

CNVLIST_TAKE(bool, bool, BOOL)
CNVLIST_TAKE(uint64_t, number, NUMBER)
CNVLIST_TAKE(char *, string, STRING)
CNVLIST_TAKE(nvlist_t *, nvlist, NVLIST)
#ifndef _KERNEL
CNVLIST_TAKE(int, descriptor, DESCRIPTOR)
#endif

#undef CNVLIST_TAKE

#define CNVLIST_TAKE_ARRAY(ftype, type, nvtype)                         \
ftype                                                                   \
cnvlist_take_##type(nvlist_t *nvl, void *cookiep, size_t *nitemsp)      \
{                                                                       \
        if (nvpair_type(cookiep) != NV_TYPE_##nvtype) {                 \
		const char *name = nvpair_name(cookiep);		\
		nvlist_report_missing(NV_TYPE_##nvtype,	name);		\
	}								\
        ftype value;							\
        value = (ftype)(intptr_t)					\
			nvpair_get_##type(cookiep, nitemsp);		\
	nvlist_remove_nvpair(nvl, cookiep);				\
	nvpair_free_structure(cookiep);					\
        return (value);							\
}

CNVLIST_TAKE_ARRAY(bool *, bool_array, BOOL_ARRAY)
CNVLIST_TAKE_ARRAY(uint64_t *, number_array, NUMBER_ARRAY)
CNVLIST_TAKE_ARRAY(char **, string_array, STRING_ARRAY)
CNVLIST_TAKE_ARRAY(nvlist_t **, nvlist_array, NVLIST_ARRAY)
#ifndef _kernel
CNVLIST_TAKE_ARRAY(int *, descriptor_array, DESCRIPTOR_ARRAY);
#endif

#undef  cnvlist_get_array

void *
cnvlist_take_binary(nvlist_t *nvl, void *cookiep, size_t *sizep)
{
        if (nvpair_type(cookiep) == NV_TYPE_BINARY) {                 
                void *value;                                            
                value = (void *)(intptr_t)				
				nvpair_get_binary(cookiep, sizep);   	
       		nvlist_remove_nvpair(nvl, cookiep);                     
		nvpair_free(cookiep);
        	return (value);                                         
	}								
	return (0);							
}


#define CNVLIST_FREE(type, nvtype)                         		\
void                                                                  	\
cnvlist_free_##type(nvlist_t *nvl, void *cookiep)                    	\
{                                                                     	\
        if (nvpair_type(cookiep) == NV_TYPE_##nvtype)                 	\
	        nvlist_free_nvpair(nvl, cookiep);			\
}

CNVLIST_FREE(bool, BOOL)
CNVLIST_FREE(number, NUMBER)
CNVLIST_FREE(string, STRING)
CNVLIST_FREE(nvlist, NVLIST)
CNVLIST_FREE(descriptor, DESCRIPTOR)
CNVLIST_FREE(binary, BINARY);

#undef CNVLIST_FREE

#define CNVLIST_FREE_ARRAY(type, nvtype)                         	\
void                                                                    \
cnvlist_free_##type(nvlist_t *nvl, void *cookiep)		        \
{                                                                       \
        if (nvpair_type(cookiep) == NV_TYPE_##nvtype)			\
       		nvlist_free_nvpair(nvl, cookiep);  		        \
}

CNVLIST_FREE_ARRAY(bool_array, BOOL_ARRAY)
CNVLIST_FREE_ARRAY(number_array, NUMBER_ARRAY)
CNVLIST_FREE_ARRAY(string_array, STRING_ARRAY)
CNVLIST_FREE_ARRAY(nvlist_array, NVLIST_ARRAY)
#ifndef _kernel
CNVLIST_FREE_ARRAY(descriptor_array, DESCRIPTOR_ARRAY)
#endif

#undef CNVLIST_FREE_ARRAY
