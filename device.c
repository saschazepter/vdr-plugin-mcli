/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

/*
 *  $Id: device.c 1671 2009-05-15 17:22:22Z fliegl $
 */

#include "filter.h"
#include "device.h"

#include <vdr/channels.h>
#include <vdr/ringbuffer.h>
#include <vdr/eit.h>
#include <vdr/timers.h>

#include <time.h>
#include <iostream>

#define st_Pos  0x07FF
#define st_Neg  0x0800

using namespace std;

// --- cMyTSBuffer -------------------------------------------------------------

cMyTSBuffer::cMyTSBuffer (int Size, const char *desc, int CardIndex)
{
	char buf[80];
	SetDescription ("%s (%d)", desc, CardIndex);
	sprintf(buf, "%s (%d)", desc, CardIndex);
	cardIndex = CardIndex;
	delivered = false;
	m_bufsize = Size;
	ringBuffer = new cRingBufferLinear (Size, TS_SIZE, true, buf);
	ringBuffer->SetTimeouts (100, 100);
	m_count = 0;
}

cMyTSBuffer::~cMyTSBuffer ()
{
	delete ringBuffer;
}

void cMyTSBuffer::Action (void)
{
}

uchar *cMyTSBuffer::Get (void)
{
	int Count = 0;
	if (delivered) {
		ringBuffer->Del (TS_SIZE);
		delivered = false;
	}
	uchar *p = ringBuffer->Get (Count);
	if(p && *p!=TS_SYNC_BYTE) {
		esyslog ("WARN: Get TS packet missing TS_SYNC_BYTE (%02x) at pos: %d with %d bytes left", *p, m_count%m_bufsize, Count);
	}
	if (p && Count >= TS_SIZE) {
		m_count+=TS_SIZE;
		delivered = true;
		return p;
	}
	return NULL;
}

int cMyTSBuffer::Put (const uchar * data, int count)
{
	for(int i=0; i<count; i+=TS_SIZE) {
		if(data[i]!=TS_SYNC_BYTE) {
			esyslog ("WARN: Put TS packet missing TS_SYNC_BYTE at %d (%02x)", i, data[i]);
		}
	}
	Lock();
	int ret=ringBuffer->Put (data, count);
	Unlock();
	return ret;
}

static int handle_ts (unsigned char *buffer, size_t len, void *p)
{
	cMcliDevice *m = (cMcliDevice *) p;
	unsigned int delivered=m->m_TSB->Put (buffer, len);
	if(delivered != len) {
		esyslog ("ERROR: could not deliver %d/%d bytes to ringbuffer of device %d", len-delivered, len, m->m_TSB->cardIndex);
	}
	
	m->m_filters->PutTS (buffer, len);
	return len;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static int handle_ten (tra_t * ten, void *p)
{
	cMcliDevice *m = (cMcliDevice *) p;
	if (ten) {
//              fprintf (stderr, "Status: %02X, Strength: %04X, SNR: %04X, BER: %04X\n", ten->s.st, ten->s.strength, ten->s.snr, ten->s.ber);
		m->SetTenData (ten);
		if (ten->s.st & FE_HAS_LOCK) {
			m->m_locked.Broadcast ();
		}
	} else {
		tra_t ten;
		memset (&ten, 0, sizeof (tra_t));
		m->SetTenData (&ten);
//              fprintf (stderr, "Signal lost\n");
	}
	return 0;
}

void cMcliDevice::SetTenData (tra_t * ten)
{
	m_ten = *ten;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void cMcliDevice::SetEnable (bool val)
{
	LOCK_THREAD;
	m_enable = val;
	if (!m_enable) {
		recv_stop (m_r);
	} else {
		recv_tune (m_r, m_fetype, m_pos, &m_sec, &m_fep, NULL);
	}
}

void cMcliDevice::SetFEType (fe_type_t val)
{
	m_fetype = val;
}

cMcliDevice::cMcliDevice (void)
{
	StartSectionHandler ();
	m_TSB = new cMyTSBuffer (10000*TS_SIZE, "Mcli TS buffer", CardIndex () + 1);
	m_filters = new cMcliFilters ();
	printf ("cMcliDevice: got device number %d\n", CardIndex () + 1);
	m_pidsnum = 0;
	m_chan = NULL;
	m_fetype = FE_QPSK;
	m_r = recv_add ();
	m_enable = 0;
	register_ten_handler (m_r, handle_ten, this);
	register_ts_handler (m_r, handle_ts, this);
}

cMcliDevice::~cMcliDevice ()
{
	LOCK_THREAD;
	StopSectionHandler ();
	printf ("Device %d gets destructed\n", CardIndex()+1);
	Cancel (0);
	m_locked.Broadcast ();
	register_ten_handler (m_r, NULL, NULL);
	register_ts_handler (m_r, NULL, NULL);
	recv_del (m_r);
	DELETENULL (m_TSB);
	DELETENULL (m_filters);
}

bool cMcliDevice::ProvidesSource (int Source) const
{
//      printf ("ProvidesSource, Source=%d\n", Source);
	if (!m_enable) {
		return false;
	}
	int type = Source & cSource::st_Mask;
	return type == cSource::stNone || type == cSource::stCable && m_fetype == FE_QAM || type == cSource::stSat && m_fetype == FE_QPSK || type == cSource::stSat && m_fetype == FE_DVBS2 || type == cSource::stTerr && m_fetype == FE_OFDM;
}

bool cMcliDevice::ProvidesTransponder (const cChannel * Channel) const
{
//      printf ("ProvidesTransponder %s\n", Channel->Name ());
	return ProvidesSource (Channel->Source ());
}

bool cMcliDevice::IsTunedToTransponder (const cChannel * Channel)
{
//      printf ("IsTunedToTransponder %s == %s \n", Channel->Name (), m_chan ? m_chan->Name () : "");

	if (m_chan && (Channel->Transponder () == m_chan->Transponder ())) {
//              printf ("Yes!!!");
		return true;
	}
//      printf ("Nope!!!");
	return false;
}

bool cMcliDevice::ProvidesChannel (const cChannel * Channel, int Priority, bool * NeedsDetachReceivers) const
{
	bool result = false;
	bool hasPriority = Priority < 0 || Priority > this->Priority ();
	bool needsDetachReceivers = false;

//      printf ("ProvidesChannel, Channel=%s, Prio=%d this->Prio=%d\n", Channel->Name (), Priority, this->Priority ());
	if (ProvidesSource (Channel->Source ())) {
		result = hasPriority;
		if (Priority >= 0 && Receiving (true)) {
			if (m_chan && (Channel->Transponder () != m_chan->Transponder ())) {
				needsDetachReceivers = true;
			} else {
				result = true;
			}
		}
	}
//      printf ("NeedsDetachReceivers: %d\n", needsDetachReceivers);
//      printf ("Result: %d\n", result);
	if (NeedsDetachReceivers) {
		*NeedsDetachReceivers = needsDetachReceivers;
	}
	return result;
}

bool cMcliDevice::SetChannelDevice (const cChannel * Channel, bool LiveView)
{
//      printf ("SetChannelDevice Channel: %s, Provider: %s, Source: %d, LiveView: %s\n", Channel->Name (), Channel->Provider (), Channel->Source (), LiveView ? "true" : "false");
	if(!m_enable) {
		return false;
	}
	LOCK_THREAD;
	memset (&m_sec, 0, sizeof (recv_sec_t));
	memset (&m_fep, 0, sizeof (struct dvb_frontend_parameters));
	memset (&m_pids, 0, sizeof (m_pids));
	if (!IsTunedToTransponder (Channel)) {
		memset (&m_ten, 0, sizeof (tra_t));
	}
	m_pidsnum = 0;

	m_chan = Channel;

	switch (m_fetype) {
	case FE_DVBS2:
	case FE_QPSK:{		// DVB-S

			unsigned int frequency = Channel->Frequency ();

			fe_sec_voltage_t volt = (Channel->Polarization () == 'v' || Channel->Polarization () == 'V' || Channel->Polarization () == 'r' || Channel->Polarization () == 'R') ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
			m_sec.voltage = volt;
			frequency =::abs (frequency);	// Allow for C-band, where the frequency is less than the LOF
			m_fep.frequency = frequency * 1000UL;
			m_fep.inversion = fe_spectral_inversion_t (Channel->Inversion ());
			m_fep.u.qpsk.symbol_rate = Channel->Srate () * 1000UL;
			m_fep.u.qpsk.fec_inner = fe_code_rate_t (Channel->CoderateH () | (Channel->Modulation()<<16));
		}
		break;
	case FE_QAM:{		// DVB-C

			// Frequency and symbol rate:
			m_fep.frequency = Channel->Frequency () * 1000000;
			m_fep.inversion = fe_spectral_inversion_t (Channel->Inversion ());
			m_fep.u.qam.symbol_rate = Channel->Srate () * 1000UL;
			m_fep.u.qam.fec_inner = fe_code_rate_t (Channel->CoderateH ());
			m_fep.u.qam.modulation = fe_modulation_t (Channel->Modulation ());
		}
		break;
	case FE_OFDM:{		// DVB-T

			// Frequency and OFDM paramaters:
			m_fep.frequency = Channel->Frequency () * 1000000;
			m_fep.inversion = fe_spectral_inversion_t (Channel->Inversion ());
			m_fep.u.ofdm.bandwidth = fe_bandwidth_t (Channel->Bandwidth ());
			m_fep.u.ofdm.code_rate_HP = fe_code_rate_t (Channel->CoderateH ());
			m_fep.u.ofdm.code_rate_LP = fe_code_rate_t (Channel->CoderateL ());
			m_fep.u.ofdm.constellation = fe_modulation_t (Channel->Modulation ());
			m_fep.u.ofdm.transmission_mode = fe_transmit_mode_t (Channel->Transmission ());
			m_fep.u.ofdm.guard_interval = fe_guard_interval_t (Channel->Guard ());
			m_fep.u.ofdm.hierarchy_information = fe_hierarchy_t (Channel->Hierarchy ());
		}
		break;
	default:
		esyslog ("ERROR: attempt to set channel with unknown DVB frontend type");
		return false;
	}

	int spos = Channel->Source ();
	m_pos = ((spos & st_Neg) ? 1 : -1) * (spos & st_Pos);
//      printf ("Position: %d\n", pos);
	if (m_pos) {
		m_pos += 1800;
	} else {
		m_pos = NO_SAT_POS;
	}

	recv_tune (m_r, m_fetype, m_pos, &m_sec, &m_fep, NULL);

	return true;
}

bool cMcliDevice::HasLock (int TimeoutMs)
{
//      LOCK_THREAD;
//      printf ("HasLock TimeoutMs:%d\n", TimeoutMs);

	if ((m_ten.s.st & FE_HAS_LOCK) || !TimeoutMs) {
		return m_ten.s.st & FE_HAS_LOCK;
	}
	cMutexLock MutexLock (&mutex);
	if (TimeoutMs && !(m_ten.s.st & FE_HAS_LOCK)) {
		m_locked.TimedWait (mutex, TimeoutMs);
	}
	if (m_ten.s.st & FE_HAS_LOCK) {
		return true;
	}
	return false;
}

bool cMcliDevice::SetPid (cPidHandle * Handle, int Type, bool On)
{
//      printf ("SetPid, Pid=%d, Type=%d, On=%d, used=%d\n", Handle->pid, Type, On, Handle->used);
	dvb_pid_t pi;
	if(!m_enable) {
		return false;
	}
	LOCK_THREAD;
	if (Handle->pid && (On || !Handle->used)) {
		m_pidsnum += On ? 1 : -1;
		if (m_pidsnum < 0) {
			m_pidsnum = 0;
		}

		if (m_pidsnum < 1) {
//                      printf ("SetPid: 0 pids left -> CloseDvr()\n");
		}

		if (On) {
			pi.pid = Handle->pid;
			if(m_chan && m_chan->Ca(0)) {
				pi.id = m_chan->Sid();
			} else {
				pi.id = 0;
			}
//                      printf ("Add Pid: %d\n", Handle->pid);
			recv_pid_add (m_r, &pi);
		} else {
//                      printf ("Del Pid: %d\n", Handle->pid);
			recv_pid_del (m_r, Handle->pid);
		}
	}
	int pidsnum = recv_pids_get (m_r, m_pids);
//      printf ("Pidsnum: %d m_pidsnum: %d\n", pidsnum, m_pidsnum);
	for (int i = 0; i < pidsnum; i++) {
//              printf ("Pid: %d\n", m_pids[i].pid);
	}
	return true;
}

bool cMcliDevice::OpenDvr (void)
{
	printf ("OpenDvr\n");
	m_dvr_open = true;
	LOCK_THREAD;
	return true;
}

void cMcliDevice::CloseDvr (void)
{
	printf ("CloseDvr\n");
	m_dvr_open = false;
	LOCK_THREAD;
}

bool cMcliDevice::GetTSPacket (uchar * &Data)
{
	if (!m_enable || !m_dvr_open) {
		return false;
	}
//      printf("GetTSPacket\n");
	Data = m_TSB->Get ();
	return true;
}

int cMcliDevice::OpenFilter (u_short Pid, u_char Tid, u_char Mask)
{
	if (!m_enable) {
		return -1;
	}
	LOCK_THREAD;
//      printf ("OpenFilter pid:%d tid:%d mask:%04x\n", Pid, Tid, Mask);
	dvb_pid_t pi;

	pi.pid = Pid;
	if(m_chan && m_chan->Ca(0)) {
		pi.id = m_chan->Sid();
	} else {
		pi.id = 0;
	}
//      printf ("Add Pid: %d\n", pi.pid);
	recv_pid_add (m_r, &pi);

	return m_filters->OpenFilter (Pid, Tid, Mask);
}

void cMcliDevice::CloseFilter (int Handle)
{
	if (!m_enable) {
		return;
	}

	LOCK_THREAD;
//      printf ("CloseFilter Handle:%d \n", Handle);
	int pid = m_filters->GetPid (Handle);
	if (pid != -1) {
		recv_pid_del (m_r, pid);
	}
	m_filters->CloseFilter (Handle);
}

void cMcliDevice::SetUUID (const char *uuid)
{
	strcpy(m_uuid, uuid);
}

const char *cMcliDevice::GetUUID (void)
{
	return m_uuid;
}

#ifdef DEVICE_ATTRIBUTES
/* Attribute classes for dvbdevice
  main  main attributes
      .name (String) "DVB", "IPTV", ...

  fe : frontend attributes (-> get from tuner)
      .type (int) FE_QPSK, ...
      .name (string) Tuner name
      .status,.snr,... (int)
*/
int cMcliDevice::GetAttribute(const char *attr_name, uint64_t *val)
{
	int ret=0,v;
	uint64_t rval=0;	

	if (!strcmp(attr_name,"fe.status")) {
		rval=m_ten.s.st;
	}
	else if (!strcmp(attr_name,"fe.signal")) {
		rval=m_ten.s.strength;
	}
	else if (!strcmp(attr_name,"fe.snr")) {
		rval=m_ten.s.snr;
	}
	else if (!strcmp(attr_name,"fe.ber")) {
		rval=m_ten.s.ber;
	}
	else if (!strcmp(attr_name,"fe.unc")) {
		rval=m_ten.s.ucblocks;
	}
	else if (!strcmp(attr_name,"fe.type")) {
		rval=m_fetype;		
	}
	else
		ret=-1;

	if (val)
		*val=rval;
	return ret;
}

int cMcliDevice::GetAttribute(const char *attr_name, char *val, int maxret)
{
	int ret=0;
	if (!strcmp(attr_name,"fe.uuid")) {
		strncpy(val, m_uuid, maxret);
		val[maxret-1]=0;
	}
	else if (!strcmp(attr_name,"fe.name")) {
		strncpy(val, "NetCeiver", maxret);
		val[maxret-1]=0;
	}
	else if (!strncmp(attr_name,"main.",5)) {
		if (!strncmp(attr_name+5,"name",4)) {
			if (val && maxret>0) {
				strncpy(val,"NetCeiver",maxret);
				val[maxret-1]=0;
			}
			return 0;
		}
	} else {
		ret=-1;
	}
	return ret;
}
#endif
