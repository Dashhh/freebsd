/*-
 * Copyright (c) 2013 The FreeBSD Foundation
 * Copyright (c) 2013-2015 Mariusz Zaborski <oshogbo@FreeBSD.org>
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

#ifndef	_NVLIST_IMPL_H_
#define	_NVLIST_IMPL_H_

#include <sys/nv.h>
#include <sys/tree.h>

#ifndef _KERNEL
#include <stdint.h>
#endif

/* Red-black tree. Each node represents list of nvpairs with specified name and
 * type.
 */
RB_HEAD(nvl_tree, nvl_node);

/* Queue for elements with the same type and name. */
TAILQ_HEAD(nvln_head, nvpair); 

struct nvl_node {
	RB_ENTRY(nvl_node)	 nvln_entry;    /* RB_TREE interface */
	struct nvln_head	 nvln_head;	/* nvpair list */

	/* Dumped pair's name. If flag NV_NO_UNIQUE set, name is lowercased. */
	char			*nvln_key;
	int			 nvln_type;     /* Type of pair */
};

nvpair_t *nvlist_get_nvpair_parent(const nvlist_t *nvl);
const unsigned char *nvlist_unpack_header(nvlist_t *nvl,
    const unsigned char *ptr, size_t nfds, bool *isbep, size_t *leftp);

void nvlist_remove_node(const nvlist_t *nvl, struct nvl_node *node);

#endif	/* !_NVLIST_IMPL_H_ */
