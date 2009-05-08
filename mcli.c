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
 * $Id: mcli.c 1640 2009-05-08 20:57:27Z fliegl $
 */

#include <vdr/plugin.h>
#include "filter.h"
#include "device.h"
#define MCLI_MAX_DEVICES 8
#define MCLI_DEVICE_TIMEOUT 30

static const char *VERSION = trNOOP("0.0.1");
static const char *DESCRIPTION = trNOOP("NetCeiver Client Application");
static const char *MENUSETUPPLUGINENTRY = trNOOP("NetCeiver Client Application");

//static const char *MAINMENUENTRY = "NetCeiver-Client";

static int recv_init_done = 0;
static int mld_init_done = 0;
static int api_init_done = 0;
static int devices = 0;
static int reconf = 0;

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


class cPluginMcli:public cPlugin, public cThread
{
      private:
	// Add any member variables or functions you may need here.
	cMcliDeviceList m_devs;
	cmdline_t m_cmd;

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
	virtual const char *MainMenuEntry (void)
	{
		return NULL;
//              return MAINMENUENTRY;
	}
	virtual cOsdObject *MainMenuAction (void);
	virtual cMenuSetupPage *SetupMenu (void);
	virtual bool SetupParse (const char *Name, const char *Value);
	virtual bool Service (const char *Id, void *Data = NULL);
	virtual const char **SVDRPHelpPages (void);
	virtual cString SVDRPCommand (const char *Command, const char *Option, int &ReplyCode);
	virtual void Action (void);
	void reconfigure(void);
};

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
