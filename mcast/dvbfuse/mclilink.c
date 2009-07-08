#include "dvbfuse.h"

#define BUFFER_SIZE (10000*188)

int gen_pat(unsigned char *buf, unsigned int program_number, unsigned int pmt_pid, unsigned int ts_cnt)
{
	int pointer_field=0;
	int section_length=13;
	int transport_stream_id=0;
	int version_number=0;
	int current_next_indicator=1;
	int section_number=0;
	int last_section_number=0;
	int i=0;
	u_long crc; 
	
	buf[i++] = 0x47;
	buf[i++] = 0x40;
	buf[i++] = 0x00;
	buf[i++] = 0x10 | (ts_cnt&0xf);
	
	buf[i++] = pointer_field;
	buf[i++] = 0; // table_id
	buf[i] = 0xB0; // section_syntax_indicator=1, 0, reserved=11
	buf[i++]|= (section_length>>8)&0x0f;
	buf[i++] = section_length&0xff;

	buf[i++] = transport_stream_id>>8;
	buf[i++] = transport_stream_id&0xff;

	buf[i++] = 0xc0 | ((version_number&0x1f) << 1) | (current_next_indicator&1);
	buf[i++] = section_number;
	buf[i++] = last_section_number;

	buf[i++] = program_number>>8;
	buf[i++] = program_number&0xff;
	buf[i++] = 0xe0 | (pmt_pid<<8);
	buf[i++] = pmt_pid&0xff;
	
	crc=dvb_crc32 ((char *)buf+5, i-5);
	buf[i++] = (crc>>24)&0xff;
	buf[i++] = (crc>>16)&0xff;
	buf[i++] = (crc>>8)&0xff;
	buf[i++] = crc&0xff;
	
	for(;i<188;i++) {
		buf[i] = 0xff;
	}
	
//	printhex_buf ("PAT", buf, 188);
	return i;
}

int lastwp = 0;
/*-------------------------------------------------------------------------*/
int mcli_handle_ts (unsigned char *buffer, size_t len, void *p)
{
	stream_info_t *si = (stream_info_t *) p;
	
	if(si->stop) {
		return len;
	}
	
	pthread_mutex_lock(&si->lock_wr);

	int wp = si->wp;
	int olen = len;

	int ret;
	unsigned int i;
	
again:	
	switch (si->si_state) {
	case 3:
	case 0:
		si->psi.start = 0;
		si->psi.len = 0;
		si->si_state++;
		goto again;

	case 1:
		for(i=0; i<len; i+=188) {
			ret = ts2psi_data (buffer+i, &si->psi, 188, 0);
			if(ret){
				break;
			}
		}
		if (ret < 0) {
			si->si_state = 0;
		}

		if (ret == 1) {
			printf ("Got PAT\n");
			pmt_pid_list_t pat;
			ret = parse_pat_sect (si->psi.buf, &pat);
			if (ret < 0) {
				si->si_state = 0;
			} else if (ret == 0) {
//				print_pat (&pat.p, pat.pl, pat.pmt_pids);
				unsigned int n;
				for (n = 0; n < pat.pmt_pids; n++) {
					if (pat.pl[n].program_number == si->cdata->sid) {
						si->pmt_pid = pat.pl[n].network_pmt_pid;
						printf ("SID %d has PMT Pid %d\n", si->cdata->sid, si->pmt_pid);
						break;
					}
				}
				if (pat.pmt_pids) {
					free (pat.pl);
				}
				si->si_state++;
			}
		}
		break;
	case 4:
		for(i=0; i<len; i+=188) {
			ret = ts2psi_data (buffer+i, &si->psi, 188, si->pmt_pid);
			if(ret){
				break;
			}
		}
		if (ret < 0) {
			si->si_state = 2;
		}

		if (ret == 1) {
			printf ("Got PMT\n");
			pmt_t hdr;
			si_ca_pmt_t pm, es;
//			printhex_buf ("Section", si->psi.buf, si->psi.len);
			si->fta=1;
			ret = parse_pmt_ca_desc (si->psi.buf, &pm, &es, &hdr, &si->fta);
			if (ret < 0) {
				si->si_state = 2;
			} else if (ret == 0) {
				si->es_pidnum = get_pmt_es_pids (es.cad, es.size, si->es_pids, 1);
				if (si->es_pidnum <= 0) {
					si->si_state = 2;
				} else {
					si->si_state++;
				}
			}
			if (pm.size) {
				free (pm.cad);
			}
			if (es.size) {
				free (es.cad);
			}
			break;

		}
	case 6:
		if ((wp + len) >= BUFFER_SIZE) {
			int l;
			l = BUFFER_SIZE - wp;
			memcpy (si->buffer + wp, buffer, l);
			len -= l;
			wp = 0;
			buffer += l;
		}
		memcpy (si->buffer + wp, buffer, len);
		wp += len;
		si->wp = wp % (BUFFER_SIZE);
#if 1
		if (abs (si->wp - lastwp) > 500 * 1000) {
//			printf ("wp %i, rp %i\n", si->wp, si->rp);
			lastwp = si->wp;
		}
#endif
		break;
	}

	pthread_mutex_unlock(&si->lock_wr);
	return olen;
}

/*-------------------------------------------------------------------------*/
int mcli_handle_ten (tra_t * ten, void *p)
{
	if(ten) {
		printf("Status: %02X, Strength: %04X, SNR: %04X, BER: %04X\n",ten->s.st,ten->s.strength, ten->s.snr, ten->s.ber);
	}
	return 0;
}

#ifdef WIN32THREADS
DWORD WINAPI stream_watch(__in  LPVOID p)
#else
void *stream_watch (void *p)
#endif
{
	unsigned char ts[188];
	stream_info_t *si = (stream_info_t *) p;
	while (!si->stop) {
		if (si->pmt_pid && si->si_state == 2) {
			dvb_pid_t pids[3];
			memset (&pids, 0, sizeof (pids));
			pids[0].pid = si->pmt_pid;
			pids[1].pid = -1;
			printf ("Add PMT-PID: %d\n", si->pmt_pid);
			recv_pids (si->r, pids);
			si->si_state++;
		}
		if (si->es_pidnum && si->si_state == 5) {
			int i,k=0;
			size_t sz=sizeof(dvb_pid_t)*(si->es_pidnum+2);
			dvb_pid_t *pids=(dvb_pid_t*)malloc(sz);
			if(pids==NULL) {
				err("Can't get memory for pids\n");
			}
			memset (pids, 0, sz);
			pids[k++].pid = si->pmt_pid;

			for (i = 0; i < si->es_pidnum; i++) {
				printf ("Add ES-PID: %d\n", si->es_pids[i]);
				pids[i + k].pid = si->es_pids[i];
//				if(si->cdata->NumCaids) {
				if(!si->fta) {
        				pids[i + k].id =  si->cdata->sid;
				}
				pids[i + k +1].pid = -1;
			}
			recv_pids (si->r, pids);
			free(pids);
			si->si_state++;
		}
		if(si->si_state == 6) {
			gen_pat(ts, si->cdata->sid, si->pmt_pid, si->ts_cnt++);
			mcli_handle_ts (ts, 188, si);
		}
		usleep (50000);
	}
	return NULL;
}

/*-------------------------------------------------------------------------*/
void *mcli_stream_setup (const char *path)
{
	int cnum;
	stream_info_t *si;
	recv_info_t *r;
	struct dvb_frontend_parameters fep;
	recv_sec_t sec;
	dvb_pid_t pids[4];
	int source; 
	fe_type_t tuner_type;

	cnum = atoi (path + 1);
	cnum--;
	printf ("mcli_stream_setup %i\n", cnum);
	if (cnum < 0 || cnum > get_channel_num ())
		return 0;

	si = (stream_info_t *) malloc (sizeof (stream_info_t));
	memset(si, 0, sizeof(stream_info_t));

	si->buffer = (char *) malloc (BUFFER_SIZE);
	si->psi.buf = (unsigned char *) malloc (PSI_BUF_SIZE);
	pthread_mutex_init (&si->lock_wr, NULL);
	pthread_mutex_init (&si->lock_rd, NULL);

	r = recv_add ();
	if (!r) {
		fprintf (stderr, "Cannot get memory for receiver\n");
		return 0;
	}

	si->r = r;

	register_ten_handler (r, mcli_handle_ten, si);
	register_ts_handler (r, mcli_handle_ts, si);

	si->cdata = get_channel_data (cnum);

	fep.frequency = si->cdata->frequency;
	fep.inversion = INVERSION_AUTO;

	if (si->cdata->source >= 0) {
		fep.u.qpsk.symbol_rate = si->cdata->srate * 1000;
		fep.u.qpsk.fec_inner = (fe_code_rate_t)(si->cdata->coderateH | (si->cdata->modulation<<16));
		fep.frequency *= 1000;
		fep.inversion = (fe_spectral_inversion_t)si->cdata->inversion;

		sec.voltage = (fe_sec_voltage_t)si->cdata->polarization;
		sec.mini_cmd = (fe_sec_mini_cmd_t)0;
		sec.tone_mode = (fe_sec_tone_mode_t)0;
		tuner_type = FE_QPSK;
		source = si->cdata->source;
	}

	memset (&pids, 0, sizeof (pids));

	pids[0].pid = 0;
	pids[1].pid = -1;

	printf ("source %i, frequ %i, vpid %i, apid %i  srate %i\n", source, si->cdata->frequency, pids[0].pid, pids[1].pid, fep.u.qpsk.symbol_rate);
	recv_tune (r, tuner_type, source, &sec, &fep, pids);

#ifdef WIN32THREADS
	CreateThread(NULL, 0, stream_watch, si, 0, NULL);
#else
	pthread_create (&si->t, NULL, stream_watch, si);
#endif
	printf("mcli_setup %p\n",si);
	return si;
}

/*-------------------------------------------------------------------------*/
size_t mcli_stream_read (void *handle, char *buf, size_t maxlen, off_t offset)
{
	stream_info_t *si = (stream_info_t *) handle;
	size_t len, rlen;
	int wp;

//	printf("mcli_read %p\n",handle);
	if (!handle || si->stop)
		return 0;
#if 0
	if (offset < si->offset) {
		// Windback
		si->rp = (si->rp - (si->offset - offset)) % BUFFER_SIZE;
		si->offset = offset;
	}
/*	else if (offset>si->offset && ) { 
		si->rp=(si->rp+(offset-si->offset))%BUFFER_SIZE;
		si->offset=offset;
		}*/
#endif
	wp = si->wp;

	if (si->rp == wp) {
		return 0;
	} 

	if (si->rp < wp) {
		len = wp - si->rp;
		if (maxlen < len)
			rlen = maxlen;
		else
			rlen = len;

		memcpy (buf, si->buffer + si->rp, rlen);
		si->rp += rlen;
	} else {
		len = BUFFER_SIZE - (si->rp - wp);
		if (maxlen < len)
			rlen = maxlen;
		else
			rlen = len;

		if (si->rp + rlen > BUFFER_SIZE) {
			int xlen = BUFFER_SIZE - si->rp;
			memcpy (buf, si->buffer + si->rp, xlen);
			memcpy (buf + xlen, si->buffer, rlen - xlen);
		} else
			memcpy (buf, si->buffer + si->rp, rlen);
		si->rp += rlen;
	}

	si->rp %= (BUFFER_SIZE);
	si->offset += rlen;
//      printf("rlen %i, rp %i wp %i\n",rlen,si->rp, si->wp);
	return rlen;
}

/*-------------------------------------------------------------------------*/
int mcli_stream_stop (void *handle)
{
	if (handle) {
		stream_info_t *si = (stream_info_t *) handle;
		recv_info_t *r = si->r;
		pthread_mutex_lock(&si->lock_rd);
		if (pthread_exist(si->t)) {
			si->stop = 1;
			pthread_join (si->t, NULL);
		}
		pthread_mutex_unlock(&si->lock_rd);
		
		pthread_mutex_lock(&si->lock_wr);
		if (r) {
			register_ten_handler (r, NULL, NULL);
			register_ts_handler (r, NULL, NULL);
			recv_del (r);
		}
		pthread_mutex_unlock(&si->lock_wr);
		if (si->buffer)
			free (si->buffer);
		if (si->psi.buf) {
			free (si->psi.buf);
		}
		
		free (si);
	}
	return 0;
}

cmdline_t cmd = { 0 };

/*-------------------------------------------------------------------------*/
void mcli_startup (void)
{
#ifdef PTW32_STATIC_LIB
	pthread_win32_process_attach_np();
#endif
	netceiver_info_list_t *nc_list = nc_get_list ();

#if defined WIN32 || defined APPLE
	cmd.mld_start = 1;
#endif

//	printf ("Using Interface %s\n", cmd.iface);
	recv_init (cmd.iface, cmd.port);

	if (cmd.mld_start) {
		mld_client_init (cmd.iface);
	}
#if 1
	int n, i;
	printf ("Looking for netceivers out there....\n");
	while (1) {
		nc_lock_list ();
		for (n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			printf ("\nFound NetCeiver: %s\n", nci->uuid);
			for (i = 0; i < nci->tuner_num; i++) {
				printf ("  Tuner: %s, Type %d\n", nci->tuner[i].fe_info.name, nci->tuner[i].fe_info.type);
			}
		}
		nc_unlock_list ();
		if (nc_list->nci_num) {
			break;
		}
		sleep (1);
	}
#endif
}
