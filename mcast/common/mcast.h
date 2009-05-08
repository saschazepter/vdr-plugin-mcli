/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef __MCAST_H__
#define __MCAST_H__

typedef struct
{
	SOCKET udp_fd;
	int ttl;
	int idx;
	int is_multicast;
	int local_port;
	int reuse_socket;
	struct sockaddr_storage dest_addr;
	size_t dest_addr_len;
} UDPContext;

#define	SA	struct sockaddr

#define UDP_TX_BUF_SIZE 131072
#define UDP_RX_BUF_SIZE 131072
#define MCAST_TTL 16

UDPContext *server_udp_open_host (const char *host, int port, const char *ifname);
UDPContext *client_udp_open_host (const char *host, int port, const char *ifname);
UDPContext *server_udp_open (const struct in6_addr *mcg, int port, const char *ifname);
UDPContext *client_udp_open (const struct in6_addr *mcg, int port, const char *ifname);

int udp_read (UDPContext * s, uint8_t * buf, int size, int timeout, struct sockaddr_storage *from);
int udp_write (UDPContext * s, uint8_t * buf, int size);
int udp_close (UDPContext * s);
int udp_ipv6_join_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr);
int udp_ipv6_leave_multicast_group (SOCKET sockfd, int iface, struct sockaddr *addr);
#endif
