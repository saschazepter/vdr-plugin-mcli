#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "dvbfuse.h"

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
/*-------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
//	uint64_t h;

	if (!read_channel_list("channels.conf"))
		exit(-1);

	mcli_startup();
	
	start_fuse(argc, argv);
	return 0;
}
/*-------------------------------------------------------------------------*/
