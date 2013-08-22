#ifndef __NET_PULSE_VIEW_H__
#define __NET_PULSE_VIEW_H__

#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <String.h>
#include <MessageRunner.h>
#include <net/net_control.h>
#include "NetPulseStatsView.h"

class _EXPORT CNetPulseView : public BView {
public:
	enum {
		C_NET_PULSE_CONNECT			= 10000,
		C_NET_PULSE_DISCONNECT		= 10001,
		C_NET_PULSE_UPDATE 			= 10002,
		C_NET_PULSE_QUIT			= 10003,
		C_NET_PULSE_STATISTICS		= 10004,
		C_NET_PULSE_CHANGE_INTERFACE= 10010
	};
	
	enum {
		C_NET_PULSE_COLOR_LEVELS	= 32
	};

public:
	CNetPulseView(const char *name);
	
	CNetPulseView(BMessage *message);
	
	virtual ~CNetPulseView();
	
	void AttachedToWindow();
	
	void MessageReceived(BMessage *message);

	void MouseDown(BPoint where);
	
	void Draw(BRect updateRect);
		
	void Update();
	
	static CNetPulseView *Instantiate(BMessage *message);
	
	virtual status_t Archive(BMessage *data, bool deep) const;

public:
	rgb_color InputColor() const;
	
	rgb_color OutputColor() const;
	
	void SetInputColor(rgb_color color);
	
	void SetOutputColor(rgb_color color);
	
	bigtime_t UpdateInterval() const;
	
	float DecayRate() const;
	
	void SetUpdateInterval(bigtime_t interval);
	
	void SetDecayRate(float rate);

private:
	void UpdateBitmap();

	void UpdateColorTable();
			
private:
	BMessenger *fMessengerNetPulseStatsView;
	BMessageRunner *fMessageRunner;
	BBitmap *fModemDownBitmap;
	BBitmap *fModemUpBitmap;
	bool fEnable;
	if_index_t fIndex;
	uint32 fInputBytes;
	uint32 fOutputBytes;
	float fInputRate;
	float fOutputRate;
	float fMaxInputRate;
	float fMaxOutputRate;
	uint8 fBitmapInputColor;
	uint8 fBitmapOutputColor;
	rgb_color fInputColor;
	rgb_color fOutputColor;
	float fDecayRate;
	bigtime_t fUpdateInterval;
	uint8 fInputColorTable[C_NET_PULSE_COLOR_LEVELS];
	uint8 fOutputColorTable[C_NET_PULSE_COLOR_LEVELS];
};

#endif
