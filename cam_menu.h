#ifndef CAM_MENU_H
#define CAM_MENU_H

#include <vdr/osdbase.h>
#include "filter.h"
#include "device.h"

#define MAX_CAMS_IN_MENU 16

typedef struct {
	char uuid[UUID_SIZE];
	int slot;
	char info[MMI_TEXT_LENGTH];
} cam_list_t;

typedef struct
{
	int port;
	char iface[IFNAMSIZ];
	char cmd_sock_path[_POSIX_PATH_MAX];
	int tuner_type_limit[FE_DVBS2 + 1];
	int mld_start;
} cmdline_t;

enum eInputRequest { eInputNone, eInputBlind, eInputNotBlind };

class cCamMenu : public cOsdMenu {

    private:
        int CamFind(cam_list_t *cam_list);
    	int CamMenuOpen(cam_list_t *cam);
    	int CamMenuSend(int fd, const char *c);
    	int CamMenuReceive(int fd, char *buf, int bufsize);
    	void CamMenuClose(int fd);
    	int CamPollText(mmi_info_t *text);
    	cmdline_t *m_cmd;
    	UDPContext *m_cam_mmi;
        cam_list_t cam_list[MAX_CAMS];
        int mmi_session;
        bool inCamMenu;
        bool end;
        int currentSelected;
        eInputRequest inputRequested;
        char pin[32];
        int pinCounter;
        char buf[MMI_TEXT_LENGTH];
        bool alreadyReceived;

    public:
        cCamMenu(cmdline_t *m_cmd);
        ~cCamMenu();
        eOSState ProcessKey(eKeys Key);
        void OpenCamMenu();
        void Receive();
};

#endif
