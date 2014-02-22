#include "NetPulse.h"

#include <Alert.h>
#include <Debug.h>
#include <Deskbar.h>
#include <Roster.h>
#include <Window.h>

#include "NetPulseView.h"


const char* kAppSignature = "application/x-vnd.pel.NetPulse";


NetPulse::NetPulse()
	:
	BApplication(kAppSignature)
{
}


NetPulse::~NetPulse()
{
}


void
NetPulse::ReadyToRun()
{
	BDeskbar deskbar;
	entry_ref entry;

	if (!deskbar.HasItem("NetPulseView")) {
		if (be_roster->FindApp(kAppSignature, &entry) != B_OK ||
			deskbar.AddItem(&entry) != B_OK) {
			BAlert *alert = new BAlert("Error",
				"Can't install NetPulse in the Deskbar!",
				"Damn", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
			alert->Go();
		}
	}

	PostMessage(B_QUIT_REQUESTED);
}


extern "C" _EXPORT BView *instantiate_deskbar_item()
{
	return new NetPulseView("NetPulseView");
}


int main()
{
	NetPulse application;
	application.Run();
	return 0;
}
