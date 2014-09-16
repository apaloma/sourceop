/*
    This file is part of SourceOP.

    SourceOP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SourceOP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FUNCDETOURS_H
#define FUNCDETOURS_H

#include "CDetour.h"
#include "steam/isteamuser.h"
#include "steamext.h"

class IClient;
extern CDetour NET_SendPacketDetour;

int SOP_NET_ReceiveDatagramTrampoline(int a, void *structure);
void SOP_NET_SendPacketTrampoline(void *netchan, int a, const netadr_t &sock, unsigned char const *data, int length, bf_write *bitdata, bool b);
void SOP_BroadcastVoiceTrampoline(IClient *client, int a, char *b, long long c);
#ifdef __linux__
int SOP_usleepTrampoline(useconds_t usec);
#endif
void SOP_SleepTrampoline(DWORD msec);
bool SOP_SteamGameServer_InitSafeTrampoline( uint32 unIP, uint16 usPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString );
bool SOP_BGetCallbackTrampoline( HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg, HSteamCall *phSteamCall );
edict_t *SOP_UTIL_PlayerByIndexTrampoline(int client);

int SOP_recvfrom(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
int SOP_NET_ReceiveDatagram(int a, void *structure);
void SOP_NET_SendPacket( void *netchan, int a, const netadr_t &sock, unsigned char const *data, int length, bf_write *bitdata, bool b );
void SOP_BroadcastVoice( IClient *client, int a, char *b, long long c);
#ifdef __linux__
int SOP_usleep(useconds_t usec);
#endif
void __cdecl SOP_Sleep(DWORD msec);
bool SOP_SteamGameServer_InitSafe( uint32 unIP, uint16 usPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString );
bool SOP_BGetCallback( HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg, HSteamCall *phSteamCall );
edict_t *SOP_UTIL_PlayerByIndex(int client);

class GenericClass {};
typedef void (GenericClass::*VoidFunc)();

inline void *GetCodeAddr(VoidFunc mfp)
{
    return *(void **)&mfp;
}

/**
 * Converts a member function pointer to a void pointer.
 * This relies on the assumption that the code address lies at mfp+0
 * This is the case for both g++ and later MSVC versions on non virtual functions but may be different for other compilers
 * Based on research by Don Clugston : http://www.codeproject.com/cpp/FastDelegate.asp
 */
#define GetCodeAddress(mfp) GetCodeAddr(reinterpret_cast<VoidFunc>(mfp))

bool HookSetServerType(unsigned int unServerFlags, unsigned int unGameIP, unsigned short unGamePort, unsigned short unSpectatorPort, unsigned short usQueryPort, char const *pchGameDir, char const *pchVersion, bool bLANMode);

int HookClientGCSendMessage(int unAppID, unsigned int messageId, const void *data, unsigned int cbData);
int HookGCSendMessage(unsigned int messageId, const void *data, unsigned int cbData);
bool HookIsMessageAvailable(unsigned int *cbData);
int HookClientGCRetrieveMessage(int unAppID, unsigned int *messageId, void *data, unsigned int cbData, unsigned int *cbDataActual);
int HookGCRetrieveMessage(unsigned int *messageId, void *data, unsigned int cbData, unsigned int *cbDataActual);

#endif // FUNCDETOURS_H
