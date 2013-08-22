#ifndef __NET_PULSE_STATS_VIEW_H__
#define __NET_PULSE_STATS_VIEW_H__

#include <Window.h>
#include <View.h>
#include <Box.h>
#include <StringView.h>
#include <Message.h>
#include <net/net_control.h>

class CNetPulseStatsView : public BView {
public:
	enum {
		C_VIEW_WIDTH		= 280,
		C_VIEW_HEIGHT		= 200,
		C_QUIT				= 100000,
		C_CHANGE_INTERFACE	= 100001
	};
	
public:
	CNetPulseStatsView(const char *name, if_index_t index);
	
	virtual ~CNetPulseStatsView();
	
	void AttachedToWindow();
	
	void MessageReceived(BMessage *message);

	virtual void Pulse();
			
private:
	BStringView *fConnectedToView;
	BStringView *fElapsedTimeView;
	BStringView *fLocalAddressView;
	BStringView *fIBytesView;
	BStringView *fOBytesView;
	BStringView *fIPacketsView;
	BStringView *fOPacketsView;
	BStringView *fCollisionsView;
	BStringView *fErrorsView;
	if_index_t fIndex;
	ifreq fRequest;
};

#endif
