#ifndef __SIPARSER_H__
#define __SIPARSER_H__

#define TS_SECT_LEN(buf) \
	unsigned char *ptr = buf; \
	int slen = (((ptr[1] << 8) | ptr[2]) & 0x0fff); 
	

#define TS_PACKET_LEN (188)              /* TS RDSIZE is fixed !! */
#define TS_SYNC_BYTE  (0x47)             /* SyncByte for TS  ISO 138181-1 */
#define TS_BUF_SIZE   (256 * 1024)       /* default DMX_Buffer Size for TS */
#define PSI_BUF_SIZE  (2 * 4096)	/* Section length max. 12 bits */
#define READ_BUF_SIZE (256*TS_PACKET_LEN)  /* min. 2x TS_PACKET_LEN!!! */
#define BILLION  1000000000L;
#define MAX_DESC_LEN 256 //descriptor_length field 8-bit ISO/IEC 13818-1
#define MAX_ES_PIDS 32


typedef struct ts_packet_hdr 
{
  unsigned int sync_byte;
  unsigned int transport_error_indicator;
  unsigned int payload_unit_start_indicator;
  unsigned int transport_priority;
  unsigned int pid;
  unsigned int transport_scrambling_control;
  unsigned int adaptation_field_control;
  unsigned int continuity_counter;
} ts_packet_hdr_t;

typedef struct  pat {
  unsigned int table_id;
  unsigned int section_syntax_indicator;		
  unsigned int reserved_1;
  unsigned int section_length;
  unsigned int transport_stream_id;
  unsigned int reserved_2;
  unsigned int version_number;
  unsigned int current_next_indicator;
  unsigned int section_number;
  unsigned int last_section_number;

  // FIXME: list of programs

  unsigned int crc32;
} pat_t;

typedef struct _pat_list {
  unsigned int program_number; //SID
  unsigned int reserved;
  unsigned int network_pmt_pid;

  int cads_present;
  int cads_num;

} pat_list_t;

typedef struct pmt_pid_list {

  pat_t p;
  pat_list_t *pl;
  unsigned int pmt_pids;
 
} pmt_pid_list_t;

typedef struct psi_buf {

  unsigned char *buf;
  unsigned int len;//used for offset
  unsigned int start;  

  int pid;
  int continuity;

} psi_buf_t;

typedef struct pmt {
    unsigned int table_id;
    unsigned int section_syntax_indicator;		
    unsigned int reserved_1;
    unsigned int section_length;
    unsigned int program_number;
    unsigned int reserved_2;
    unsigned int version_number;
    unsigned int current_next_indicator;
    unsigned int section_number;
    unsigned int last_section_number;
    unsigned int reserved_3;
    unsigned int pcr_pid;
    unsigned int reserved_4;
    unsigned int program_info_length;

    // N descriptors
    
    // N1 stream types and descriptors

    unsigned int crc32;  
} pmt_t;

typedef struct es_pmt_info {
    unsigned int stream_type;
    unsigned int reserved_1; 
    unsigned int elementary_pid;
    unsigned int reserved_2;
    unsigned int es_info_length;

    // N2 descriptor

} es_pmt_info_t;

typedef struct  ca_descriptor {
    
    unsigned int descriptor_tag;
    unsigned int descriptor_length;		
    unsigned int  ca_system_id;
    unsigned int  reserved;
    unsigned int  ca_pid;
    unsigned char private_data[MAX_DESC_LEN];

} si_desc_t;

typedef struct pmt_descriptor {

    pmt_t pmt_hdr;

    int cas;
    si_desc_t *cad;

} si_pmt_desc_t;

typedef struct ca_descriptor_list {

    int cads;
    si_desc_t *cad;

} si_cad_t;

typedef struct ca_sid_info {

    int sid;
    int version;
    int offset;
    int len;
  
} ca_sid_t;

typedef struct ca_pmt_descriptors {

    int cads;
    int size;
    unsigned char *cad;
            
} si_ca_pmt_t;


typedef struct ca_pmt_list {
    
    int sid;
    
    pmt_t p;
    si_ca_pmt_t pm;
    si_ca_pmt_t es;    
  
} ca_pmt_list_t;


typedef struct ca_sid_list {

    int tc; //total number of CA desc.
    int num;
    ca_pmt_list_t *l;

} ca_sid_list_t;

typedef struct  _cat {
    unsigned int table_id;
    unsigned int section_syntax_indicator;		
    unsigned int reserved_1;
    unsigned int section_length;
    unsigned int reserved_2;
    unsigned int version_number;
    unsigned int current_next_indicator;
    unsigned int section_number;
    unsigned int last_section_number;

    // private section
    
    unsigned int crc32;
} cat_t;

typedef struct tdt_sect {

      uint8_t  table_id;
      uint8_t  section_syntax_indicator;
      uint8_t  reserved; //0 future use
      uint8_t  reserved_1;
      uint16_t section_length;
      uint8_t  dvbdate[5];
} tdt_sect_t;

int parse_ca_descriptor(unsigned char *desc, si_desc_t *t);

int ts2psi_data(unsigned char *buf,psi_buf_t *p,int len, int pid_req);	
int parse_pat_sect(unsigned char *buf, pmt_pid_list_t *pmt);
int parse_pmt_ca_desc(unsigned char *buf, si_ca_pmt_t *pm_cads, si_ca_pmt_t *es_cads, pmt_t *pmt_hdr, int *fta);
int parse_cat_sect(unsigned char *buf, si_cad_t *emm);
int parse_tdt_sect(unsigned char *buf, tdt_sect_t *tdt);
int get_ts_packet_hdr(unsigned char *buf, ts_packet_hdr_t *p);
int si_get_video_pid(unsigned char *esi_buf, int size, int *vpid);
int si_get_audio_pid(unsigned char *esi_buf, int size, int *apid);
int get_pmt_es_pids(unsigned char *esi_buf, int size, int *es_pids, int all);
void print_pat(pat_t *p, pat_list_t *pl, int pmt_num);
void printhex_buf(char *msg,unsigned char *buf,int len);
void print_cad_lst(si_cad_t *l, int ts_id);
void print_ca_bytes(si_desc_t *p);
void get_time_mjd (unsigned long mjd, long *year , long *month, long *day);
void print_tdt(tdt_sect_t *tdt, uint16_t mjd, uint32_t utc);
int ca_free_cpl_desc(ca_pmt_list_t *cpl);


#endif





