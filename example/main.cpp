#include "ottdate.hpp"

int main(int argc, char*argv[])
{
	OttDate* od=OttDate::instance();

	OttDate::EState state=OttDate::EState_Checking;
	od->next_state(state);
	while(state!=OttDate::EState_Idle) {
		state=od->main_loop();
	}
}
