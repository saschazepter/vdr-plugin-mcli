/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

/*
 *  $Id: filter.c 1755 2009-06-03 22:50:42Z fliegl $
 */

#include "filter.h"
#include "device.h"

#include <vdr/device.h>

#define PID_MASK_HI 0x1F

class cMcliPid:public cListObject
{
      private:
	int m_Pid;
	int m_Tid;

      public:
	  cMcliPid (int Pid, int Tid)
	{
		m_Pid = Pid;
		m_Tid = Tid;
	}
	 ~cMcliPid ()
	{
	}

	int Tid (void)
	{
		return m_Tid;
	}
	int Pid (void)
	{
		return m_Pid;
	}
	void SetTid (int Tid)
	{
		m_Tid = Tid;
	}
};

// --- cMcliFilter ------------------------------------------------------

class cMcliFilter:public cListObject
{
      private:
	uchar m_Buffer[65536];
	int m_Used;
	int m_Pipe[2];
	u_short m_Pid;
	u_char m_Tid;
	u_char m_Mask;

      public:
	  cMcliFilter (u_short Pid, u_char Tid, u_char Mask);
	  virtual ~ cMcliFilter ();

	bool Matches (u_short Pid, u_char Tid);
	bool PutSection (const uchar * Data, int Length, bool Pusi);
	int ReadPipe (void) const
	{
		return m_Pipe[0];
	}

	bool IsClosed (void);
	void Reset (void);

	u_short Pid (void) const
	{
		return m_Pid;
	}
	u_char Tid (void) const
	{
		return m_Tid;
	}
	u_char Mask (void) const
	{
		return m_Mask;
	}
};

inline bool cMcliFilter::Matches (u_short Pid, u_char Tid)
{
//      printf("Match: %d == %d m_Tid %d == %d %02x\n", m_Pid, Pid, m_Tid, Tid & m_Mask, m_Mask);
	return m_Pid == Pid && m_Tid == (Tid & m_Mask);
}

cMcliFilter::cMcliFilter (u_short Pid, u_char Tid, u_char Mask)
{
	m_Used = 0;
	m_Pid = Pid;
	m_Tid = Tid;
	m_Mask = Mask;
	m_Pipe[0] = m_Pipe[1] = -1;

#ifdef SOCK_SEQPACKET
	// SOCK_SEQPACKET (since kernel 2.6.4)
	if (socketpair (AF_UNIX, SOCK_SEQPACKET, 0, m_Pipe) != 0) {
		esyslog ("mcli: socketpair(SOCK_SEQPACKET) failed: %m, trying SOCK_DGRAM");
	}
#endif
	if (m_Pipe[0] < 0 && socketpair (AF_UNIX, SOCK_DGRAM, 0, m_Pipe) != 0) {
		esyslog ("mcli: couldn't open section filter socket: %m");
	}

	else if (fcntl (m_Pipe[0], F_SETFL, O_NONBLOCK) != 0 || fcntl (m_Pipe[1], F_SETFL, O_NONBLOCK) != 0) {
		esyslog ("mcli: couldn't set section filter socket to non-blocking mode: %m");
	}
}

cMcliFilter::~cMcliFilter ()
{
//      printf ("~cMcliFilter %p\n", this);

	// ownership of handle m_Pipe[0] has been transferred to VDR section handler
	//if (m_Pipe[0] >= 0)
	//      close(m_Pipe[0]);
	if (m_Pipe[1] >= 0)
		close (m_Pipe[1]);
}


bool cMcliFilter::PutSection (const uchar * Data, int Length, bool Pusi)
{

	if (!m_Used && !Pusi) {	/* wait for payload unit start indicator */
//              printf("Mittendrin pid %d tid %d mask %02x \n", Pid(), Tid(), Mask());
		return true;
	}
	if (m_Used && Pusi) {	/* reset at payload unit start */
//              int length = (((m_Buffer[1] & 0x0F) << 8) | m_Buffer[2]) + 3;
//              printf("RESET expect %d got %d for pid %d tid %d mask %02x \n",length, m_Used, Pid(), Tid(), Mask());
		Reset ();
	}
	if (m_Used + Length >= (int) sizeof (m_Buffer)) {
		esyslog ("ERROR: Mcli: Section handler buffer overflow (%d bytes lost)", Length);
		Reset ();
		return true;
	}

	memcpy (m_Buffer + m_Used, Data, Length);
	m_Used += Length;
	if (m_Used > 3) {
		int length = (((m_Buffer[1] & 0x0F) << 8) | m_Buffer[2]) + 3;
//              printf("-> pid %d Tid %d Mask %02x expect %d got %d\n",Pid(), Tid(), Mask(), length, m_Used);
		if (m_Used >= length) {
//                      printf("Section complete\n");
			m_Used = 0;
			if (write (m_Pipe[1], m_Buffer, length) < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK)
					//dsyslog ("cMcliFilter::PutSection socket overflow, " "Pid %4d Tid %3d", m_Pid, m_Tid);
					;
				else
					return false;
			}
		}

		if (m_Used > length) {
			dsyslog ("cMcliFilter::PutSection: m_Used > length !  Pid %2d, Tid%2d " "(len %3d, got %d/%d)", m_Pid, m_Tid, Length, m_Used, length);
			if (Length < TS_SIZE - 5) {
				// TS packet not full -> this must be last TS packet of section data -> safe to reset now
				Reset ();
			}
		}
	}
	return true;
}

void cMcliFilter::Reset (void)
{
	if (m_Used)
		dsyslog ("cMcliFilter::Reset skipping %d bytes", m_Used);
	m_Used = 0;
}

bool cMcliFilter::IsClosed (void)
{
	char m_Buffer[3] = { 0, 0, 0 };	/* tid 0, 0 bytes */

	// Test if pipe/socket has been closed by writing empty section
	if (write (m_Pipe[1], m_Buffer, 3) < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		if (errno != ECONNREFUSED && errno != ECONNRESET && errno != EPIPE)
			esyslog ("cMcliFilter::IsClosed failed: %m");

		return true;
	}

	return false;
}

// --- cMcliFilters -----------------------------------------------------

cMcliFilters::cMcliFilters (void):
cThread ("mcli: sections assembler")
{
	m_PB = NULL;
}

cMcliFilters::~cMcliFilters ()
{
}

int cMcliFilters::PutTS (const uchar * data, int len)
{
	u_short pid = (((u_short) data[1] & PID_MASK_HI) << 8) | data[2];
	if (m_PB && WantPid (pid)) {
		int i;
		for (i = 0; i < len; i += TS_SIZE) {
			unsigned char *ptr = m_PB->PutStart (TS_SIZE);
			if (ptr) {
				memcpy (ptr, data + i, TS_SIZE);
				m_PB->PutEnd (TS_SIZE, 0, 0);
			}
		}
	}
	return 0;
}

void cMcliFilters::CloseFilter (int Handle)
{
//      printf("cMcliFilters::CloseFilter: %d\n", Handle);

	close (Handle);
	GarbageCollect ();
	int pid = GetPid (Handle);
	if (pid != -1) {
		m_pl.SetPid (pid, -1);
	}
}

int cMcliFilters::OpenFilter (u_short Pid, u_char Tid, u_char Mask)
{
	GarbageCollect ();
//      printf("cMcliFilters::OpenFilter: %d %d %02x\n", Pid, Tid, Mask);

	if (!WantPid (Pid)) {
		m_pl.SetPid (Pid, 0xffff);
	}

	if (!m_PB) {
		m_PB = new cMyPacketBuffer (10000 * TS_SIZE, 10000);
		m_PB->SetTimeouts (0, 1000 * 20);
	}
	Start ();

	cMcliFilter *f = new cMcliFilter (Pid, Tid, Mask);
	int fh = f->ReadPipe ();

	Lock ();
	Add (f);
	Unlock ();

	return fh;
}

int cMcliPidList::GetTidFromPid (int pid)
{
	for (cMcliPid * p = First (); p; p = Next (p)) {
		if (p->Pid () == pid) {
//                      printf("Found pid %d -> tid %d\n",pid, p->Tid());
			return p->Tid ();
		}
	}
	return -1;
}

void cMcliPidList::SetPid (int Pid, int Tid)
{
	if (Tid >= 0) {
		for (cMcliPid * p = First (); p; p = Next (p)) {
			if (p->Pid () == Pid) {
//                              printf("Change pid %d -> tid %d\n", Pid, Tid);
				if (Tid != 0xffff) {
					p->SetTid (Tid);
				}
				return;
			}
		}
		cMcliPid *pid = new cMcliPid (Pid, Tid);
		Add (pid);
//                      printf("Add pid %d -> tid %d\n", Pid, Tid);
	} else {
		for (cMcliPid * p = First (); p; p = Next (p)) {
			if (p->Pid () == Pid) {
//                              printf("Del pid %d\n", Pid);
				Del (p);
				return;
			}
		}
	}
}

int cMcliFilters::GetPid (int Handle)
{
	int used = 0;
	int pid = -1;

	LOCK_THREAD;
	for (cMcliFilter * fi = First (); fi; fi = Next (fi)) {
		if (fi->ReadPipe () == Handle) {
			pid = fi->Pid ();
			used++;
		} else if (pid != -1 && (pid == fi->Pid ())) {
			used++;
			break;
		}
	}
	if (used == 1) {
		return pid;
	}
	return -1;
}

bool cMcliFilters::WantPid (int pid)
{
	LOCK_THREAD;
	for (cMcliFilter * fi = First (); fi; fi = Next (fi)) {
		if (pid == fi->Pid ()) {
			return true;
		}
	}
	return false;
}

void cMcliFilters::GarbageCollect (void)
{
	LOCK_THREAD;
	for (cMcliFilter * fi = First (); fi;) {
		if (fi->IsClosed ()) {
			if (errno == ECONNREFUSED || errno == ECONNRESET || errno == EPIPE) {
//                              printf ("cMcliFilters::GarbageCollector: filter closed: Pid %4d, Tid %3d, Mask %2x (%d filters left)\n", (int) fi->Pid (), (int) fi->Tid (), fi->Mask (), Count () - 1);

				cMcliFilter *next = Prev (fi);
				Del (fi);
				fi = next ? Next (next) : First ();
			} else {
				esyslog ("cMcliFilters::GarbageCollector() error: " "Pid %4d, Tid %3d, Mask %2x (%d filters left) failed", (int) fi->Pid (), (int) fi->Tid (), fi->Mask (), Count () - 1);
				LOG_ERROR;
				fi = Next (fi);
			}
		} else {
			fi = Next (fi);
		}
	}
}

void cMcliFilters::Action (void)
{

	while (Running ()) {
		m_PB->GetEnd ();
		int size;
		const uchar *block = m_PB->GetStart (&size, 0, 0);
		if (block) {
			int tid = -1;
			u_short pid = (((u_short) block[1] & PID_MASK_HI) << 8) | block[2];
			bool Pusi = (block[1] & 0x40) >> 6;
			if (Pusi) {
				tid = (int) block[5];
				m_pl.SetPid (pid, tid);
//                              printf("Pusi pid %d tid %d\n", pid, tid);
			} else {
				tid = m_pl.GetTidFromPid (pid);
				if (tid == -1) {
					printf ("Failed to get tid for pid %d\n", pid);
				}
			}
			int len = 188 - 4 - Pusi;	//(block[5+Pusi]&0xf)<<8|block[6+Pusi];
//                      printf("pid:%d tid:%d Pusi: %d len: %d\n", pid, tid, Pusi, len);

			LOCK_THREAD;
			cMcliFilter *f = First ();
			while (f) {
				cMcliFilter *next = Next (f);
				if (tid != -1 && f->Matches (pid, tid)) {
//                                      printf("Match!!!!");
					if (!f->PutSection (block + 4 + Pusi, len, Pusi)) {
						if (errno != ECONNREFUSED && errno != ECONNRESET && errno != EPIPE) {
							esyslog ("mcli: couldn't send section packet");
						}
						Del (f);
						// Filter was closed.
						//  - need to check remaining filters for another match
					}	// if
				}
				f = next;
			}
		}
	}

	DELETENULL (m_PB);
	dsyslog ("McliFilters::Action() ended");
}
