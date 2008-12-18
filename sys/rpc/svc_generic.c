/*	$NetBSD: svc_generic.c,v 1.3 2000/07/06 03:10:35 christos Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

#if defined(LIBC_SCCS) && !defined(lint)
#ident	"@(#)svc_generic.c	1.19	94/04/24 SMI" 
static char sccsid[] = "@(#)svc_generic.c 1.21 89/02/28 Copyr 1988 Sun Micro";
#endif
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

/*
 * svc_generic.c, Server side for RPC.
 *
 */

#include "opt_inet6.h"

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/protosw.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/sx.h>
#include <sys/ucred.h>

#include <rpc/rpc.h>
#include <rpc/rpcb_clnt.h>
#include <rpc/nettype.h>

#include <rpc/rpc_com.h>

extern int __svc_vc_setflag(SVCXPRT *, int);

/*
 * The highest level interface for server creation.
 * It tries for all the nettokens in that particular class of token
 * and returns the number of handles it can create and/or find.
 *
 * It creates a link list of all the handles it could create.
 * If svc_create() is called multiple times, it uses the handle
 * created earlier instead of creating a new handle every time.
 */
int
svc_create(
	SVCPOOL *pool,
	void (*dispatch)(struct svc_req *, SVCXPRT *),
	rpcprog_t prognum,		/* Program number */
	rpcvers_t versnum,		/* Version number */
	const char *nettype)		/* Networktype token */
{
	int num = 0;
	SVCXPRT *xprt;
	struct netconfig *nconf;
	void *handle;

	if ((handle = __rpc_setconf(nettype)) == NULL) {
		printf("svc_create: unknown protocol");
		return (0);
	}
	while ((nconf = __rpc_getconf(handle)) != NULL) {
		mtx_lock(&pool->sp_lock);
		TAILQ_FOREACH(xprt, &pool->sp_xlist, xp_link) {
			if (strcmp(xprt->xp_netid, nconf->nc_netid) == 0) {
				/* Found an old one, use it */
				mtx_unlock(&pool->sp_lock);
				(void) rpcb_unset(prognum, versnum, nconf);
				if (svc_reg(xprt, prognum, versnum,
					dispatch, nconf) == FALSE) {
					printf(
		"svc_create: could not register prog %u vers %u on %s\n",
					(unsigned)prognum, (unsigned)versnum,
					 nconf->nc_netid);
					mtx_lock(&pool->sp_lock);
				} else {
					num++;
					mtx_lock(&pool->sp_lock);
					break;
				}
			}
		}
		mtx_unlock(&pool->sp_lock);
		if (xprt == NULL) {
			/* It was not found. Now create a new one */
			xprt = svc_tp_create(pool, dispatch, prognum, versnum,
			    NULL, nconf);
			if (xprt)
				num++;
		}
	}
	__rpc_endconf(handle);
	/*
	 * In case of num == 0; the error messages are generated by the
	 * underlying layers; and hence not needed here.
	 */
	return (num);
}

/*
 * The high level interface to svc_tli_create().
 * It tries to create a server for "nconf" and registers the service
 * with the rpcbind. It calls svc_tli_create();
 */
SVCXPRT *
svc_tp_create(
	SVCPOOL *pool,
	void (*dispatch)(struct svc_req *, SVCXPRT *),
	rpcprog_t prognum,		/* Program number */
	rpcvers_t versnum,		/* Version number */
	const char *uaddr,		/* Address (or null for default) */
	const struct netconfig *nconf) /* Netconfig structure for the network */
{
	struct netconfig nconfcopy;
	struct netbuf *taddr;
	struct t_bind bind;
	SVCXPRT *xprt;

	if (nconf == NULL) {
		printf(
	"svc_tp_create: invalid netconfig structure for prog %u vers %u\n",
				(unsigned)prognum, (unsigned)versnum);
		return (NULL);
	}
	if (uaddr) {
		taddr = uaddr2taddr(nconf, uaddr);
		bind.addr = *taddr;
		free(taddr, M_RPC);
		bind.qlen = SOMAXCONN;
		xprt = svc_tli_create(pool, NULL, nconf, &bind, 0, 0);
		free(bind.addr.buf, M_RPC);
	} else {
		xprt = svc_tli_create(pool, NULL, nconf, NULL, 0, 0);
	}
	if (xprt == NULL) {
		return (NULL);
	}
	/*LINTED const castaway*/
	nconfcopy = *nconf;
	(void) rpcb_unset(prognum, versnum, &nconfcopy);
	if (svc_reg(xprt, prognum, versnum, dispatch, nconf) == FALSE) {
		printf(
		"svc_tp_create: Could not register prog %u vers %u on %s\n",
				(unsigned)prognum, (unsigned)versnum,
				nconf->nc_netid);
		xprt_unregister(xprt);
		return (NULL);
	}
	return (xprt);
}

/*
 * If so is NULL, then it opens a socket for the given transport
 * provider (nconf cannot be NULL then). If the t_state is T_UNBND and
 * bindaddr is NON-NULL, it performs a t_bind using the bindaddr. For
 * NULL bindadr and Connection oriented transports, the value of qlen
 * is set to 8.
 *
 * If sendsz or recvsz are zero, their default values are chosen.
 */
SVCXPRT *
svc_tli_create(
	SVCPOOL *pool,
	struct socket *so,		/* Connection end point */
	const struct netconfig *nconf,	/* Netconfig struct for nettoken */
	const struct t_bind *bindaddr,	/* Local bind address */
	size_t sendsz,			/* Max sendsize */
	size_t recvsz)			/* Max recvsize */
{
	SVCXPRT *xprt = NULL;		/* service handle */
	bool_t madeso = FALSE;		/* whether so opened here  */
	struct __rpc_sockinfo si;
	struct sockaddr_storage ss;

	if (!so) {
		if (nconf == NULL) {
			printf("svc_tli_create: invalid netconfig\n");
			return (NULL);
		}
		so = __rpc_nconf2socket(nconf);
		if (!so) {
			printf(
			    "svc_tli_create: could not open connection for %s\n",
					nconf->nc_netid);
			return (NULL);
		}
		__rpc_nconf2sockinfo(nconf, &si);
		madeso = TRUE;
	} else {
		/*
		 * It is an open socket. Get the transport info.
		 */
		if (!__rpc_socket2sockinfo(so, &si)) {
			printf(
		"svc_tli_create: could not get transport information\n");
			return (NULL);
		}
	}

	/*
	 * If the socket is unbound, try to bind it.
	 */
	if (madeso || !__rpc_sockisbound(so)) {
		if (bindaddr == NULL) {
			if (bindresvport(so, NULL)) {
				memset(&ss, 0, sizeof ss);
				ss.ss_family = si.si_af;
				ss.ss_len = si.si_alen;
				if (sobind(so, (struct sockaddr *)&ss,
					curthread)) {
					printf(
			"svc_tli_create: could not bind to anonymous port\n");
					goto freedata;
				}
			}
			solisten(so, SOMAXCONN, curthread);
		} else {
			if (bindresvport(so,
				(struct sockaddr *)bindaddr->addr.buf)) {
				printf(
		"svc_tli_create: could not bind to requested address\n");
				goto freedata;
			}
			solisten(so, (int)bindaddr->qlen, curthread);
		}
			
	}
	/*
	 * call transport specific function.
	 */
	switch (si.si_socktype) {
		case SOCK_STREAM:
#if 0
			slen = sizeof ss;
			if (_getpeername(fd, (struct sockaddr *)(void *)&ss, &slen)
			    == 0) {
				/* accepted socket */
				xprt = svc_fd_create(fd, sendsz, recvsz);
			} else
#endif
				xprt = svc_vc_create(pool, so, sendsz, recvsz);
			if (!nconf || !xprt)
				break;
#if 0
			/* XXX fvdl */
			if (strcmp(nconf->nc_protofmly, "inet") == 0 ||
			    strcmp(nconf->nc_protofmly, "inet6") == 0)
				(void) __svc_vc_setflag(xprt, TRUE);
#endif
			break;
		case SOCK_DGRAM:
			xprt = svc_dg_create(pool, so, sendsz, recvsz);
			break;
		default:
			printf("svc_tli_create: bad service type");
			goto freedata;
	}

	if (xprt == NULL)
		/*
		 * The error messages here are spitted out by the lower layers:
		 * svc_vc_create(), svc_fd_create() and svc_dg_create().
		 */
		goto freedata;

	/* Fill in type of service */
	xprt->xp_type = __rpc_socktype2seman(si.si_socktype);

	if (nconf) {
		xprt->xp_netid = strdup(nconf->nc_netid, M_RPC);
	}
	return (xprt);

freedata:
	if (madeso)
		(void)soclose(so);
	if (xprt) {
		if (!madeso) /* so that svc_destroy doesnt close fd */
			xprt->xp_socket = NULL;
		xprt_unregister(xprt);
	}
	return (NULL);
}
