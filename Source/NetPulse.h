#ifndef _NET_PULSE_H
#define _NET_PULSE_H

#include <Application.h>


extern const char* kAppSignature;


class NetPulse : public BApplication {
public:
		 						NetPulse();
	virtual						~NetPulse();

	virtual	void				ReadyToRun();
};


#endif	// _NET_PULSE_H
