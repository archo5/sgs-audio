

#include <pthread.h>
#include "sa_sound.h"

#include <windows.h>


struct SGFileStream : SGStream
{
	SGFileStream( const char* file )
	{
		f = fopen( file, "rb" );
		error = f ? 0 : 1;
		size = 0;
		if( f )
		{
			fseek( f, SEEK_END, 0 );
			size = ftell( f );
			fseek( f, SEEK_SET, 0 );
		}
	}
	~SGFileStream()
	{
		if( f )
			fclose( f );
	}
	int32_t Read( void* to, int32_t max )
	{
		if( !f )
			return 0;
		return fread( to, 1, max, f );
	}
	int Seek( int32_t pos, int mode = SEEK_SET )
	{
		return fseek( f, pos, mode ) == 0;
	}
	int32_t Tell()
	{
		return ftell( f );
	}
	int32_t GetSize()
	{
		return size;
	}
	
	FILE* f;
	int32_t size;
};

struct SGFileSource : SGDataSource
{
	SGStream* GetStream( const char* filename )
	{
		return new SGFileStream( filename );
	}
};


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


#define _WARN( x ) return sgs_Printf( C, SGS_WARNING, x )



#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

#define sgsthread_sleep( ms ) Sleep( ms )

#define sgsmutex_t CRITICAL_SECTION
#define sgsthread_t HANDLE
#define threadret_t DWORD __stdcall

#define sgsthread_create( toT, func, data ) toT = CreateThread( NULL, 1024, func, data, 0, NULL )
#define sgsthread_self( toT ) toT = GetCurrentThread()
#define sgsthread_join( T ) WaitForMultipleObjects( 1, &T, TRUE, INFINITE )
#define sgsthread_equal( T1, T2 ) (T1 == T2)

#define sgsmutex_init( M ) InitializeCriticalSection( &M )
#define sgsmutex_destroy( M ) DeleteCriticalSection( &M )
#define sgsmutex_lock( M ) EnterCriticalSection( &M )
#define sgsmutex_unlock( M ) LeaveCriticalSection( &M )


#else

#  include <unistd.h>

static void sgsthread_sleep( int ms )
{
	if( ms >= 1000 )
	{
		sleep( ms / 1000 );
		ms %= 1000;
	}
	if( ms > 0 )
	{
		usleep( ms * 1000 );
	}
}

#define sgsmutex_t pthread_mutex_t
#define sgsthread_t pthread_t
#define threadret_t void*

#define sgsthread_create( toT, func, data ) pthread_create( &toT, NULL, func, data )
#define sgsthread_self( toT ) toT = pthread_self()
#define sgsthread_join( T ) pthread_join( T, NULL )
#define sgsthread_equal( T1, T2 ) pthread_equal( T1, T2 )

#define sgsmutex_init( M ) pthread_mutex_init( &M, NULL )
#define sgsmutex_destroy( M ) pthread_mutex_destroy( &M )
#define sgsmutex_lock( M ) pthread_mutex_lock( &M )
#define sgsmutex_unlock( M ) pthread_mutex_unlock( &M )

#endif


struct SGAudioSystem;
struct SGAudioEmitter
{
	struct SGLock
	{
		SGLock( sgsmutex_t* mtx ) : mutex( mtx ), locked( 1 )
		{ sgsmutex_lock( *mutex ); }
		~SGLock(){ sgsmutex_unlock( *mutex ); }
		sgsmutex_t* mutex;
		int locked;
	};
#define SGLOCK SGLock LOCK(&Mutex)
	
	int play( SGS_CTX )
	{
		SGLOCK;
		Emitter.Play();
		return 0;
	}
	int stop( SGS_CTX )
	{
		SGLOCK;
		Emitter.Stop();
		return 0;
	}
	int pause( SGS_CTX )
	{
		SGLOCK;
		Emitter.Pause();
		return 0;
	}
	
	SGAudioSystem* GetSystem()
	{
		return (SGAudioSystem*) System.data.O->data;
	}
	
	int load( SGS_CTX );
	
	int unload( SGS_CTX )
	{
		SGLOCK;
		sgs_PushBool( C, Emitter.System != NULL );
		if( Emitter.System )
			Emitter.System->ReleaseEmitter( &Emitter );
		return 1;
	}
	
	int fade_volume( SGS_CTX )
	{
		SGLOCK;
		sgs_Real vol, tm;
		if( !sgs_ParseReal( C, 1, &vol ) ||
			!sgs_ParseReal( C, 2, &tm ) )
			_WARN( "expected 2 arguments of type 'real' (target,time)" );
		Emitter.FadeVolume( vol, tm );
		return 0;
	}
	
	int set_position( SGS_CTX )
	{
		SGLOCK;
		sgs_Real x, y, z;
		if( !sgs_ParseReal( C, 1, &x ) || !sgs_ParseReal( C, 2, &y ) ||
			!sgs_ParseReal( C, 3, &z ) )
			_WARN( "expected 3 arguments of type 'real' (x,y,z)" );
		float pos[3] = {(float)x,(float)y,(float)z};
		Emitter.SetPosition( pos );
		return 0;
	}
	
	int getter_playing( SGS_CTX )
	{
		SGLOCK;
		sgs_PushBool( C, Emitter.IsPlaying() );
		return SGS_SUCCESS;
	}
	
	int getter_loop( SGS_CTX )
	{
		SGLOCK;
		sgs_PushBool( C, Emitter.StreamLoop );
		return SGS_SUCCESS;
	}
	int setter_loop( SGS_CTX )
	{
		SGLOCK;
		int loop;
		if( sgs_ParseBool( C, 1, &loop ) )
		{
			Emitter.SetLooping( loop );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	int getter_volume( SGS_CTX )
	{
		SGLOCK;
		sgs_PushReal( C, Emitter.GetVolume() );
		return SGS_SUCCESS;
	}
	int setter_volume( SGS_CTX )
	{
		SGLOCK;
		sgs_Real vol;
		if( sgs_ParseReal( C, 1, &vol ) )
		{
			Emitter.SetVolume( vol );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	int getter_pitch( SGS_CTX )
	{
		SGLOCK;
		sgs_PushReal( C, Emitter.GetPitch() );
		return SGS_SUCCESS;
	}
	int setter_pitch( SGS_CTX )
	{
		SGLOCK;
		sgs_Real pitch;
		if( sgs_ParseReal( C, 1, &pitch ) )
		{
			Emitter.SetPitch( pitch );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	int getter_groups( SGS_CTX )
	{
		SGLOCK;
		sgs_PushInt( C, Emitter.GetType() );
		return SGS_SUCCESS;
	}
	int setter_groups( SGS_CTX )
	{
		SGLOCK;
		sgs_Real groups;
		if( sgs_ParseReal( C, 1, &groups ) )
		{
			Emitter.SetType( groups );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	int getter_offset( SGS_CTX )
	{
		SGLOCK;
		sgs_PushReal( C, Emitter.GetPlayOffset() );
		return SGS_SUCCESS;
	}
	int setter_offset( SGS_CTX )
	{
		SGLOCK;
		sgs_Real off;
		if( sgs_ParseReal( C, 1, &off ) )
		{
			Emitter.SetPlayOffset( off );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	int setter_mode3d( SGS_CTX )
	{
		SGLOCK;
		int m3d;
		if( sgs_ParseBool( C, 1, &m3d ) )
		{
			Emitter.Set3DMode( m3d );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	int setter_panning( SGS_CTX )
	{
		SGLOCK;
		sgs_Real pan;
		if( sgs_ParseReal( C, 1, &pan ) )
		{
			Emitter.SetPanning( pan );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	int setter_factor( SGS_CTX )
	{
		SGLOCK;
		sgs_Real df;
		if( sgs_ParseReal( C, 1, &df ) )
		{
			Emitter.SetDistanceFactor( df );
			return SGS_SUCCESS;
		}
		return SGS_EINVAL;
	}
	
	sgs_Variable System;
	SSEmitter Emitter;
	sgsmutex_t Mutex;
};

int SGAudioEmitter_gcmark( SGS_CTX, sgs_VarObj* data, int dco )
{
	SGAudioEmitter* em = (SGAudioEmitter*) data->data;
	sgs_GCMark( C, &em->System );
	return SGS_SUCCESS;
}

int SGAudioEmitter_destruct( SGS_CTX, sgs_VarObj* data, int dco )
{
	SGAudioEmitter* em = (SGAudioEmitter*) data->data;
	sgs_ReleaseOwned( C, &em->System, dco );
	delete em;
	return SGS_SUCCESS;
}


#define SGS_CLASS   SGAudioEmitter
SGS_DECLARE_IFACE;
SGS_METHOD_WRAPPER( load );
SGS_METHOD_WRAPPER( unload );
SGS_METHOD_WRAPPER( play );
SGS_METHOD_WRAPPER( stop );
SGS_METHOD_WRAPPER( pause );
SGS_METHOD_WRAPPER( fade_volume );
SGS_METHOD_WRAPPER( set_position );
SGS_BEGIN_GENERIC_GETINDEXFUNC
	SGS_GIF_METHOD( load )
	SGS_GIF_METHOD( unload )
	SGS_GIF_METHOD( play )
	SGS_GIF_METHOD( stop )
	SGS_GIF_METHOD( pause )
	SGS_GIF_METHOD( fade_volume )
	SGS_GIF_METHOD( set_position )
	SGS_GIF_GETTER( playing )
	SGS_GIF_GETTER( loop )
	SGS_GIF_GETTER( volume )
	SGS_GIF_GETTER( pitch )
	SGS_GIF_GETTER( groups )
	SGS_GIF_GETTER( offset )
SGS_END_GENERIC_GETINDEXFUNC;
SGS_BEGIN_GENERIC_SETINDEXFUNC
	SGS_GIF_SETTER( loop )
	SGS_GIF_SETTER( volume )
	SGS_GIF_SETTER( pitch )
	SGS_GIF_SETTER( groups )
	SGS_GIF_SETTER( offset )
	SGS_GIF_SETTER( mode3d )
	SGS_GIF_SETTER( panning )
	SGS_GIF_SETTER( factor )
SGS_END_GENERIC_SETINDEXFUNC;
SGS_DEFINE_IFACE
	SGS_IFACE_GETINDEX,
	SGS_IFACE_SETINDEX,
	SOP_GCMARK, (void*) SGAudioEmitter_gcmark,
	SOP_DESTRUCT, (void*) SGAudioEmitter_destruct,
SGS_DEFINE_IFACE_END;
#undef SGS_CLASS



threadret_t SGA_Ticker( void* );

struct SGAudioSystem
{
	SGAudioSystem() : ThreadEnable( TRUE )
	{
		Source = new SGFileSource;
		Sound.Init( Source );
		sgsthread_create( Thread, SGA_Ticker, this );
		sgsmutex_init( Mutex );
	}
	~SGAudioSystem()
	{
		ThreadEnable = FALSE;
		sgsthread_join( Thread );
		sgsmutex_destroy( Mutex );
		Sound.Destroy();
		delete Source;
	}
	
	struct SGLock
	{
		SGLock( sgsmutex_t* mtx ) : mutex( mtx ), locked( 1 )
		{ sgsmutex_lock( *mutex ); }
		~SGLock(){ sgsmutex_unlock( *mutex ); }
		sgsmutex_t* mutex;
		int locked;
	};
#define SGLOCK SGLock LOCK(&Mutex)
	
	int set_volume( SGS_CTX )
	{
		SGLOCK;
		sgs_Integer grp = 0;
		sgs_Real vol;
		if( !sgs_ParseReal( C, 1, &vol ) )
			_WARN( "expected argument 1 (volume) as real" );
		sgs_ParseInt( C, 2, &grp );
		if( grp < 0 || grp > 32 )
			_WARN( "group ID expected between 0 and 32" );
		Sound.SetVolume( vol, grp );
		return 0;
	}
	
	int get_volume( SGS_CTX )
	{
		SGLOCK;
		sgs_Integer grp = 0;
		sgs_ParseInt( C, 1, &grp );
		if( grp < 0 || grp > 32 )
			_WARN( "group ID expected between 0 and 32" );
		sgs_PushReal( C, Sound.GetVolume( grp ) );
		return 1;
	}
	
	int set_listener_position( SGS_CTX )
	{
		SGLOCK;
		sgs_Real x, y, z;
		if( !sgs_ParseReal( C, 1, &x ) || !sgs_ParseReal( C, 2, &y ) ||
			!sgs_ParseReal( C, 3, &z ) )
			_WARN( "expected 3 real arguments (x,y,z)" );
		float lp[3] = {(float)x,(float)y,(float)z};
		Sound.SetListenerPosition( lp );
		return 0;
	}
	
	int set_listener_orientation( SGS_CTX )
	{
		SGLOCK;
		sgs_Real dx, dy, dz, ux, uy, uz;
		if( !sgs_ParseReal( C, 1, &dx ) || !sgs_ParseReal( C, 2, &dy ) ||
			!sgs_ParseReal( C, 3, &dz ) || !sgs_ParseReal( C, 4, &ux ) ||
			!sgs_ParseReal( C, 5, &uy ) || !sgs_ParseReal( C, 6, &uz ) )
			_WARN( "expected 6 real arguments (dx,dy,dz,ux,uy,uz)" );
		float ld[3] = {(float)dx,(float)dy,(float)dz};
		float lu[3] = {(float)ux,(float)uy,(float)uz};
		Sound.SetListenerDirection( ld, lu );
		return 0;
	}
	
	int suspend( SGS_CTX )
	{
		SGLOCK;
		Sound.ChangeState( 1 );
		return 0;
	}
	int resume( SGS_CTX )
	{
		SGLOCK;
		Sound.ChangeState( 0 );
		return 0;
	}
	
	int create_emitter( SGS_CTX )
	{
		SGLOCK;
		char* file;
		SGAudioEmitter* em = new SGAudioEmitter;
		sgs_GetStackItem( C, 0, &em->System );
		sgs_Acquire( C, &em->System );
		em->Mutex = Mutex;
		if( sgs_ParseString( C, 1, &file, NULL ) )
			Sound.SetupEmitter( file, &em->Emitter );
		sgs_PushObject( C, em, SGS_IFN(SGAudioEmitter) );
		return 1;
	}
	
	SGDataSource* Source;
	SSoundSystem Sound;
	sgsthread_t Thread;
	sgsmutex_t Mutex;
	int ThreadEnable;
};

threadret_t SGA_Ticker( void* ss )
{
	SGAudioSystem* sys = (SGAudioSystem*) ss;
	alcMakeContextCurrent( sys->Sound.Context );
	while( sys->ThreadEnable )
	{
		sgsmutex_lock( sys->Mutex );
		sys->Sound.Tick( 1.0f / 30.0f );
		sgsmutex_unlock( sys->Mutex );
		sgsthread_sleep( 33 );
	}
	return 0;
}


#define SGS_CLASS   SGAudioSystem
SGS_DECLARE_IFACE;
SGS_METHOD_WRAPPER( set_volume );
SGS_METHOD_WRAPPER( get_volume );
SGS_METHOD_WRAPPER( set_listener_position );
SGS_METHOD_WRAPPER( set_listener_orientation );
SGS_METHOD_WRAPPER( suspend );
SGS_METHOD_WRAPPER( resume );
SGS_METHOD_WRAPPER( create_emitter );
SGS_BEGIN_GENERIC_GETINDEXFUNC
	SGS_GIF_METHOD( set_volume )
	SGS_GIF_METHOD( get_volume )
	SGS_GIF_METHOD( set_listener_position )
	SGS_GIF_METHOD( set_listener_orientation )
	SGS_GIF_METHOD( suspend )
	SGS_GIF_METHOD( resume )
	SGS_GIF_METHOD( create_emitter )
SGS_END_GENERIC_GETINDEXFUNC;
SGS_GENERIC_DESTRUCTOR;
SGS_DEFINE_IFACE
	SGS_IFACE_GETINDEX,
	SGS_IFACE_DESTRUCT,
SGS_DEFINE_IFACE_END;
#undef SGS_CLASS


int sgs_audio_create( SGS_CTX )
{
	sgs_PushObject( C, new SGAudioSystem, SGS_IFN(SGAudioSystem) );
	return 1;
}


extern "C" int sgscript_main( SGS_CTX )
{
	sgs_PushCFunction( C, sgs_audio_create );
	sgs_StoreGlobal( C, "sgs_audio_create" );
	return SGS_SUCCESS;
}


int SGAudioEmitter::load( SGS_CTX )
{
	char* file;
	SGLOCK;
	if( !sgs_ParseString( C, 1, &file, NULL ) )
		_WARN( "expected 1 string argument (file)" );
	sgs_PushBool( C, GetSystem()->Sound.SetupEmitter( file, &Emitter ) );
	return 1;
}

