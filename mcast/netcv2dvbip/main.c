#include "dvbipstream.h"
#include "iface.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifndef WIN32
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#endif

#include "streamer.h"

#define PORT_MAX 6

#ifndef WIN32
// Local function Prototypes
static void signalHandler(int);

// Global vars...
static int sighandled = 0;
#define GOT_SIGINT      0x01
#define GOT_SIGHUP      0x02
#define GOT_SIGTERM     0x04
#endif

channel_t *channels=NULL;
int channel_num=0;
int channel_max_num=0;


/*-------------------------------------------------------------------------*/
channel_t * read_channel_list(char *filename)
{
	FILE *f;
	char buf[512];

	f=fopen(filename,"r");
	if (!f) {
		printf("Can't read %s: %s\n",filename,strerror(errno));
		return NULL;
	}

	while(!feof(f)) {
		if (channel_num==channel_max_num) {
			channel_max_num+=200;
			channels=(channel_t*)realloc(channels,channel_max_num*sizeof(channel_t));
		}
		fgets(buf,512,f);
		if ( !feof(f) && ParseLine(buf,&channels[channel_num])) {
			printf("%i: udp://239.255.0.%i:%i - %s \n", 
				channel_num+1, channel_num+1, 12345, channels[channel_num].name);
			channel_num++;
		}
	}
	printf("Read %i channels\n",channel_num);
	fclose(f);
	return channels;
}
/*-------------------------------------------------------------------------*/
int get_channel_num(void)
{
	return channel_num;
}
/*-------------------------------------------------------------------------*/
int get_channel_name(int n, char *str, int maxlen)
{
#ifndef WIN32
	snprintf(str,maxlen,"%04i_%s.ts",n+1,channels[n].name);
#else
	sprintf(str,"%04i_%s.ts",n+1,channels[n].name);
#endif
	while(*str) {
		char c=*str;
		if (c=='/' || c=='\\' || c==':' || c=='?' || c=='*' || !isprint(c))
			*str='_';
		str++;
	}
	return 0;
}
/*-------------------------------------------------------------------------*/
channel_t *get_channel_data(int n)
{	
	return &channels[n];
}


extern cmdline_t cmd;

void usage (void)
{
	printf("Usage: netcv2dvbip  [-b <multicast interface>] [-p <port>] [-i <netceiver interface>] [-c channels.conf]\n");
	exit(0);
}

/*-------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	char c;
	char channels[_POSIX_PATH_MAX];
	strcpy(channels, "channels.conf");
	char bindiface[IFNAMSIZ];
	int portnum = 12345;
	iface_t iflist[MAXIF];

#ifndef WIN32
	uid_t uid;
	struct sigaction sa;

	uid = getuid();
	if(uid != 0) 
	{ 
	   printf("You must be root! Current uid: %d.\n",uid);
	   return(1); 
	}

	sa.sa_handler = signalHandler;
    sa.sa_flags = 0;    /* Interrupt system calls */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

#else
	bool IsAdmin = false;
	IsUserAdmin(&IsAdmin);
	if( IsAdmin == false)
	{
	   printf("You must have administrative priviledges!\n");
	   return(1); 
	}
#endif

	memset(bindiface, 0, sizeof(bindiface));

	while(1)
	{
		int ret = getopt(argc,argv, "i:hc:b:p:");
		if (ret==-1)
			break;
			
		c=(char)ret;

		switch (c) {
			case 'i':
				strncpy(cmd.iface, optarg, IFNAMSIZ-1);
				cmd.iface[IFNAMSIZ-1]=0;
				break;
			case 'c':
				strncpy(channels, optarg, _POSIX_PATH_MAX-1);
				channels[_POSIX_PATH_MAX-1]=0;
				break;
			case 'b':
				strncpy(bindiface, optarg, IFNAMSIZ-1);
				bindiface[IFNAMSIZ-1] = 0;
				break;
			case 'p':
				portnum = atoi(optarg);
				break;
			case 'h':
				usage();
				return(0);
		}
	}

	memset( iflist, 0, sizeof(iflist));
	if ( !discover_interfaces( (iface_t*) &iflist ) )
		exit(-1);

	int ifindex = 0;
	if ( strlen(bindiface) > 0 )
	{
		int found = false;
		while ( strlen(iflist[ifindex].name) > 0 )
		{
			if (strncmp(iflist[ifindex].name, bindiface, IFNAMSIZ) == 0)
			{
				found = true;
				break;
			}
			ifindex++;
		}
		if (!found)
		{
			printf("Cannot find interface %s. Exiting...\n", bindiface);
			exit(-1);
		} 
		
	}
	
	printf("Starting netcv2dvbip. Streams will be sent to port: %d\n", portnum);

	if (!read_channel_list(channels))
		exit(-1);

	mcli_startup();

	printf("\n");
	
#ifdef WIN32
	WSADATA ws;
	WSAStartup(MAKEWORD (2, 2),&ws);
#endif
	
	cStreamer streamer;

	struct sockaddr_in listenaddr;
	listenaddr.sin_addr.s_addr = iflist[ifindex].ipaddr;
	printf("Listening on interface %s (%s)\n\n\n", iflist[ifindex].name, inet_ntoa(listenaddr.sin_addr));
		
	streamer.SetBindIf(iflist[ifindex]);
	streamer.SetStreamPort(portnum);
	streamer.SetNumGroups(get_channel_num());
	
	streamer.Run();

#ifdef WIN32
	printf("\nPlease press enter to stop.\n\n\n");
	getchar();
	printf("Stopping...please wait...\n");
#else
	while (1)
	{
        // Process signaling...
        if (sighandled) {
            if (sighandled & GOT_SIGINT) {
                sighandled &= ~GOT_SIGINT;
                printf("\nGot SIGINT signal. Exiting.\n");
                break;
            }
            if (sighandled & GOT_SIGTERM) {
                sighandled &= ~GOT_SIGTERM;
                printf("Got SIGTERM signal. Exiting.\n");
                break;
            }

        }
		usleep(10000);
	}
#endif

	streamer.Stop();

	printf("netcv2dvbip stopped.\n");

#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}
/*-------------------------------------------------------------------------*/
#ifndef WIN32
/*
 * Signal handler.  Take note of the fact that the signal arrived
 * so that the main loop can take care of it.
 */
static void signalHandler(int sig) {
    switch (sig) {
    case SIGINT:
		sighandled |= GOT_SIGINT;
		break;
    case SIGTERM:
        sighandled |= GOT_SIGTERM;
        break;
        /* XXX: Not in use.
        case SIGHUP:
            sighandled |= GOT_SIGHUP;
            break;
        */
    }
}
#endif
