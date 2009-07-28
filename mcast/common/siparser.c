#include "headers.h"

//#define DBG 1
#define CRC32_CHECK 1

enum ca_desc_type { EMM, ECM };

//-----------------------------------------------------------------------------------
void printhex_buf(char *msg,unsigned char *buf,int len)
{
  int i,j,k;
  int width=8;

  i=k=0;
  printf("%s: %d bytes (0x%04x)\n",msg,len,len);
  printf("---------------------------------------------------------------\n");    
  while(len) {
    printf("%04x	",k++*width*2);
    j=i;
    for(;i < j + width ; i++){ 
      if (i >= len) break; 
      printf("%02x ",buf[i]);  
    }
    if (i >= len) {
      printf("\n");
      break;
    }
    printf("	");
    j=i;
    for(;i < j + width ; i++){
      if (i >= len) break;
      printf("%02x ",buf[i]);      
    }
    printf("\n");
    if (i >= len) break;
  }    
  printf("---------------------------------------------------------------\n");    
}
//-----------------------------------------------------------------------------------
void print_ts_header(ts_packet_hdr_t *p)
{
  printf("--------------------------------------------------------------\n");
  printf("TS header data:\n");
  printf("Sync-byte			: 0x%04x\n",p->sync_byte);
  printf("Transport error indicator	: 0x%04x\n",p->transport_error_indicator);
  printf("Payload unit start indicator	: 0x%04x\n",p->payload_unit_start_indicator);
  printf("Transport priority		: 0x%04x\n",p->transport_priority);
  printf("PID				: 0x%04x\n",p->pid);
  printf("Transport scrambling control	: 0x%04x\n",p->transport_scrambling_control);
  printf("Adaptation field control	: 0x%04x\n",p->adaptation_field_control);
  printf("Continuity_counter		: 0x%04x\n",p->continuity_counter);

}
//-----------------------------------------------------------------------------------
void print_pmt(pmt_t *p)
{
  printf("--------------------------------------------------------------\n");    
  printf("PMT section:\n");  
  printf("Table ID                 : %-5d (0x%04x)\n",p->table_id,p->table_id);
  printf("(fixed):                 : %-5d (0x%04x)\n",0,0);  
  printf("Section syntax indicator : %-5d (0x%04x)\n",p->section_syntax_indicator,p->section_syntax_indicator);		
  printf("Reserved 1               : %-5d (0x%04x)\n",p->reserved_1,p->reserved_1);
  printf("Section length           : %-5d (0x%04x)\n",p->section_length,p->section_length);
  printf("Program number           : %-5d (0x%04x)\n",p->program_number,p->program_number);
  printf("Reserved 2               : %-5d (0x%04x)\n",p->reserved_2,p->reserved_2);
  printf("Version number           : %-5d (0x%04x)\n",p->version_number,p->version_number);
  printf("Current next indicator   : %-5d (0x%04x)\n",p->current_next_indicator,p->current_next_indicator);
  printf("Section number           : %-5d (0x%04x)\n",p->section_number,p->section_number);
  printf("Last section number      : %-5d (0x%04x)\n",p->last_section_number,p->last_section_number);
  printf("Reserved 3               : %-5d (0x%04x)\n",p->reserved_3,p->reserved_3);
  printf("PCR pid                  : %-5d (0x%04x)\n",p->pcr_pid,p->pcr_pid);
  printf("Reserved 4               : %-5d (0x%04x)\n",p->reserved_4,p->reserved_4);
  printf("Program info length      : %-5d (0x%04x)\n",p->program_info_length,p->program_info_length);



  printf("CRC32                    : 0x%04x\n",p->crc32);		
}
//-----------------------------------------------------------------------------------
void print_pat(pat_t *p, pat_list_t *pl, int pmt_num)
{
  printf("--------------------------------------------------------------\n");
  printf("PAT section:\n");
  printf("Table_id                 : %-5d (0x%04x)\n",p->table_id,p->table_id);
  printf("(fixed):                 : %-5d (0x%04x)\n",0,0);
  printf("Section syntax indicator : %-5d (0x%04x)\n",p->section_syntax_indicator,p->section_syntax_indicator);
  printf("Reserved_1               : %-5d (0x%04x)\n",p->reserved_1,p->reserved_1);
  printf("Section length           : %-5d (0x%04x)\n",p->section_length,p->section_length);
  printf("Transport stream id      : %-5d (0x%04x)\n",p->transport_stream_id,p->transport_stream_id);
  printf("Reserved 2               : %-5d (0x%04x)\n",p->reserved_2,p->reserved_2);
  printf("Version number           : %-5d (0x%04x)\n",p->version_number,p->version_number);
  printf("Current next indicator   : %-5d (0x%04x)\n",p->current_next_indicator,p->current_next_indicator);
  printf("Section number           : %-5d (0x%04x)\n",p->section_number,p->section_number);
  printf("Last section number      : %-5d (0x%04x)\n",p->last_section_number,p->last_section_number);

  if (pl && pmt_num){
    int i;
    printf("Number of PMTs in PAT : %-5d \n", pmt_num);
    for(i=0;i<pmt_num;i++) {
      pat_list_t *pat = pl + i;
      printf("\nProgram number  : %-5d (0x%04x)\n",pat->program_number,pat->program_number);
      printf("Reserved        : %-5d (0x%04x)\n",pat->reserved,pat->reserved);
      printf("Network PMT PID : %-5d (0x%04x)\n",pat->network_pmt_pid,pat->network_pmt_pid);
    }
  }

  printf("CRC32                   : 0x%04x\n",p->crc32);


}
//-----------------------------------------------------------------------------------
void get_time_mjd (unsigned long mjd, long *year , long *month, long *day)
{
    if (mjd > 0) {
        long   y,m,d ,k;

        // algo: ETSI EN 300 468 - ANNEX C

        y =  (long) ((mjd  - 15078.2) / 365.25);
        m =  (long) ((mjd - 14956.1 - (long)(y * 365.25) ) / 30.6001);
        d =  (long) (mjd - 14956 - (long)(y * 365.25) - (long)(m * 30.6001));
        k =  (m == 14 || m == 15) ? 1 : 0;
        y = y + k + 1900;
        m = m - 1 - k*12;
        *year = y;
        *month = m;
        *day = d;

    }

} 
//-----------------------------------------------------------------------------------
void print_tdt(tdt_sect_t *tdt, uint16_t mjd, uint32_t utc)
{
    printf("--------------------------------------------------------------\n");
    printf("TDT section:\n");
    printf("Table_id                 : %-5d (0x%04x)\n",tdt->table_id,tdt->table_id);
    printf("Reserved                 : %-5d (0x%04x)\n",tdt->reserved,tdt->reserved);
    printf("Reserved_1               : %-5d (0x%04x)\n",tdt->reserved_1,tdt->reserved_1);
    printf("Section length           : %-5d (0x%04x)\n",tdt->section_length,tdt->section_length);    
    printf("UTC_time                 : 0x%2x%2x%2x%2x%2x\n",tdt->dvbdate[0],tdt->dvbdate[1],tdt->dvbdate[2],tdt->dvbdate[3],tdt->dvbdate[4]); 
    
    long y,m,d;
    get_time_mjd(mjd, &y, &m, &d);
    printf("TIME: [= %02d-%02d-%02d  %02lx:%02lx:%02lx (UTC) ]\n\n",y,m,d,(utc>>16) &0xFF, (utc>>8) &0xFF, (utc) &0xFF);
    printf("--------------------------------------------------------------\n");

}
//-----------------------------------------------------------------------------------
void print_ca_desc(si_desc_t *p)
{
  printf("CA desc. tag    : %d (%#x)\n",p->descriptor_tag,p->descriptor_tag);
  printf("CA desc. length : %d (%#x)\n",p->descriptor_length,p->descriptor_length);
  printf("CA system id    : %d (%#x)\n",p->ca_system_id,p->ca_system_id);
  printf("Reserverd       : %d (%#x)\n",p->reserved,p->reserved);
  printf("CA pid          : %d (%#x)\n",p->ca_pid,p->ca_pid);

  printhex_buf("Private data",p->private_data,p->descriptor_length-4);

}
//-----------------------------------------------------------------------------------
void print_ca_bytes(si_desc_t *p)
{
  unsigned int i; 
  printf("%x %x %x %x %x ",p->descriptor_tag, p->descriptor_length, p->ca_system_id, p->reserved, p->ca_pid);
  for (i = 0; i < p->descriptor_length - 4; i++)
      printf("%x ",p->private_data[i]);
  printf(";");

}
//-----------------------------------------------------------------------------------
void print_cad_lst(si_cad_t *l, int ts_id)
{
  int i;
  
  for (i = 0; i < l->cads; i++) {
    print_ca_desc(&l->cad[i]);
  }
  printf("Total CA desc. for TS ID %d : %d\n",ts_id,l->cads);
}
//-----------------------------------------------------------------------------------
int parse_ca_descriptor(unsigned char *desc, si_desc_t *t)
{
  unsigned char *ptr=desc;
  int tag=0,len=0;
  
  tag=ptr[0];
  len=ptr[1]; 

  if (len > MAX_DESC_LEN) {
    printf("descriptor():Descriptor too long !\n");
    return -1;
  }
  
  switch(tag){
    case 0x09: {  
        t->descriptor_tag=tag;
        t->descriptor_length=len; //???
        t->ca_system_id=((ptr[2] << 8) | ptr[3]);
        t->reserved=(ptr[4] >> 5) & 7;
        t->ca_pid=((ptr[4] << 8) | ptr[5]) & 0x1fff; 
        //header 4 bytes + 2 bytes
        memcpy(t->private_data,ptr+6,len-4);

        //print_ca_desc(t);
        
        break; 
      }
    default: 
        break;
    }

  return len + 2; //2 bytes tag + length
}
//--------------------------------------------------------------------------------------------
int ca_free_cpl_desc(ca_pmt_list_t *cpl)
{
	if (cpl->pm.size > 0 && cpl->pm.cad)
		free(cpl->pm.cad);
	if (cpl->es.size > 0 && cpl->es.cad)
		free(cpl->es.cad);

	memset(cpl,0,sizeof(ca_pmt_list_t));				

	return 0;
}
//--------------------------------------------------------------------------------------------
int descriptor(unsigned char *desc, si_cad_t *c)
{
  unsigned char *ptr=desc;
  int tag=0,len=0;
  
  tag=ptr[0];
  len=ptr[1]; 

  if (len > MAX_DESC_LEN) {
    printf("descriptor():Descriptor too long !\n");
    return -1;
  }
  
  switch(tag){
    case 0x09: {
        c->cads++;
        c->cad = (si_desc_t*)realloc(c->cad,sizeof(si_desc_t)*c->cads);
        if (!c->cad) {
          c->cads--;
          printf("descriptor():realloc error\n");
          return -1;
        }
        si_desc_t *t = c->cad + c->cads - 1;
        t->descriptor_tag=tag;
        t->descriptor_length=len; //???
        t->ca_system_id=((ptr[2] << 8) | ptr[3]);
        t->reserved=(ptr[4] >> 5) & 7;
        t->ca_pid=((ptr[4] << 8) | ptr[5]) & 0x1fff; 
        //header 4 bytes + 2 bytes
        memcpy(t->private_data,ptr+6,len-4);

        break; 
      }
    default: { 
#if 0
        other_desc_t d;
        d.descriptor_tag=tag;
        d.descriptor_length=len;
        memcpy(d.data,ptr+2,len);
        //print_desc(d);
#endif
      }
    }

  return len + 2; //2 bytes tag + length
}

//-----------------------------------------------------------------------------------
int si_get_video_pid(unsigned char *esi_buf, int size, int *vpid)
{
      int index, pid_num, es_len;
      unsigned char *ptr = esi_buf;

      index = pid_num = 0;      
      while(index < size) {
          //ptr[0] //stream type 
          if (ptr[0] == 2 || ptr[0] == 0x1b) 
          {
              *vpid = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
              return 1;
          }
          es_len = ((ptr[3] << 8) | ptr[4]) & 0x0fff;
          index += 5 + es_len;
          ptr += 5 + es_len;   
      }

      *vpid = -1;
      return 0;

}
//-----------------------------------------------------------------------------------
int si_get_audio_pid(unsigned char *esi_buf, int size, int *apid)
{
      int index, pid_num, es_len;
      unsigned char *ptr = esi_buf;

      index = pid_num = 0;      
      while(index < size) {
          //ptr[0] //stream type 
          if (ptr[0] == 0x1 || ptr[0] == 0x3 || ptr[0] == 0x4) 
          {
              *apid = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
              return 1;
          }
          es_len = ((ptr[3] << 8) | ptr[4]) & 0x0fff;
          index += 5 + es_len;
          ptr += 5 + es_len;   
      }

      *apid = -1;
      return 0;

}
//-----------------------------------------------------------------------------------
int get_pmt_es_pids(unsigned char *esi_buf, int size, int *es_pids, int all)
{
      int index, pid_num, es_len;
      unsigned char *ptr = esi_buf;

      index = pid_num = 0;      
      while(index < size) {
          //ptr[0] //stream type 
          if (ptr[0] == 0x1 || ptr[0] == 0x2 ||  ptr[0] == 0x3 || ptr[0] == 0x4 || ptr[0] == 0x1b || all)
          {
              es_pids[pid_num] = ((ptr[1] << 8) | ptr[2]) & 0x1fff;
              pid_num++;
              if (pid_num >= MAX_ES_PIDS) {
                    info ("error: ES pids number out of bounds !\n");
                    return -1;
              }
          }
          es_len = ((ptr[3] << 8) | ptr[4]) & 0x0fff;
          index += 5 + es_len;
          ptr += 5 + es_len;   
      }

      return pid_num;
}
//-----------------------------------------------------------------------------------
int parse_pmt_ca_desc(unsigned char *buf, si_ca_pmt_t *pm_cads, si_ca_pmt_t *es_cads, pmt_t *pmt_hdr, int *fta)
{
      unsigned char *ptr=buf, tmp[PSI_BUF_SIZE]; //sections can be only 12 bit long

      memset(pm_cads,0,sizeof(si_ca_pmt_t));
      memset(es_cads,0,sizeof(si_ca_pmt_t));  

      pmt_hdr->table_id=ptr[0];		         
      pmt_hdr->section_syntax_indicator=(ptr[1] >> 7) & 1;
      pmt_hdr->reserved_1=(ptr[1] >> 4) & 3; 
      pmt_hdr->section_length=((ptr[1] << 8) | ptr[2]) & 0xfff;

      u_long crc = dvb_crc32 ((char *)buf,pmt_hdr->section_length+3);  

#ifdef DBG
      printf("CRCcc: 0x%lx\n",crc);
#endif
      if (crc & 0xffffffff) { //FIXME: makr arch flags
        printf("ERROR: parse_pmt_ca_desc() : CRC err. crc = 0x%lx\n", crc);
        return -1;
      }

      pmt_hdr->program_number=(ptr[3] << 8) | ptr[4];		        
      pmt_hdr->reserved_2=(ptr[5] >> 6) & 3;		        
      pmt_hdr->version_number=(ptr[5] >> 1) & 0x1f;		        
      pmt_hdr->current_next_indicator=ptr[5] & 1;	        
      pmt_hdr->section_number=ptr[6];		        
      pmt_hdr->last_section_number=ptr[7];	        
      pmt_hdr->reserved_3=(ptr[8] >> 5) & 7;		        
      pmt_hdr->pcr_pid=((ptr[8] << 8) | ptr[9]) & 0x1fff;		        
      pmt_hdr->reserved_4=(ptr[10] >> 4) & 0xf;		        
      pmt_hdr->program_info_length=((ptr[10] << 8) | ptr[11]) & 0xfff;	        
            
      int buf_len=0,len=0;
	  unsigned int i=0;
      
      buf_len = pmt_hdr->section_length - 9;
      ptr += 12; // 12 byte header
     
      pm_cads->size = pm_cads->cads =  0;
      for (i = 0; i < pmt_hdr->program_info_length;) {
        int dtag = ptr[0];
        int dlen = ptr[1] + 2;
        if (dtag == 0x09) { //we have CA descriptor
            memcpy(tmp + pm_cads->size, ptr, dlen);
            pm_cads->size+=dlen; 
            pm_cads->cads++;
            *fta=0;
        }
        ptr+=dlen; //desc. length plus 2 bytes for tag and header;
        buf_len-=dlen;     
        i+=dlen;
      }      
      //parsing ok we can take this program level descriptors
      if (pm_cads->size && pm_cads->cads) {
           pm_cads->cad = (unsigned char*)malloc(sizeof(unsigned char)*pm_cads->size);
           memcpy(pm_cads->cad, tmp, pm_cads->size);     
      }

#ifdef DBG  
      printf("%d bytes remaining (program info len = %d bytes)\n",buf_len,i);    
#endif  
      
      es_pmt_info_t esi;
      es_cads->size = es_cads->cads = 0;
      while (buf_len > 4) { //end of section crc32 is 4 bytes
        esi.stream_type=ptr[0];
        esi.reserved_1=(ptr[1] >> 5) & 7;
        esi.elementary_pid=((ptr[1] << 8) | ptr[2]) & 0x1fff;
        esi.reserved_2=(ptr[3] >> 4) & 0xf;
        esi.es_info_length=((ptr[3] << 8) | ptr[4]) & 0x0fff;

        
        memcpy(tmp + es_cads->size, ptr, 5);
        tmp[es_cads->size+1] &= 0x1f; //remove reserved value ???
        tmp[es_cads->size+3] &= 0x0f; //remove reserved value ???

        int es_info_len_pos = es_cads->size+3; //mark length position to set it later
        int cur_len = 0; //current ES stream descriptor length

        es_cads->size += 5;
        ptr += 5;
        buf_len -= 5;
        len=esi.es_info_length;
        while(len > 0) {
           int dtag = ptr[0];
           int dlen = ptr[1] + 2; //2 bytes for tag and len
           if (dtag == 0x09) { //we have CA descriptor
               memcpy(tmp + es_cads->size, ptr, dlen);
               es_cads->size += dlen;   
               es_cads->cads++;           
               cur_len += dlen;
               *fta=0;
           } 
           ptr += dlen;      
           len -= dlen;
           buf_len -= dlen;
        }
        tmp[es_info_len_pos] = (cur_len >> 8) & 0xff;
        tmp[es_info_len_pos+1] = cur_len & 0xff;       
      }
      
      //parsing ok we can take this ES level descriptors
      if (((es_cads->cads && es_cads->size) || (pm_cads->cads && es_cads->size)) || *fta) { //take ES stream info if we have PM or ES desc. 
           es_cads->cad = (unsigned char*)malloc(sizeof(unsigned char)*es_cads->size);
           memcpy(es_cads->cad, tmp, es_cads->size);
           
      }
       
#ifdef DBG
      printf("%d bytes remaining\n",buf_len);    
#endif

      pmt_hdr->crc32=(ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]; 

      if (len < 0) {
        printf("ERROR: parse_ca_desc() : section index out of bounds %d or (CRC err.) crc in sec. = 0x%x crc calc. = 0x%lx\n", buf_len,pmt_hdr->crc32, crc);
#ifdef DBG 
        print_pmt(&pmt_hdr);
#endif
        //cleanup ...
        if (pm_cads->size) 
            free(pm_cads->cad);
        if (es_cads->size)
            free(es_cads->cad);
        memset(pm_cads,0,sizeof(si_ca_pmt_t));
        memset(es_cads,0,sizeof(si_ca_pmt_t));  
        return -1;
      }

#ifdef DBG
      printf("#####################################\n");
      printf("parse_ca_desc(): section parsed: OK !\n");  
#endif
      return 0;
}  
//-----------------------------------------------------------------------------------
int parse_cat_sect(unsigned char *buf, si_cad_t *emm)
{
          unsigned char *ptr=buf;
          int len,i,ret;
          cat_t c;

          c.table_id = ptr[0];		         
          c.section_syntax_indicator = (ptr[1] >> 7) & 1;
          c.reserved_1 = (ptr[1] >> 4) & 3; 
          c.section_length = ((ptr[1] << 8) | ptr[2]) & 0xfff;

#ifdef CRC32_CHECK
          u_long crc = dvb_crc32 ((char *)buf,c.section_length+3);  
#ifdef DBG
          printf("CRCcc: 0x%lx\n",crc);
#endif
          if (crc & 0xffffffff) {
            printf("CRC32 error (0x%lx)!\n",crc);
            return -1;
          }
#endif

          c.reserved_2 = (ptr[3] << 10) | (ptr[4] << 2) | ((ptr[5] >> 6) & 3);		        
          c.version_number = (ptr[5] >> 1) & 0x1f;		        
          c.current_next_indicator = ptr[5] & 1;	        
          c.section_number = ptr[6];		        
          c.last_section_number = ptr[7];	        


          //do desc. here
          len = c.section_length - 5;
          ptr+=8; //go after hdr.

          for (i = 0; i < len - 4; i += ret) {  //crc32 4 bytes
            ret = descriptor(ptr, emm);
            if (ret < 0)
              return -1;
            ptr+=ret;
          }

#ifdef DBG  
          printf("%d bytes remaining (program info len = %d bytes)\n",len-i,len);    
#endif  
          c.crc32 = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]; 


          return 0;
}
//-----------------------------------------------------------------------------------
int parse_pat_sect(unsigned char *buf, pmt_pid_list_t *pmt)
{
        unsigned char *ptr=buf;
        pat_t p;
        pat_list_t *pat_info = NULL;
        
        memset(&p,0,sizeof(p));

        p.table_id=ptr[0];
        p.section_syntax_indicator=(ptr[1] & 0x80) >> 7;
        p.reserved_1=(ptr[1] & 0x30) >> 4;
        p.section_length=((ptr[1] << 8) | ptr[2]) & 0x0fff;

#ifdef CRC32_CHECK                
        u_long crc = dvb_crc32 ((char *)buf,p.section_length+3);  
        //FIXME: is it the right way ?
        if (crc & 0xffffffff) {
          printf("CRC32 error (0x%lx)!\n",crc);
          return -1;
        }
#endif
        
        p.transport_stream_id=(ptr[3] << 8) | ptr[4];
        p.reserved_2=(ptr[5] & 0xc0) >> 6;
        p.version_number=(ptr[5] & 0x3e) >> 1;
        p.current_next_indicator=(ptr[5] & 1);
        p.section_number=ptr[6];
        p.last_section_number=ptr[7];

        int n,i,pmt_num;
        
        n = p.section_length - 5 - 4; //bytes following section_length field + crc32 chk_sum

        ptr+=8;
        pmt_num=0;
        pat_info=(pat_list_t *)malloc(sizeof(pat_list_t)*n/4);
        for(i=0;i<n;i+=4) {
          pat_list_t *pat = pat_info + pmt_num;
          pat->program_number=(ptr[0] << 8) | (ptr[1]);
          pat->reserved=(ptr[2] & 0xe0) >> 5;
          pat->network_pmt_pid=((ptr[2] << 8) | ptr[3]) & 0x1fff;
          if (pat->network_pmt_pid != 0x10) { //NIT => FIXME: remove other known pids
      //      memset(&pat->desc,0,sizeof(pmt_desc_list_t));
            pmt_num++;
          }
          ptr+=4;
        }

        p.crc32=(ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3]; 

        pat_info=(pat_list_t *)realloc(pat_info,sizeof(pat_list_t)*pmt_num);
        
        if (pmt) {
          pmt->p=p;
          pmt->pl=pat_info;
          pmt->pmt_pids=pmt_num;
        }
        
        return 0;
}
int parse_tdt_sect(unsigned char *buf, tdt_sect_t *tdt)
{
          unsigned char *ptr = buf;
                
          tdt->table_id=ptr[0];
          tdt->section_syntax_indicator=(ptr[1] & 0x80) >> 7;
          tdt->reserved_1=(ptr[1] >> 4) >> 3;
          tdt->section_length=((ptr[1] << 8) | ptr[2]) & 0x0fff;            
          
          //copy UTC time MJD + UTC
          memcpy(tdt->dvbdate, ptr + 3, 5);  

          return 0;

}
//-----------------------------------------------------------------------------------
//TS packets handling
int get_ts_packet_hdr(unsigned char *buf, ts_packet_hdr_t *p)
{
        unsigned char *ptr=buf;

        memset(p,0,sizeof(p));

        p->sync_byte=ptr[0]; 
        p->transport_error_indicator=(ptr[1] & 0x80) >> 7;
        p->payload_unit_start_indicator=(ptr[1] & 0x40) >> 6;
        p->transport_priority=(ptr[1] & 0x20) >> 5;
        p->pid=((ptr[1] << 8) | ptr[2]) & 0x1fff;
        p->transport_scrambling_control=(ptr[3] & 0xC0) >> 6;
        p->adaptation_field_control=(ptr[3] & 0x30) >> 4;
        p->continuity_counter=(ptr[3] & 0xf);
          
#ifdef DBG
        print_ts_header(p);
#endif  

        return 0;

}
//-----------------------------------------------------------------------------------
int ts2psi_data(unsigned char *buf,psi_buf_t *p,int len, int pid_req)
{
        unsigned char *b=buf;
        ts_packet_hdr_t h;


        get_ts_packet_hdr(buf,&h);
       
        b+=4;
        len-=4;
        
        if (h.sync_byte != 0x47) {
              printf("%s:No sync byte in header !\n",__FUNCTION__);
              return -1;
        }

        
        if (pid_req != h.pid) {
#ifdef DBG
              printf("%s:pids mismatch  (pid req = %#x ts pid = %#x )!\n", __FUNCTION__,pid_req, h.pid);
#endif
              return -1;
        }
        
        //FIXME:Handle adaptation field if present/needed 
        if (h.adaptation_field_control & 0x2) {
          int n;
          
          n=b[0]+1;
          b+=n;
          len-=n; 

        }
          
        if (h.adaptation_field_control & 0x1) {
          if (h.transport_error_indicator) {
#ifdef DBG
            printf("Transport error flag set !\n");
#endif
            return -1;
          }
          if (h.transport_scrambling_control) {
            printf("Transport scrambling flag set !\n");
            //return -1;
          }   
          
          if (h.payload_unit_start_indicator && p->start) { //whole section new begin packet
#ifdef DBG      
              printf("%s:section read !\n",__FUNCTION__);
#endif
              return 1;
          }
          
          if (h.payload_unit_start_indicator && !p->start) { //packet beginning
            int si_offset=b[0]+1; //always pointer field in first byte of TS packet payload with start indicator set
            b+=si_offset;
            len-=si_offset; 
            if (len < 0 || len > 184) {
                  printf("WARNING 1: TS Packet damaged !\n");
                  return -1;
            }
            //move to buffer              
            memcpy(p->buf,b,len);
            p->len=len;
            p->start=((b[1] << 8) | b[2]) & 0x0fff; //get section length, using start for length
            p->pid=h.pid;
            p->continuity=h.continuity_counter;

          } 

          if (!h.payload_unit_start_indicator && p->start) { //packet continuation
            //duplicate packet
            if ((p->pid == h.pid) && (p->continuity == h.continuity_counter)){
#ifdef DBG
              printf("Packet duplicate ???\n");      
#endif
              return -1;
            }
            //new packet
            if (p->pid != h.pid) {
#ifdef DBG
              printf("New pid buf start %d len %d bytes (pid in buf = %d pid in ts = %d) !\n", p->start,p->len, p->pid, h.pid);
#endif
              return -1;
            }
            //discontinuity of packets
            if (((++p->continuity)%16) != h.continuity_counter) {
#ifdef DBG
              printf("Discontinuity of ts stream !!!\n");
#endif
              return -1;        
            }
            p->continuity=h.continuity_counter;
            if (len < 0 || len > 184) {
                  printf("WARNING 2: TS Packet damaged !\n");
                  return -1;
            }
            //move to buffer
            memcpy(p->buf+p->len,b,len);      
            p->len+=len; //FIXME: circular buffer
            if (p->len + 188 > PSI_BUF_SIZE) {
              printf("Error: Buffer full !\n");
              return -1;
              //FIXME:realloc
            }
          }
        }
        
#if 1  
        //3 bytes for bytes containing table id and section length
        TS_SECT_LEN(b);
        if (slen+3 <= len && h.payload_unit_start_indicator)  //len = 188 bytes - 4 bytes ts hdr. - adapt. field bytes - 1 byte offset - offset
              return 1;
#else //possible opt.
        /*if (p->start+3 == len)  
              return 1;*/
#endif  

        return 0;
}
//TS packets handling end
//-----------------------------------------------------------------------------------



