/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

/*
 *  $Id: filter.h 1640 2009-05-08 20:57:27Z fliegl $
 */

#ifndef VDR_STREAMDEV_FILTER_H
#define VDR_STREAMDEV_FILTER_H

#include <vdr/config.h>
#include <vdr/tools.h>
#include <vdr/thread.h>

class cMyTSBuffer;
class cMcliFilter;
class cMcliPid;

class cMcliPidList: public cList<cMcliPid>{
public:
	cMcliPidList(void) {};
	~cMcliPidList() {};
	int GetTidFromPid (int pid);
	void SetPid (int Pid, int Tid);
};

class cMcliFilters: public cList<cMcliFilter>, public cThread {
private:
	cMyTSBuffer  *m_TSBuffer;
	cMcliPidList m_pl;

protected:
	virtual void Action(void);
	void GarbageCollect(void);

public:
	cMcliFilters(void);
	virtual ~cMcliFilters();
	bool WantPid (int pid);
	int GetTidFromPid (int pid);
	int GetPid (int Handle);
	int PutTS(const uchar *data, int len);
	int OpenFilter(u_short Pid, u_char Tid, u_char Mask);
	void CloseFilter(int Handle);
};

#endif // VDR_STREAMDEV_FILTER_H
