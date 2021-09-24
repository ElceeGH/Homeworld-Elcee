

#include "NetworkInterface.h"
#include "TitanInterfaceC.h"
#include "Debug.h"
#include <string.h>
#include <stdio.h>
#include "SDL.h"

#ifdef HW_ENABLE_NETWORK
#include "SDL_net.h"
#endif



#ifdef HW_ENABLE_NETWORK


static int       broadcastThreadFunc(void*);
static int       TCPServerThreadFunc(void*);
static int		 pingSendThreadFunc(void*);
static void      addSockToList(TCPsocket);
static TCPsocket findSockInList(Uint32);
static void      removeSockFromList(int);
static int       checkList(IPaddress, IpList);
static IpList    addList  (IPaddress, IpList);
static bool      tryHandlePacket(TCPsocket);
static int	     tcpSendChecked(TCPsocket, const void*, int);



static SDL_Thread* listenBroadcastThread;
static SDL_Thread* listenTCPThread;
static UDPsocket   broadcastSendSock;
static UDPsocket   broadcastRecvSock;
static TCPsocket   clientSock;
static bool		   endNetwork;
static bool		   clientActive;
static bool        clientInSet;
static Client*     TCPClientsConnected;
static sdword	   numTCPClientConnected;
static SDL_sem*    semList;



// These used to be exit() calls. Let's log something more meaningful.
#define netAssert(expression, message)                      \
	if ( ! (expression)) {			     					\
		dbgFatalf(DBG_Loc, "Network error: %s", message); 	\
	}										     			\



void initNetwork(void)
{
	// Init vars
	endNetwork			  = FALSE;
	clientActive		  = FALSE;
	numTCPClientConnected = 0;
	semList				  = SDL_CreateSemaphore(1);

	const int netInitRes = SDLNet_Init();
	netAssert(netInitRes != -1, "SDLnet init failed.");

	// Initialisation of Broadcast Sockets
	// Open Udp Port to send Broadcast Packet
	broadcastRecvSock = SDLNet_UDP_Open(UDPPORT);
	netAssert(broadcastRecvSock, "Failed to open UDP receive socket." );

	// open udp client socket
	broadcastSendSock = SDLNet_UDP_Open(0);
	netAssert(broadcastSendSock, "Failed to open UDP send socket." );

	// Create Thread to listen to Broadcast Packet on UDP
	IPaddress broadcastIp = { .host = INADDR_BROADCAST, .port = UDPPORT };
	listenBroadcastThread = SDL_CreateThread(broadcastThreadFunc, "broadcastThread", NULL);
	netAssert(listenBroadcastThread, "Failed to create broadcast start thread." );

    const int resolveRes = SDLNet_ResolveHost(&broadcastIp, "255.255.255.255", UDPPORT);
	netAssert(resolveRes != -1, "Failed to resolve host IP address." );

	// bind server address to channel 0
	const int udpBindRes = SDLNet_UDP_Bind(broadcastSendSock, 0, &broadcastIp);
	netAssert(udpBindRes != -1, "Could not bind UDP socket.");

	// Initialisation of TCP Server Thread
	listenTCPThread = SDL_CreateThread(TCPServerThreadFunc, "tcpServerThread", NULL);
	netAssert(listenTCPThread, "Failed to create TCP server thread.");
}



void sendBroadcastPacket(const void* packet, int len)
{
	UDPpacket *out = SDLNet_AllocPacket(2048);
	memcpy(out->data, packet, len);
	out->len = len;

	int sendRes = SDLNet_UDP_Send(broadcastSendSock, 0, out);
	netAssert( sendRes, "Failed to send UDP broadcast packet." );

	SDLNet_FreePacket(out);
}



void shutdownNetwork(void)
{
	if (endNetwork == TRUE)
		return;

	endNetwork = TRUE;
	SDLNet_UDP_Close(broadcastSendSock);
	SDL_WaitThread(listenBroadcastThread, NULL);
	SDL_WaitThread(listenTCPThread,       NULL);

	broadcastSendSock     = NULL;
	listenBroadcastThread = NULL;
	listenTCPThread       = NULL;

	SDLNet_Quit();
}



int broadcastThreadFunc(void *threadparam)
{
	UDPpacket *packet  = SDLNet_AllocPacket(2048);
	IpList     listIps = NULL;

	while ( ! endNetwork)
	{
		if (SDLNet_UDP_Recv(broadcastRecvSock, packet) <= 0)
		{
			SDL_Delay(100);
			continue;
		}

		Uint32 ipaddr = SDL_SwapBE32(packet->address.host);
		IPaddress newIp;
		SDLNet_ResolveHost(&newIp, "255.255.255.255", UDPPORT);
		newIp.host = packet->address.host;

		if (checkList(newIp, listIps) == -1)
		{
			listIps = addList(newIp, listIps);
			SDLNet_UDP_Bind(broadcastSendSock, 0, &newIp);
		}

		titanReceivedLanBroadcastCB(packet->data, packet->len);
	}
	
	SDLNet_UDP_Close(broadcastRecvSock);
	broadcastRecvSock = NULL;
	SDLNet_FreePacket( packet );
	
	return 0;
}



static int checkList(IPaddress Ip, IpList list)
{
	while (list != NULL) {
		if (list->IP.host == Ip.host) {
			return 0;
		} else {
			list = list->nextIP;
		}
	}

	return -1;
}



static IpList addList(IPaddress newIp, IpList list)
{
	IpElem* new = malloc(sizeof(IpElem));
	new->IP     = newIp;
	new->nextIP = list;
	list        = new;
	return list;
}



// TODO : Sometimes bad packet is received, which will make the connection hang
Uint32 getMyAddress(void)
{
	UDPsocket recvSock = SDLNet_UDP_Open(45268);
	netAssert(recvSock, "Failed to open UDP socket." );

    SDL_Thread* pingRecv = SDL_CreateThread(pingSendThreadFunc,"pingsendthread",NULL);
	UDPpacket*  packet   = SDLNet_AllocPacket(512);
	while (SDLNet_UDP_Recv(recvSock, packet) <= 0) {};

	Uint32 ipAdd = packet->address.host;
    SDLNet_FreePacket(packet);

    SDL_WaitThread(pingRecv,NULL);
    SDLNet_UDP_Close(recvSock);
	return ipAdd;
}



static int pingSendThreadFunc(void *data)
{
	IPaddress pingIp = { .port = 45268 };
	const int resolveRes = SDLNet_ResolveHost(&pingIp, NULL, pingIp.port);
	netAssert(resolveRes != -1, "Failed to resolve host.");
	
	UDPsocket sendSock = SDLNet_UDP_Open(0);
	netAssert(sendSock, "Failed to open UDP socket.");
	
	pingIp.host = INADDR_BROADCAST;
	const int bindRes = SDLNet_UDP_Bind(sendSock, 1, &pingIp);
	netAssert(bindRes, "Failed to bind IP address for ping request.");
	
	SDL_Delay(100);
	
	UDPpacket *out = SDLNet_AllocPacket(512);
	out->len = 10;
	char message[] = "ping";
	memcpy(out->data, message, sizeof(message));
	
	const int sendRes = SDLNet_UDP_Send(sendSock, 1, out);
	netAssert(sendRes, "no destination send\n");

	SDLNet_FreePacket(out);
	SDLNet_UDP_Close(sendSock);
	return 0;
}



Uint32 connectToServer(Uint32 serverIP)
{
	IPaddress ipToConnect;
	const int resolveRes = SDLNet_ResolveHost(&ipToConnect,NULL,TCPPORT);
	netAssert( resolveRes != -1, "TCP host resolve failed." );

	dbgMessagef("addresse recu en paramètre %d\n",serverIP);
	ipToConnect.host = serverIP;

	clientSock = SDLNet_TCP_Open(&ipToConnect);
	netAssert( clientSock, "Can't create TCP socket to connect to server" );

	SDLNet_SocketSet set = SDLNet_AllocSocketSet(1);
	SDLNet_TCP_AddSocket(set, clientSock);
	int numrdy = SDLNet_CheckSockets(set, (Uint32)-1);

	Uint32 ipViewed = 0;
	if (SDLNet_SocketReady(clientSock)) {
		int result = SDLNet_TCP_Recv(clientSock, &ipViewed, sizeof(Uint32));
		if (result<sizeof(Uint32)) {
			dbgMessagef("SDLNet_TCP_Recv: %s\n", SDLNet_GetError());
			return 0;
		}
	} else {
		dbgMessagef("Error during connection");
	}


	// Switching to Client Mode.
	clientActive = TRUE;
	dbgMessagef("Client connected");
	
	return ipViewed;
}



static bool tryHandlePacket( TCPsocket socket ) {
	unsigned char  typMsg    = 0;
	unsigned short lenPacket = 0;
	Uint8*         packet    = NULL;
	bool           result    = getPacket(socket, &typMsg, &packet, &lenPacket);

	if (result) {
		IPaddress* fromIp = SDLNet_TCP_GetPeerAddress(socket);
		HandleTCPMessage(fromIp->host, typMsg, packet, lenPacket);
		free( packet );
	}

	return result;
}



static void TCPServerProcessServer( SDLNet_SocketSet setSock, TCPsocket serverSock ) {
	const int socketsReady   = SDLNet_CheckSockets(setSock, 500u);
	int       socketsHandled = 0;

	if (socketsReady == -1) {
		SDL_Delay( 100 );
		return;
	}

	if (socketsReady == 0)
		return;

	if (SDLNet_SocketReady(serverSock)) {
		socketsHandled++;
		TCPsocket sock = SDLNet_TCP_Accept(serverSock);

		if (sock) {
			SDLNet_TCP_AddSocket(setSock, sock);
			addSockToList(sock);

			IPaddress* fromIp = SDLNet_TCP_GetPeerAddress(sock);
			tcpSendChecked( sock, &fromIp->host, sizeof(fromIp->host) );
		}
	}

	for (int i=0; i<numTCPClientConnected; i++) {
		if (socketsHandled >= socketsReady)
			break;

		TCPsocket clientSock = TCPClientsConnected[i].sock;
		if ( ! SDLNet_SocketReady(clientSock))
			continue;
			
		if (tryHandlePacket( clientSock ))
			 socketsHandled++;
		else removeSockFromList( i );
	}
}



static void TCPServerProcessClient( SDLNet_SocketSet clientSet, TCPsocket clientSock ) {
	if ( ! clientInSet) {
		SDLNet_TCP_AddSocket(clientSet, clientSock);
		clientInSet = TRUE;
	}

	SDLNet_CheckSockets(clientSet, 500u);
			
	// Code as a client
	if (SDLNet_SocketReady(clientSock))
	if ( ! tryHandlePacket(clientSock)) {
		SDLNet_TCP_Close(clientSock);
		clientInSet  = FALSE;
		clientActive = FALSE;
	}
}



int TCPServerThreadFunc(void *threadparam)
{
	// Resolve the argument into an IPaddress type
	IPaddress ip;
	const int resolveRes = SDLNet_ResolveHost(&ip,NULL,TCPPORT);
	netAssert(resolveRes != -1, "Host IP resolve failed.");

	TCPsocket serverTCPSock = SDLNet_TCP_Open(&ip);
	netAssert(serverTCPSock, "Can't create Server TCP socket");

	SDLNet_SocketSet clientSet = SDLNet_AllocSocketSet(1);
	SDLNet_SocketSet setSock   = SDLNet_AllocSocketSet(100);
	SDLNet_TCP_AddSocket(setSock, serverTCPSock);

	clientInSet = FALSE;

	while ( ! endNetwork) {
		if ( ! clientActive)
			 TCPServerProcessServer(setSock,   serverTCPSock);
		else TCPServerProcessClient(clientSet, clientSock   );
	}

	for (int i=0; i<numTCPClientConnected; i++)
		removeSockFromList(i);

	SDLNet_TCP_Close(clientSock);
	SDLNet_TCP_Close(serverTCPSock);

	return 0;
}



static void addSockToList(TCPsocket sock)
{
	IPaddress *remoteIp = SDLNet_TCP_GetPeerAddress(sock);
	netAssert( remoteIp, "Server socket used instead of client." );
	
	SDL_SemWait(semList);
	TCPClientsConnected = realloc(TCPClientsConnected, (numTCPClientConnected+1)*sizeof(Client));
	TCPClientsConnected[numTCPClientConnected].sock = sock;
	TCPClientsConnected[numTCPClientConnected].IP = *remoteIp;
	numTCPClientConnected++;
	SDL_SemPost(semList);
}



static void removeSockFromList(int num)
{
	SDL_SemWait(semList);
	SDLNet_TCP_Close(TCPClientsConnected[num].sock);
	numTCPClientConnected--;

	if (num < numTCPClientConnected)
		memmove(&TCPClientsConnected[num], &TCPClientsConnected[num+1], (numTCPClientConnected-num)*sizeof(Client));

	TCPClientsConnected = realloc (TCPClientsConnected, numTCPClientConnected*sizeof(Client));
	SDL_SemPost(semList);
}



static TCPsocket findSockInList(Uint32 addressSock) {
	for (int i=0; i < numTCPClientConnected; i++)
		if (addressSock == TCPClientsConnected[i].IP.host)
			return TCPClientsConnected[i].sock;
	return NULL;
}



static int tcpSendChecked( TCPsocket sock, const void* data, int len ) {
	const int result = SDLNet_TCP_Send( sock, data, len );

	if (result < len) {
		dbgMessagef("SDLNet_TCP_Send: %s", SDLNet_GetError());
	}

	return result;
}



// Implementation of putPacket and getPacket which are Homeworld specific
// In homeworld with each message, a message type is send
// We first send the message type, then the message len, and finally the message data
void putPacket(Uint32 address, unsigned char msgType, const void* data, unsigned short dataLen)
{
	const bool asClient = clientActive;

	TCPsocket socket = NULL;
	if (asClient) {
		socket = clientSock;
	} else {
		socket = findSockInList(address);
		SDL_SemWait(semList);
	}

	if (socket) {
		tcpSendChecked( socket, &msgType, sizeof(msgType) );
		tcpSendChecked( socket, &dataLen, sizeof(dataLen) );
		tcpSendChecked( socket, data,     dataLen         );
	}

	if ( ! asClient) {
		SDL_SemPost(semList);
	}
}



/// Attempt to receive a packet.
/// Returns whether it succeeded.
/// If it succeeds: type, data and length are written. You take ownership of the packet data buffer.
/// If it fails: type, data and length are not written and no buffer is allocated.
static bool getPacket(TCPsocket sock, unsigned char* outMsgType, Uint8** outPacketData, unsigned short* outPacketLen)
{
	if ( ! sock)
		return FALSE;

	unsigned char  type = 0;
	unsigned short len  = 0;
	Uint8*		   data = NULL;

	const int typeRes = SDLNet_TCP_Recv(sock, &type, sizeof(type));
	if (typeRes < sizeof(type)) 
		return FALSE;

	const int lenRes = SDLNet_TCP_Recv(sock, &len, sizeof(len));
	if (lenRes < sizeof(len))
		return FALSE;

	if (len > 0) {
		data = malloc(len);
		const int dataRes = SDLNet_TCP_Recv(sock, data, len);

		if (dataRes < len) {
			free( data );
			return FALSE;
		}
	}

	*outMsgType    = type;
	*outPacketLen  = len;
	*outPacketData = data;

	return TRUE;	
}



#endif
