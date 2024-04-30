#include "NetPulseStatsView.h"

#include <stdio.h>
#include <string.h>

#include <Box.h>
#include <Debug.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <NetworkRoster.h>
#include <StringView.h>
#include <Window.h>


NetPulseStatsView::NetPulseStatsView(const char* name, int32 index)
	:
	BView(name, B_PULSE_NEEDED),
	fCookie(index - 1),
	fConnectedToView(new BStringView("Interface", "N/A")),
	fElapsedTimeView(new BStringView("ElapsedTime", "N/A")),
	fLocalAddressView(new BStringView("LocalAddress", "N/A")),
	fOBytesView(new BStringView("OBytes", "0")),
	fIBytesView(new BStringView("IBytes", "0")),
	fOPacketsView(new BStringView("OPackets", "0")),
	fIPacketsView(new BStringView("IPackets", "0")),
	fCollisionsView(new BStringView("Collisions", "0")),
	fErrorsView(new BStringView("Errors", "0"))
{
	BStringView* connectedToLabel = new BStringView("ConnectedToLabel", "Connected to:");
	connectedToLabel->SetAlignment(B_ALIGN_RIGHT);

	BStringView* elapsedTimeLabel = new BStringView("ElappsedTimeLabel", "Elapsed time:");
	elapsedTimeLabel->SetAlignment(B_ALIGN_RIGHT);

	fConnectedToView->SetAlignment(B_ALIGN_LEFT);
	fElapsedTimeView->SetAlignment(B_ALIGN_RIGHT);

	BStringView* localAddressLabel = new BStringView("LocalAddressLabel", "IP address:");
	localAddressLabel->SetAlignment(B_ALIGN_RIGHT);

	fLocalAddressView->SetAlignment(B_ALIGN_RIGHT);

	BBox* status = new BBox(B_FANCY_BORDER, BLayoutBuilder::Grid<>(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(connectedToLabel, 0, 0)
			.AddGlue(1, 0)
			.Add(fConnectedToView, 2, 0)
			.Add(elapsedTimeLabel, 0, 1)
			.AddGlue(1, 1)
			.Add(fElapsedTimeView, 2, 1)
			.Add(localAddressLabel, 0, 2)
			.AddGlue(1, 2)
			.Add(fLocalAddressView, 2, 2)
			.SetInsets(B_USE_DEFAULT_SPACING)
		    .View());
            
	status->SetLabel("Status");

	BStringView* oBytesLabel = new BStringView("OBytesLabel", "Bytes sent:");
	oBytesLabel->SetAlignment(B_ALIGN_RIGHT);
    fOBytesView->SetAlignment(B_ALIGN_RIGHT);

	BStringView* iBytesLabel = new BStringView("IBytesLabel", "Bytes received:");
	iBytesLabel->SetAlignment(B_ALIGN_RIGHT);
    fIBytesView->SetAlignment(B_ALIGN_RIGHT);

	BStringView* oPacketsLabel = new BStringView("OPacketsLabel", "Packets sent:");
	oPacketsLabel->SetAlignment(B_ALIGN_RIGHT);
	fOPacketsView->SetAlignment(B_ALIGN_RIGHT);

	BStringView* iPacketsLabel = new BStringView("IPacketsLabel", "Packets received:");
	iPacketsLabel->SetAlignment(B_ALIGN_RIGHT);
	fIPacketsView->SetAlignment(B_ALIGN_RIGHT);

	BBox* sentReceived = new BBox(B_FANCY_BORDER, BLayoutBuilder::Grid<>(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(oBytesLabel, 0, 0)
			.Add(fOBytesView, 1, 0)
			.AddGlue(2, 0)
			.Add(oPacketsLabel, 3, 0)
			.Add(fOPacketsView, 4, 0)
			.Add(iBytesLabel, 0, 1)
			.Add(fIBytesView, 1, 1)
			.AddGlue(2, 1)
			.Add(iPacketsLabel, 3, 1)
			.Add(fIPacketsView, 4, 1)
			.SetInsets(B_USE_DEFAULT_SPACING)
		    .View());
            
	sentReceived->SetLabel("Sent/Received");

	BStringView* collisionsLabel = new BStringView("CollisionsLabel", "Collisions:");
	collisionsLabel->SetAlignment(B_ALIGN_RIGHT);
	fCollisionsView->SetAlignment(B_ALIGN_RIGHT);

	BStringView* errorsLabel = new BStringView("ErrorsLabel", "Errors:");
	errorsLabel->SetAlignment(B_ALIGN_RIGHT);
	fErrorsView->SetAlignment(B_ALIGN_RIGHT);

	BBox* errors = new BBox(B_FANCY_BORDER, BLayoutBuilder::Grid<>(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(collisionsLabel, 0, 0)
			.AddGlue(1, 0)
			.Add(fCollisionsView, 2, 0)
			.Add(errorsLabel, 3, 0)
			.AddGlue(4, 0)
			.Add(fErrorsView, 5, 0)
			.SetInsets(B_USE_DEFAULT_SPACING)
		    .View());
            
	errors->SetLabel("Errors");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(status)
		.Add(sentReceived)
		.Add(errors)
		.SetInsets(B_USE_WINDOW_INSETS)
		.End();
}


NetPulseStatsView::~NetPulseStatsView()
{
	delete fConnectedToView;
	delete fElapsedTimeView;
	delete fLocalAddressView;
	delete fOBytesView;
	delete fIBytesView;
	delete fOPacketsView;
	delete fIPacketsView;
	delete fCollisionsView;
	delete fErrorsView;
}


void
NetPulseStatsView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BView::AttachedToWindow();
}


void
NetPulseStatsView::MessageReceived(BMessage* message)
{
	if (message->what == kMsgChangeInterface) {
		if (message->FindInt32("interface_index", (int32*)&fCookie) != B_OK) {
			fCookie = 0;
        }
	} 
    else if (message->what == kMsgQuit) {
		Window()->Quit();
    }
	else
		BView::MessageReceived(message);
}


void
NetPulseStatsView::Pulse()
{
	char buffer[1024];
	BNetworkInterface interface;
	BNetworkRoster &roster = BNetworkRoster::Default();
	status_t result;
	uint32 cookie = fCookie;

	result = roster.GetNextInterface(&cookie, interface);
	
    if (result != B_OK) {
		fprintf(stderr, "error getting interface: %s\n", strerror(result));
		return;
	}	

	// time
	if ((interface.Flags() & IFF_UP) != 0) {
		bigtime_t elapsedTime = system_time() - interface.MTU();
		
        sprintf(buffer, "%02d:%02d:%02d",
			int32(elapsedTime / (60 * 60 * 1000000LL)) % 24,
			int32(elapsedTime / (     60 * 1000000LL)) % 60,
			int32(elapsedTime /            1000000LL ) % 60);
	} 
    else
		sprintf(buffer, "N/A");

	fElapsedTimeView->SetText(buffer);

	// interface
	if (interface.Index() != fInterface.Index()) {
		strncpy(buffer, interface.Name(), sizeof(buffer));
		fConnectedToView->SetText(buffer);
	}

	// IP address
	BNetworkInterfaceAddress address;
	
    result = interface.GetAddressAt(0, address);
	
    if (result != B_OK) {
		fprintf(stderr, "error getting IP address: %s\n", strerror(result));
    }
	else {
		fLocalAddressView->SetText(address.Address().ToString().String());
    }
    
	ifreq_stats stats;
	
    result = interface.GetStats(stats);
	
    if (result != B_OK) {
		fprintf(stderr, "error getting stats: %s\n", strerror(result));
    }
    
	ifreq_stats otherStats;
    
	status_t otherResult = fInterface.GetStats(otherStats);
	
    if (otherResult != B_OK) {
		fprintf(stderr, "error getting other stats: %s\n", strerror(otherResult));
	}

	if (result == B_OK && otherResult == B_OK) {
		// bytes
		if (stats.receive.bytes != otherStats.receive.bytes) {
			sprintf(buffer, "%lu", stats.receive.bytes);
			fIBytesView->SetText(buffer);
		}
		
        if (stats.send.bytes != otherStats.send.bytes) {
			sprintf(buffer, "%lu", stats.send.bytes);
			fOBytesView->SetText(buffer);
		}

		// packets
		if (stats.receive.packets != otherStats.receive.packets) {
			sprintf(buffer, "%d", stats.receive.packets);
			fIPacketsView->SetText(buffer);
		}
		
        if (stats.send.packets != otherStats.send.packets) {
			sprintf(buffer, "%d", stats.send.packets);
			fOPacketsView->SetText(buffer);
		}

		// errors
		if (stats.send.dropped != otherStats.send.dropped) {
			sprintf(buffer, "%d", stats.send.dropped);
			fCollisionsView->SetText(buffer);
		}
        
		if (stats.receive.errors != otherStats.receive.errors
			|| stats.send.errors != otherStats.send.errors) {
			sprintf(buffer, "%d", stats.receive.errors + stats.send.errors);
			fErrorsView->SetText(buffer);
		}
	}

	fInterface.SetTo(fCookie);
}
