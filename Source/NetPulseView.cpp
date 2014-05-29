#include "NetPulseView.h"

#include <string.h>

#include <Debug.h>
#include <Deskbar.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuItem.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <TextView.h>
#include <Window.h>

#include "NetPulse.h"
#include "NetPulseBitmaps.h"
#include "SettingsFile.h"


static const uint32 kMsgConnect			= 10000;
static const uint32 kMsgDisconnect		= 10001;
static const uint32 kMsgUpdate 			= 10002;

static const rgb_color kDefaultInputColor = (rgb_color){ 255, 0, 0 };
static const rgb_color kDefaultOutputColor = (rgb_color){ 0, 0, 255 };
static const bigtime_t kDefaultUpdateInterval = 250000LL;
static const float kDefaultDecayRate = 0.90;


NetPulseView::NetPulseView(const char* name)
	:
	BView(BRect(0, 0, C_MODEM_ICON_WIDTH - 1, C_MODEM_ICON_HEIGHT - 1),
		name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	fMessenger(NULL),
	fMessageRunner(NULL),
	fModemDownBitmap(NULL),
	fModemUpBitmap(NULL),
	fEnable(false),
	fCookie(0),
	fInputBytes(0),
	fOutputBytes(0),
	fInputRate(0),
	fOutputRate(0),
	fMaxInputRate(0),
	fMaxOutputRate(0),
	fBitmapInputColor(C_MODEM_ICON_INPUT_COLOR),
	fBitmapOutputColor(C_MODEM_ICON_OUTPUT_COLOR),
	fInputColor(kDefaultInputColor),
	fOutputColor(kDefaultOutputColor),
	fDecayRate(kDefaultDecayRate),
	fUpdateInterval(kDefaultUpdateInterval)
{
}


NetPulseView::NetPulseView(BMessage* message)
	:
	BView(message),
	fMessenger(NULL),
	fMessageRunner(NULL),
	fModemDownBitmap(NULL),
	fModemUpBitmap(NULL),
	fEnable(false),
	fCookie(0),
	fInputBytes(0),
	fOutputBytes(0),
	fInputRate(0),
	fOutputRate(0),
	fMaxInputRate(0),
	fMaxOutputRate(0),
	fBitmapInputColor(C_MODEM_ICON_INPUT_COLOR),
	fBitmapOutputColor(C_MODEM_ICON_OUTPUT_COLOR),
	fInputColor(kDefaultInputColor),
	fOutputColor(kDefaultOutputColor),
	fDecayRate(kDefaultDecayRate),
	fUpdateInterval(kDefaultUpdateInterval)
{
	SettingsFile settings("Settings", "NetPulse");
	if (settings.Load() == B_OK) {
		settings.FindInt32("Interface", (int32*)&fCookie);
		settings.FindInt32("InputColor", (int32*)&fInputColor);
		settings.FindInt32("OutputColor", (int32*)&fOutputColor);
		settings.FindFloat("DecayRate", &fDecayRate);
		settings.FindInt64("UpdateInterval", &fUpdateInterval);
	}
}


NetPulseView::~NetPulseView()
{
	if (fMessenger != NULL
		&& fMessenger->IsValid()) {
		fMessenger->SendMessage(kMsgQuit);
	}

	delete fMessageRunner;
	delete fModemDownBitmap;
	delete fModemUpBitmap;
}


NetPulseView*
NetPulseView::Instantiate(BMessage* message)
{
	if (validate_instantiation(message, "NetPulseView"))
		return new NetPulseView(message);

	return NULL;
}


status_t
NetPulseView::Archive(BMessage* data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddString("add_on", kAppSignature);
	data->AddString("class", "NetPulseView");

	return B_OK;
}


void
NetPulseView::SetInputColor(rgb_color color)
{
	fInputColor = color;
	UpdateColorTable();
}


void
NetPulseView::SetOutputColor(rgb_color color)
{
	fOutputColor = color;
	UpdateColorTable();
}


void
NetPulseView::SetUpdateInterval(bigtime_t interval)
{
	if (interval >= 50000LL && interval <= 1000000LL) {
		fUpdateInterval = interval;
		if (fMessageRunner != NULL)
			fMessageRunner->SetInterval(fUpdateInterval);
	}
}


void
NetPulseView::SetDecayRate(float rate)
{
	if (rate >= 0 && rate <= 1)
		fDecayRate = rate;
}


void
NetPulseView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fMessageRunner = new BMessageRunner(BMessenger(this),
		new BMessage(kMsgUpdate), fUpdateInterval);

	fModemDownBitmap = new BBitmap(
		BRect(0, 0, C_MODEM_ICON_WIDTH - 1, C_MODEM_ICON_HEIGHT - 1), B_COLOR_8_BIT);

	if (fModemDownBitmap != NULL) {
		fModemDownBitmap->SetBits(kModemDownIconBits, sizeof(kModemDownIconBits),
			0, B_COLOR_8_BIT);
	}

	fModemUpBitmap = new BBitmap(
		BRect(0, 0, C_MODEM_ICON_WIDTH - 1, C_MODEM_ICON_HEIGHT - 1), B_COLOR_8_BIT);
	if (fModemUpBitmap != NULL) {
		fModemUpBitmap->SetBits(kModemUpIconBits, sizeof(kModemUpIconBits),
			0, B_COLOR_8_BIT);
	}

	if (Parent() != NULL) {
		if ((Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0)
			SetViewColor(B_TRANSPARENT_COLOR);
		else
			SetViewColor(Parent()->ViewColor());
	} else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLowColor(ViewColor());

	UpdateColorTable();
	Update();
}


void
NetPulseView::MessageReceived(BMessage* message)
{
	if (message->what == kMsgUpdate) {
		Update();
		Draw(Bounds());
	} else if (message->what == kMsgQuit) {
		SettingsFile settings("Settings", "NetPulse");
		settings.AddInt32("Interface", fCookie);
		settings.AddInt32("InputColor", *((int32 *) &fInputColor));
		settings.AddInt32("OutputColor", *((int32 *) &fOutputColor));
		settings.AddFloat("DecayRate", fDecayRate);
		settings.AddInt64("UpdateInterval", fUpdateInterval);
		settings.Save();

		BDeskbar deskbar;
		deskbar.RemoveItem(Name());
	} else if (message->what == kMsgConnect) {
		if (fCookie != 0) {
			BNetworkInterface interface(fCookie);
			interface.SetFlags(interface.Flags() & IFF_UP);
		}
	} else if (message->what == kMsgDisconnect) {
		if (fCookie != 0) {
			BNetworkInterface interface(fCookie);
			interface.SetFlags(interface.Flags() & ~IFF_UP);
		}
	} else if (message->what >= kMsgChangeInterface + 1
		&& message->what <= kMsgChangeInterface + 1000) {
		fCookie = message->what - kMsgChangeInterface;
		fInputRate = fOutputRate = 0;
		fMaxInputRate = fMaxOutputRate = 0;

		BNetworkInterface interface(fCookie);
		ifreq_stats stats;
		if (interface.GetStats(stats) == B_OK) {
			fInputBytes = stats.receive.bytes;
			fOutputBytes = stats.send.bytes;
		}

		Update();

		if (fMessenger != NULL) {
			BMessage message(kMsgChangeInterface);
			message.AddInt32("interface_index", fCookie);
			if (fMessenger->SendMessage(&message) != B_OK)
				fMessenger = NULL;
		}
	} else if (message->what == kMsgStatistics) {
		if (fMessenger == NULL || !fMessenger->IsValid()) {
			BRect frame(BScreen().Frame());
			BWindow* window = new BWindow(
				BRect(frame.right - kViewWidth - 20, frame.top + 60,
					frame.right - 20 - 1, frame.top + 60 + kViewHeight - 1),
				"Statistics", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_NOT_RESIZABLE
						| B_AUTO_UPDATE_SIZE_LIMITS);
			BView* view = new NetPulseStatsView("Statistics", fCookie);
			view->Pulse();
			BLayoutBuilder::Group<>(window, B_VERTICAL)
				.Add(view)
				.End();
			window->SetPulseRate(1000000LL);
			window->Show();
			fMessenger = new BMessenger(view);
		}
	} else
		BView::MessageReceived(message);
}


void
NetPulseView::Draw(BRect updateRect)
{
	if (fEnable) {
		SetDrawingMode(B_OP_OVER);
		if (fModemUpBitmap != NULL)
			DrawBitmapAsync(fModemUpBitmap, BPoint(0, 0));
	} else {
		SetHighColor(ViewColor());
		FillRect(updateRect);

		SetDrawingMode(B_OP_OVER);
		if (fModemDownBitmap != NULL)
			DrawBitmapAsync(fModemDownBitmap, BPoint(0, 0));
	}
}


void
NetPulseView::Update()
{
	BNetworkInterface interface(fCookie);

	if ((interface.Flags() & IFF_UP) != 0) {
		ifreq_stats stats;
		if (interface.GetStats(stats) == B_OK) {
			fInputRate = fDecayRate * (stats.receive.bytes - fInputBytes)
				+ (1 - fDecayRate) * fInputRate;
			fOutputRate = fDecayRate * (stats.send.bytes - fOutputBytes)
				+ (1 - fDecayRate) * fOutputRate;

			fInputBytes = stats.receive.bytes;
			fOutputBytes = stats.send.bytes;
		}

		fMaxInputRate = fInputRate >= fMaxInputRate
			? fInputRate
			: 0.9 * fMaxInputRate;
		fMaxOutputRate = fOutputRate >= fMaxOutputRate
			? fOutputRate
			: 0.9 * fMaxOutputRate;

		if (!fEnable || fMaxInputRate > 0 || fMaxOutputRate > 0) {
			UpdateBitmap();
			Draw(Bounds());
		}
	} else {
		fInputRate = fOutputRate = 0;
		fMaxInputRate = fMaxOutputRate = 0;
		if (fEnable)
			Draw(Bounds());
	}
	fEnable = ((interface.Flags() & IFF_UP) != 0);
}


void
NetPulseView::MouseDown(BPoint where)
{
	int32 buttons;
	int32 clicks;
	if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK
		|| Window()->CurrentMessage()->FindInt32("clicks", &clicks) != B_OK) {
		return;
	}

	if (buttons == B_SECONDARY_MOUSE_BUTTON) {	
		BPopUpMenu* popUpMenu = new BPopUpMenu("NetPulseMenu");

		popUpMenu->AddItem(new BMenuItem("Connect",
			new BMessage(kMsgConnect)));
		popUpMenu->AddItem(new BMenuItem("Disconnect",
			new BMessage(kMsgDisconnect)));
		popUpMenu->AddSeparatorItem();

		popUpMenu->AddItem(new BMenuItem("Statistics...",
			new BMessage(kMsgStatistics)));
		popUpMenu->AddSeparatorItem();

		uint32 cookie = 0;
		BNetworkInterface interface;
		BNetworkRoster& roster = BNetworkRoster::Default();
		while (roster.GetNextInterface(&cookie, interface) == B_OK) {
			const char* name = interface.Name();
			if (strncmp(name, "loop", 4) != 0) {
				BMenuItem* menuItem = new BMenuItem(name,
					new BMessage(kMsgChangeInterface + cookie));
				menuItem->SetMarked(cookie == fCookie);
				popUpMenu->AddItem(menuItem);
			}
		}

		popUpMenu->AddSeparatorItem();
		popUpMenu->AddItem(new BMenuItem("Quit", new BMessage(kMsgQuit)));
		popUpMenu->SetTargetForItems(this);

		popUpMenu->FindItem(kMsgConnect)->SetEnabled(!fEnable && fCookie != 0);
		popUpMenu->FindItem(kMsgDisconnect)->SetEnabled(fEnable && fCookie != 0);
		popUpMenu->FindItem(kMsgStatistics)->SetEnabled(fEnable && fCookie != 0);

		popUpMenu->Go(ConvertToScreen(BPoint(0, 0)), true, false, false);
		delete popUpMenu;
	} else if (buttons == B_PRIMARY_MOUSE_BUTTON && clicks == 2)
		be_roster->Launch("application/x-vnd.Haiku-Network");
}


void
NetPulseView::UpdateBitmap()
{
	if (fModemUpBitmap != NULL) {
		BScreen screen(Window());
		
		const uint8 inputColor = fInputColorTable[fMaxInputRate <= 0
			? 0
			: int32((kColorLevels - 1) * fInputRate / fMaxInputRate)];
		const uint8 outputColor = fOutputColorTable[fMaxOutputRate <= 0
			? 0
			: int32((kColorLevels - 1) * fOutputRate / fMaxOutputRate)];

		if (fBitmapInputColor != inputColor || fBitmapOutputColor != outputColor) {
			fBitmapInputColor = inputColor;
			fBitmapOutputColor = outputColor;
			uint8 * bits = (uint8 *) fModemUpBitmap->Bits();
			for (uint32 offset = 0; offset < sizeof(kModemUpIconBits); offset++) {
				const uint8 color = kModemUpIconBits[offset];
				if (color == C_MODEM_ICON_INPUT_COLOR)
					bits[offset] = inputColor;
				if (color == C_MODEM_ICON_OUTPUT_COLOR)
					bits[offset] = outputColor;
			}
		}
	}
}


void
NetPulseView::UpdateColorTable()
{
	BScreen screen(Window());

	for (int32 index = 0; index < kColorLevels; index++) {
		const float scale = float(index) / (kColorLevels - 1);

		fInputColorTable[index] = screen.IndexForColor(
			uint8((1 - scale) * 208 + scale * fInputColor.red),
			uint8((1 - scale) * 208 + scale * fInputColor.green),
			uint8((1 - scale) * 208 + scale * fInputColor.blue));

		fOutputColorTable[index] = screen.IndexForColor(
			uint8((1 - scale) * 208 + scale * fOutputColor.red),
			uint8((1 - scale) * 208 + scale * fOutputColor.green),
			uint8((1 - scale) * 208 + scale * fOutputColor.blue));
	}
}
