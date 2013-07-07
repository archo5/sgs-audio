

#include <pthread.h>
#include "sa_sound.h"

#include <windows.h>


#define gmReal double
#define gmString char*
#define gmFunc extern "C" __declspec( dllexport )
#define gmFuncR gmFunc gmReal
#define gmFuncS gmFunc gmString


gmFuncR sga_Free();

//// Win32 Stuff ////

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
			// Initialize once for each new process.
			// Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
			// Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
			// Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
			// Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

//// Threaded Processing ///

static SSoundSystem GSound;
static pthread_t SGMainThread;
static pthread_mutex_t SGMutex;
static int SGThreadEnable = FALSE;

struct SGLock
{
	SGLock() : Locked( TRUE ){ pthread_mutex_lock( &SGMutex ); }
	~SGLock(){ Unlock(); }
	void Unlock(){ if( Locked ){ pthread_mutex_unlock( &SGMutex ); Locked = FALSE; } }
	int Locked;
};
#define SGLOCK SGLock LOCK;
#define SGUNLOCK LOCK.Unlock()


void* SGA_Ticker( void* )
{
	while( SGThreadEnable )
	{
		SGLOCK;
		GSound.Tick( 1.0f / 30.0f );
		SGUNLOCK;
		Sleep( 33 );
	}
	return 0;
}


extern "C" int sgscript_main( SGS_CTX )
{
	return SGS_SUCCESS;
}


/*


//// The API ////
TPersistentArray< SSEmitter* > SGEmitters;
SGFileSource* GDataSource;

/// Sound system creation & destruction
int Initialized = FALSE;
gmFuncR sga_Init()
{
	GCurrentDirectory[0] = 0;
	if( Initialized )
		return 0;

	GDataSource = new SGFileSource;
	GSound.Init( GDataSource );

	SGThreadEnable = TRUE;
	SGMainThread.Run( SGA_Ticker );
	Initialized = TRUE;
	return 1;
}
gmFuncR sga_Free()
{
	if( !Initialized )
		return 0;

	SGThreadEnable = FALSE;
	SGMainThread.WaitForEnd();

	// free automatically
	for( uint32_t i = 0; i < SGEmitters.Width(); ++i )
	{
		if( !SGEmitters.Used( i ) )
			continue;
		delete SGEmitters.Get( i );
	}

	GSound.Destroy();
	delete GDataSource;
	Initialized = FALSE;
	return 1;
}
gmFuncR sga_SetListenerPosition( gmReal X, gmReal Y, gmReal Z )
{
	SGLOCK;
	GSound.SetListenerPosition( mVec3( X, Y, Z ) );
	return 1;
}
gmFuncR sga_SetListenerDirection( gmReal dirX, gmReal dirY, gmReal dirZ, gmReal upX, gmReal upY, gmReal upZ )
{
	SGLOCK;
	GSound.SetListenerDirection( mVec3( dirX, dirY, dirZ ), mVec3( upX, upY, upZ ) );
	return 1;
}
gmFuncR sga_SetGroupVolume( gmReal Group, gmReal Vol )
{
	SGLOCK;
	GSound.SetVolume( Vol, (int32_t)Group );
	return 1;
}
gmFuncR sga_GetGroupVolume( gmReal Group )
{
	SGLOCK;
	GSound.GetVolume( (int32_t)Group );
	return 1;
}

/// Emitter creation & destruction
// Emitter creation function
// Can load wav & ogg files
// Returns the emitter ID
gmFuncR sga_CreateEmitter( gmString file )
{
	SGLOCK;
	SSEmitter* e = new SSEmitter;
	GSound.SetupEmitter( file, e );
	return (gmReal) SGEmitters.Add( e );
}
// Emitter destroying function
// Destroys an emitter of the given ID
gmFuncR sga_DestroyEmitter( gmReal ID )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	delete SGEmitters.Get( id );
	SGEmitters.Remove( id );
	return 1;
}

/// Emitter basic controls
gmFuncR sga_Play( gmReal ID )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->Play();
	return 1;
}
gmFuncR sga_Pause( gmReal ID )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->Pause();
	return 1;
}
gmFuncR sga_Stop( gmReal ID )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->Stop();
	return 1;
}
// TODO: REMOVE
gmFuncR sga_Rewind( gmReal ID )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->Stop();
	return 1;
}
gmFuncR sga_IsPlaying( gmReal ID )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	int playing = SGEmitters.Get( id )->IsPlaying();
	return playing;
}

/// Emitter basic parameters
gmFuncR sga_SetVolume( gmReal ID, gmReal Volume )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetVolume( Volume );
	return 1;
}
gmFuncR sga_GetVolume( gmReal ID )
{
//	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	float vol = SGEmitters.Get( id )->GetVolume();
	return vol;
}
gmFuncR sga_FadeVolume( gmReal ID, gmReal Volume, gmReal Time )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->FadeVolume( Volume, Time );
	return 1;
}
gmFuncR sga_SetType( gmReal ID, gmReal Type )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetType( Type );
	return 1;
}
gmFuncR sga_GetType( gmReal ID )
{
//	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	float ty = SGEmitters.Get( id )->GetType();
	return ty;
}
gmFuncR sga_SetLooping( gmReal ID, gmReal Loop )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetLooping( !!Loop );
	return 1;
}
gmFuncR sga_SetPanning( gmReal ID, gmReal Pan )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetPanning( Pan );
	return 1;
}
gmFuncR sga_SetPitch( gmReal ID, gmReal Pitch )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetPitch( Pitch );
	return 1;
}
gmFuncR sga_SetPlayOffset( gmReal ID, gmReal off )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetPlayOffset( off );
	return 1;
}
gmFuncR sga_GetPlayOffset( gmReal ID )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	float tm = SGEmitters.Get( id )->GetPlayOffset();
	return tm;
}

/// Emitter 2D/3D parameters
gmFuncR sga_Set3DMode( gmReal ID, gmReal Enabled )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->Set3DMode( !!Enabled );
	return 1;
}
gmFuncR sga_SetDistFactor( gmReal ID, gmReal Factor )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetDistanceFactor( Factor );
	return 1;
}
gmFuncR sga_SetPosition( gmReal ID, gmReal X, gmReal Y, gmReal Z )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	if( !SGEmitters.Used( id ) )
		return 0;
	SGEmitters.Get( id )->SetPosition( mVec3( X, Y, Z ) );
	return 1;
}

//// The Track-emitter System ////
gmFuncR sga_TrackPlay( gmReal ID, gmString File, gmReal Loop, gmReal TypeFlags )
{
	SGLOCK;
	int32_t id = (int32_t) ID;
	SSEmitter* e;
	if( id >= 0 && SGEmitters.Used( id ) )
		e = SGEmitters.Get( id );
	else
	{
		e = new SSEmitter;
		id = SGEmitters.Add( e );
	}
	GSound.SetupEmitter( File, e );
	e->Set3DMode( FALSE );
	e->SetLooping( Loop );
	e->SetType( TypeFlags );
	e->Play();
	return (gmReal) id;
}
*/
