#ifndef _NET_PULSE_VIEW_H
#define _NET_PULSE_VIEW_H


#include <Bitmap.h>
#include <MessageRunner.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "NetPulseStatsView.h"


const int32 kColorLevels = 32;


class _EXPORT NetPulseView : public BView {
public:
								NetPulseView(const char* name);
								NetPulseView(BMessage* message);
	virtual						~NetPulseView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				Draw(BRect updateRect);
	virtual	void				Update();

	static	NetPulseView*		Instantiate(BMessage* message);

	virtual	status_t			Archive(BMessage* data, bool deep) const;

public:
			rgb_color			InputColor() const { return fInputColor; };
			void				SetInputColor(rgb_color color);

			rgb_color			OutputColor() const { return fOutputColor; };
			void				SetOutputColor(rgb_color color);

			bigtime_t			UpdateInterval() const { return fUpdateInterval; };
			void				SetUpdateInterval(bigtime_t interval);

			float				DecayRate() const { return fDecayRate; };
			void				SetDecayRate(float rate);

private:
			void				UpdateBitmap();

			void				UpdateColorTable();

			BMessenger*			fMessenger;
			BMessageRunner*		fMessageRunner;
			BBitmap*			fModemDownBitmap;
			BBitmap*			fModemUpBitmap;
			bool				fEnable;
			uint32				fCookie;
			uint32				fInputBytes;
			uint32				fOutputBytes;
			float				fInputRate;
			float				fOutputRate;
			float				fMaxInputRate;
			float				fMaxOutputRate;
			uint8				fBitmapInputColor;
			uint8				fBitmapOutputColor;
			rgb_color			fInputColor;
			rgb_color			fOutputColor;
			float				fDecayRate;
			bigtime_t			fUpdateInterval;
			uint8				fInputColorTable[kColorLevels];
			uint8				fOutputColorTable[kColorLevels];
};


#endif	// _NET_PULSE_VIEW_H
