/*============================================================================
 * TitanInterfaceC.stub.c
 * Dummy functions to simulate a WONnet connection that never works.
 *
 * Author:  Ted Cipicchio <ted@thereisnospork.com>
 * Created: Sat Oct 4 2003
 *==========================================================================*/
#include "TitanInterfaceC.h"
#include "NetworkInterface.h"

#include "Debug.h"

/*----------------------------------------------------------------------------
 * Global Variables
 *--------------------------------------------------------------------------*/

wchar_t ChannelPassword[MAX_PASSWORD_LEN];
TPChannelList tpChannelList;
TPServerList tpServerList;
CaptainGameInfo tpGameCreated;
Address myAddress;
unsigned long DIRSERVER_PORTS[MAX_PORTS];
unsigned long PATCHSERVER_PORTS[MAX_PORTS];
ipString DIRSERVER_IPSTRINGS[MAX_IPS];
ipString PATCHSERVER_IPSTRINGS[MAX_IPS];
unsigned long HomeworldCRC[4];
wchar_t GameWereInterestedIn[MAX_TITAN_GAME_NAME_LEN];
void *GameWereInterestedInMutex = 0;


/*----------------------------------------------------------------------------
 * Functions
 *--------------------------------------------------------------------------*/

void HandleJoinReject(Uint32 address, const void* data, unsigned short len);



void titanGotNumUsersInRoomCB(const wchar_t *theRoomName, int theNumUsers)
{
	dbgMessagef("titanGotNumUsersInRoomCB");
}



static void dbgPrintIpAddress( char* format, unsigned long ip ) {
	dbgMessagef( format,
		(myAddress.AddrPart.IP >>  0) & 0xFF,
		(myAddress.AddrPart.IP >>  8) & 0xFF,
		(myAddress.AddrPart.IP >> 16) & 0xFF,
		(myAddress.AddrPart.IP >> 24) & 0xFF
	);
}


unsigned long titanStart(unsigned long isLan, unsigned long isIP)
{
	dbgMessagef("titanStart");
#ifdef HW_ENABLE_NETWORK
	initNetwork();
	myAddress.AddrPart.IP = getMyAddress();
	myAddress.Port = TCPPORT;
	dbgPrintIpAddress( "getMyAddress(): %d.%d.%d.%d", myAddress.AddrPart.IP );
	return 1; 
#else
	return 0;
#endif
}


unsigned long titanCheckCanNetwork(unsigned long isLan, unsigned long isIP)
{
	dbgMessagef("titanCheckCanNetwork");
	return isLan && isIP;
}


// --MikeN
// Call this method to begin shutdown of titan.  Parameters specify packet to send
// to connected client(s) (a shutdown message).  The callback titanNoClientsCB() will
// be invoked when complete.
void titanStartShutdown(unsigned long titanMsgType, const void* thePacket,
                        unsigned short theLen)
{
	dbgMessagef("titanStartShutdown");
	/* Probably won't ever be called, but we'll be consistent anyway... */
	titanNoClientsCB();
}


void titanLeaveGameNotify(void)
{
	dbgMessagef("titanLeaveGameNotify");
}


void titanShutdown(void)
{
#ifdef HW_ENABLE_NETWORK
	shutdownNetwork();
#endif
	dbgMessagef("titanShutdown");
}


void titanRefreshRequest(char* theDir)
{
	dbgMessagef("titanRefreshRequest");
}


unsigned long titanReadyToStartGame(unsigned char *routingaddress)
{
	dbgMessagef("titanReadyToStartGame");
	return 0; 
}


unsigned long titanBehindFirewall(void)
{
	dbgMessagef("titanBehindFirewall");
	return 0; 
}


void titanCreateGame(wchar_t *str, DirectoryCustomInfo* myInfo)
{
	dbgMessagef("titanCreateGame");

}


void titanRemoveGame(wchar_t *str)
{
	dbgMessagef("titanRemoveGame");

}


void titanCreateDirectory(char *str, char* desc)
{
	dbgMessagef("titanCreateDirectory");

}


void titanSendLanBroadcast(const void* thePacket, unsigned short theLen)
{
//	dbgMessagef("titanSendLanBroadcast");
#ifdef HW_ENABLE_NETWORK
	sendBroadcastPacket(thePacket, theLen);
#endif
//	titanReceivedLanBroadcastCB(thePacket, theLen);
}


void titanSendPacketTo(Address *address, unsigned char titanMsgType,
                       const void* thePacket, unsigned short theLen)
{
	dbgMessagef("titanSendPacketTo");
#ifdef HW_ENABLE_NETWORK
	if(!InternetAddressesAreEqual(*address,myAddress))
		putPacket(address->AddrPart.IP,titanMsgType,thePacket,theLen);
	else
		HandleTCPMessage(address->AddrPart.IP, titanMsgType, thePacket, theLen);
#endif

}


void titanBroadcastPacket(unsigned char titanMsgType, const void* thePacket, unsigned short theLen)
{
	dbgMessagef("titanBroadcastPacket");
#ifdef HW_ENABLE_NETWORK
    	int i;

	if(mGameCreationState==GAME_NOT_STARTED) {
		for (i=0;i<tpGameCreated.numPlayers;i++)
		{
			if (!InternetAddressesAreEqual(tpGameCreated.playerInfo[i].address,myAddress))
			{
				putPacket((tpGameCreated.playerInfo[i].address.AddrPart.IP), titanMsgType, thePacket, theLen);
			}
		}
	}
#endif

}


void titanAnyoneSendPacketTo(Address *address, unsigned char titanMsgType,
                       const void* thePacket, unsigned short theLen)
{
	dbgMessagef("titanAnyoneSendPacketTo");

}


void titanAnyoneBroadcastPacket(unsigned char titanMsgType, const void* thePacket, unsigned short theLen)
{
	dbgMessagef("titanAnyoneBroadcastPacket");

}


void titanConnectToClient(Address *address)
{
	dbgMessagef("titanConnectToClient");
#ifdef HW_ENABLE_NETWORK
	myAddress.AddrPart.IP = connectToServer(address->AddrPart.IP);
	dbgMessagef("myAddress Ip : %d",myAddress.AddrPart.IP);
#endif
}


int titanStartChatServer(wchar_t *password)
{
	dbgMessagef("titanStartChatServer");
	return 0;
}


void titanSendPing(Address *address,unsigned int pingsizebytes)
{
	dbgMessagef("titanSendPing");
}


void titanPumpEngine()
{
	dbgMessagef("titanPumpEngine");
}


void titanSetGameKey(unsigned char *key)
{
	dbgMessagef("titanSetGameKey");
}


const unsigned char *titanGetGameKey(void)
{
	dbgMessagef("titanGetGameKey");
	return 0; 
}


Address titanGetMyPingAddress(void)
{ 
	dbgMessagef("titanGetMyPingAddress");
	return myAddress; 
}


int titanGetPatch(char *filename,char *saveFileName)
{
	dbgMessagef("titanGetPatch");
	titanGetPatchFailedCB(PATCHFAIL_UNABLE_TO_CONNECT);
	return PATCHFAIL_UNABLE_TO_CONNECT;
}


void titanReplaceGameInfo(wchar_t *str, DirectoryCustomInfo* myInfo, unsigned long replaceTimeout)
{
	dbgMessagef("titanReplaceGameInfo");
}


void chatConnect(wchar_t *password)
{
	dbgMessagef("chatConnect");
}


void chatClose(void)
{
	dbgMessagef("chatClose");
}


void BroadcastChatMessage(unsigned short size, const void* chatData)
{
	dbgMessagef("BroadcastChatMessage");
}


void SendPrivateChatMessage(unsigned long* userIDList, unsigned short numUsersInList,
                            unsigned short size, const void* chatData)
{
	dbgMessagef("SendPrivateChatMessage");
}


void authAuthenticate(char *loginName, char *password)
{
	dbgMessagef("authAuthenticate");
}


void authCreateUser(char *loginName, char *password)
{
	dbgMessagef("authCreateUser");
}


void authChangePassword(char *loginName, char *oldpassword, char *newpassword)
{
	dbgMessagef("authChangePassword");
}


int titanSaveWonstuff()
{ 
	dbgMessagef("titanSaveWonstuff");
	return 0; 
}


void titanWaitShutdown(void)
{
	dbgMessagef("titanWaitShutdown");
}


void titanConnectingCancelHit(void)
{
	dbgMessagef("titanConnectingCancelHit");
}

#ifdef HW_ENABLE_NETWORK
void HandleTCPMessage(Uint32 address, unsigned char msgTyp, const void* data, unsigned short len)
{
	switch(msgTyp)
	{
		case TITANMSGTYPE_JOINGAMEREQUEST :
			HandleJoinGame(address,data,len);
			break;
		case TITANMSGTYPE_JOINGAMECONFIRM :
			HandleJoinConfirm(address,data,len);
			break;
		case TITANMSGTYPE_JOINGAMEREJECT :
			HandleJoinReject(address,data,len);
			break;
		case TITANMSGTYPE_UPDATEGAMEDATA :
			HandleGameData(data,len);
			break;
		case TITANMSGTYPE_LEAVEGAMEREQUEST :
			dbgMessagef("TITANMSGTYPE_LEAVEGAMEREQUEST HandleTCPMessage");
			break;
		case TITANMSGTYPE_GAMEISSTARTING :
			HandleGameStart(data,len);
			break;
		case TITANMSGTYPE_PING :
			dbgMessagef("TITANMSGTYPE_PING HandleTCPMessage");
			break;
		case TITANMSGTYPE_PINGREPLY :
			dbgMessagef("TITANMSGTYPE_PINGREPLY HandleTCPMessage");
			break;
		case TITANMSGTYPE_GAME :
			HandleGameMsg(data,len);
			break;
		case TITANMSGTYPE_GAMEDISOLVED :
			dbgMessagef("TITANMSGTYPE_GAMEDISOLVED HandleTCPMessage");
			break;
		case TITANMSGTYPE_UPDATEPLAYER :
			dbgMessagef("TITANMSGTYPE_UPDATEPLAYER HandleTCPMessage");
			break;
		case TITANMSGTYPE_BEGINSTARTGAME :
			dbgMessagef("TITANMSGTYPE_BEGINSTARTGAME HandleTCPMessage");
			break;
		case TITANMSGTYPE_CHANGEADDRESS :
			dbgMessagef("TITANMSGTYPE_CHANGEADDRESS HandleTCPMessage");
			break;
		case TITANMSGTYPE_REQUESTPACKETS :
			dbgMessagef("TITANMSGTYPE_REQUESTPACKETS HandleTCPMessage");
			break;
		case TITANMSGTYPE_RECONNECT :
			dbgMessagef("TITANMSGTYPE_RECONNECT HandleTCPMessage");
			break;
		case TITANMSGTYPE_KICKPLAYER :
			dbgMessagef("TITANMSGTYPE_KICKPLAYER HandleTCPMessage");
			break;
	}
}

void HandleJoinGame(Uint32 address, const void* data, unsigned short len)
{
	Address anAddress;
	anAddress.AddrPart.IP = address;
	anAddress.Port = TCPPORT;
	long requestResult;

	if(mGameCreationState==GAME_NOT_STARTED)
		 requestResult = titanRequestReceivedCB(&anAddress, data, len);
	else requestResult = REQUEST_RECV_CB_JUSTDENY;
	
	if (requestResult == REQUEST_RECV_CB_ACCEPT)
    {
        titanSendPacketTo(&anAddress, TITANMSGTYPE_JOINGAMECONFIRM, NULL, 0);
        titanBroadcastPacket(TITANMSGTYPE_UPDATEGAMEDATA, &tpGameCreated, sizeof(tpGameCreated));
    }
    else
    {
        titanSendPacketTo(&anAddress, TITANMSGTYPE_JOINGAMEREJECT, NULL, 0);
    }
}

void HandleJoinConfirm(Uint32 address, const void* data, unsigned short len)
{
	Address anAddress;
	anAddress.AddrPart.IP = address;
	anAddress.Port = TCPPORT;
    titanConfirmReceivedCB(&anAddress, data, len);
}

void HandleJoinReject(Uint32 address, const void* data, unsigned short len)
{
	Address anAddress;
	anAddress.AddrPart.IP = address;
	anAddress.Port = TCPPORT;
	titanRejectReceivedCB(&anAddress, data, len);
}

void HandleGameData(const void* data, unsigned short len)
{
	titanUpdateGameDataCB(data,len);
}

void HandleGameStart(const void* data, unsigned short len)
{
	mgGameStartReceivedCB(data,len);
}

void HandleGameMsg(const void* data, unsigned short len)
{
	titanGameMsgReceivedCB(data,len);
}

#endif
