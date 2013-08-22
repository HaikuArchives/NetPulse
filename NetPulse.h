#ifndef __NET_PULSE_APP_H__
#define __NET_PULSE_APP_H__

#include <Application.h>

extern const char *C_NET_PULSE_APP_SIGNATURE;

class CNetPulseApplication : public BApplication {
public:
	 CNetPulseApplication();
	 
	 virtual ~CNetPulseApplication();
	 
	 virtual void ReadyToRun();
};

#endif
