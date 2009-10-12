#include "headers.h"

int main (int argc, char **argv)
{
	UDPContext *s;
	unsigned char buf[UDP_TX_BUF_SIZE];
	memset (buf, 0x55, sizeof (buf));
	int ret, len, i, mode = 1, mcg_num = 1, port = 23000, c, needhelp = 0;
	char mcg[10][1024];
	char *ifname = NULL;
	strcpy (mcg[0], "ff18:5100::");

	do {
		ret = getopt_long (argc, argv, "hrtp:g:xi:", NULL, NULL);
		if(ret<0) {
			break;
		}
		c=(char)ret;
		switch (c) {
		case 'i':
			ifname = optarg;
			break;
		case 'r':
			mode = 1;
			break;
		case 't':
			mode = 2;
			break;
		case 'x':
			mode = 3;
			break;
		case 'p':
			port = atoi (optarg);
			break;
		case 'g':
			for (mcg_num = 0, optind--; optind < argc; optind++, mcg_num++) {
				if (argv[optind][0] != '-') {
					strcpy (mcg[mcg_num], argv[optind]);
				} else {
					break;
				}
			}
			break;
		case 'h':
			needhelp = 1;
			break;
		}
	}
	while (c >= 0);

	if (needhelp) {
		fprintf (stderr, "usage: netcvlogview -i <network interface> <-r|-t> -g <multicast groups> -p <port>\n");
		return -1;
	}

	fprintf (stderr, "mode:%d port:%d mcg:%d [ ", mode, port, mcg_num);
	for (i = 0; i < mcg_num; i++) {
		fprintf (stderr, "%s ", mcg[i]);
	}
	fprintf (stderr, "]\n");

	switch (mode) {

	case 1:
		s = client_udp_open_host (mcg[0], port, ifname);
		if (s) {
			struct sockaddr_in6 addr;
			addr.sin6_family = AF_INET6;
			addr.sin6_port = htons (port);

			for (i = 1; i < mcg_num; i++) {
				fprintf (stderr, "mcg: [%s]\n", mcg[i]);
				inet_pton (AF_INET6, mcg[i], &addr.sin6_addr);
				if (udp_ipv6_join_multicast_group (s->udp_fd, s->idx, (struct sockaddr *) &addr) < 0) {
					err ("Cannot join multicast group !\n");
				}

			}
			while (1) {
				len = udp_read (s, buf, sizeof (buf), 50, NULL);
				for (i = 0; i < len; i++) {
					fputc (buf[i], stdout);
				}
			}
			udp_close (s);
		}
		break;

	case 2:
		s = server_udp_open_host (mcg[0], port, ifname);
		if (s) {
			while (1) {
				if(fread (buf, 1316, 1, stdin)<0) {
					break;
				}
				udp_write (s, buf, 1316);
			}
			udp_close (s);
		}
		break;

	case 3:
		s = server_udp_open_host (mcg[0], port, ifname);
		if (s) {
			int i;
			for (i = 0; i < sizeof (buf); i++) {
				buf[i] = rand ();
			}
			while (1) {
				i = rand ();
				udp_write (s, buf, ((i % 4) + 4) * 188);
			}
			udp_close (s);
		}
		break;
	}

	return 0;
}
