#include <Debug.h>
#include <Deskbar.h>
#include <Screen.h>
#include <Roster.h>
#include <Window.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextView.h>
#include <string.h>
#include "NetPulse.h"
#include "NetPulseView.h"
#include "NetPulseBitmaps.h"
#include "SettingsFile.h"

static const rgb_color C_DEFAULT_INPUT_COLOR = { 255, 0, 0 };
static const rgb_color C_DEFAULT_OUTPUT_COLOR = { 0, 0, 255 };
static const bigtime_t C_DEFAULT_UPDATE_INTERVAL = 250000LL;
static const float C_DEFAULT_DECAY_RATE = 0.90;

CNetPulseView::CNetPulseView(const char *name)
	:	BView(BRect(0, 0, C_MODEM_ICON_WIDTH-1, C_MODEM_ICON_HEIGHT-1),
			  name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
		fMessengerNetPulseStatsView(NULL),
		fMessageRunner(NULL),
		fModemDownBitmap(NULL),
		fModemUpBitmap(NULL),
		fEnable(false),
		fIndex(0),
		fInputBytes(0),
		fOutputBytes(0),
		fInputRate(0),
		fOutputRate(0),
		fMaxInputRate(0),
		fMaxOutputRate(0),
		fBitmapInputColor(C_MODEM_ICON_INPUT_COLOR),
		fBitmapOutputColor(C_MODEM_ICON_OUTPUT_COLOR),
		fInputColor(C_DEFAULT_INPUT_COLOR),
		fOutputColor(C_DEFAULT_OUTPUT_COLOR),
		fDecayRate(C_DEFAULT_DECAY_RATE),
		fUpdateInterval(C_DEFAULT_UPDATE_INTERVAL)
{
	PRINT(("CNetPulseView::CNetPulseView()\n"));
}

CNetPulseView::CNetPulseView(BMessage *message)
	:	BView(message),
		fMessengerNetPulseStatsView(NULL),
		fMessageRunner(NULL),
		fModemDownBitmap(NULL),
		fModemUpBitmap(NULL),
		fEnable(false),
		fIndex(0),
		fInputBytes(0),
		fOutputBytes(0),
		fInputRate(0),
		fOutputRate(0),
		fMaxInputRate(0),
		fMaxOutputRate(0),
		fBitmapInputColor(C_MODEM_ICON_INPUT_COLOR),
		fBitmapOutputColor(C_MODEM_ICON_OUTPUT_COLOR),
		fInputColor(C_DEFAULT_INPUT_COLOR),
		fOutputColor(C_DEFAULT_OUTPUT_COLOR),
		fDecayRate(C_DEFAULT_DECAY_RATE),
		fUpdateInterval(C_DEFAULT_UPDATE_INTERVAL)
{
	PRINT(("CNetPulseView::CNetPulseView()\n"));
	
	SettingsFile settings("Settings", "NetPulse");
	if (settings.Load() == B_OK) {
		settings.FindInt32("Interface", (int32 *) &fIndex);
		settings.FindInt32("InputColor", (int32 *) &fInputColor);
		settings.FindInt32("OutputColor", (int32 *) &fOutputColor);
		settings.FindFloat("DecayRate", &fDecayRate);
		settings.FindInt64("UpdateInterval", &fUpdateInterval);
	}
}

CNetPulseView::~CNetPulseView()
{
	PRINT(("CNetPulseView::~CNetPulseView()\n"));

	if (fMessengerNetPulseStatsView != NULL && fMessengerNetPulseStatsView->IsValid()) {
		fMessengerNetPulseStatsView->SendMessage(CNetPulseStatsView::C_QUIT);
	}
	
	if (fMessageRunner != NULL)
		delete fMessageRunner;

	if (fModemDownBitmap != NULL)
		delete fModemDownBitmap;

	if (fModemUpBitmap != NULL)
		delete fModemUpBitmap;
}

CNetPulseView *CNetPulseView::Instantiate(BMessage *message)
{
	PRINT(("CNetPulseView::Instantiate()\n"));

	if (validate_instantiation(message, "CNetPulseView"))
		return new CNetPulseView(message);
	return NULL;
}

status_t CNetPulseView::Archive(BMessage *data, bool deep) const
{
	PRINT(("CNetPulseView::Archive()\n"));

	BView::Archive(data, deep);
	data->AddString("add_on", C_NET_PULSE_APP_SIGNATURE);
	data->AddString("class", "CNetPulseView");
		
	return B_OK;
}

rgb_color CNetPulseView::InputColor() const
{
	PRINT(("CNetPulseView::InputColor()\n"));
	
	return fInputColor;
}

rgb_color CNetPulseView::OutputColor() const
{
	PRINT(("CNetPulseView::OutputColor()\n"));

	return fOutputColor;
}

void CNetPulseView::SetInputColor(rgb_color color)
{
	PRINT(("CNetPulseView::SetInputColor()\n"));

	fInputColor = color;
	UpdateColorTable();
}

void CNetPulseView::SetOutputColor(rgb_color color)
{
	PRINT(("CNetPulseView::SetOutputColor()\n"));

	fOutputColor = color;
	UpdateColorTable();
}

bigtime_t CNetPulseView::UpdateInterval() const
{
	PRINT(("CNetPulseView::UpdateInterval()\n"));

	return fUpdateInterval;
}

float CNetPulseView::DecayRate() const
{
	PRINT(("CNetPulseView::DecayRate()\n"));

	return fDecayRate;
}

void CNetPulseView::SetUpdateInterval(bigtime_t interval)
{
	PRINT(("CNetPulseView::SetUpdateInterval()\n"));
	
	if (interval >= 50000LL && interval <= 1000000LL) {
		fUpdateInterval = interval;
		if (fMessageRunner != NULL)
			fMessageRunner->SetInterval(fUpdateInterval);
	}
}

void CNetPulseView::SetDecayRate(float rate)
{
	PRINT(("CNetPulseView::SetDecayRate()\n"));
	
	if (rate >= 0 && rate <= 1)
		fDecayRate = rate;
}

void CNetPulseView::AttachedToWindow()
{
	PRINT(("CNetPulseView::AttachedToWindow()\n"));

	fMessageRunner = new BMessageRunner(BMessenger(this), new BMessage(C_NET_PULSE_UPDATE), fUpdateInterval);

	fModemDownBitmap = new BBitmap(BRect(0, 0, C_MODEM_ICON_WIDTH - 1, C_MODEM_ICON_HEIGHT - 1), B_COLOR_8_BIT);
	if (fModemDownBitmap != NULL)
		fModemDownBitmap->SetBits(kModemDownIconBits, sizeof(kModemDownIconBits), 0, B_COLOR_8_BIT);
	
	fModemUpBitmap = new BBitmap(BRect(0, 0, C_MODEM_ICON_WIDTH - 1, C_MODEM_ICON_HEIGHT - 1), B_COLOR_8_BIT);
	if (fModemUpBitmap != NULL)
		fModemUpBitmap->SetBits(kModemUpIconBits, sizeof(kModemUpIconBits), 0, B_COLOR_8_BIT);

	SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	
	UpdateColorTable();
	Update();
}

void CNetPulseView::MessageReceived(BMessage *message)
{
//	PRINT(("CNetPulseView::MessageReceived()\n"));

	if (message->what == C_NET_PULSE_UPDATE) {
		Update();
		Draw(Bounds());
	}
	else if (message->what == C_NET_PULSE_QUIT) {
		SettingsFile settings("Settings", "NetPulse");
		settings.AddInt32("Interface", fIndex);
		settings.AddInt32("InputColor", *((int32 *) &fInputColor));
		settings.AddInt32("OutputColor", *((int32 *) &fOutputColor));
		settings.AddFloat("DecayRate", fDecayRate);
		settings.AddInt64("UpdateInterval", fUpdateInterval);
		settings.Save();
	
		BDeskbar deskbar;
		deskbar.RemoveItem(Name());
	}
	else if (message->what == C_NET_PULSE_CONNECT) {
		if (fIndex != 0) {
			ifreq_t ifr;
			get_interface_by_index(fIndex, &ifr);
			set_iface_flag_by_name(ifr.ifr_name, IFF_UP);
		}
	}
	else if (message->what == C_NET_PULSE_DISCONNECT) {
		if (fIndex != 0) {
			ifreq_t ifr;
			get_interface_by_index(fIndex, &ifr);
			clr_iface_flag_by_name(ifr.ifr_name, IFF_UP);
		}
	}
	else if (message->what >= C_NET_PULSE_CHANGE_INTERFACE + 1 &&
			 message->what <= C_NET_PULSE_CHANGE_INTERFACE + 1000) {
		fIndex = message->what - C_NET_PULSE_CHANGE_INTERFACE;
		fInputRate = fOutputRate = 0;
		fMaxInputRate = fMaxOutputRate = 0;
	
		ifreq_t ifr;
		if (get_interface_by_index(fIndex, &ifr) == B_OK) {
			fInputBytes = ifr.ifr_ibytes;
			fOutputBytes = ifr.ifr_obytes;
		}
		
		Update();
		
		if (fMessengerNetPulseStatsView != NULL) {
			BMessage message(CNetPulseStatsView::C_CHANGE_INTERFACE);
			message.AddInt32("interface_index", fIndex);
			if (fMessengerNetPulseStatsView->SendMessage(&message) != B_OK)
				fMessengerNetPulseStatsView = NULL;
		}
	}
	else if (message->what == C_NET_PULSE_STATISTICS) {
		if (fMessengerNetPulseStatsView == NULL || !fMessengerNetPulseStatsView->IsValid()) {
			BRect frame(BScreen().Frame());
			BWindow *window = new BWindow(BRect(frame.right - CNetPulseStatsView::C_VIEW_WIDTH - 20, frame.top + 60,
												frame.right - 20 - 1, frame.top + 60 + CNetPulseStatsView::C_VIEW_HEIGHT - 1),
												"Statistics", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE + B_NOT_RESIZABLE);
			BView *view = new CNetPulseStatsView("Statistics", fIndex);
			view->Pulse();
			window->AddChild(view);
			window->SetPulseRate(1000000LL);
			window->Show();
			fMessengerNetPulseStatsView = new BMessenger(view);
		}
	}
	else {	
		BView::MessageReceived(message);
	}
}

void CNetPulseView::Draw(BRect updateRect)
{
//	PRINT(("CNetPulseView::Draw()\n"));

	if (fEnable) {
		SetDrawingMode(B_OP_OVER);
		if (fModemUpBitmap != NULL)
			DrawBitmapAsync(fModemUpBitmap, BPoint(0, 0));
	}
	else {
		SetHighColor(ViewColor());
		FillRect(updateRect);
		
		SetDrawingMode(B_OP_OVER);
		if (fModemDownBitmap != NULL)
			DrawBitmapAsync(fModemDownBitmap, BPoint(0, 0));
	}
}

void CNetPulseView::Update()
{
//	PRINT(("CNetPulseView::Update()\n"));

	ifreq_t ifr;

	if (get_interface_by_index(fIndex, &ifr) == B_OK) {
		if ((ifr.ifr_flags & IFF_UP) != 0) {
			fInputRate = fDecayRate * (ifr.ifr_ibytes - fInputBytes) + (1 - fDecayRate) * fInputRate;
			fOutputRate = fDecayRate * (ifr.ifr_obytes - fOutputBytes) + (1 - fDecayRate) * fOutputRate;

			fInputBytes = ifr.ifr_ibytes;
			fOutputBytes = ifr.ifr_obytes;
			
			fMaxInputRate = (fInputRate >= fMaxInputRate ? fInputRate : 0.9 * fMaxInputRate);
			fMaxOutputRate = (fOutputRate >= fMaxOutputRate ? fOutputRate : 0.9 * fMaxOutputRate);
			
			if (!fEnable || fMaxInputRate > 0 || fMaxOutputRate > 0) {
				UpdateBitmap();
				Draw(Bounds());
			}
		}
		else {
			fInputRate = fOutputRate = 0;
			fMaxInputRate = fMaxOutputRate = 0;
			if (fEnable)
				Draw(Bounds());
		}
		fEnable = ((ifr.ifr_flags & IFF_UP) != 0);
	}
}

void CNetPulseView::MouseDown(BPoint where)
{
	PRINT(("CNetPulseView::MouseDown()\n"));

	static const char *kNames[][2] = {
		{ "loop0", "Loopback" },
		{ "ppp0", "Modem" },
		{ "/dev/net/eepro100", "EtherExpress" },
		{ "/dev/net/ec9xx", "3Com 905" },
		{ "/dev/net/tcm589", "3Com 589" },
		{ "/dev/net/tcm509", "3Com 509" },
		{ "/dev/net/tlan", "TLan" },
		{ "/dev/net/tulip", "DEC Tulip" },
		{ "/dev/net/netmate", "Netmate USB" },
		{ "/dev/net/dp83815", "National DP83815" },
		{ "/dev/net/rtl8139", "RealTek 8139" },
		{ "/dev/net/ne2k", "NE2000" },
		{ "/dev/net/prism", "Prism 802.11" },
		{ "/dev/net/vt86c100", "VT86c100" },
		{ "/dev/net/wavelan", "Wavelan 802.11" },
		{ NULL, NULL }
	};

	int32 buttons, clicks;
	
	if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK ||
		Window()->CurrentMessage()->FindInt32("clicks", &clicks) != B_OK)
		return;
	
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {	
		BPopUpMenu *popUpMenu = new BPopUpMenu("NetPulseMenu");
		
		popUpMenu->AddItem(new BMenuItem("Connect", new BMessage(C_NET_PULSE_CONNECT)));
		popUpMenu->AddItem(new BMenuItem("Disconnect", new BMessage(C_NET_PULSE_DISCONNECT)));
		popUpMenu->AddSeparatorItem();
		
		popUpMenu->AddItem(new BMenuItem("Statistics...", new BMessage(C_NET_PULSE_STATISTICS)));
		popUpMenu->AddSeparatorItem();
			
		ifconf_t ifc;
		
		if (get_interface_configuration(&ifc) == B_OK) {
			for (uint32 index = 0; index < ifc.ifc_len / sizeof(ifreq_t); index++) {
				char name[1024];
				strncpy(name, ifc.ifc_ifcu.ifcu_req[index].ifr_name, sizeof(name));
				if (strcmp(name, "loop0") != 0) {
					for (int32 i = 0; kNames[i][0] != NULL; i++) {
						if (strncmp(ifc.ifc_ifcu.ifcu_req[index].ifr_name, kNames[i][0], strlen(kNames[i][0])) == 0) {
							strncat(strncpy(name, kNames[i][1], sizeof(name)), ifc.ifc_ifcu.ifcu_req[index].ifr_name + strlen(kNames[i][0]), sizeof(name));
							break;
						}
					}
					BMenuItem *menuItem = new BMenuItem(name,
						new BMessage(C_NET_PULSE_CHANGE_INTERFACE + ifc.ifc_ifcu.ifcu_req[index].ifr_index ));
					menuItem->SetMarked(ifc.ifc_ifcu.ifcu_req[index].ifr_index == fIndex);
					popUpMenu->AddItem(menuItem);
				}
			}
			free_interface_configuration(&ifc);
		}
		
		popUpMenu->AddSeparatorItem();
		popUpMenu->AddItem(new BMenuItem("Quit", new BMessage(C_NET_PULSE_QUIT)));
		popUpMenu->SetTargetForItems(this);
	
		popUpMenu->FindItem(C_NET_PULSE_CONNECT)->SetEnabled(!fEnable && fIndex != 0);
		popUpMenu->FindItem(C_NET_PULSE_DISCONNECT)->SetEnabled(fEnable && fIndex != 0);
		popUpMenu->FindItem(C_NET_PULSE_STATISTICS)->SetEnabled(fEnable && fIndex != 0);
	
		popUpMenu->Go(ConvertToScreen(BPoint(0, 0)), true, false, false);
		delete popUpMenu;
	}
	else if (buttons == B_PRIMARY_MOUSE_BUTTON && clicks == 2) {
		be_roster->Launch("application/x-vnd.Be-BoneYard");
	}
}

void CNetPulseView::UpdateBitmap()
{
	PRINT(("CNetPulseView::UpdateBitmaps()\n"));

	if (fModemUpBitmap != NULL) {
		BScreen screen(Window());
		
		const uint8 inputColor = fInputColorTable[fMaxInputRate <= 0 ? 0 : int32((C_NET_PULSE_COLOR_LEVELS - 1) * fInputRate / fMaxInputRate)];
		const uint8 outputColor = fOutputColorTable[fMaxOutputRate <= 0 ? 0 : int32((C_NET_PULSE_COLOR_LEVELS - 1) * fOutputRate / fMaxOutputRate)];
		
		if (fBitmapInputColor != inputColor || fBitmapOutputColor != outputColor) {
			PRINT(("CNetPulseView::UpdateBitmaps() - Updating cached bitmap\n"));
	
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

void CNetPulseView::UpdateColorTable()
{
	PRINT(("CNetPulseView::UpdateColorTable()\n"));

	BScreen screen(Window());
	
	for (int32 index = 0; index < C_NET_PULSE_COLOR_LEVELS; index++) {
		const float scale = float(index) / (C_NET_PULSE_COLOR_LEVELS - 1);
		
		fInputColorTable[index] = screen.IndexForColor(uint8((1 - scale) * 208 + scale * fInputColor.red),
													   uint8((1 - scale) * 208 + scale * fInputColor.green),
													   uint8((1 - scale) * 208 + scale * fInputColor.blue));
		
		fOutputColorTable[index] = screen.IndexForColor(uint8((1 - scale) * 208 + scale * fOutputColor.red),
													    uint8((1 - scale) * 208 + scale * fOutputColor.green),
														uint8((1 - scale) * 208 + scale * fOutputColor.blue));
	}
}
