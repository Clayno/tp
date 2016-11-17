/***********************************************************************
Site: https://tangentsoft.net/wskfaq/examples/basics/threaded-server.html

threaded-server.cpp - Implements a simple Winsock server that accepts
connections and spins each one off into its own thread, where it's
treated as a blocking socket.

Each connection thread reads data off the socket and echoes it
back verbatim.

Compiling:
VC++: cl -GX threaded-server.cpp main.cpp ws-util.cpp wsock32.lib
BC++: bcc32 threaded-server.cpp main.cpp ws-util.cpp

This program is hereby released into the public domain.  There is
ABSOLUTELY NO WARRANTY WHATSOEVER for this product.  Caveat hacker.
***********************************************************************/

#include "ws-util.h"

#include <winsock.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

typedef struct Candidat 
{
	char *nom;
	int nbVotes;
} Candidat;

//// Constants /////////////////////////////////////////////////////////

const int kBufferSize = 1024;
char *cand; // Contenu du fichier candidats.txt
int cand_length; // Taille en bytes du fichiers candidats.txt
int nbCand; // Nombre de candidats
Candidat *resultats; // Tableau des résultats du vote

//// Prototypes ////////////////////////////////////////////////////////

SOCKET SetUpListener(const char* pcAddress, int nPort);
void AcceptConnections(SOCKET ListeningSocket);
bool sendMessage(SOCKET sd);
bool getResult(SOCKET sd);
DWORD WINAPI voterHandler(void* sd_);


//// DoWinsock /////////////////////////////////////////////////////////
// The module's driver function -- we just call other functions and
// interpret their results.

int DoWinsock(const char* pcAddress, int nPort)
{
	// Lecture du fichier contenant les candidats
	ifstream fichier("candidats.txt");
	if (fichier)
	{
		// Obtiention du nombre de candidats
		string ligne;
		int size = 0;
		while (getline(fichier, ligne)) {
			size++;
		}
		nbCand = size;

		//  Récupération des bytes du fichiers candidats.txt
		fichier.clear();
		fichier.seekg(0, fichier.beg);
		fichier.seekg(0, fichier.end);
		int length = fichier.tellg();
		fichier.seekg(0, fichier.beg);
		char * buffer = new char[length];
		fichier.read(buffer, length);
		buffer[length] = '\0';
		cand = buffer;
		cand_length = length;

		// Remplissage du tableau résultats
		fichier.clear();
		resultats = new Candidat[size];
		fichier.seekg(0 ,fichier.beg);
		int i = 0;
		while (getline(fichier, ligne)) {
			char *ptr(0);
			ptr = strtok(&ligne[0], ":");
			resultats[i].nom = strdup(strtok(NULL, ":"));
			resultats[i].nbVotes = 0;
			i++;
		}
		fichier.close();
	}
	else
	{
		cout << "ERREUR: Impossible d'ouvrir le fichier en lecture." << endl;
		return 1;
	}

	
	// Lancement du listener
	cout << "Establishing the listener..." << endl;
	SOCKET ListeningSocket = SetUpListener(pcAddress, htons(nPort));
	if (ListeningSocket == INVALID_SOCKET) {
		cout << endl << WSAGetLastErrorMessage("establish listener") <<
			endl;
		delete[] cand;
		delete[] resultats;
		return 3;
	}
	cout << "Adresse ip : " << pcAddress << endl;
	cout << "Port d'ecoute : " << nPort << endl;
	cout << "Waiting for connections..." << flush << endl;
	while (1) {
		AcceptConnections(ListeningSocket);
		cout << "Acceptor restarting..." << endl;
	}
	
	delete[] cand;
	delete[] resultats;

#if defined(_MSC_VER)
	return 0;   // warning eater
#endif
}


//// SetUpListener /////////////////////////////////////////////////////
// Sets up a listener on the given interface and port, returning the
// listening socket if successful; if not, returns INVALID_SOCKET.

SOCKET SetUpListener(const char* pcAddress, int nPort)
{
	u_long nInterfaceAddr = inet_addr(pcAddress);
	if (nInterfaceAddr != INADDR_NONE) {
		SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);
		if (sd != INVALID_SOCKET) {
			sockaddr_in sinInterface;
			sinInterface.sin_family = AF_INET;
			sinInterface.sin_addr.s_addr = nInterfaceAddr;
			sinInterface.sin_port = nPort;
			if (bind(sd, (sockaddr*)&sinInterface,
				sizeof(sockaddr_in)) != SOCKET_ERROR) {
				listen(sd, SOMAXCONN);
				return sd;
			}
			else {
				cerr << WSAGetLastErrorMessage("bind() failed") <<
					endl;
			}
		}
	}

	return INVALID_SOCKET;
}


//// AcceptConnections /////////////////////////////////////////////////
// Spins forever waiting for connections.  For each one that comes in, 
// we create a thread to handle it and go back to waiting for
// connections.  If an error occurs, we return.

void AcceptConnections(SOCKET ListeningSocket)
{
	sockaddr_in sinRemote;
	int nAddrSize = sizeof(sinRemote);

	while (1) {
		SOCKET sd = accept(ListeningSocket, (sockaddr*)&sinRemote,
			&nAddrSize);
		if (sd != INVALID_SOCKET) {
			// R�cup�ration du temps local
			time_t now = time(0);
			struct tm *time = localtime(&now);
			// Ouverture du journal des connexions
			string const nomFichier("journal.txt");
			ofstream flux(nomFichier.c_str(), ios::app);
			if (!flux) {
				cout << "ERREUR: Impossible d'ouvrir le fichier." << endl;
			}
			else
			{
				// ecriture dans le journal
				flux << time->tm_hour << ":";
				flux << time->tm_min << ":";
				flux << time->tm_sec << "  ";
				flux << "connexion from " <<
					inet_ntoa(sinRemote.sin_addr) << ":" <<
					ntohs(sinRemote.sin_port) << "." <<
					endl;
			}
			// �criture dans le terminal du serveur
			cout << time->tm_hour << ":";
			cout << time->tm_min << ":";
			cout << time->tm_sec << "  ";
			cout << "Accepted connection from " <<
				inet_ntoa(sinRemote.sin_addr) << ":" <<
				ntohs(sinRemote.sin_port) << "." <<
				endl;
			// Lancement du thread
			DWORD nThreadID;
			CreateThread(0, 0, voterHandler, (void*)sd, 0, &nThreadID);
		}
		else {
			cerr << WSAGetLastErrorMessage("accept() failed") <<
				endl;
			return;
		}
	}
}


DWORD WINAPI voterHandler(void* sd_) {
	int nRetval = 0;
	SOCKET sd = (SOCKET)sd_;
	
	if (!sendMessage(sd)) {
		cerr << endl << WSAGetLastErrorMessage(
			"Probl�me lors de l'envoi de la liste des candidats") << endl;
		nRetval = 3;
	}

	if (!getResult(sd)) {
		cerr << endl << WSAGetLastErrorMessage(
			"Probl�me lors de la r�cup�ration de la r�ponse") << endl;
		nRetval = 3;
	}

	for (int i = 0; i < nbCand; i++) {
		cout << resultats[i].nom << " : " << resultats[i].nbVotes << endl;
	}

	cout << "Shutting connection down..." << flush;
	if (ShutdownConnection(sd)) {
		cout << "Connection is down." << endl;
	}
	else {
		cerr << endl << WSAGetLastErrorMessage(
			"Connection shutdown failed") << endl;
		nRetval = 3;
	}


	return nRetval;
}


bool sendMessage(SOCKET sd) {
		int nTemp = send(sd, cand, cand_length, 0);
		if (nTemp > 0) {
			cout << "Sent " << nTemp <<
				" bytes back to client." << endl;
		}
		else if (nTemp == SOCKET_ERROR) {
			return false;
		}
		else {
			// Client closed connection before we could reply to
			// all the data it sent, so bomb out early.
			cout << "Peer unexpectedly dropped connection!" <<
				endl;
		}
	return true;
}

bool getResult(SOCKET sd) {
	// Read data from client
	char acReadBuffer[32];
	int nReadBytes;
	nReadBytes = recv(sd, acReadBuffer, 32, 0);
	if (nReadBytes > 0) {
		cout << "Received " << nReadBytes <<
			" bytes from client." << endl;;
		printf("La reponse est : %s\n", acReadBuffer);
		int res = atoi(acReadBuffer);
		if (res < cand_length && res > 0) {
			resultats[res-1].nbVotes++;
		}
		else {
			return false;
		}
		return true;
	}
	else if (nReadBytes == SOCKET_ERROR) {
		return false;
	}
	return false;
}