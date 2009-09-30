#include "dvbipstream.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "streamer.h"

#define IP_ADDR_MAX 16
#define PORT_MAX 6

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
		if (ParseLine(buf,&channels[channel_num])) {
			printf("%i %s \n",channel_num+1,channels[channel_num].name);
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
	printf("Usage: dvbipstream [-i <network interface>] [-c channels.conf] \n");
	exit(0);
}


/*-------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	char c;
	char channels[_POSIX_PATH_MAX];
	strcpy(channels, "channels.conf");
	char bindaddr[IP_ADDR_MAX];
	int portnum = 12345;
	
	memset(bindaddr, 0, sizeof(bindaddr));

	if (argc <2)
	{
		usage();
		return 0;
	}

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
				strncpy(bindaddr, optarg, IP_ADDR_MAX-1);
				bindaddr[IP_ADDR_MAX-1] = 0;
				break;
			case 'p':
				portnum = atoi(optarg);
				break;
			case 'h':
				usage();
				return(0);
		}
	}

	if ( inet_addr(bindaddr) == INADDR_NONE)
	{
		printf("Please specify valid interface IP address with -b option.\n");
		return(0);
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

	printf("Listening on interface %s\n", bindaddr);
	streamer.SetBindAddress(inet_addr(bindaddr));
	streamer.SetStreamPort(portnum);
	
	streamer.Run();

	getchar();

	streamer.Stop();

	printf("netcv2dvbip stopped.\n");

#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}
/*-------------------------------------------------------------------------*/
