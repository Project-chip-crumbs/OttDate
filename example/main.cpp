#include <iostream>
#include <unistd.h>
#include "ottdate.hpp"

int main(int argc, char*argv[])
{
  std::cerr<<"ottdate v?.?.?"<<std::endl;
	OttDate* od=OttDate::instance();

	sleep(2);
	OttDate::EState state=OttDate::EState_Checking;
	od->trigger_update();

	while(true) {
		std::cerr<<"\n main thread: current state: "<<od->state_name()<<"\n";
		sleep(1);
	}
}
