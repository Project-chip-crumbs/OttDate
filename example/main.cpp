#include <iostream>
#include <unistd.h>

#include "fossa.h"
#include "ottdate.hpp"
#include "stdio.h"

int main(int argc, char*argv[])
{
  std::string server="http://update.s-t-a-k.com:8080/";
  std::string host;
  char buf[100];
 

  std::cerr<<"ottdate v?.?.?"<<std::endl;
  
  host = OttDate::resolve_url(server);

  if( !host.length() ) {
    std::cerr<<"cannot resolve hostname\n\n";
    return 1;
  }  

	OttDate* od=OttDate::instance();

	sleep(2);
	OttDate::EState state=OttDate::EState_Checking;
	od->trigger_update();

	while(true) {
		std::cerr<<"\n main thread: current state: "<<od->state_name()<<"\n";
		sleep(1);
	}
}
