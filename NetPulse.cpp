#include <Debug.h>
#include <Deskbar.h>
#include <Roster.h>
#include <Window.h>
#include <Alert.h>
#include "NetPulse.h"
#include "NetPulseView.h"

const char *C_NET_PULSE_APP_SIGNATURE = "application/x-vnd.pel.NetPulse";

CNetPulseApplication::CNetPulseApplication()
	:	BApplication(C_NET_PULSE_APP_SIGNATURE)
{
	PRINT(("CNetPulseApplication::CNetPulseApplication()\n"));
}

CNetPulseApplication::~CNetPulseApplication()
{
	PRINT(("CNetPulseApplication::~CNetPulseApplication()\n"));
}

void CNetPulseApplication::ReadyToRun()
{
	PRINT(("CNetPulseApplication::ReadyToRun()\n"));

#if DEBUG
	BWindow *window = new BWindow(BRect(200, 200, 220, 220), "NetPulse", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE);
	window->AddChild(new CNetPulseView("NetPulseView"));
	window->Show();
#else
	BAlert *alert = new BAlert("About", "NetPulse Release 0.2\nCopyright " B_UTF8_COPYRIGHT " 2001 Carlos Hasan",
		"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_IDEA_ALERT);
	alert->Go();

	BDeskbar deskbar;
	entry_ref entry;
	
	if (!deskbar.HasItem("NetPulseView")) {
		if (be_roster->FindApp(C_NET_PULSE_APP_SIGNATURE, &entry) != B_OK ||
			deskbar.AddItem(&entry) != B_OK) {
			BAlert *alert = new BAlert("Error",
				"Can't install NetPulse in the Deskbar!",
				"Damn", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
			alert->Go();
		}
	}

	PostMessage(B_QUIT_REQUESTED);
#endif
}

extern "C" _EXPORT BView *instantiate_deskbar_item()
{
	return new CNetPulseView("NetPulseView");
}

int main()
{
	CNetPulseApplication application;
	application.Run();
	return 0;
}
