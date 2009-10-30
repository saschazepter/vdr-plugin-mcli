/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef VDR_MCLI_DEVICE_H
#define VDR_MCLI_DEVICE_H

#include <vdr/device.h>
#include <mcliheaders.h>

#include "packetbuffer.h"

class cPluginMcli;

class cMcliDevice:public cDevice
{

      private:
	int m_pidsnum;
	int m_mcpidsnum;
	bool m_dvr_open;
	recv_info_t *m_r;
	recv_sec_t m_sec;
	int m_pos;
	struct dvb_frontend_parameters m_fep;
	dvb_pid_t m_pids[RECV_MAX_PIDS];
	char m_uuid[UUID_SIZE+1];
	tra_t m_ten;
	fe_type_t m_fetype;
	const cChannel *m_chan;
	cMutex mutex;
	bool m_enable;
	time_t m_last;
	int m_filternum;
	int m_disabletimeout;
	bool m_tuned;
	bool m_showtuning;
	bool m_ca_enable;
	bool m_ca_override;
      	char m_satlistname[UUID_SIZE+1];

      protected:
      	cPluginMcli *m_mcli;
	virtual bool SetChannelDevice (const cChannel * Channel, bool LiveView);
	virtual bool HasLock (int TimeoutMs);
	virtual bool SetPid (cPidHandle * Handle, int Type, bool On);
	virtual bool OpenDvr (void);
	virtual void CloseDvr (void);
	virtual bool GetTSPacket (uchar * &Data);

	virtual int OpenFilter (u_short Pid, u_char Tid, u_char Mask);
	virtual void CloseFilter (int Handle);
	virtual bool CheckCAM(const cChannel * Channel) const;
#ifdef GET_TS_PACKETS
	virtual int GetTSPackets (uchar *, int);
#endif
	satellite_list_t *find_sat_list (netceiver_info_t * nc_info, const char *SatelliteListName) const;
	bool SatelitePositionLookup(satellite_list_t *satlist, int pos) const;
	bool IsTunedToTransponderConst (const cChannel * Channel) const;
	
      public:
	cCondVar m_locked;
	cMyPacketBuffer *m_PB;
	cMcliFilters *m_filters;
	cMcliDevice (void);
	virtual ~ cMcliDevice ();
	void SetMcliRef(cPluginMcli *m)
	{
		m_mcli=m;
	}
	void SetSateliteListName(char *s);

#ifdef REELVDR
	const cChannel *CurChan () const
	{
		return m_chan;
	};
#endif
	unsigned int FrequencyToHz (unsigned int f)
	{
		while (f && f < 1000000)
			f *= 1000;
		return f;
	}
	virtual bool HasInternalCam (void)
	{
		return m_ca_enable;
	}
	virtual bool ProvidesSource (int Source) const;
	virtual bool ProvidesTransponder (const cChannel * Channel) const;
	virtual bool ProvidesChannel (const cChannel * Channel, int Priority = -1, bool * NeedsDetachReceivers = NULL) const;
	virtual bool IsTunedToTransponder (const cChannel * Channel);

	virtual int HandleTsData (unsigned char *buffer, size_t len);
	tra_t *GetTenData (void) {
		return &m_ten;
	}
	void SetTenData (tra_t * ten);
	void SetCaEnable (bool val = true) 
	{
		m_ca_enable=val;
	}
	bool GetCaEnable (void) const
	{
		return m_ca_enable;
	}
	void SetCaOverride (bool val = true) 
	{
		m_ca_override=val;
	}
	bool GetCaOverride (void) const
	{
		return m_ca_override;
	}
	void SetEnable (bool val = true);
	bool SetTempDisable (void);
	void SetFEType (fe_type_t val);
	fe_type_t GetFEType (void)
	{
		return m_fetype;
	};
	void SetUUID (const char *uuid);
	const char *GetUUID (void);
	void InitMcli (void);
	void ExitMcli (void);
	virtual bool ProvidesS2 () const
	{
		return m_fetype == FE_DVBS2;
	}
#ifdef REELVDR
	virtual bool HasInput (void) const
	{
		return m_enable;
	}
#endif
#ifdef DEVICE_ATTRIBUTES
	// Reel extension
	virtual int GetAttribute (const char *attr_name, uint64_t * val);
	virtual int GetAttribute (const char *attr_name, char *val, int maxret);
#endif
};

#endif // VDR_MCLI_DEVICE_H
