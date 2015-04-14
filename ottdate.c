/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 *
 * This program fetches HTTP URLs.
 */

#include "fossa.h"
#include <termios.h>
#include <unistd.h>

#define MD5_DIGEST_LENGTH 16


typedef struct UpdateResponse 
{
	int update_available;
	char *url;
	unsigned int size;
	char *checksum;
	unsigned int new_version;
} UpdateResponse;

typedef enum EState
{ EState_Idle            = 0     // remain in this state until user triggers update check       
, EState_Checking        = 1     // delete all temp file, get json data from update.s-t-a-k.com 
, EState_NoUpdate        = 2     // display "you are up-to-date" for a second and go to idle    
, EState_NoInternet      = 3     // display "no internet connection" for a second and go to idle
, EState_NoPower         = 4     // display "no power connection" for a second and go to idle   
, EState_Downloading     = 5     // download files, show progress                               
, EState_Verifying       = 6     // verify checksum, show progress                              
, EState_DownloadFailed  = 7     // display download failed go to idle                          
, EState_ApplyingUpdate  = 8     // apply update, show progress                                 
, EState_UpdateFailed    = 9     // display "update failed" for a second and go to idle         
, EState_AskForReboot    = 10    // display "update successful - please reboot" for ever        
, EState_Undefined       = 0xffff
} EState;


// nasty global variables
static int s_exit_flag = 0;
static UpdateResponse update_response;
static EState cur_state =  EState_Idle;
static struct http_message last_http_message;

char *StateNames[] =
{ "EState_Idle"
, "EState_Checking"
, "EState_NoUpdate"
, "EState_NoInternet"
, "EState_NoPower"
, "EState_Downloading"   
, "EState_Verifying"      
, "EState_DownloadFailed"
, "EState_ApplyingUpdate" 
, "EState_UpdateFailed"
, "EState_AskForReboot"
};


static EState process_data( const char *json, int json_len )
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

			update_response.update_available = !strncmp("true",tok->ptr,4);
		}
		tok = find_json_token(arr, "url");
		if(tok!=NULL) {
			update_response.url = malloc(sizeof(char)*tok->len+1);
			strncpy(update_response.url,tok->ptr,tok->len);
			update_response.url[tok->len]=0;
			
			fprintf(stderr,"url= [%.*s]\n", tok->len, tok->ptr);
		}
		tok = find_json_token(arr, "size");
		if(tok!=NULL) {
			strncpy(buf,tok->ptr,tok->len);
			buf[tok->len]=0;
			fprintf(stderr,"size= [%d]\n", atoi(buf));
			update_response.size = atoi(buf);
		}
		tok = find_json_token(arr, "checksum");
		if(tok!=NULL) {
			update_response.checksum = malloc(sizeof(char)*tok->len+1);
			strncpy(update_response.checksum,tok->ptr,tok->len);
			update_response.checksum[tok->len]=0;
  		fprintf(stderr,"checksum= [%.*s]\n", tok->len, tok->ptr);
		}
		tok = find_json_token(arr, "new_version");
		if(tok!=NULL) {
			strncpy(buf,tok->ptr,tok->len);
			buf[tok->len]=0;
			update_response.size = atoi(buf);
			fprintf(stderr,"new_version= [%d]\n", atoi(buf));
		}

		// Do not forget to free allocated tokens array
		free(arr);

		if(update_response.update_available) {
			return EState_Downloading;
		}

		return EState_NoUpdate;
}

static void handler_EState_Check(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case NS_CONNECT:
      if (* (int *) ev_data != 0) {
        fprintf(stderr, "connect() failed: %s\n", strerror(* (int *) ev_data));
				cur_state=EState_NoUpdate;
        s_exit_flag = 1;
      } else {
				fprintf(stderr,"connected\n");
			}
      break;

    case NS_HTTP_REPLY:
      nc->flags |= NSF_CLOSE_IMMEDIATELY;

			last_http_message = *hm;
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

static void handler_EState_Downloading(struct ns_connection *nc, int ev, void *ev_data) {
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
				cur_state=EState_Idle;
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


int verify_md5(const char *filename,const char* md5sum)
{
    unsigned char c[MD5_DIGEST_LENGTH];
    unsigned char d[MD5_DIGEST_LENGTH*2+1];
    int i;
    FILE *inFile = fopen (filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL) {
        printf ("%s can't be opened.\n", filename);
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
		int r=strncmp(d,md5sum,MD5_DIGEST_LENGTH);
    fprintf (stderr,"%s %s %s %s\n"
           , r==0 ? "SUCCESS:" : "ERROR:"
           , md5sum
           , r==0 ? "==" : "!="
           , d );
    return r;
}

char* getRaspiSerial()
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

int main(int argc, char *argv[])
{
  struct ns_mgr mgr;
 // char *url="http://stak-images.s3.amazonaws.com/firmware/otto/update/latest";
  char *url="http://update.s-t-a-k.com";
	char *output_filename="ottdate.zip";

	EState last_state =  EState_Undefined;

	//assemble data for request:
	int post_data_len=1024;
	char post_data[post_data_len];
	json_emit( post_data, post_data_len
           , "{s: s, s: s, s: i, s: s}"
           , "product", "otto"
           , "component", "fullstak"
           , "version", 101
           , "id", getRaspiSerial()
           );
	//TODO: READ version from STAK

	//for now: just immediately trigger the update
	cur_state=EState_Checking;

	while(1)
	{
		if(last_state!=cur_state) {
			fprintf(stderr,"switched to state %s\n",StateNames[cur_state]);
		}

		switch(cur_state)
	  {
      case EState_Idle:
				
				//for now: quit in idle state
				fprintf(stderr,"last state %s\n",StateNames[last_state]);
				exit(last_state);
				last_state=cur_state;
				cur_state=EState_Checking;
				break;

			case EState_Checking:
				//TODO: check for power: cur_state=EState_NoPower;
				//TODO: check for internet: cur_state=EState_NoInternet;

			  fprintf(stderr,"sending json: %s\n",post_data);

				ns_mgr_init(&mgr, NULL);
				ns_connect_http(&mgr, handler_EState_Check, url, NULL, post_data, NULL);
				//ns_connect_http(&mgr, handler_EState_Check, url, NULL, NULL, NULL);
				s_exit_flag=0;
			  fprintf(stderr,"entering loop\n");
				while (s_exit_flag == 0) {
					ns_mgr_poll(&mgr, 1000);
				}
				ns_mgr_free(&mgr);
						
				struct ns_str* content=ns_get_http_header(&last_http_message,"Content-Type");
				if( content ) {
					cur_state=process_data(last_http_message.body.p, last_http_message.body.len);

					//char *content_type="application/json";
					//if( !strncmp(content_type,content->p, strlen(content_type)) ) {
					//	fprintf(stderr,"got application/json data\n");

					//}
					//else {
					//	cur_state=EState_NoUpdate;
					//}
				} else {
					cur_state=EState_NoUpdate;
				}
				break;

      case EState_NoUpdate:
				sleep(1);
				last_state=cur_state;
				cur_state=EState_Idle;
				break;

      case EState_NoInternet:
				sleep(1);
				last_state=cur_state;
				cur_state=EState_Idle;
				break;

      case EState_NoPower:
				sleep(1);
				last_state=cur_state;
				cur_state=EState_Idle;
				break;

      case EState_Downloading:
				ns_mgr_init(&mgr, NULL);
				ns_connect_http(&mgr, handler_EState_Downloading, update_response.url, NULL, NULL, output_filename);
				s_exit_flag=0;
				while (s_exit_flag == 0) {
					ns_mgr_poll(&mgr, 1000);
				}
				ns_mgr_free(&mgr);
				cur_state=EState_Verifying;
				break;

      case EState_Verifying:
				if(verify_md5(output_filename,update_response.checksum)) {
					last_state=cur_state;
					cur_state=EState_DownloadFailed;
				} else {
					last_state=cur_state;
					cur_state=EState_ApplyingUpdate;
				}
				break;

			case EState_DownloadFailed:
				sleep(1);
				last_state=cur_state;
				cur_state=EState_Idle;
				break;
      case EState_ApplyingUpdate:
				sleep(3);
				last_state=cur_state;
			  cur_state=EState_AskForReboot; 	
				break;
      case EState_UpdateFailed:
				sleep(1);
				last_state=cur_state;
				cur_state=EState_Idle;
				break;
      case EState_AskForReboot:
				sleep(1);
				last_state=cur_state;
				cur_state=EState_Idle;
				break;
		}

	}

  return 0;
}
