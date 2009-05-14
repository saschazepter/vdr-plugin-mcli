/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

/*
 * mcli.c: A plugin for the Video Disk Recorder
 *
 * $Id: mcli.c 1664 2009-05-14 10:18:48Z fliegl $
 */

#include <vdr/plugin.h>
#include "filter.h"
#include "device.h"
#define MCLI_MAX_DEVICES 8
#define MCLI_DEVICE_TIMEOUT 120

#define CAM_MENU_TEST

static const char *VERSION = trNOOP("0.0.1");
static const char *DESCRIPTION = trNOOP("NetCeiver Client Application");
static const char *MENUSETUPENTRY = trNOOP("NetCeiver Client Application");
//static const char *MAINMENUENTRY = "NetCeiver Client";

static int recv_init_done = 0;
static int mld_init_done = 0;
static int api_init_done = 0;
static int devices = 0;
static int reconf = 0;
static int mmi_init_done = 0;

typedef struct
{
	int port;
	char iface[IFNAMSIZ];
	char cmd_sock_path[_POSIX_PATH_MAX];
	int tuner_type_limit[FE_DVBS2 + 1];
	int mld_start;
} cmdline_t;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------


class cMcliDeviceObject:public cListObject
{
      public:
	cMcliDeviceObject (cMcliDevice * d)
	{
		m_d = d;
	}
	 ~cMcliDeviceObject (void)
	{
	}
	cMcliDevice *d (void)
	{
		return m_d;
	}
      private:
	cMcliDevice * m_d;
};

class cMcliDeviceList:public cList < cMcliDeviceObject >
{
      public:
	cMcliDeviceList (void)
	{
	};
	~cMcliDeviceList () {
		printf ("Delete my Dev list\n");
	};
	cMcliDeviceObject *find_dev_by_uuid (const char *uuid);
	int count_dev_by_type (const fe_type_t type);
};

cMcliDeviceObject *cMcliDeviceList::find_dev_by_uuid (const char *uuid)
{
	for (cMcliDeviceObject * d = First (); d; d = Next (d)) {
		if (!strcmp (d->d ()->GetUUID (), uuid)) {
			return d;
		}
	}
	return NULL;
}

int cMcliDeviceList::count_dev_by_type (const fe_type_t type)
{
	int ret = 0;
	for (cMcliDeviceObject * d = First (); d; d = Next (d)) {
		if (type == d->d ()->GetFEType ()) {
			ret++;
		}
	}
	return ret;
}

class cMenuSetupMcli:public cMenuSetupPage
{
      private:
	cmdline_t * m_cmd;
      protected:
	virtual void Store (void);
      public:
	cMenuSetupMcli (cmdline_t * cmd);
};

cMenuSetupMcli::cMenuSetupMcli (cmdline_t * cmd)
{
	m_cmd = cmd;
	Add (new cMenuEditIntItem (tr ("DVB-C"), &m_cmd->tuner_type_limit[FE_QAM]));
	Add (new cMenuEditIntItem (tr ("DVB-T"), &m_cmd->tuner_type_limit[FE_OFDM]));
	Add (new cMenuEditIntItem (tr ("DVB-S"), &m_cmd->tuner_type_limit[FE_QPSK]));
	Add (new cMenuEditIntItem (tr ("DVB-S2"), &m_cmd->tuner_type_limit[FE_DVBS2]));		
}

void cMenuSetupMcli::Store (void)
{
	SetupStore ("DVB-C", m_cmd->tuner_type_limit[FE_QAM]);
	SetupStore ("DVB-T", m_cmd->tuner_type_limit[FE_OFDM]);
	SetupStore ("DVB-S", m_cmd->tuner_type_limit[FE_QPSK]);
	SetupStore ("DVB-S2", m_cmd->tuner_type_limit[FE_DVBS2]);
	reconf=1;
}

typedef struct {
	char uuid[UUID_SIZE];
	int slot;
	char info[MMI_TEXT_LENGTH];
} cam_list_t;

class cPluginMcli:public cPlugin, public cThread
{
      private:
	// Add any member variables or functions you may need here.
	cMcliDeviceList m_devs;
	cmdline_t m_cmd;
	UDPContext *m_cam_mmi;

      public:
	cPluginMcli (void);
	virtual ~ cPluginMcli ();
	virtual const char *Version (void)
	{
		return VERSION;
	}
	virtual const char *Description (void)
	{
		return DESCRIPTION;
	}
	virtual const char *CommandLineHelp (void);
	virtual bool ProcessArgs (int argc, char *argv[]);
	virtual bool Initialize (void);
	virtual bool Start (void);
	virtual void Stop (void);
	virtual void Housekeeping (void);
	virtual void MainThreadHook (void);
	virtual cString Active (void);
	virtual time_t WakeupTime (void);
        virtual const char *MenuSetupPluginEntry(void)
        { 
        	return tr(MENUSETUPENTRY);
	}
//	virtual const char *MainMenuEntry (void)
//	{
//              return MAINMENUENTRY;
//	}
	virtual cOsdObject *MainMenuAction (void);
	virtual cMenuSetupPage *SetupMenu (void);
	virtual bool SetupParse (const char *Name, const char *Value);
	virtual bool Service (const char *Id, void *Data = NULL);
	virtual const char **SVDRPHelpPages (void);
	virtual cString SVDRPCommand (const char *Command, const char *Option, int &ReplyCode);
	virtual void Action (void);
	void reconfigure(void);

	int CamFind(cam_list_t *cam_list, int *len);
	int CamMenuOpen(cam_list_t *cam);
	int CamMenuSend(int fd, char *c);
	int CamMenuReceive(int fd, char *buf, int bufsize);
	void CamMenuClose(int fd);
	int CamPollText(mmi_info_t *text);
	int CamGetMMIBroadcast(void);
#ifdef CAM_MENU_TEST	
	void CamMenuTest(void);
#endif	
};

int cPluginMcli::CamGetMMIBroadcast(void)
{
	// Call this code periodically to find out if any CAM out there want's us to tell something.
	// If it's relevant to us we need to check if any of our DVB-Devices gets programm from a NetCeiver with this UUID.
	// The text received should pop up via OSD with a CAM-Session opened afterwards (CamMenuOpen...CamMenuReceive...CamMenuSend...CamMenuClose).
	mmi_info_t m;
	if(CamPollText(&m)>0) {
		printf("NetCeiver %s CAM slot %d Received %s valid for:\n", m.uuid, m.slot, m.mmi_text);
		for(int i=0; i<m.caid_num; i++) {
			caid_mcg_t *c=m.caids+i;
			int sid;
			int satpos;
			fe_type_t type;
			recv_sec_t sec;
			struct dvb_frontend_parameters fep;
			int vpid;
			
			mcg_get_id(&c->mcg, &sid);
			mcg_get_satpos(&c->mcg, &satpos);
			mcg_to_fe_parms(&c->mcg, &type, &sec, &fep, &vpid);
			
			printf("CAID:%04x, SatPos:%d Freqency:%d SID:%04x\n", c->caid, satpos, fep.frequency, sid);
		}
		if(m.caid_num && m.caids) {
			free(m.caids);
		}
	}
	return 0;
}		


int cPluginMcli::CamFind(cam_list_t *cam_list, int *len)
{
	int n, cnt=0, i;
	netceiver_info_list_t *nc_list=nc_get_list();
	printf("Looking for netceivers out there....\n");
        nc_lock_list();
        for (n = 0; n < nc_list->nci_num; n++) {
		netceiver_info_t *nci = nc_list->nci + n;
		printf("\nFound NetCeiver: %s \n",nci->uuid);
		printf("    CAMS [%d]: \n",nci->cam_num);
		for (i = 0; i < nci->cam_num; i++) {
			switch(nci->cam[i].status) {
				case 2://DVBCA_CAMSTATE_READY:
					printf("    %i.CAM - %s\n",i+1, nci->cam[i].menu_string);
					if(cnt < *len) {
						cam_list[cnt].slot=i;
						strcpy(cam_list[cnt].uuid, nci->uuid);
						strcpy(cam_list[cnt].info, nci->cam[i].menu_string);
					}
					cnt++;
					break;
                        }
                }
	}
	nc_unlock_list();
	*len=cnt;
	return cnt;
}

int cPluginMcli::CamMenuOpen(cam_list_t *cam)
{
	int mmi_session = mmi_open_menu_session(cam->uuid, m_cmd.iface, m_cmd.port, cam->slot);
	if(mmi_session>0) {
		sleep(1);
		CamMenuSend(mmi_session, (char *)"00000000000000\n");
	} 
	return mmi_session;
}

int cPluginMcli::CamMenuSend(int mmi_session, char *c)
{
	return mmi_send_menu_answer(mmi_session, c, strlen(c));
}

int cPluginMcli::CamMenuReceive(int mmi_session, char *buf, int bufsize)
{
	return mmi_get_menu_text( mmi_session, buf, bufsize, 50000);
}

void cPluginMcli::CamMenuClose(int mmi_session)
{
	close(mmi_session);
}

int cPluginMcli::CamPollText(mmi_info_t *text)
{
	return mmi_poll_for_menu_text(m_cam_mmi, text, 10);
}

#ifdef CAM_MENU_TEST
void cPluginMcli::CamMenuTest(void)
{
	cam_list_t c[16];
	int len=16;

	// Find all operational CAMs.
	if(CamFind(c, &len)) {
		printf("Opening CAM Menu at NetCeiver %s Slot %d\n", c[0].uuid, c[0].slot);
		
		// connect to CAM slot 0 of first NetCeiver (THIS CODE IS JUST FOR TESTING)
		int mmi_session=CamMenuOpen(&c[0]);
		char buf[MMI_TEXT_LENGTH];
		printf("mmi_session: %d\n", mmi_session);
		if (mmi_session>0) {
			time_t t=time(NULL);
			while((time(NULL)-t)<10) {
				// receive the CAM MENU
				if(CamMenuReceive(mmi_session, buf, MMI_TEXT_LENGTH)>0) {
					printf("MMI: %s\n",buf);
					break;
				}
				
				// send key events to the CAM via CamMenuSend (NOT SHOWN HERE)
				
				sleep(1);
			}
			CamMenuClose(mmi_session);
		}
	}
}
#endif

cPluginMcli::cPluginMcli (void)
{
	// Initialize any member variables here.
	// DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
	// VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
	printf ("cPluginMcli::cPluginMcli\n");
	int i;
	//init parameters
	memset (&m_cmd, 0, sizeof (cmdline_t));

	for (i = 0; i <= FE_DVBS2; i++) {
		m_cmd.tuner_type_limit[i] = MCLI_MAX_DEVICES;
	}
	m_cmd.port = 23000;
	m_cmd.mld_start = 1;
	strcpy (m_cmd.cmd_sock_path, API_SOCK_NAMESPACE);
}

cPluginMcli::~cPluginMcli ()
{
	printf ("cPluginMcli::~cPluginMcli\n");
	
	if(mmi_init_done) {
		mmi_broadcast_client_exit(m_cam_mmi);
	}
	if (api_init_done) {
		api_sock_exit ();
	}
	if (mld_init_done) {
		mld_client_exit ();
	}
	if (recv_init_done) {
		recv_exit ();
	}
	// Clean up after yourself!
}

const char *cPluginMcli::CommandLineHelp (void)
{
	return ("  --ifname <network interface>\n" "  --port <port> (default: -port 23000)\n" "  --dvb-s <num> --dvb-c <num> --dvb-t <num> --atsc <num> --dvb-s2 <num>\n" "    limit number of device types (default: 8 of every type)\n" "  --mld-reporter-disable\n" "  --sock-path <filepath>\n" "\n");
}

bool cPluginMcli::ProcessArgs (int argc, char *argv[])
{
	printf ("cPluginMcli::ProcessArgs\n");
	int tuners = 0,i;
	char c;
	int ret;

	while (1) {
		//int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"port", 1, 0, 0},	//0
			{"ifname", 1, 0, 0},	//1
			{"dvb-s", 1, 0, 0},	//3
			{"dvb-c", 1, 0, 0},	//4
			{"dvb-t", 1, 0, 0},	//5
			{"atsc", 1, 0, 0},	//6
			{"dvb-s2", 1, 0, 0},	//7
			{"mld-reporter-disable", 0, 0, 0},	//9
			{"sock-path", 1, 0, 0},	//10
			{NULL, 0, 0, 0}
		};

		ret = getopt_long_only (argc, argv, "", long_options, &option_index);
		c = (char) ret;
		if (ret == -1 || c == '?') {
			break;
		}

		switch (option_index) {
		case 0:
			m_cmd.port = atoi (optarg);
			break;
		case 1:
			strncpy (m_cmd.iface, optarg, IFNAMSIZ - 1);
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			i = atoi (optarg);
			if (!tuners) {
				memset (m_cmd.tuner_type_limit, 0, sizeof (m_cmd.tuner_type_limit));
			}
			m_cmd.tuner_type_limit[option_index - 2] = i;
			tuners += i;
			break;
		case 7:
			m_cmd.mld_start = 0;
			break;
		case 8:
			strncpy (m_cmd.cmd_sock_path, optarg, _POSIX_PATH_MAX - 1);
			break;
		default:
			printf ("?? getopt returned character code 0%o ??\n", c);
		}
	}
	// Implement command line argument processing here if applicable.
	return true;
}

void cPluginMcli::Action (void)
{
	netceiver_info_list_t *nc_list = nc_get_list ();
	printf ("Looking for netceivers out there....\n");
	while (Running ()) {
		Lock ();
		nc_lock_list ();
		time_t now=time(NULL);
		
		for (int n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			for (int i = 0; i < nci->tuner_num; i++) {
				cMcliDeviceObject *d=m_devs.find_dev_by_uuid (nci->tuner[i].uuid);
				if ((now-nci->lastseen)>MCLI_DEVICE_TIMEOUT) {
					if(d) {
						printf("Remove Tuner %s [%s]\n",nci->tuner[i].fe_info.name, nci->tuner[i].uuid);
						d->d ()->SetEnable (false);
#if VDRVERSNUM >= 10600
						d->d ()->SetAvoidDevice (d->d ());
#endif
						delete d->d ();
						m_devs.Del (d);
					}
					continue;
				}
				
				if (devices >= MCLI_MAX_DEVICES) {
//                                      printf("MCLI_MAX_DEVICES reached\n");
					continue;
				}
				fe_type_t type = nci->tuner[i].fe_info.type;
				if (m_devs.count_dev_by_type (type) == m_cmd.tuner_type_limit[type]) {
//                                      printf("Limit: %d %d>%d\n", type, m_devs.count_dev_by_type (type), m_cmd.tuner_type_limit[type]);
					continue;
				}
				if (!d) {
					printf ("  Tuner: %s [%s], Type %d\n", nci->tuner[i].fe_info.name, nci->tuner[i].uuid, type);
					cMcliDevice *m = new cMcliDevice;
					d = new cMcliDeviceObject (m);

					m->SetUUID (nci->tuner[i].uuid);
					m->SetFEType (type);
					m->SetEnable (true);
					m_devs.Add (d);
				}
			}
		}
		nc_unlock_list ();
		Unlock ();

		CamGetMMIBroadcast();
		
		usleep (250 * 1000);
	}
}

bool cPluginMcli::Initialize (void)
{
	printf ("cPluginMcli::Initialize\n");
	// Initialize any background activities the plugin shall perform.
	if (!recv_init (m_cmd.iface, m_cmd.port)) {
		recv_init_done = 1;
	}
	if (m_cmd.mld_start && !mld_client_init (m_cmd.iface)) {
		mld_init_done = 1;
	}
	if (!api_sock_init (m_cmd.cmd_sock_path)) {
		api_init_done = 1;
	}
	m_cam_mmi = mmi_broadcast_client_init(m_cmd.port, m_cmd.iface);
        if (!m_cam_mmi) {
        	mmi_init_done = 1;
        }
	
	return true;
}


bool cPluginMcli::Start (void)
{
	printf ("cPluginMcli::Start\n");
	cThread::Start ();
	// Start any background activities the plugin shall perform.
	return true;
}

void cPluginMcli::Stop (void)
{
	printf ("cPluginMcli::Stop\n");
	cThread::Cancel (0);
	for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d)) {
		d->d ()->SetEnable (false);
	}
	// Stop any background activities the plugin is performing.
}

void cPluginMcli::Housekeeping (void)
{
	printf ("cPluginMcli::Housekeeping\n");
	// Perform any cleanup or other regular tasks.
}

void cPluginMcli::MainThreadHook (void)
{
	// Perform actions in the context of the main program thread.
	// WARNING: Use with great care - see PLUGINS.html!
//      printf("cPluginMcli::MainThreadHook\n");
	if(reconf) {
		reconfigure();
		reconf=0;
	}
}

cString cPluginMcli::Active (void)
{
	printf ("cPluginMcli::Active\n");
	// Return a message string if shutdown should be postponed
	return NULL;
}

time_t cPluginMcli::WakeupTime (void)
{
	printf ("cPluginMcli::WakeupTime\n");
	// Return custom wakeup time for shutdown script
	return 0;
}

void cPluginMcli::reconfigure(void)
{
	LOCK_THREAD;
	for (cMcliDeviceObject * d = m_devs.First (); d;) {
		cMcliDeviceObject *next = m_devs.Next (d);
		d->d ()->SetEnable (false);
#if VDRVERSNUM >= 10600
		d->d ()->SetAvoidDevice (d->d ());
#endif
		delete d->d ();
		m_devs.Del (d);
		d = next;
	}
}

cOsdObject *cPluginMcli::MainMenuAction (void)
{
	printf ("cPluginMcli::MainMenuAction\n");
	// Perform the action when selected from the main VDR menu.
	return NULL;
}


cMenuSetupPage *cPluginMcli::SetupMenu (void)
{
	printf ("cPluginMcli::SetupMenu\n");
#ifdef CAM_MENU_TEST
	CamMenuTest();
#endif
	// Return a setup menu in case the plugin supports one.
	return new cMenuSetupMcli(&m_cmd);
}

bool cPluginMcli::SetupParse (const char *Name, const char *Value)
{
	printf ("cPluginMcli::SetupParse\n");
	if(!strcasecmp(Name, "DVB-C") && m_cmd.tuner_type_limit[FE_QAM]==MCLI_MAX_DEVICES ) m_cmd.tuner_type_limit[FE_QAM]=atoi(Value);
	else if(!strcasecmp(Name, "DVB-T") && m_cmd.tuner_type_limit[FE_OFDM]==MCLI_MAX_DEVICES ) m_cmd.tuner_type_limit[FE_OFDM]=atoi(Value);
	else if(!strcasecmp(Name, "DVB-S") && m_cmd.tuner_type_limit[FE_QPSK]==MCLI_MAX_DEVICES ) m_cmd.tuner_type_limit[FE_QPSK]=atoi(Value);
	else if(!strcasecmp(Name, "DVB-S2") && m_cmd.tuner_type_limit[FE_DVBS2]==MCLI_MAX_DEVICES ) m_cmd.tuner_type_limit[FE_DVBS2]=atoi(Value);
	else return false;
	return true;
}

bool cPluginMcli::Service (const char *Id, void *Data)
{
	printf ("cPluginMcli::Service\n");
	// Handle custom service requests from other plugins
	return false;
}

const char **cPluginMcli::SVDRPHelpPages (void)
{
	printf ("cPluginMcli::SVDRPHelpPages\n");
	// Return help text for SVDRP commands this plugin implements
	return NULL;
}

cString cPluginMcli::SVDRPCommand (const char *Command, const char *Option, int &ReplyCode)
{
	printf ("cPluginMcli::SVDRPCommand\n");
	// Process SVDRP commands this plugin implements
	return NULL;
}

VDRPLUGINCREATOR (cPluginMcli);	// Don't touch this!
