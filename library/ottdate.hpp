#ifndef OTTDATE_HPP
#define OTTDATE_HPP

#include <pthread.h>
#include <string>
#include <vector>

struct ns_mgr;

class OttDate
{
public:
	struct UpdateResponse 
	{
		int update_available;
		char *url;
		unsigned int size;
		char *checksum;
		unsigned int new_version;
	};

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

	virtual ~OttDate();
	static  OttDate *instance();
	
	int    download_percentage();
	EState current_state() {return s_cur_state;}
	void   state_name( std::string &s );
	std::string state_name();
	bool   trigger_update();
 
private:
	OttDate();
	OttDate(OttDate const&) {};
	OttDate& operator=(OttDate const&) {};
	static OttDate* s_instance;

	void enter_state( OttDate::EState state );
	static void next_state( OttDate::EState state );
  char* getRaspiSerial();
	int getStakVersion();

	EState process_data( const char *json );
	EState main_loop();
	static void* run_main_loop(void*); //different thread
	
	static void handler_EState_Check(struct ns_connection *nc, int ev, void *ev_data);
	static void handler_EState_Downloading(struct ns_connection *nc, int ev, void *ev_data);

  int verify_md5(const std::string &filename,const std::string &md5sum);

	
  std::vector<std::string> m_state_names;
	int                      m_post_data_len;
  char *                   m_post_data;
  struct ns_mgr*           m_mgr;
	std::string              m_url;
	std::string              m_output_filename;
	UpdateResponse           m_update_response;

	static char *            s_last_http_message;
	static int               s_exit_flag;
  static EState            s_cur_state;
  static EState            s_last_state;
  static int							 s_download_percentage;
	static time_t						 s_last_recv;

	pthread_t                m_thread;

	const static int         MD5_DIGEST_LENGTH;
	const static int         TIMEOUT;
};

#endif //OTTDATE_HPP
