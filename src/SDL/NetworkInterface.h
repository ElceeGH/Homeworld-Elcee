#ifndef ___NETWORKINTERFACE_H
#define ___NETWORKINTERFACE_H



#ifdef HW_ENABLE_NETWORK
#include "Types.h"
#include "SDL_Net.h"

typedef struct IpElemTmp { IPaddress IP; struct IpElemTmp *nextIP; } IpElem;

typedef IpElem* IpList;

typedef struct { TCPsocket sock; IPaddress IP; } Client;

void initNetwork();
void shutdownNetwork();






// Function defined depending on the protocol used
void putPacket(Uint32,    unsigned char, const void*, unsigned short);
bool getPacket(TCPsocket, unsigned char*, Uint8**, unsigned short*);

Uint32 connectToServer(Uint32 serverIP );
Uint32 getMyAddress(void);
void sendBroadcastPacket(const void* packet, int len);

// Callback to the function that Handle messages.
void HandleTCPMessage (Uint32 address, unsigned char type, const void* data, unsigned short len);
void HandleJoinGame   (Uint32 address, const void* data, unsigned short len);
void HandleJoinConfirm(Uint32 address, const void* data, unsigned short len);
void HandleGameData   (                const void* data, unsigned short len);
void HandleGameStart  (                const void* data, unsigned short len);
void HandleGameMsg    (                const void* data, unsigned short len);


#define TCPPORT 10500
#define UDPPORT 10600

#endif
#endif
