/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

/*
 * mcli.c: A plugin for the Video Disk Recorder
 */

#include <vdr/plugin.h>
#include <vdr/player.h>
#include "filter.h"
#include "device.h"
#include "cam_menu.h"

#define MCLI_DEVICE_VERSION "0.9.0"
#define MCLI_PLUGIN_VERSION "0.9.0"
#define MCLI_PLUGIN_DESCRIPTION trNOOP ("NetCeiver Client Application")
#define MCLI_SETUPMENU_DESCRIPTION trNOOP ("NetCeiver Client Application")
#define MCLI_MAINMENU_DESCRIPTION trNOOP ("Common Interface")

#define MCLI_MAX_DEVICES 8
#define MCLI_DEVICE_TIMEOUT 120
#define TEMP_DISABLE_DEVICE

typedef enum { CAM_POOL_SINGLE, CAM_POOL_MULTI, CAM_POOL_EXTRA, CAM_POOL_MAX} cam_pool_t;

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


class cPluginMcli:public cPlugin, public cThread
{
      private:
	// Add any member variables or functions you may need here.
	cMcliDeviceList m_devs;
	cmdline_t m_cmd;
	UDPContext *m_cam_mmi;
	int m_cam_pool[CAM_POOL_MAX]; 
	int m_cams_inuse;
	
      public:
	cPluginMcli (void);
	virtual ~ cPluginMcli ();
	virtual const char *Version (void)
	{
		return MCLI_PLUGIN_VERSION;
	}
	virtual const char *Description (void)
	{
		return MCLI_PLUGIN_DESCRIPTION;
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
		return MCLI_SETUPMENU_DESCRIPTION;
#endif
	}
	virtual const char *MainMenuEntry (void)
	{
		return MCLI_MAINMENU_DESCRIPTION;
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
	
	int SetCAMPool(cam_pool_t type, int num);
	int AvailableCAMs(void);
	int AllocCAM(void);
	int FreeCAM(void);

	int CamPollText (mmi_info_t * text);
	virtual cOsdObject *AltMenuAction (void);
};
