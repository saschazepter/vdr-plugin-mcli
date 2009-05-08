/*
 * (c) BayCom GmbH, http://www.baycom.de, info@baycom.de
 *
 * See the COPYING file for copyright information and
 * how to reach the author.
 *
 */

#ifndef _MMI_HANDLER_H
#define _MMI_HANDLER_H

#define MAX_TEXT_SIZE 1024

typedef struct caid_mcg {
  
    int caid;
    struct in6_addr mcg;


} caid_mcg_t;

typedef struct mmi_info {
        
      int slot;
      caid_mcg_t  *caids;
      int caid_num;

      struct in6_addr ipv6;
      struct in6_addr uuid;

      char mmi_text[MAX_TEXT_SIZE];

} mmi_info_t;

DLL_SYMBOL void print_mmi_info(mmi_info_t *m);
DLL_SYMBOL int mmi_get_menu_text(int sockfd, char *buf, int buf_len);
DLL_SYMBOL int mmi_send_menu_answer(int sockfd, char *buf, int buf_len);
DLL_SYMBOL UDPContext *mmi_init_broadcast_client(int port, char *iface);
DLL_SYMBOL int mmi_poll_for_menu_text(UDPContext *s, char *buf, int buf_len, int timeout);
DLL_SYMBOL int open_mmi_menu_session(struct in6_addr *ipv6, char *iface, int port, int cmd);
DLL_SYMBOL int get_mmi_data(xmlChar * xmlbuff, int buffersize, mmi_info_t *mmi_info);
#endif
