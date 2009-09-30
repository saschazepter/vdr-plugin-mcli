#ifndef	__STREAMER_H
#define __STREAMER_H

#include "misc.h"

class cMulticastGroup;
class cIgmpMain;

class cStreamer
{
	public:
			cStreamer();
			
			void Run();
			void Stop();
			void SetBindAddress(in_addr_t bindaddr);
			void SetStreamPort(int portnum);
			
			bool IsGroupinRange(in_addr_t groupaddr);
			void StartMulticast(cMulticastGroup* Group);
			void StopMulticast(cMulticastGroup* Group);
			

	private:
			cIgmpMain* m_IgmpMain;
			in_addr_t m_bindaddr;
			int m_portnum;
};

#endif