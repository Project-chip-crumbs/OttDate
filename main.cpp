#include "ottdate.hpp"

int main(int argc, char*argv[])
{
	OttDate* od=OttDate::instance();

	od->next_state(OttDate::EState_Checking);
	while(true) {
		od->main_loop();
	}
}
