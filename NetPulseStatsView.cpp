#include <stdio.h>
#include <string.h>
#include <Debug.h>
#include "NetPulseStatsView.h"

CNetPulseStatsView::CNetPulseStatsView(const char *name, if_index_t index)
	:	BView(BRect(0, 0, C_VIEW_WIDTH - 1, C_VIEW_HEIGHT - 1), name, B_FOLLOW_ALL_SIDES, B_PULSE_NEEDED),
		fConnectedToView(NULL),
		fElapsedTimeView(NULL),
		fLocalAddressView(NULL),
		fIBytesView(NULL),
		fOBytesView(NULL),
		fIPacketsView(NULL),
		fOPacketsView(NULL),
		fCollisionsView(NULL),
		fErrorsView(NULL),
		fIndex(index)
{
	PRINT(("CNetPulseStatsView::CNetPulseStatsView()\n"));

	memset(&fRequest, 0, sizeof(fRequest));
	
	BBox *status = new BBox(BRect(10, 10, 10+260, 10+60), "Status");
	status->SetLabel("Status");

	BBox *statistics = new BBox(BRect(10, 80, 10+260, 80+60), "Statistics");
	statistics->SetLabel("Statistics");
	
	BBox *errors = new BBox(BRect(10, 150, 10+260, 150+40), "Errors");
	errors->SetLabel("Errors");

	/* Interface */
	status->AddChild(new BStringView(BRect(10, 10, 10+70, 10+22), "InterfaceLabel", "Connected to:"));
	status->AddChild(new BStringView(BRect(10, 30, 10+130, 30+20), "LocalAddressLabel", "Local IP address:"));
	
	status->AddChild(fConnectedToView = new BStringView(BRect(10+70, 10, 10+180, 10+22), "Interface", "N/A"));
	status->AddChild(fElapsedTimeView = new BStringView(BRect(10+180, 10, 10+240, 10+22), "ElapsedTime", "N/A"));
	status->AddChild(fLocalAddressView = new BStringView(BRect(10+130, 30, 10+240, 30+20), "LocalAddress", "N/A"));
	
	fConnectedToView->SetAlignment(B_ALIGN_LEFT);
	fElapsedTimeView->SetAlignment(B_ALIGN_RIGHT);
	fLocalAddressView->SetAlignment(B_ALIGN_RIGHT);
	
	/* Bytes */
	statistics->AddChild(new BStringView(BRect(10, 10, 10+60, 10+22), "OBytesLabel", "IP bytes sent:"));
	statistics->AddChild(new BStringView(BRect(10+125, 10, 10+180, 10+22), "IBytesLabel", "Received:"));

	statistics->AddChild(fOBytesView = new BStringView(BRect(10+60, 10, 10+120, 10+22), "OBytes", "0"));
	statistics->AddChild(fIBytesView = new BStringView(BRect(10+180, 10, 10+240, 10+22), "IBytes", "0"));

	fOBytesView->SetAlignment(B_ALIGN_RIGHT);
	fIBytesView->SetAlignment(B_ALIGN_RIGHT);

	/* Packets */
	statistics->AddChild(new BStringView(BRect(10, 30, 10+60, 30+20), "OPacketsLabel", "Frames sent:"));
	statistics->AddChild(new BStringView(BRect(10+125, 30, 10+180, 30+20), "IPacketsLabel", "Received:"));

	statistics->AddChild(fOPacketsView = new BStringView(BRect(10+60, 30, 10+120, 30+20), "OPackets", "0"));
	statistics->AddChild(fIPacketsView = new BStringView(BRect(10+180, 30, 10+240, 30+20), "IPackets", "0"));

	fOPacketsView->SetAlignment(B_ALIGN_RIGHT);
	fIPacketsView->SetAlignment(B_ALIGN_RIGHT);

	/* Errors */
	errors->AddChild(new BStringView(BRect(10, 10, 10+60, 10+20), "CollisionsLabel", "Collisions:"));
	errors->AddChild(new BStringView(BRect(10+120, 10, 10+180, 10+20), "ErrorsLabel", "Errors:"));

	errors->AddChild(fCollisionsView = new BStringView(BRect(10+60, 10, 10+120, 10+20), "Collisions", "0"));
	errors->AddChild(fErrorsView = new BStringView(BRect(10+180, 10, 10+240, 10+20), "Errors", "0"));

	fCollisionsView->SetAlignment(B_ALIGN_RIGHT);
	fErrorsView->SetAlignment(B_ALIGN_RIGHT);
	
	AddChild(status);
	AddChild(statistics);
	AddChild(errors);
}

CNetPulseStatsView::~CNetPulseStatsView()
{
	PRINT(("CNetPulseStatsView::~CNetPulseStatsView()\n"));
}

void CNetPulseStatsView::AttachedToWindow()
{
	PRINT(("CNetPulseStatsView::AttachedToWindow()\n"));

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BView::AttachedToWindow();
}

void CNetPulseStatsView::MessageReceived(BMessage *message)
{
	PRINT(("CNetPulseStatsView::MessageReceived()\n"));

	if (message->what == C_CHANGE_INTERFACE) {
		if (message->FindInt32("interface_index", (int32 *) &fIndex) != B_OK)
			fIndex = 0;
	}
	else if (message->what == C_QUIT) {
		Window()->Quit();
	}
	else {
		BView::MessageReceived(message);
	}
}

void CNetPulseStatsView::Pulse()
{
	PRINT(("CNetPulseStatsView::Pulse()\n"));

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

	char buffer[1024];
	ifreq request;
	
	if (get_interface_by_index(fIndex, &request) != B_OK)
		return;

	// Time	
	if ((request.ifr_flags & IFF_UP) != 0) {
		bigtime_t elapsedTime = system_time() - request.ifr_lastchange;
		sprintf(buffer, "%02ld:%02ld:%02ld",
			int32(elapsedTime / (60 * 60 * 1000000LL)) % 24,
			int32(elapsedTime / (     60 * 1000000LL)) % 60,
			int32(elapsedTime /            1000000LL ) % 60);
	}
	else {
		sprintf(buffer, "N/A");
	}
	fElapsedTimeView->SetText(buffer);
	
	// Interface
	if (request.ifr_index != fRequest.ifr_index) {
		strncpy(buffer, request.ifr_name, sizeof(buffer));
		for (int32 index = 0; kNames[index][0] != NULL; index++) {
			if (strncmp(request.ifr_name, kNames[index][0], strlen(kNames[index][0])) == 0) {
				strncat(strncpy(buffer, kNames[index][1], sizeof(buffer)), request.ifr_name + strlen(kNames[index][0]), sizeof(buffer));
				break;
			}
		}
		fConnectedToView->SetText(buffer);
	}
	
	// IP Address
	if (request.ifr_addr.sa_data[2] != fRequest.ifr_addr.sa_data[2] ||
		request.ifr_addr.sa_data[3] != fRequest.ifr_addr.sa_data[3] ||
		request.ifr_addr.sa_data[4] != fRequest.ifr_addr.sa_data[4] ||
		request.ifr_addr.sa_data[5] != fRequest.ifr_addr.sa_data[5]) {
		sprintf(buffer, "%d.%d.%d.%d", request.ifr_addr.sa_data[2], request.ifr_addr.sa_data[3], request.ifr_addr.sa_data[4], request.ifr_addr.sa_data[5]);
		fLocalAddressView->SetText(buffer);
	}
	
	// Bytes
	if (request.ifr_ibytes != fRequest.ifr_ibytes) {
		sprintf(buffer, "%lu", request.ifr_ibytes);
		fIBytesView->SetText(buffer);
	}
	if (request.ifr_obytes != fRequest.ifr_obytes) {
		sprintf(buffer, "%lu", request.ifr_obytes);
		fOBytesView->SetText(buffer);
	}
	
	// Packets
	if (request.ifr_ipackets != fRequest.ifr_ipackets) {
		sprintf(buffer, "%lu", request.ifr_ipackets);
		fIPacketsView->SetText(buffer);
	}
	if (request.ifr_opackets != fRequest.ifr_opackets) {
		sprintf(buffer, "%lu", request.ifr_opackets);
		fOPacketsView->SetText(buffer);
	}
	
	// Errors
	if (request.ifr_collisions != fRequest.ifr_collisions) {
		sprintf(buffer, "%lu", request.ifr_collisions);
		fCollisionsView->SetText(buffer);
	}
	if (request.ifr_ierrors != fRequest.ifr_ierrors ||
	    request.ifr_oerrors != fRequest.ifr_oerrors) {
		sprintf(buffer, "%lu", request.ifr_ierrors + request.ifr_oerrors);
		fErrorsView->SetText(buffer);
	}
	
	memcpy(&fRequest, &request, sizeof(fRequest));
}
