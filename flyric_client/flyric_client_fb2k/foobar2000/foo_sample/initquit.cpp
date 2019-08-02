#include "stdafx.h"

// Sample initquit implementation. See also: initquit class documentation in relevant header.
#include <atlsocket.h>

class myinitquit : public initquit {
public:
	void on_init() {
		/* copy from https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup */

		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
		wVersionRequested = MAKEWORD(2, 2);

		err = WSAStartup(wVersionRequested, &wsaData);
		if (err != 0) {
			console::printf("[error]flyric_progress_init:WSAStartup failed with error: %d\n", err);
			return;
		}

		/* Confirm that the WinSock DLL supports 2.2.*/
		/* Note that if the DLL supports versions greater */
		/* than 2.2 in addition to 2.2, it will still return */
		/* 2.2 in wVersion since that is the version we */
		/* requested. */

		if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
			console::formatter() << "[warring]flyric_progress_init:WSAStartup version" << LOBYTE(wsaData.wVersion) << "." << HIBYTE(wsaData.wVersion);
		}
	}
	void on_quit() {
		WSACleanup();
	}
};

static initquit_factory_t<myinitquit> g_myinitquit_factory;
