#ifndef __IGMP_H
#define __IGMP_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <mstcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "clist.h"
#include "thread.h"
#include "misc.h"


#define IGMPV3_MR_MODE_IS_INCLUDE   0x01
#define IGMPV3_MR_MODE_IS_EXCLUDE   0x02
#define IGMPV3_MR_CHANGE_TO_INCLUDE 0x03
#define IGMPV3_MR_CHANGE_TO_EXCLUDE 0x04
#define IGMPV3_MR_ALLOW_NEW_SOURCES 0x05
#define IGMPV3_MR_BLOCK_OLD_SOURCES 0x06

class cStream;
class cStreamer;

class cMulticastGroup : public cListObject
{
	public:
		cMulticastGroup(in_addr_t Group);
        in_addr_t group;
		in_addr_t reporter;
		struct timeval timeout;
		struct timeval v1timer;
		struct timeval retransmit;
		cStream* stream;
};

class cIgmpMain;

class cIgmpListener : public cThread
{
	public:
		cIgmpListener(cIgmpMain* igmpmain);

		bool Initialize(in_addr_t bindaddr);
		void Destruct(void);
		bool Membership(in_addr_t mcaddr, bool Add);
		void IGMPSendQuery(in_addr_t Group, int Timeout);

	private:
		int m_socket;
		cIgmpMain* m_IgmpMain;

		void Parse(char*, int);
		
		virtual void Action();	

};

//-------------------------------------------------------------------------------------------------------------------

class cIgmpMain : public cThread
{
	public:
		cIgmpMain(cStreamer* streamer, in_addr_t bindaddr);
		~cIgmpMain(void);
		bool StartListener(void);
		void Destruct(void);
		void ProcessIgmpV3QueryMessage(in_addr_t Group, in_addr_t Sender);
		void ProcessIgmpV3ReportMessage(int type, in_addr_t Group, in_addr_t Sender);
		
	private:
		// Parent streamer
		cStreamer* m_streamer;
		
		// Listener
		cIgmpListener* m_IgmpListener;
		in_addr_t m_bindaddr;
		cList<cMulticastGroup> m_Groups;
		
		// General Query / Timer
		struct timeval m_GeneralQueryTimer;
		bool m_Querier;
		int m_StartupQueryCount;

		void IGMPStartGeneralQueryTimer();
		void IGMPStartOtherQuerierPresentTimer();
		void IGMPSendGeneralQuery();

		// Group Query / Timer
	        void IGMPStartTimer(cMulticastGroup* Group, in_addr_t Member);
        	void IGMPStartV1HostTimer(cMulticastGroup* Group);
	        void IGMPStartTimerAfterLeave(cMulticastGroup* Group, unsigned int MaxResponseTime);
	        void IGMPStartRetransmitTimer(cMulticastGroup* Group);
	        void IGMPClearRetransmitTimer(cMulticastGroup* Group);
	        void IGMPSendGroupQuery(cMulticastGroup* Group);
	        void IGMPStartMulticast(cMulticastGroup* Group);
	        void IGMPStopMulticast(cMulticastGroup* Group);

		// Main thread 				
		virtual void Action();
		cCondWait m_CondWait;

		// Helper				
		cMulticastGroup* FindGroup(in_addr_t Group) const;
		bool IsGroupinRange(in_addr_t groupaddr);
};

#endif
