/* debug option */
#define DEBUG_PIDS
#define DEBUG_TUNE_EXTRA
#define DEBUG_TUNE
#define DEBUG_RESOURCES
#define DEBUG_FILTER
#define DEBUG_PIDS_ADD_DEL
#define DEBUG_TUNE_PC

#define DEBUG_BIT_PIDS	 	0x01
#define DEBUG_BIT_TUNE_EXTRA	0x02
#define DEBUG_BIT_TUNE		0x04
#define DEBUG_BIT_RESOURCES	0x08
#define DEBUG_BIT_PIDS_ADD_DEL 	0x10
#define DEBUG_BIT_TUNE_PC	0x40	// ProvideChannel
#define DEBUG_BIT_FILTER	0x80

// hidden test option		0x1000	// function: cPluginMcli::Action

#define DEBUG_MASK(bit, code)	if ((m_debugmask & bit) != 0) { code };

extern int m_debugmask;


/* Log skip option */

#define LOGSKIP_BIT_recv_ts_func_pid_Data	0x01	// skip log of issues with Data pids (16-18) like Mcli::recv_ts_func: Discontinuity on receiver 0x559f735c7e00 for pid 18: 5->7 at pos 0/7
#define LOGSKIP_BIT_SetChannelDevice_Reject	0x02	// skip log of Mcli::SetChannelDevice: Reject tuning on DVB

#define LOGSKIP_MASK(bit, code)	if ((m_logskipmask & bit) == 0) { code };

extern int m_logskipmask;
