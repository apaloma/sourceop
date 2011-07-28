#ifndef INTERFACES_H
#define INTERFACES_H

class IVEngineServer;
class IVoiceServer;
class IFileSystem;
class IGameEventManager;
class IGameEventManager2;
class IPlayerInfoManager;
class IServerPluginHelpers;
class ICvar;
class INetworkStringTableContainer;
class IServerGameDLL;
class IServerGameEnts;
class IServerGameClients;
class IEffects;
class IEngineSound;
class IEngineTrace;
class IStaticPropMgrServer;
class IUniformRandomStream;
class ICommandLine;
class IServerTools;
class IMDLCache;
class CSysModule;
class IPhysics;
class IPhysicsSurfaceProps;
class IPhysicsCollision;
class IPhysicsEnvironment;
class IVPhysicsDebugOverlay;
class IPhysicsObjectPairHash;
class ITempEntsSystem;

extern IVEngineServer   *engine; // helper functions (messaging clients, loading content, making entities, running commands, etc)
extern IVoiceServer     *g_pVoiceServer;
extern IFileSystem      *filesystem; // file I/O 
extern IGameEventManager *gameeventmanager_old; // game events interface
extern IGameEventManager2 *gameeventmanager; // game events interface
extern IPlayerInfoManager *playerinfomanager; // game dll interface to interact with players
extern IServerPluginHelpers *helpers; // special 3rd party plugin helpers from the engine
extern ICvar *cvar;
extern INetworkStringTableContainer *networkstringtable;
extern IServerGameDLL *servergame;
extern IServerGameEnts *servergameents;
extern IServerGameClients *servergameclients;
extern IEffects *effects;
extern IEngineSound *enginesound;
extern IEngineTrace *enginetrace;
extern IStaticPropMgrServer *staticpropmgr;
extern IUniformRandomStream *random;
extern ICommandLine *commandline;
extern IServerTools *servertools;
extern IMDLCache *mdlcache;

extern CSysModule *g_PhysicsDLL ;
extern IPhysics *physics; // physics functions
extern IPhysicsSurfaceProps *physprops;
extern IPhysicsCollision *physcollision;
extern IPhysicsEnvironment *physenv;
extern IVPhysicsDebugOverlay *physdebugoverlay;
extern IPhysicsObjectPairHash *g_EntityCollisionHash;

extern ITempEntsSystem* te;

#endif