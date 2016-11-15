/***********************************************************************
main.cpp - The main() routine for all the "Basic Winsock" suite of
programs from the Winsock Programmer's FAQ.  This function parses
the command line, starts up Winsock, and calls an external function
called DoWinsock to do the actual work.

This program is hereby released into the public domain.  There is
ABSOLUTELY NO WARRANTY WHATSOEVER for this product.  Caveat hacker.
***********************************************************************/

#include <winsock.h>

#include <stdlib.h>
#include <iostream>

#pragma comment( lib, "ws2_32.lib" )

using namespace std;


//// Prototypes ////////////////////////////////////////////////////////

extern int DoWinsock(const char* pcHost, int nPort);


//// Constants /////////////////////////////////////////////////////////

// Default port to connect to on the server
const int kDefaultServerPort = 5000;


//// main //////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	// On choisi un d'oeuvrer en local pour le moment sur le port 5000
	const char* pcHost = "127.0.0.1";
	int nPort = kDefaultServerPort;

	// Start Winsock up
	WSAData wsaData;
	int nCode;
	if ((nCode = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		cerr << "WSAStartup() returned error code " << nCode << "." <<
			endl;
		return 255;
	}

	// Call the main example routine.
	int retval = DoWinsock(pcHost, nPort);

	// Shut Winsock back down and take off.
	WSACleanup();
	return retval;
}

