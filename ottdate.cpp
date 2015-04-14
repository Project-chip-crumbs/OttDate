#include "ottdate.hpp"
#include "fossa.h"

#include <iostream>

OttDate*            OttDate::s_instance   = 0;
int                 OttDate::s_exit_flag  = 0;
OttDate::EState     OttDate::s_cur_state  = OttDate::EState_Idle;
OttDate::EState     OttDate::s_last_state = OttDate::EState_Undefined;
struct http_message OttDate::s_last_http_message;

const int           OttDate::MD5_DIGEST_LENGTH = 16;

//-----------------------------------------------------------------------------
OttDate::OttDate()
	: m_url("http://update.s-t-a-k.com")
	, m_output_filename("ottdate.zip")
{
	m_state_names.push_back("EState_Idle");
	m_state_names.push_back("EState_Checking");
	m_state_names.push_back("EState_NoUpdate");
	m_state_names.push_back("EState_NoInternet");
	m_state_names.push_back("EState_NoPower");
	m_state_names.push_back("EState_Downloading");
	m_state_names.push_back("EState_Verifying");
	m_state_names.push_back("EState_DownloadFailed");
	m_state_names.push_back("EState_ApplyingUpdate");
	m_state_names.push_back("EState_UpdateFailed");
	m_state_names.push_back("EState_AskForReboot");

	//assemble data for request:
	m_post_data_len=1024;
	m_post_data = new char[m_post_data_len];
	json_emit( m_post_data, m_post_data_len
           , "{s: s, s: s, s: i, s: s}"
           , "product", "otto"
           , "component", "fullstak"
           , "version", 101
           , "id", getRaspiSerial()
           );
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


// This defines what is done when a state is entered
//-----------------------------------------------------------------------------
void OttDate::enter_state( OttDate::EState state )
{
	std::cerr<<"switched from state "<<s_last_state<<"\n";
	std::cerr<<"switched to state "<<m_state_names[state]<<"\n";

	switch(state) {
		case EState_Idle:
//			//for now: quit in idle state
//			if(s_last_state!=EState_Undefined) {
//				std::cerr<<"last state: "<<m_state_names[s_last_state]<<"\n";
//			}
//			exit(s_last_state);
			break;

		case EState_Checking:
			//TODO: check for power: s_cur_state=EState_NoPower;
			//TODO: check for internet: s_cur_state=EState_NoInternet;

			fprintf(stderr,"sending json: %s\n",m_post_data);

			ns_mgr_init(&m_mgr, NULL);
			ns_connect_http(&m_mgr, handler_EState_Check, m_url.c_str(), NULL, m_post_data, NULL);
			//ns_connect_http(&mgr, handler_EState_Check, url, NULL, NULL, NULL);
			s_exit_flag=0;
			break;

		case EState_NoUpdate:
			break;

		case EState_NoInternet:
			break;

		case EState_NoPower:
			break;

		case EState_Downloading:
			ns_mgr_init(&m_mgr, NULL);
			ns_connect_http( &m_mgr, handler_EState_Downloading
                     , m_update_response.url, NULL, NULL
                     , m_output_filename.c_str()
                     );
			s_exit_flag=0;
			break;

		case EState_Verifying:
			break;

		case EState_DownloadFailed:
			break;

		case EState_ApplyingUpdate:
			break;
		case EState_UpdateFailed:
			break;
		case EState_AskForReboot:
			break;
	}

	s_last_state=s_cur_state;
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
			next_state(EState_Checking);
			break;

		case EState_Checking:
			if (s_exit_flag == 0) {
				ns_mgr_poll(&m_mgr, 1000);
				std::cerr<<"still checking...\n";
			} else {
				ns_mgr_free(&m_mgr);

				next_state( process_data( s_last_http_message.body.p
                                , s_last_http_message.body.len
                                )
                  );
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
				ns_mgr_poll(&m_mgr, 1000);
			} else {
				ns_mgr_free(&m_mgr);
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
			sleep(1);
			next_state(EState_Idle);
			break;

		case EState_ApplyingUpdate:
			sleep(3);
			next_state(EState_AskForReboot);	
			break;

		case EState_UpdateFailed:
			sleep(1);
			next_state(EState_Idle);
			break;

		case EState_AskForReboot:
			sleep(1);
			next_state(EState_Idle);
			break;
	}

	return s_cur_state;
	
}

//-----------------------------------------------------------------------------
OttDate::EState OttDate::process_data( const char *json, int json_len )
{
	struct json_token *arr, *tok;
	static char buf[4096];

	// Tokenize json string, fill in tokens array
	arr = parse_json2(json, strlen(json));
	if(!arr) {
		fprintf(stderr,"error parsing json: [%.*s]\n",json_len,json);
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
	fprintf(stderr,"ENTERING %s\n",__FUNCTION__);

	struct http_message *hm = (struct http_message *) ev_data;

	switch (ev) {
		case NS_CONNECT:
			if (* (int *) ev_data != 0) {
				fprintf(stderr, "connect() failed: %s\n", strerror(* (int *) ev_data));
				s_cur_state=EState_NoUpdate;
				s_exit_flag = 1;
			} else {
				fprintf(stderr,"connected\n");
			}
			break;

		case NS_HTTP_REPLY:
			nc->flags |= NSF_CLOSE_IMMEDIATELY;

			s_last_http_message = *hm;
			s_exit_flag = 1;
			break;

			//		case NS_RECV:
			//			fprintf(stderr,"r\n");
			//      break;

		case NS_CLOSE:
			s_exit_flag = 1;
			break;

		default:
			break;
	}
}


//-----------------------------------------------------------------------------
void OttDate::handler_EState_Downloading(struct ns_connection *nc, int ev, void *ev_data)
{
	struct http_message *hm = (struct http_message *) ev_data;
	struct ns_str* content=0;

	static int full=0;
	static int some=0;
	char progress[]="-\\|/";
	static int progress_i=0;

	switch (ev) {
		case NS_CONNECT:
			if (* (int *) ev_data != 0) {
				fprintf(stderr, "connect() failed: %s\n", strerror(* (int *) ev_data));
				s_cur_state=EState_Idle;
				s_exit_flag = 1;
			}
			break;

		case NS_RECV:
			some+=*(int*)ev_data;
			if(full>0) {
				fprintf(stderr,"\rdownloading: %10d / %10d",some,full);
			} else {
				fprintf(stderr,"\rdownloading: %c",progress[(progress_i++)%4]);
			}
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


