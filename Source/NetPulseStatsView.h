#ifndef _NET_PULSE_STATS_VIEW_H
#define _NET_PULSE_STATS_VIEW_H


#include <NetworkInterface.h>
#include <View.h>


const uint32 kMsgQuit				= 10003;
const uint32 kMsgStatistics			= 10004;
const uint32 kMsgChangeInterface	= 10010;

const int32 kViewWidth				= 380;
const int32 kViewHeight				= 260;


class BStringView;


class NetPulseStatsView : public BView {
public:
								NetPulseStatsView(const char* name, int32 index);
	virtual						~NetPulseStatsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Pulse();

private:
			uint32				fCookie;
			BNetworkInterface	fInterface;

			BStringView*		fConnectedToView;
			BStringView*		fElapsedTimeView;
			BStringView*		fLocalAddressView;
			BStringView*		fOBytesView;
			BStringView*		fIBytesView;
			BStringView*		fOPacketsView;
			BStringView*		fIPacketsView;
			BStringView*		fCollisionsView;
			BStringView*		fErrorsView;
};


#endif	// _NET_PULSE_STATS_VIEW_H
