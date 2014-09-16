#ifndef STEAMEXT_H
#define STEAMEXT_H

typedef uint32 AppId_t;
typedef uint64 SteamAPICall_t;

class ISteamNetworking;
class ISteamRemoteStorage;

// client has been denied to connection to this game server
struct GSClientDeny_t
{
    enum { k_iCallback = k_iSteamGameServerCallbacks + 2 };

    CSteamID m_SteamID;

    EDenyReason m_eDenyReason;
    char m_pchOptionalText[ 128 ];
};

struct GSGameplayStats_t 
{ 
    enum { k_iCallback = k_iSteamGameServerCallbacks + 7 }; 
    EResult m_eResult; 
    int32 m_nRank; 
    uint32 m_unTotalConnects; 
    uint32 m_unTotalMinutesPlayed; 
};

typedef enum EServerMode
{
    eServerModeInvalid = 0,                 // DO NOT USE
    eServerModeNoAuthentication = 1,        // Don't authenticate user logins and don't list on the server list
    eServerModeAuthentication = 2,          // Authenticate users, list on the server list, don't run VAC on clients that connect
    eServerModeAuthenticationAndSecure = 3, // Authenticate users, list on the server list and VAC protect clients
} EServerMode;

typedef enum
{
    k_EUserHasLicenseResultHasLicense = 0,                  // User has a license for specified app
    k_EUserHasLicenseResultDoesNotHaveLicense = 1,          // User does not have a license for the specified app
    k_EUserHasLicenseResultNoAuth = 2,                      // User has not been authenticated
} EUserHasLicenseForAppResult;

// game server flags
const uint32 k_unServerFlagNone         = 0x00;
const uint32 k_unServerFlagActive       = 0x01;
const uint32 k_unServerFlagSecure       = 0x02;
const uint32 k_unServerFlagDedicated    = 0x04;
const uint32 k_unServerFlagLinux        = 0x08;
const uint32 k_unServerFlagPassworded   = 0x10;
const uint32 k_unServerFlagPrivate      = 0x20;     // server shouldn't list on master server and
                                                    // won't enforce authentication of users that connect to the server.
                                                    // Useful when you run a server where the clients may not
                                                    // be connected to the internet but you want them to play (i.e LANs)


#pragma pack(push, 1)
struct GC_ItemsList
{
    // k_ESOMsg_CacheSubscribed 
    enum { k_iMessage = 24 };

    uint16 version; // 0x01
    char unknownbytes[16]; // FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
    CSteamID steamid;
    uint32 unknown; // 0x10001
    uint16 padding;
    uint16 itemcount;
};

struct GC_ItemsList_Item
{
    uint64 itemid;
    uint32 accountid;
    uint16 itemdefindex;
    uint8 itemlevel;
    uint8 itemquality;
    uint32 position;
    uint32 quantity;
    uint16 attrbutecount;
};

const int k_iSteamGameCoordinatorCallbacks = 1700;
struct GCMessageAvailable_t
{
    enum { k_iCallback = k_iSteamGameCoordinatorCallbacks + 1 };
    uint32 messageLength;
};

class ISteamGameCoordinator001
{
public:
        virtual int SendMessage(unsigned int messageId, const void *data, unsigned int cbData) = 0;

        virtual bool IsMessageAvailable(unsigned int *cbData) = 0;

        virtual int RetrieveMessage(unsigned int *messageId, void *data, unsigned int cbData, unsigned int *cbDataActual) = 0;
};

class IClientGameCoordinator
{
public:
    virtual int SendMessage( int unAppID, uint32 unMsgType, const void *pubData, uint32 cubData ) = 0;

    virtual bool IsMessageAvailable( int unAppID, uint32 *pcubMsgSize ) = 0;

    virtual int RetrieveMessage( int unAppID, uint32 *punMsgType, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize ) = 0;
};

#define STEAMGAMECOORDINATOR_INTERFACE_VERSION_001 "SteamGameCoordinator001"
#define CLIENTGAMECOORDINATOR_INTERFACE_VERSION "CLIENTGAMECOORDINATOR_INTERFACE_VERSION001"

struct GC_ItemsList_Item_Attrib
{
    uint16 attribindex;
    float value;
};
#pragma pack(pop)

extern ISteamGameCoordinator001* SteamGameCoordinator();


class IClientApps;
class IClientBilling;
class IClientContentServer;
class IClientFriends;
class IClientGameCoordinator;
class IClientGameServer;
class IClientGameServerItems;
class IClientGameStats;
class IClientMasterServerUpdater;
class IClientMatchmaking;
class IClientMatchmakingServers;
class IClientNetworking;
class IClientRemoteStorage;
class IClientUser;
class IClientUserItems;
class IClientUserStats;
class IClientGameServerStats;
class IClientUtils;
class IP2PController;
class IClientAppManager;
class IClientConfigStore;
class IClientDepotBuilder;
class IConCommandBaseAccessor;
class IClientGameCoordinator;
class IClientHTTP;

class ISteamGameServer008 {
public:
    virtual void LogOn();
    virtual void LogOff();
    virtual bool BLoggedOn();
    virtual bool BSecure();
    virtual CSteamID GetSteamID();
    virtual bool SendUserConnectAndAuthenticate(unsigned int unIPClient, void const *pvToken, unsigned int cbTokenSize, CSteamID *pSteamID);
    virtual CSteamID CreateUnauthenticatedUserConnection();
    virtual void SendUserDisconnect(CSteamID);
    virtual bool BUpdateUserData(CSteamID, char const*, unsigned int);
    virtual bool BSetServerType(unsigned int unServerFlags, unsigned int unGameIP, unsigned short unGamePort, unsigned short unSpectatorPort, unsigned short usQueryPort, char const *pchGameDir, char const *pchVersion, bool bLANMode);
    virtual void UpdateServerStatus(int cPlayers, int cPlayersMax, int cBotPlayers, char const *pchServerName, char const *pSpectatorServerName, char const *pchMapName);
    virtual void UpdateSpectatorPort(unsigned short);
    // sv_tags
    virtual void SetGameType(char const *pchGameType);
    virtual bool BGetUserAchievementStatus(CSteamID, char const*);
    virtual void GetGameplayStats();
    virtual bool RequestUserGroupStatus(CSteamID, CSteamID);
    virtual uint32 GetPublicIP();
    virtual uint32 GetAuthSessionTicket(void*, int, unsigned int*);
    virtual uint32 BeginAuthSession(void const*, int, CSteamID);
    virtual void EndAuthSession(CSteamID);
    virtual void CancelAuthTicket(unsigned int);
};

//-----------------------------------------------------------------------------
// Purpose: Functions for authenticating users via Steam to play on a game server
//-----------------------------------------------------------------------------
class ISteamGameServer010
{
public:

    // connection functions
    virtual void LogOn() = 0;
    virtual void LogOff() = 0;

    // status functions
    virtual bool LoggedOn() = 0;
    virtual bool Secure() = 0; 
    virtual CSteamID GetSteamID() = 0;

    // Handles receiving a new connection from a Steam user.  This call will ask the Steam
    // servers to validate the users identity, app ownership, and VAC status.  If the Steam servers 
    // are off-line, then it will validate the cached ticket itself which will validate app ownership 
    // and identity.  The AuthBlob here should be acquired on the game client using SteamUser()->InitiateGameConnection()
    // and must then be sent up to the game server for authentication.
    //
    // Return Value: returns true if the users ticket passes basic checks. pSteamIDUser will contain the Steam ID of this user. pSteamIDUser must NOT be NULL
    // If the call succeeds then you should expect a GSClientApprove_t or GSClientDeny_t callback which will tell you whether authentication
    // for the user has succeeded or failed (the steamid in the callback will match the one returned by this call)
    virtual bool SendUserConnectAndAuthenticate( uint32 unIPClient, const void *pvAuthBlob, uint32 cubAuthBlobSize, CSteamID *pSteamIDUser ) = 0;

    // Creates a fake user (ie, a bot) which will be listed as playing on the server, but skips validation.  
    // 
    // Return Value: Returns a SteamID for the user to be tracked with, you should call HandleUserDisconnect()
    // when this user leaves the server just like you would for a real user.
    virtual CSteamID CreateUnauthenticatedUserConnection() = 0;

    // Should be called whenever a user leaves our game server, this lets Steam internally
    // track which users are currently on which servers for the purposes of preventing a single
    // account being logged into multiple servers, showing who is currently on a server, etc.
    virtual void SendUserDisconnect( CSteamID steamIDUser ) = 0;


    // Update the data to be displayed in the server browser and matchmaking interfaces for a user
    // currently connected to the server.  For regular users you must call this after you receive a
    // GSUserValidationSuccess callback.
    // 
    // Return Value: true if successful, false if failure (ie, steamIDUser wasn't for an active player)
    virtual bool UpdateUserData( CSteamID steamIDUser, const char *pchPlayerName, uint32 uScore ) = 0;

    // You shouldn't need to call this as it is called internally by SteamGameServer_Init() and can only be called once.
    //
    // To update the data in this call which may change during the servers lifetime see UpdateServerStatus() below.
    //
    // Input:   nGameAppID - The Steam assigned AppID for the game
    //          unServerFlags - Any applicable combination of flags (see k_unServerFlag____ constants below)
    //          unGameIP - The IP Address the server is listening for client connections on (might be INADDR_ANY)
    //          unGamePort - The port which the server is listening for client connections on
    //          unSpectatorPort - the port on which spectators can join to observe the server, 0 if spectating is not supported
    //          usQueryPort - The port which the ISteamMasterServerUpdater API should use in order to listen for matchmaking requests
    //          pchGameDir - A unique string identifier for your game
    //          pchVersion - The current version of the server as a string like 1.0.0.0
    //          bLanMode - Is this a LAN only server?
    //          
    // bugbug jmccaskey - figure out how to remove this from the API and only expose via SteamGameServer_Init... or make this actually used,
    // and stop calling it in SteamGameServer_Init()?
    virtual bool SetServerType( uint32 unServerFlags, uint32 unGameIP, uint16 unGamePort, uint16 unSpectatorPort, uint16 usQueryPort, const char *pchGameDir, const char *pchVersion, bool bLANMode ) = 0;

    // Updates server status values which shows up in the server browser and matchmaking APIs
    virtual void UpdateServerStatus( int cPlayers, int cPlayersMax, int cBotPlayers, const char *pchServerName, const char *pSpectatorServerName, const char *pchMapName ) = 0;

    // This can be called if spectator goes away or comes back (passing 0 means there is no spectator server now).
    virtual void UpdateSpectatorPort( uint16 unSpectatorPort ) = 0;

    // Sets a string defining the "gametags" for this server, this is optional, but if it is set
    // it allows users to filter in the matchmaking/server-browser interfaces based on the value
    virtual void SetGameTags( const char *pchGameTags ) = 0; 

    // Ask for the gameplay stats for the server. Results returned in a callback
    virtual void GetGameplayStats( ) = 0;

    // Gets the reputation score for the game server. This API also checks if the server or some
    // other server on the same IP is banned from the Steam master servers.
    virtual SteamAPICall_t GetServerReputation( ) = 0;

    // Ask if a user in in the specified group, results returns async by GSUserGroupStatus_t
    // returns false if we're not connected to the steam servers and thus cannot ask
    virtual bool RequestUserGroupStatus( CSteamID steamIDUser, CSteamID steamIDGroup ) = 0;

    // Returns the public IP of the server according to Steam, useful when the server is 
    // behind NAT and you want to advertise its IP in a lobby for other clients to directly
    // connect to
    virtual uint32 GetPublicIP() = 0;

    // Sets a string defining the "gamedata" for this server, this is optional, but if it is set
    // it allows users to filter in the matchmaking/server-browser interfaces based on the value
    // don't set this unless it actually changes, its only uploaded to the master once (when
    // acknowledged)
    virtual void SetGameData( const char *pchGameData) = 0; 

    // After receiving a user's authentication data, and passing it to SendUserConnectAndAuthenticate, use this function
    // to determine if the user owns downloadable content specified by the provided AppID.
    virtual EUserHasLicenseForAppResult UserHasLicenseForApp( CSteamID steamID, AppId_t appID ) = 0;
};

#ifndef __linux__
#define CLIENTENGINE_INTERFACE_VERSION "CLIENTENGINE_INTERFACE_VERSION001"

#define unknown_ret int
#define SteamAPIWarningMessageHook_t int
#define ENotificationPosition int

class IClientEngine
{
public:
    virtual HSteamPipe CreateSteamPipe() = 0; 
    virtual bool ReleaseSteamPipe( HSteamPipe hSteamPipe ) = 0;

    virtual HSteamUser CreateGlobalUser( HSteamPipe* phSteamPipe ) = 0;
    virtual HSteamUser ConnectToGlobalUser( HSteamPipe hSteamPipe ) = 0;

    virtual HSteamUser CreateLocalUser( HSteamPipe* phSteamPipe, EAccountType eAccountType ) = 0;

    virtual void ReleaseUser( HSteamPipe hSteamPipe, HSteamUser hUser ) = 0;

    virtual bool IsValidHSteamUserPipe( HSteamPipe hSteamPipe, HSteamUser hUser ) = 0;

    virtual IClientUser* GetIClientUser( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientGameServer *GetIClientGameServer( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

    virtual void SetLocalIPBinding( uint32 unIP, uint16 usPort ) = 0;
    virtual char const* GetUniverseName( EUniverse eUniverse ) = 0;

    virtual IClientFriends* GetIClientFriends( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientUtils* GetIClientUtils( HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientBilling* GetIClientBilling( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientMatchmaking* GetIClientMatchmaking( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientApps* GetIClientApps( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientContentServer* GetIClientContentServer( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientMasterServerUpdater* GetIClientMasterServerUpdater( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientMatchmakingServers* GetIClientMatchmakingServers( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;

    virtual void RunFrame() = 0;
    virtual uint32 GetIPCCallCount() = 0;

    virtual IClientUserStats* GetIClientUserStats( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientGameServerStats *GetIClientGameServerStats( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;

    virtual IClientNetworking* GetIClientNetworking( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientRemoteStorage* GetIClientRemoteStorage( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;

    virtual void SetWarningMessageHook( SteamAPIWarningMessageHook_t pFunction ) = 0;

    virtual IClientGameCoordinator* GetIClientGameCoordinator( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

    virtual void SetOverlayNotificationPosition( ENotificationPosition eNotificationPosition ) = 0;

    virtual bool IsOverlayEnabled() = 0;
    virtual bool GetAPICallResult( HSteamPipe hSteamPipe, SteamAPICall_t hSteamAPICall, void* pCallback, int cubCallback, int iCallbackExpected, bool* pbFailed ) = 0;

    virtual IClientDepotBuilder* GetIClientDepotBuilder( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;

    virtual void ConCommandInit( IConCommandBaseAccessor *pAccessor ) = 0;

    virtual IClientAppManager* GetIClientAppManager( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientConfigStore *GetIClientConfigStore( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    
    virtual bool OverlayNeedsPresent() = 0;

    virtual IClientGameStats* GetIClientGameStats( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;
    virtual IClientHTTP *GetIClientHTTP( HSteamUser hSteamUser, HSteamPipe hSteamPipe, char const* pchVersion ) = 0;

    virtual void *GetIPCServerMap() = 0;

    virtual unknown_ret OnDebugTextArrived( const char *pchDebugText ) = 0;

};

#endif // __linux__

//-----------------------------------------------------------------------------
// Purpose: Interface to creating a new steam instance, or to
//			connect to an existing steam instance, whether it's in a
//			different process or is local
//-----------------------------------------------------------------------------
class ISteamClient008
{
public:
	// Creates a communication pipe to the Steam client
	virtual HSteamPipe CreateSteamPipe() = 0;

	// Releases a previously created communications pipe
	virtual bool BReleaseSteamPipe( HSteamPipe hSteamPipe ) = 0;

	// connects to an existing global user, failing if none exists
	// used by the game to coordinate with the steamUI
	virtual HSteamUser ConnectToGlobalUser( HSteamPipe hSteamPipe ) = 0;

	// used by game servers, create a steam user that won't be shared with anyone else
	virtual HSteamUser CreateLocalUser( HSteamPipe *phSteamPipe, EAccountType accountType ) = 0;

	// removes an allocated user
	virtual void ReleaseUser( HSteamPipe hSteamPipe, HSteamUser hUser ) = 0;

	// retrieves the ISteamUser interface associated with the handle
	virtual ISteamUser *GetISteamUser( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// retrieves the ISteamGameServer interface associated with the handle
	virtual ISteamGameServer *GetISteamGameServer( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// set the local IP and Port to bind to
	// this must be set before CreateLocalUser()
	virtual void SetLocalIPBinding( uint32 unIP, uint16 usPort ) = 0; 

	// returns the ISteamFriends interface
	virtual ISteamFriends *GetISteamFriends( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamUtils interface
	virtual ISteamUtils *GetISteamUtils( HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamMatchmaking interface
	virtual ISteamMatchmaking *GetISteamMatchmaking( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamContentServer interface
	//virtual ISteamContentServer *GetISteamContentServer( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamMasterServerUpdater interface
	virtual ISteamMasterServerUpdater *GetISteamMasterServerUpdater( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamMatchmakingServers interface
	virtual ISteamMatchmakingServers *GetISteamMatchmakingServers( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

    virtual void *GetISteamGenericInterface( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteam2Bridge interface
	//virtual ISteam2Bridge *GetISteam2Bridge( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns the ISteamUserStats interface
	virtual ISteamUserStats *GetISteamUserStats( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	// returns apps interface
	virtual ISteamApps *GetISteamApps( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

    virtual ISteamNetworking *GetISteamNetworking( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

    virtual ISteamRemoteStorage *GetISteamRemoteStorage( HSteamUser hSteamUser, HSteamPipe hSteamPipe, const char *pchVersion ) = 0;

	virtual void RunFrame() = 0;

	// returns the number of IPC calls made since the last time this function was called
	// Used for perf debugging so you can understand how many IPC calls your game makes per frame
	// Every IPC call is at minimum a thread context switch if not a process one so you want to rate
	// control how often you do them.
	virtual uint32 GetIPCCallCount() = 0;

    virtual void SetWarningMessageHook(void *func);
};

#define STEAMCLIENT_INTERFACE_VERSION_V008		"SteamClient008"

#endif // STEAMEXT_H
