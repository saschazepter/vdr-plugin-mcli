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
 * $Id: mcli.c 1769 2009-06-04 18:32:48Z fliegl $
 */

#include <vdr/plugin.h>
#include "filter.h"
#include "device.h"
#include "cam_menu.h"
#define MCLI_MAX_DEVICES 8
#define MCLI_DEVICE_TIMEOUT 120

static const char *VERSION = "0.0.1";
static const char *DESCRIPTION = trNOOP ("NetCeiver Client Application");

#ifndef REELVDR
static const char *MENUSETUPENTRY = trNOOP ("NetCeiver Client Application");
#endif
static const char *MAINMENUENTRY = trNOOP ("Common Interface");

static int recv_init_done = 0;
static int mld_init_done = 0;
static int api_init_done = 0;
static int devices = 0;
static int reconf = 0;
static int reconf_full = 0;
static int mmi_init_done = 0;

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
	reconf = 1;
	reconf_full = 1;
}

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
#ifdef REELVDR
	virtual bool HasSetupOptions (void)
	{
		return false;
	}
#endif
	virtual const char *MenuSetupPluginEntry (void)
	{
#ifdef REELVDR
		return NULL;
#else
		return tr (MENUSETUPENTRY);
#endif
	}
	virtual const char *MainMenuEntry (void)
	{
		return tr (MAINMENUENTRY);
	}
	virtual cOsdObject *MainMenuAction (void);
	virtual cMenuSetupPage *SetupMenu (void);
	virtual bool SetupParse (const char *Name, const char *Value);
	virtual bool Service (const char *Id, void *Data = NULL);
	virtual const char **SVDRPHelpPages (void);
	virtual cString SVDRPCommand (const char *Command, const char *Option, int &ReplyCode);
	virtual void Action (void);
	void ExitMcli (void);
	bool InitMcli (void);
	void reconfigure (void);

	int CamPollText (mmi_info_t * text);
#ifdef REELVDR
	virtual cOsdObject *AltMenuAction (void);
#endif
};

#ifdef REELVDR
cOsdObject *cPluginMcli::AltMenuAction (void)
{
	// Call this code periodically to find out if any CAM out there want's us to tell something.
	// If it's relevant to us we need to check if any of our DVB-Devices gets programm from a NetCeiver with this UUID.
	// The text received should pop up via OSD with a CAM-Session opened afterwards (CamMenuOpen...CamMenuReceive...CamMenuSend...CamMenuClose).
	mmi_info_t m;
	if (CamPollText (&m) > 0) {
		printf ("NetCeiver %s CAM slot %d Received %s valid for:\n", m.uuid, m.slot, m.mmi_text);
		for (int i = 0; i < m.caid_num; i++) {
			caid_mcg_t *c = m.caids + i;
			int satpos;
			fe_type_t type;
			recv_sec_t sec;
			struct dvb_frontend_parameters fep;
			int vpid;

			mcg_get_satpos (&c->mcg, &satpos);
			mcg_to_fe_parms (&c->mcg, &type, &sec, &fep, &vpid);

			for (int j = 0; j < m_devs.Count (); j++) {
				cMcliDevice *dev = m_devs.Get (j)->d ();
				//printf("satpos: %i vpid: %i fep.freq: %i dev.freq: %i\n", satpos, vpid, fep.frequency, dev->CurChan()->Frequency());
				if ((int) fep.frequency / 1000 == dev->CurChan ()->Frequency ())
					return new cCamMenu (&m_cmd, &m);
			}
			printf ("SID/Program Number:%04x, SatPos:%d Freqency:%d\n", c->caid, satpos, fep.frequency);
		}
		if (m.caid_num && m.caids) {
			free (m.caids);
		}
	}
	return NULL;
}
#endif

int cPluginMcli::CamPollText (mmi_info_t * text)
{
	if (mmi_init_done && !reconf) {
		return mmi_poll_for_menu_text (m_cam_mmi, text, 10);
	} else {
		return 0;
	}
}

cPluginMcli::cPluginMcli (void)
{
	// printf ("cPluginMcli::cPluginMcli\n");
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
	ExitMcli ();

}

bool cPluginMcli::InitMcli (void)
{
	if (!recv_init (m_cmd.iface, m_cmd.port)) {
		recv_init_done = 1;
	}
	if (m_cmd.mld_start && !mld_client_init (m_cmd.iface)) {
		mld_init_done = 1;
	}
	if (!api_sock_init (m_cmd.cmd_sock_path)) {
		api_init_done = 1;
	}
	m_cam_mmi = mmi_broadcast_client_init (m_cmd.port, m_cmd.iface);
	if (m_cam_mmi > 0) {
		mmi_init_done = 1;
	}
	return true;
}

void cPluginMcli::ExitMcli (void)
{
	if (mmi_init_done) {
		mmi_broadcast_client_exit (m_cam_mmi);
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
}

const char *cPluginMcli::CommandLineHelp (void)
{
	return ("  --ifname <network interface>\n" "  --port <port> (default: -port 23000)\n" "  --dvb-s <num> --dvb-c <num> --dvb-t <num> --atsc <num> --dvb-s2 <num>\n" "    limit number of device types (default: 8 of every type)\n" "  --mld-reporter-disable\n" "  --sock-path <filepath>\n" "\n");
}

bool cPluginMcli::ProcessArgs (int argc, char *argv[])
{
	printf ("cPluginMcli::ProcessArgs\n");
	int tuners = 0, i;
	char c;
	int ret;

	while (1) {
		//int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"port", 1, 0, 0},	//0
			{"ifname", 1, 0, 0},	//1
			{"dvb-s", 1, 0, 0},	//2
			{"dvb-c", 1, 0, 0},	//3
			{"dvb-t", 1, 0, 0},	//4
			{"atsc", 1, 0, 0},	//5
			{"dvb-s2", 1, 0, 0},	//6
			{"mld-reporter-disable", 0, 0, 0},	//7
			{"sock-path", 1, 0, 0},	//8
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
	bool channel_switch_ok=false;
	
	while (Running ()) {
		Lock ();
		nc_lock_list ();
		time_t now = time (NULL);
		for (int n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			for (int i = 0; i < nci->tuner_num; i++) {
				cMcliDeviceObject *d = m_devs.find_dev_by_uuid (nci->tuner[i].uuid);
				if ((now - nci->lastseen) > MCLI_DEVICE_TIMEOUT) {
					if (d) {
						cPluginManager::CallAllServices ("OnDelMcliDevice", d->d ());
						printf ("Remove Tuner %s [%s]\n", nci->tuner[i].fe_info.name, nci->tuner[i].uuid);
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
					cPluginManager::CallAllServices ("OnNewMcliDevice", &m);
					if (m) {
						m->SetUUID (nci->tuner[i].uuid);
						m->SetFEType (type);
						m->SetEnable (true);
						d = new cMcliDeviceObject (m);
						m_devs.Add (d);
					}	// if
					if (!channel_switch_ok) {	// the first tuner that was found, so make VDR retune to the channel it wants...
						cChannel *ch = Channels.GetByNumber (cDevice::CurrentChannel ());
						if (ch) {
							channel_switch_ok=cDevice::PrimaryDevice ()->SwitchChannel (ch, true);
						}
					}
				}
			}
		}
		if (!m_devs.Count()) {
			channel_switch_ok=0;
		}
		nc_unlock_list ();
		Unlock ();

		usleep (250 * 1000);
	}
}

bool cPluginMcli::Initialize (void)
{
	return InitMcli ();
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
	// printf ("cPluginMcli::Housekeeping\n");
}

void cPluginMcli::MainThreadHook (void)
{
//      printf("cPluginMcli::MainThreadHook\n");
	if (reconf) {
		reconfigure ();
		reconf = 0;
		reconf_full = 0;
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

void cPluginMcli::reconfigure (void)
{
	LOCK_THREAD;
	for (cMcliDeviceObject * d = m_devs.First (); d;) {
		cMcliDeviceObject *next = m_devs.Next (d);
		d->d ()->SetEnable (false);
		d->d ()->ExitMcli ();
		if (reconf_full) {
			delete d->d ();
			m_devs.Del (d);
		}
		d = next;
	}

	ExitMcli ();
	InitMcli ();
	for (cMcliDeviceObject * d = m_devs.First (); d; d = m_devs.Next (d)) {
		d->d ()->InitMcli ();
		d->d ()->SetEnable (true);
	}
}

cOsdObject *cPluginMcli::MainMenuAction (void)
{
	printf ("cPluginMcli::MainMenuAction\n");
	// Perform the action when selected from the main VDR menu.
	return new cCamMenu (&m_cmd);
}


cMenuSetupPage *cPluginMcli::SetupMenu (void)
{
	printf ("cPluginMcli::SetupMenu\n");
	// Return a setup menu in case the plugin supports one.
	return new cMenuSetupMcli (&m_cmd);
}

bool cPluginMcli::SetupParse (const char *Name, const char *Value)
{
	printf ("cPluginMcli::SetupParse\n");
	if (!strcasecmp (Name, "DVB-C") && m_cmd.tuner_type_limit[FE_QAM] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_QAM] = atoi (Value);
	else if (!strcasecmp (Name, "DVB-T") && m_cmd.tuner_type_limit[FE_OFDM] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_OFDM] = atoi (Value);
	else if (!strcasecmp (Name, "DVB-S") && m_cmd.tuner_type_limit[FE_QPSK] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_QPSK] = atoi (Value);
	else if (!strcasecmp (Name, "DVB-S2") && m_cmd.tuner_type_limit[FE_DVBS2] == MCLI_MAX_DEVICES)
		m_cmd.tuner_type_limit[FE_DVBS2] = atoi (Value);
	else
		return false;
	return true;
}

#define MAX_TUNERS_IN_MENU 16
typedef struct
{
	int type[MAX_TUNERS_IN_MENU];
	char name[MAX_TUNERS_IN_MENU][128];
} mytuner_info_t;

bool cPluginMcli::Service (const char *Id, void *Data)
{
	//printf ("cPluginMcli::Service: \"%s\"\n", Id);
	mytuner_info_t *infos = (mytuner_info_t *) Data;

	if (Id && strcmp (Id, "GetTunerInfo") == 0) {
		netceiver_info_list_t *nc_list = nc_get_list ();
		nc_lock_list ();
		for (int n = 0; n < nc_list->nci_num; n++) {
			netceiver_info_t *nci = nc_list->nci + n;
			for (int i = 0; i < nci->tuner_num && i < MAX_TUNERS_IN_MENU; i++) {
				strcpy (infos->name[i], nci->tuner[i].fe_info.name);
				infos->type[i] = nci->tuner[i].fe_info.type;
				//printf("Tuner: %s\n", nci->tuner[i].fe_info.name);
			}
		}
		nc_unlock_list ();
		return true;
	} else if (Id && strcmp (Id, "Reinit") == 0) {
		if (Data && strlen ((char *) Data) && (strncmp ((char *) Data, "eth", 3) || strncmp ((char *) Data, "br", 2))) {
			strncpy (m_cmd.iface, (char *) Data, IFNAMSIZ - 1);
		}
		reconfigure ();
		return true;
	}
	// Handle custom service requests from other plugins
	return false;
}

const char **cPluginMcli::SVDRPHelpPages (void)
{
	printf ("cPluginMcli::SVDRPHelpPages\n");
	// Return help text for SVDRP commands this plugin implements
	static const char *HelpPages[] = {
		"REINIT [dev]\n" "    Reinitalize the plugin on a certain network device - e.g.: plug mcli REINIT eth0",
		NULL
	};
	return HelpPages;
}

cString cPluginMcli::SVDRPCommand (const char *Command, const char *Option, int &ReplyCode)
{
	printf ("cPluginMcli::SVDRPCommand\n");
	// Process SVDRP commands this plugin implements

	if (strcasecmp (Command, "REINIT") == 0) {
		if (Option && (strncmp (Option, "eth", 3) || strncmp (Option, "br", 2))) {
			strncpy (m_cmd.iface, (char *) Option, IFNAMSIZ - 1);
		}
		reconfigure ();
		return cString ("Mcli-plugin: reconfiguring...");
	}

	return NULL;
}

VDRPLUGINCREATOR (cPluginMcli);	// Don't touch this!
