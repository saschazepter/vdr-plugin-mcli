#ifndef _DVBIPSTREAM_H
#define _DVBIPSTREAM_H

#include "headers.h"
#include "misc.h"

#define MAXAPIDS 4
#define MAXDPIDS 4
#define MAXCAIDS 2

typedef struct
{
	int number;
	char *name;
	char *provider;
	char *shortName;
	int type;
	unsigned int frequency;
	int srate;
	int coderateH;
	int coderateL;
	int guard;
	int polarization;
	int inversion;
	int modulation;
	int source;
	int transmission;
	int bandwidth;
	int hierarchy;
	int vpid;
	int ppid;
	int sid;
	int rid;
	int tid;
	int tpid;
	int nid;
	int caids[MAXCAIDS];
	int NumCaids;
	int apids[MAXAPIDS];
	int NumApids;
	int dpids[MAXDPIDS];
	int NumDpids;
	int eitpids[1];
	int NumEitpids;
} channel_t;


typedef struct
{
	recv_info_t *r;
	char *buffer;
	int rp, wp;
	off_t offset;
	int si_state;
	psi_buf_t psi;
	channel_t *cdata;
	int pmt_pid;
	int es_pids[32];
	int es_pidnum;
	int fta;
	pthread_t t;
	int stop;
	int ts_cnt;
	pthread_mutex_t lock_wr;
	pthread_mutex_t lock_rd;
} stream_info_t;


channel_t *read_channel_list (char *filename);

int get_channel_num (void);
int get_channel_name (int n, char *str, int maxlen);
channel_t *get_channel_data (int n);

// mcilink.c
void mcli_startup (void);
void* mcli_stream_setup (const int channum);
size_t mcli_stream_read (void* handle, char *buf, size_t maxlen, off_t offset);
int mcli_stream_stop (void* handle);

// parse.c
int ParseLine (const char *s, channel_t * ch);


#endif
