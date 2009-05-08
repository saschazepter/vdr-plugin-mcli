/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#include "headers.h"

//----------------------------------------------------------------------------------------------------------------------------------
STATIC int udp_ipv6_is_multicast_address (const struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET)
		return IN_MULTICAST (ntohl (((struct sockaddr_in *) addr)->sin_addr.s_addr));
#endif
	if (addr->sa_family == AF_INET6)
		return IN6_IS_ADDR_MULTICAST (&((struct sockaddr_in6 *) addr)->sin6_addr);
	return -1;
}

//---------------------------------------------------------------------------------------------------------------------------------
STATIC int udp_ipv6_set_multicast_ttl (SOCKET sockfd, int mcastTTL, struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET) {
		if (setsockopt (sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &mcastTTL, sizeof (mcastTTL)) < 0) {
			perror ("setsockopt(IP_MULTICAST_TTL)");
			return -1;
		}
	}
#endif
	if (addr->sa_family == AF_INET6) {
		if (setsockopt (sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (_SOTYPE)&mcastTTL, sizeof (mcastTTL)) < 0) {
			perror ("setsockopt(IPV6_MULTICAST_HOPS)");
			return -1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------
int udp_ipv6_join_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET) {
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *) addr)->sin_addr.s_addr;
		mreq.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt (sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *) &mreq, sizeof (mreq)) < 0) {
			perror ("setsockopt(IP_ADD_MEMBERSHIP)");
			return -1;
		}
	}
#endif
	if (addr->sa_family == AF_INET6) {
		struct ipv6_mreq mreq6;
		memcpy (&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6 *) addr)->sin6_addr), sizeof (struct in6_addr));
		mreq6.ipv6mr_interface = iface;
		if (setsockopt (sockfd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (_SOTYPE)&mreq6, sizeof (mreq6)) < 0) {
			perror ("setsockopt(IPV6_ADD_MEMBERSHIP)");
			return -1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------
int udp_ipv6_leave_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr)
{
#ifdef IPV4
	if (addr->sa_family == AF_INET) {
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = ((struct sockaddr_in *) addr)->sin_addr.s_addr;
		mreq.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt (sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const void *) &mreq, sizeof (mreq)) < 0) {
			perror ("setsockopt(IP_DROP_MEMBERSHIP)");
			return -1;
		}
	}
#endif
	if (addr->sa_family == AF_INET6) {
	struct ipv6_mreq mreq6;
		memcpy (&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6 *) addr)->sin6_addr), sizeof (struct in6_addr));
		mreq6.ipv6mr_interface = iface;
		if (setsockopt (sockfd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (_SOTYPE)&mreq6, sizeof (mreq6)) < 0) {
			perror ("setsockopt(IPV6_DROP_MEMBERSHIP)");
			return -1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------
STATIC int sockfd_to_family (SOCKET sockfd)
{
	struct sockaddr_storage ss;
	socklen_t len;

	len = sizeof (ss);
	if (getsockname (sockfd, (SA *) & ss, &len) < 0)
		return (-1);
	return (ss.ss_family);
}

/* end sockfd_to_family */
//----------------------------------------------------------------------------------------------------------------------------------
int mcast_set_if (SOCKET sockfd, const char *ifname, u_int ifindex)
{
	switch (sockfd_to_family (sockfd)) {
#ifdef IPV4
	case AF_INET:{
			struct in_addr inaddr;
			struct ifreq ifreq;

			if (ifindex > 0) {
				if (if_indextoname (ifindex, ifreq.ifr_name) == NULL) {
					errno = ENXIO;	/* i/f index not found */
					return (-1);
				}
				goto doioctl;
			} else if (ifname != NULL) {
				memset(&ifreq, 0, sizeof(struct ifreq));
				strncpy (ifreq.ifr_name, ifname, IFNAMSIZ-1);
			      doioctl:
				if (ioctl (sockfd, SIOCGIFADDR, &ifreq) < 0)
					return (-1);
				memcpy (&inaddr, &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr, sizeof (struct in_addr));
			} else
				inaddr.s_addr = htonl (INADDR_ANY);	/* remove prev. set default */

			return (setsockopt (sockfd, IPPROTO_IP, IP_MULTICAST_IF, &inaddr, sizeof (struct in_addr)));
		}
#endif
	case AF_INET6:{
			u_int idx;
//              printf("Changing interface IPV6...\n");
			if ((idx = ifindex) == 0) {
				if (ifname == NULL) {
					errno = EINVAL;	/* must supply either index or name */
					return (-1);
				}
				if ((idx = if_nametoindex (ifname)) == 0) {
					errno = ENXIO;	/* i/f name not found */
					return (-1);
				}
			}
			return (setsockopt (sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, (_SOTYPE)&idx, sizeof (idx)));
		}

	default:
//		errno = EAFNOSUPPORT;
		return (-1);
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------
UDPContext *server_udp_open (const struct in6_addr *mcg, int port, const char *ifname)
{
	UDPContext *s;
	int sendfd;
	int n;

	pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

	s = (UDPContext *) calloc (1, sizeof (UDPContext));
	if (!s) {
		err ("Cannot allocate memory !\n");
		goto error;
	}
	struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &s->dest_addr;

	addr->sin6_addr=*mcg;;
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons (port);
	s->dest_addr_len = sizeof (struct sockaddr_in6);

	sendfd = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sendfd < 0) {
		err ("cannot get socket\n");
	}

	s->dest_addr_len = sizeof (struct sockaddr_in6);

	if ((udp_ipv6_is_multicast_address ((struct sockaddr *) &s->dest_addr))) {
		if (ifname && strlen (ifname) && (mcast_set_if (sendfd, ifname, 0) < 0)) {
			err ("mcast_set_if error\n");
			goto error;
		}
		if (udp_ipv6_set_multicast_ttl (sendfd, MCAST_TTL, (struct sockaddr *) &s->dest_addr) < 0) {
			err ("udp_ipv6_set_multicast_ttl");
		}
	}
	
	n = UDP_TX_BUF_SIZE;
	if (setsockopt (sendfd, SOL_SOCKET, SO_SNDBUF, (_SOTYPE)&n, sizeof (n)) < 0) {
		err ("setsockopt sndbuf");
		goto error;
	}
	s->is_multicast = 0;	//server
	s->udp_fd = sendfd;
	s->local_port = port;

	dbg ("Multicast streamer initialized successfully ! \n");

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

	return s;
      error:
	err ("Cannot init udp_server  !\n");
	if (s) {
		free (s);
	}

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	return NULL;
}

UDPContext *server_udp_open_host (const char *host, int port, const char *ifname)
{
	struct in6_addr addr;

	inet_pton (AF_INET6, host, &addr);

	return server_udp_open (&addr, port, ifname);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
UDPContext *client_udp_open (const struct in6_addr *mcg, int port, const char *ifname)
{
	UDPContext *s;
	int recvfd;
	int n;

	pthread_setcancelstate (PTHREAD_CANCEL_DISABLE, NULL);

	s = (UDPContext *) calloc (1, sizeof (UDPContext));
	if (!s) {
		err ("Cannot allocate memory !\n");
		goto error;
	}

	struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &s->dest_addr;
#ifndef WIN32
	addr->sin6_addr=*mcg;
#else
	struct in6_addr any=IN6ADDR_ANY_INIT;
	addr->sin6_addr=any;
#endif
	addr->sin6_family = AF_INET6;
	addr->sin6_port = htons (port);
	s->dest_addr_len = sizeof (struct sockaddr_in6);

	recvfd = socket (PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (recvfd < 0) {
		err ("cannot get socket\n");
	}
#ifdef WIN32	
# ifndef IPV6_PROTECTION_LEVEL
#   define IPV6_PROTECTION_LEVEL 23
#  endif
        n = 10 /*PROTECTION_LEVEL_UNRESTRICTED*/;
        if(setsockopt( recvfd, IPPROTO_IPV6, IPV6_PROTECTION_LEVEL, (_SOTYPE)&n, sizeof(n) ) < 0 ) {
        	err ("setsockopt IPV6_PROTECTION_LEVEL\n");
        	goto error;
        }
#endif                                    
	n = 1;
	if (setsockopt (recvfd, SOL_SOCKET, SO_REUSEADDR, (_SOTYPE)&n, sizeof (n)) < 0) {
		err ("setsockopt REUSEADDR\n");
		goto error;
	}

#if ! (defined WIN32 || defined APPLE)
	if (ifname && strlen (ifname) && setsockopt (recvfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen (ifname) + 1)) {
		dbg ("setsockopt SO_BINDTODEVICE %s failed\n", ifname);
	}
#endif
	if (bind (recvfd, (struct sockaddr *) &s->dest_addr, s->dest_addr_len) < 0) {
		err ("bind failed\n");
		goto error;
	}
#ifdef WIN32
	addr->sin6_addr=*mcg;
#endif
	if (udp_ipv6_is_multicast_address ((struct sockaddr *) &s->dest_addr)) {
#if 0
		if (ifname && strlen (ifname) && (mcast_set_if (recvfd, ifname, 0) < 0)) {
			err ("mcast_set_if error \n");
			goto error;
		}
#endif
		if (ifname) {
			if ((s->idx = if_nametoindex (ifname)) == 0) {
				s->idx = 0;
			} else {
				dbg("Selecting interface %s (%d)", ifname, s->idx);
			}
		} else {
			s->idx = 0;
		}

		if (udp_ipv6_join_multicast_group (recvfd, s->idx, (struct sockaddr *) &s->dest_addr) < 0) {
			err ("Cannot join multicast group !\n");
			goto error;
		}
		s->is_multicast = 1;
	}

	n = UDP_RX_BUF_SIZE;
	if (setsockopt (recvfd, SOL_SOCKET, SO_RCVBUF, (_SOTYPE)&n, sizeof (n)) < 0) {
		err ("setsockopt rcvbuf");
		goto error;
	}

	s->udp_fd = recvfd;
	s->local_port = port;

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

	return s;
      error:
	err ("socket error !\n");
	if (s) {
		free (s);
	}
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	return NULL;
}

UDPContext *client_udp_open_host (const char *host, int port, const char *ifname)
{
	struct in6_addr addr;

	inet_pton (AF_INET6, host, &addr);

	return client_udp_open (&addr, port, ifname);
}

//--------------------------------------------------------------------------------------------------------------------------------------------
int udp_read (UDPContext * s, uint8_t * buf, int size, int timeout, struct sockaddr_storage *from)
{
	socklen_t from_len = sizeof (struct sockaddr_storage);
	struct sockaddr_storage from_local;
	
	if(!from) {
		from=&from_local;
	}
	
	fd_set rfds;
	struct timeval tv;
	FD_ZERO(&rfds);
	FD_SET(s->udp_fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = timeout;

	if(select(s->udp_fd+1, &rfds, NULL, NULL, &tv)>0) {
		return recvfrom (s->udp_fd, (char *)buf, size, 0, (struct sockaddr *) from, &from_len);
	}
	return -1;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------
int udp_write (UDPContext * s, uint8_t * buf, int size)
{
	int ret;

	for (;;) {
		ret = sendto (s->udp_fd, (char *) buf, size, 0, (struct sockaddr *) &s->dest_addr, s->dest_addr_len);

		if (ret < 0) {
			if (errno != EINTR && errno != EAGAIN)
				return -1;
		} else {
			break;
		}
	}
	return size;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
int udp_close (UDPContext * s)
{
	if (s->is_multicast)
		udp_ipv6_leave_multicast_group (s->udp_fd, s->idx, (struct sockaddr *) &s->dest_addr);

	closesocket (s->udp_fd);
	free (s);

	return 0;
}
