#ifndef OTTDATE_HPP
#define OTTDATE_HPP

#include <string>
#include <vector>

#include "fossa.h"

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

	static OttDate *instance();
	EState main_loop();
	
	void next_state( OttDate::EState state );

private:
	OttDate();
	OttDate(OttDate const&) {};
	OttDate& operator=(OttDate const&) {};
	static OttDate* s_instance;

	void enter_state( OttDate::EState state );
  char* getRaspiSerial();

	EState process_data( const char *json, int json_len );
	
	static void handler_EState_Check(struct ns_connection *nc, int ev, void *ev_data);
	static void handler_EState_Downloading(struct ns_connection *nc, int ev, void *ev_data);

  int verify_md5(const std::string &filename,const std::string &md5sum);

//-------
	std::vector<std::string> m_state_names;
	int                      m_post_data_len;
  char *                   m_post_data;
  struct ns_mgr            m_mgr;
	std::string              m_url;
	std::string              m_output_filename;
	UpdateResponse           m_update_response;

	static struct http_message s_last_http_message;
	static int                 s_exit_flag;
  static EState              s_cur_state;
  static EState              s_last_state;

	const static int MD5_DIGEST_LENGTH;
};

#endif //OTTDATE_HPP
