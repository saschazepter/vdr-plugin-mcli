/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

/*
 *  $Id: device.h 1640 2009-05-08 20:57:27Z fliegl $
 */

#ifndef VDR_MCLI_DEVICE_H
#define VDR_MCLI_DEVICE_H

#define API_SOCK
#include <vdr/device.h>
#include <mcast/common/defs.h>
#include <mcast/common/version.h>
#include <mcast/common/list.h>
#include <mcast/common/satlists.h>
#include <mcast/common/mcast.h>
#include <mcast/common/recv_ccpp.h>
#include <mcast/client/recv_tv.h>
#include <mcast/client/mld_reporter.h>
#include <mcast/client/tca_handler.h>
#include <mcast/client/tra_handler.h>
#include <mcast/common/tools.h>
#include <mcast/client/api_server.h>

class cMyTSBuffer:public cThread
{
      private:
	int f;
	int cardIndex;
	bool delivered;
	virtual void Action (void);
	cRingBufferLinear *ringBuffer;
      public:
	  cMyTSBuffer (int Size, int CardIndex);
	 ~cMyTSBuffer ();
	uchar *Get (void);
	int Put (const uchar * data, int len);
};

class cMcliDevice:public cDevice
{

      private:
	int m_pidsnum;
	bool m_dvr_open;
	recv_info_t *m_r;
	recv_sec_t m_sec;
	int m_pos;
	struct dvb_frontend_parameters m_fep;
	dvb_pid_t m_pids[256];
	char m_uuid[UUID_SIZE];
	tra_t m_ten;
	fe_type_t m_fetype;
	const cChannel *m_chan;
	cMutex mutex;
	bool m_enable;

      protected:
	  virtual bool SetChannelDevice (const cChannel * Channel, bool LiveView);
	virtual bool HasLock (int TimeoutMs);
	virtual bool SetPid (cPidHandle * Handle, int Type, bool On);
	virtual bool OpenDvr (void);
	virtual void CloseDvr (void);
	virtual bool GetTSPacket (uchar * &Data);

	virtual int OpenFilter (u_short Pid, u_char Tid, u_char Mask);
	virtual void CloseFilter (int Handle);

      public:
	  cCondVar m_locked;
	cMyTSBuffer *m_TSB;
	cMcliFilters *m_filters;
	  cMcliDevice (void);
	  virtual ~ cMcliDevice ();

	virtual bool HasInternalCam (void)
	{
		return true;
	}
	virtual bool ProvidesSource (int Source) const;
	virtual bool ProvidesTransponder (const cChannel * Channel) const;
	virtual bool ProvidesChannel (const cChannel * Channel, int Priority = -1, bool * NeedsDetachReceivers = NULL) const;
	virtual bool IsTunedToTransponder (const cChannel * Channel);

	void SetTenData (tra_t * ten);
	void SetEnable (bool val = true);
	void SetFEType (fe_type_t val);
	fe_type_t GetFEType (void)
	{ 
		return m_fetype;
	};
	void SetUUID (const char *uuid);
	const char *GetUUID (void);
	virtual bool ProvidesS2() const
	{
		return m_fetype == FE_DVBS2;
	}
#ifdef DEVICE_ATTRIBUTES
	// Reel extension
	virtual int GetAttribute(const char *attr_name, uint64_t *val);
	virtual int GetAttribute(const char *attr_name, char *val, int maxret);
#endif
};

#endif // VDR_MCLI_DEVICE_H
