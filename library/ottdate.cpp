#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>

#include "ottdate.hpp"
#include "fossa.h"
#include "log.h"

OttDate*            OttDate::s_instance   = 0;
int                 OttDate::s_exit_flag  = 0;
OttDate::EState     OttDate::s_cur_state  = OttDate::EState_Idle;
OttDate::EState     OttDate::s_last_state = OttDate::EState_Undefined;
char *              OttDate::s_last_http_message = 0;
int                 OttDate::s_download_percentage = 0;
time_t              OttDate::s_last_recv = 0;

const int           OttDate::MD5_DIGEST_LENGTH = 16;
const int           OttDate::TIMEOUT = 10;

//-----------------------------------------------------------------------------
OttDate::OttDate()
  : m_output_filename("/mnt/otto-update.zip")
{
  m_url="";
  std::ifstream f("/stak/updateurl");
  if(f.is_open())
  {
    std::getline(f,m_url);
    f.close();
  }

  if(m_url.size()<3) {
    m_url = std::string("http://update.s-t-a-k.com");
  }

  m_state_names.push_back("Update?");
  m_state_names.push_back("Checking...");
  m_state_names.push_back("No Update");
  m_state_names.push_back("No Internet");
  m_state_names.push_back("No Power");
  m_state_names.push_back("Downloading...");
  m_state_names.push_back("Verifying...");
  m_state_names.push_back("Download failed");
  m_state_names.push_back("Applying Update");
  m_state_names.push_back("Update failed");
  m_state_names.push_back("Reboot?");

  //assemble data for request:
  m_post_data_len=1024;
  m_post_data = new char[m_post_data_len];
  json_emit( m_post_data, m_post_data_len
           , "{s: s, s: s, s: i, s: s}"
           , "product", "otto"
           , "component", "fullstak"
           , "version", getStakVersion()
           , "id", getRaspiSerial()
           );

  m_mgr = new ns_mgr;

  pthread_create(&m_thread,NULL,run_main_loop,NULL);
}


//-----------------------------------------------------------------------------
OttDate::~OttDate()
{
}


//-----------------------------------------------------------------------------
OttDate* OttDate::instance()
{
  if(!s_instance)
  {
    s_instance = new OttDate();
  }

  return s_instance;
}


//-----------------------------------------------------------------------------
bool OttDate::trigger_update()
{
  if(s_cur_state==EState_Idle) {
    next_state(EState_Checking);
    return true;
  }

  return false;
}


//-----------------------------------------------------------------------------
void* OttDate::run_main_loop(void *)
{
  LOG_MESSAGE_ENTER();
  while(true) OttDate::instance()->main_loop();
  LOG_MESSAGE_LEAVE();
}


// This defines what is done when a state is entered
//-----------------------------------------------------------------------------
void OttDate::enter_state( OttDate::EState state )
{
  std::cerr<<"switched from state "<<s_last_state<<"\n";
  std::cerr<<"switched to state "<<m_state_names[state]<<"\n";

  switch(state) {
    case EState_Idle:
//      //for now: quit in idle state
//      if(s_last_state!=EState_Undefined) {
//        std::cerr<<"last state: "<<m_state_names[s_last_state]<<"\n";
//      }
//      exit(s_last_state);
      break;

    case EState_Checking:{
     
      // prepare extra_headers 
      std::string hostname = parse_hostname(m_url);
      static char extra_headers[2048];
      std::stringstream ss;
      ss<<"Host: "<<hostname<<"\r\n";
      ss<<"Content-Type: application/json\r\n";
      strncpy(extra_headers,ss.str().c_str(),sizeof(extra_headers));
 
      //TODO: check for power: s_cur_state=EState_NoPower;

      //TODO: maybe better check other server than ours?
      //check if we can DNS resolve m_url
      std::string url = resolve_url(m_url);
      if( url.length()<1 ) {
        s_exit_flag=1;
        next_state(EState_NoInternet);
      }
      std::cerr<<"DBG: "<<url<<"\n";
 
      fprintf(stderr,"sending json: %s\n",m_post_data);

      ns_mgr_init(m_mgr, NULL);
      if(ns_connect_http(m_mgr, handler_EState_Check, url.c_str(), extra_headers, m_post_data, NULL)) {
        s_exit_flag=0;
      } else {
        s_exit_flag=1;
        next_state(EState_NoUpdate);
      }
      break;
      }

    case EState_NoUpdate:
      break;

    case EState_NoInternet:
      break;

    case EState_NoPower:
      break;

    case EState_Downloading: {
      // prepare extra_headers 
      std::string hostname = parse_hostname(m_update_response.url);
      static char extra_headers[2048];
      std::stringstream ss;
      ss<<"Host: "<<hostname<<"\r\n";
      strncpy(extra_headers,ss.str().c_str(),sizeof(extra_headers));

       //TODO: maybe better check other server than ours?
      std::string url = resolve_url(m_update_response.url);
      if( url.length()<1 ) {
        s_exit_flag=1;
        next_state(EState_DownloadFailed);
      }
  
      s_download_percentage=0;
      ns_mgr_init(m_mgr, NULL);
      std::cerr<<"Downloading "<<url<<"...\n";
      if( ns_connect_http( m_mgr, handler_EState_Downloading
                     , url.c_str(), extra_headers, NULL
                     , m_output_filename.c_str()
                     ) ) {
        s_exit_flag=0;
      } else {
        s_exit_flag=1;
        next_state(EState_DownloadFailed);
      }
      break;
    }

    case EState_Verifying:
      break;

    case EState_DownloadFailed:
      break;

    case EState_ApplyingUpdate:
      if(system("/usr/bin/fwup -a -d /dev/mmcblk0 -t upgrade -i /mnt/otto-update.zip")) {
        next_state(EState_UpdateFailed);
      } else if(system("/usr/bin/fwup -a -d /dev/mmcblk0 -t on-reboot -i /mnt/finalize.fw")) {
        next_state(EState_UpdateFailed);
      }
      system("/bin/rm /mnt/otto-update.zip");
      system("/bin/rm /mnt/finalize.fw");
      break;
    case EState_UpdateFailed:
      break;
    case EState_AskForReboot:
      break;
  }

  s_last_state=s_cur_state;
}


//-----------------------------------------------------------------------------
void OttDate::state_name(std::string &s)
{
  if( s_cur_state!=EState_Undefined ) {
    s=m_state_names[s_cur_state];
  } else {
    s="Undefinded";
  }
}


//-----------------------------------------------------------------------------
std::string OttDate::state_name()
{
  if( s_cur_state!=EState_Undefined ) {
    return m_state_names[s_cur_state];
  }

  return "Undefinded";
}



//-----------------------------------------------------------------------------
int OttDate::download_percentage()
{
  return s_download_percentage;
}


//-----------------------------------------------------------------------------
void OttDate::next_state( OttDate::EState state )
{
  s_last_state = s_cur_state;
  s_cur_state  = state;
}


//-----------------------------------------------------------------------------
OttDate::EState OttDate::main_loop()
{
  if(s_last_state!=s_cur_state) {
    enter_state(s_cur_state);
  }

  switch(s_cur_state)
  {
    case EState_Idle:
      break;

    case EState_Checking:
      if (s_exit_flag == 0) {
        ns_mgr_poll(m_mgr, 100);
        std::cerr<<"still checking...\n";
      } else {
        ns_mgr_free(m_mgr);

        next_state(process_data(s_last_http_message));
      }
      break;

    case EState_NoUpdate:
      sleep(1);
      next_state(EState_Idle);
      break;

    case EState_NoInternet:
      sleep(1);
      next_state(EState_Idle);
      break;

    case EState_NoPower:
      sleep(1);
      next_state(EState_Idle);
      break;

    case EState_Downloading:
      if (s_exit_flag == 0) {
        ns_mgr_poll(m_mgr, 100);
      } else {
        ns_mgr_free(m_mgr);
        next_state(EState_Verifying);
      }
      break;

    case EState_Verifying:
      if(verify_md5(m_output_filename,m_update_response.checksum)) {
        next_state(EState_DownloadFailed);
      } else {
        next_state(EState_ApplyingUpdate);
      }
      break;

    case EState_DownloadFailed:
      sleep(2);
      next_state(EState_Idle);
      break;

    case EState_ApplyingUpdate:
      next_state(EState_AskForReboot);  
      break;

    case EState_UpdateFailed:
      sleep(2);
      next_state(EState_Idle);
      break;

    case EState_AskForReboot:
      break;
  }

  return s_cur_state;
  
}

//-----------------------------------------------------------------------------
OttDate::EState OttDate::process_data( const char *json )
{
  struct json_token *arr, *tok;
  static char buf[4096];

  // Tokenize json string, fill in tokens array
  arr = parse_json2(json, strlen(json));
  if(!arr) {
    fprintf(stderr,"error parsing json: [%s]\n",json);
    return EState_NoUpdate;
  }

  tok = find_json_token(arr, "update_available");
  if(tok!=NULL) {
    fprintf(stderr,"update_available= [%.*s]\n", tok->len, tok->ptr);

    m_update_response.update_available = !strncmp("true",tok->ptr,4);
  }
  tok = find_json_token(arr, "url");
  if(tok!=NULL) {
    m_update_response.url = new char[tok->len+1];
    strncpy(m_update_response.url,tok->ptr,tok->len);
    m_update_response.url[tok->len]=0;

    fprintf(stderr,"url= [%.*s]\n", tok->len, tok->ptr);
  }
  tok = find_json_token(arr, "size");
  if(tok!=NULL) {
    strncpy(buf,tok->ptr,tok->len);
    buf[tok->len]=0;
    fprintf(stderr,"size= [%d]\n", atoi(buf));
    m_update_response.size = atoi(buf);
  }
  tok = find_json_token(arr, "checksum");
  if(tok!=NULL) {
    m_update_response.checksum = new char[tok->len+1];
    strncpy(m_update_response.checksum,tok->ptr,tok->len);
    m_update_response.checksum[tok->len]=0;
    fprintf(stderr,"checksum= [%.*s]\n", tok->len, tok->ptr);
  }
  tok = find_json_token(arr, "new_version");
  if(tok!=NULL) {
    strncpy(buf,tok->ptr,tok->len);
    buf[tok->len]=0;
    m_update_response.size = atoi(buf);
    fprintf(stderr,"new_version= [%d]\n", atoi(buf));
  }
  tok = find_json_token(arr, "comment");
  if(tok!=NULL) {
    fprintf(stderr,"comment= [%.*s]\n", tok->len,tok->ptr);
  }


  // Do not forget to free allocated tokens array
  free(arr);

  if(m_update_response.update_available) {
    return EState_Downloading;
  }

  return EState_NoUpdate;
}


//-----------------------------------------------------------------------------
void OttDate::handler_EState_Check(struct ns_connection *nc, int ev, void *ev_data)
{
  LOG_MESSAGE_ENTER();
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case NS_POLL:
      if(time(0)-s_last_recv > TIMEOUT) {
        fprintf(stderr,"timeout\n");
        s_exit_flag = 1;
      }
      break;

    case NS_CONNECT:
      if (* (int *) ev_data != 0) {
        fprintf(stderr, "connect() failed: %s\n", strerror(* (int *) ev_data));
        s_cur_state=EState_NoUpdate;
        s_exit_flag = 1;
      } else {
        fprintf(stderr,"connected\n");
        s_last_recv = time(0);
      }
      break;

    case NS_HTTP_REPLY:
      nc->flags |= NSF_CLOSE_IMMEDIATELY;

      if (s_last_http_message) {
        delete s_last_http_message;
      }
      s_last_http_message = new char[hm->body.len+1];
      strncpy(s_last_http_message,hm->body.p,hm->body.len);
      s_last_http_message[hm->body.len] = 0;

      s_exit_flag = 1;
      break;

   case NS_RECV:
      s_last_recv = time(0);  
      break;

    case NS_CLOSE:
      s_exit_flag = 1;
      break;

    default:
      break;
  }
  LOG_MESSAGE_LEAVE();
}


//-----------------------------------------------------------------------------
void OttDate::handler_EState_Downloading(struct ns_connection *nc, int ev, void *ev_data)
{
  LOG_MESSAGE_ENTER();
  struct http_message *hm = (struct http_message *) ev_data;
  struct ns_str* content=0;

  static int full=0;
  static int some=0;
  char progress[]="-\\|/";
  static int progress_i=0;

  switch (ev) {
    case NS_POLL:
      if(time(0)-s_last_recv > TIMEOUT) {
        fprintf(stderr,"timeout\n");
        s_exit_flag = 1;
      }
      break;

    case NS_CONNECT:
      if (* (int *) ev_data != 0) {
        fprintf(stderr, "connect() failed: %s\n", strerror(* (int *) ev_data));
        s_cur_state=EState_Idle;
        s_exit_flag = 1;
      }
      some=0;
      break;

    case NS_RECV:
      some+=*(int*)ev_data;
      if(full>0) {
        fprintf(stderr,"\rdownloading: %10d / %10d",some,full);
        s_download_percentage = (float)(some)/full*100;
      } else {
        fprintf(stderr,"\rdownloading: %c",progress[(progress_i++)%4]);
      }
      s_last_recv = time(0);  
      break;

    case NS_HTTP_REPLY:
      // we get here as soon as we have the http req header,
      // before the file download starts
      full=hm->message.len;
      break;

    case NS_CLOSE:
      s_exit_flag = 1;
      nc->flags |= NSF_CLOSE_IMMEDIATELY;
      fprintf(stderr,"\rdownloading done");
      break;

    default:
      break;
  }
  LOG_MESSAGE_LEAVE();
}


//-----------------------------------------------------------------------------
int OttDate::verify_md5(const std::string &filename,const std::string &md5sum)
{
  unsigned char c[MD5_DIGEST_LENGTH];
  char d[MD5_DIGEST_LENGTH*2+1];
  int i;
  FILE *inFile = fopen (filename.c_str(), "rb");
  MD5_CTX mdContext;
  int bytes;
  unsigned char data[1024];

  if (inFile == NULL) {
    std::cerr<<filename<<" can't be opened.\n";
    return 0;
  }

  MD5_Init (&mdContext);
  while ((bytes = fread (data, 1, 1024, inFile)) != 0)
    MD5_Update (&mdContext, data, bytes);
  MD5_Final (c,&mdContext);
  fclose (inFile);

  for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&d[i*2],"%02x", c[i]);
  }

  int r=md5sum.compare(0,MD5_DIGEST_LENGTH,d,MD5_DIGEST_LENGTH);
  std::cerr<< (r==0 ? "SUCCESS: " : "ERROR: ")
           << md5sum
           << (r==0 ? " == " : " != ")
           << d
           << "\n";

  return r;
}


//-----------------------------------------------------------------------------
char* OttDate::getRaspiSerial()
{
  static char buf[4096]="UNKNOWN";
  FILE *fp = NULL;
  if(!(fp = fopen("/proc/cpuinfo","r"))) {
    return buf;
  }

  fgets(buf,sizeof(buf),fp);
  while(!feof(fp))
  {
    char *tok = strtok(buf,"\n\r\t :");
    while(tok) {
      if(!strcmp(tok,"Serial")) {
        char *r = strtok(NULL,"\n\r\t :");
        return r;
      } 
      tok = strtok(NULL,"\n\r\t :");
    }

    fgets(buf,sizeof(buf),fp);
  }

  sprintf(buf,"UNKNOWN");
  return buf;
}


//-----------------------------------------------------------------------------
int OttDate::getStakVersion()
{
  static char buf[4096]="UNKNOWN";
  FILE *fp = NULL;
  if(!(fp = fopen("/stak/version","r"))) {
    fprintf(stderr,"cannot open /stak/version\n");
    return 1;
  }

  fgets(buf,sizeof(buf),fp);

  return atoi(buf);
}


//-----------------------------------------------------------------------------
std::string OttDate::parse_hostname( const std::string& url, int *pos, int *len )
{
  std::string host;
  int p,l;
  if(!pos) pos = &p;
  if(!len) len = &l;

  *pos=0; *len=url.length();

  if( url.find("://") != std::string::npos )
  {
    *pos = url.find("://")+3;
    *len -= *pos;
  }
  if( url.find_first_of(":/",*pos) != std::string::npos )
  {
    *len = url.find_first_of(":/",*pos)- (*pos);
  }
  host = url.substr(*pos,*len);
  
  return host;
}

//-----------------------------------------------------------------------------
std::string OttDate::resolve_url( const std::string& url )
{
  int a,b;
  std::string r;
  static char buf[100];
  
  std::string host = parse_hostname( url, &a, &b );
  if(!ns_resolve(host.c_str(),buf,sizeof(buf)))
  {
    return std::string();   
  }

  r=url.substr(0,a);
  r+=buf;

  if(b<url.length())
  {
    r+=url.substr(a+b, url.length()-b);
  }

  return r;
}
//-----------------------------------------------------------------------------
 
