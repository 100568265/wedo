
#include "../../wedo/thread/this_thread.h"
#include "../../wedo/thread/thread.h"

#include <stdio.h>
#include <time.h>
#include <errno.h>

#if   defined( __linux__ )
# include <sys/prctl.h>
# include <sys/syscall.h> //Use gettid() syscall under linux to get thread id
#elif defined(__FreeBSD__)
# include <sys/thr.h> //Use thr_self() syscall under FreeBSD to get thread id
#endif

#include <boost/weak_ptr.hpp>

#ifdef _WIN32
//
// Usage: SetThreadName ((DWORD)-1, "MainThread");
//
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
 } THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, const char* threadName) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
    }
#pragma warning(pop)
}
#endif // _WIN32

namespace wedo {

namespace this_thread {

WEDO_THREAD_LOCAL int _cache_tid = 0;
WEDO_THREAD_LOCAL char _tid_str [32] = {'0'};

WEDO_THREAD_LOCAL const char * _thr_name = "wedo::thread";

inline size_t __thread_id()
{
#if  !defined(BOOST_NO_CXX11_HDR_THREAD)
	return static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#elif defined(  _WIN32   )
    return static_cast<size_t>(::GetCurrentThreadId());
#elif defined( __linux__ )
# if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
#  define SYS_gettid __NR_gettid
# endif
    return static_cast<size_t>(::syscall(SYS_gettid));
#elif defined(__FreeBSD__)
    long tid;
    thr_self(&tid);
    return static_cast<size_t>(tid);
#else
# error Unable to match system function
    return 0;
#endif
}

inline int __pid()
{
#ifdef _WIN32
    return ::_getpid();
#else
    return static_cast<int>(::getpid());
#endif
}

inline void __init_cache_tid()
{
	if (_cache_tid == 0) {
		_cache_tid = static_cast<int>(__thread_id());
		snprintf(_tid_str, sizeof (_tid_str), "%5d", _cache_tid);
	}
}

int tid ()
{
#if defined(__GNUC__)
	if (__builtin_expect(_cache_tid == 0, 0))
#endif
		__init_cache_tid();
	return _cache_tid;
}

const char * tid_string () { return _tid_str; }

const char * name () { return _thr_name; }

bool is_main ()
{
	return tid() == __pid();
}

void msleep (int msec)
{
#if defined(_WIN32)
	Sleep (msec);
#else
    struct timespec intervals = { time_t(msec / 1000L), (msec % 1000L) * 1000000L };
    struct timespec remaining = { 0, 0 };
    while ( nanosleep(&intervals, &remaining) < 0 && errno == EINTR )
    {
        intervals = remaining;
        errno = 0;
    }
#endif
}

}; // namespace this_thread

//===========================================================================

struct thread_private
{
	thread_private() : started(false), joined(false), tid(0), thr_handle(0)
	{}

	inline void exit_clear()
	{
		started = false;
		tid = 0;
		thr_handle = 0;
	}

	bool started;
	bool joined;

	std::string 		name;
	thread::func_type   func;
	thread::tid_type 	tid;

#if   defined( _WIN32 )
	HANDLE    thr_handle;
#elif defined(__unix__)
	pthread_t thr_handle;
#endif
};

//===========================================================================

struct thread_data
{
	typedef wedo::thread::func_type func_type;
	typedef wedo::thread::tid_type tid_type;

	boost::weak_ptr<thread_private> _thr_p;

	thread_data(const boost::shared_ptr<thread_private>& thr_p):
	_thr_p(thr_p)
	{}

	void run()
	{
	    boost::shared_ptr<thread_private> thr_p = _thr_p.lock();
	    if (!thr_p || !(thr_p->func)) return;

	    tid_type tid = wedo::this_thread::tid();
	    thr_p->tid = tid;

	    wedo::this_thread::_thr_name = thr_p->name.c_str();

#if   defined( _WIN32 )
		SetThreadName(  tid, wedo::this_thread::_thr_name);
#elif defined(__unix__)
	    ::prctl(PR_SET_NAME, wedo::this_thread::_thr_name);
#endif
	    try {
	    	thr_p->func();
	    }
	    catch (std::exception& ex)
	    {
	    	fprintf(stderr, "exception caught in thread. reason: %s\n", ex.what());
	    }
	    catch (...) {
			fprintf(stderr, "unknown exception caught in thread.");
	    	throw;
	    }
        thr_p->exit_clear();
	}

};

#ifdef _WIN32
static void __cdecl proc (void * param)
#else
static void *       proc (void * param)
#endif
{
	thread_data* data = static_cast<thread_data*>(param);
	data->run();
	delete data;
#if defined(__unix__)
	return 0;
#endif
}

//===========================================================================

thread::thread (const func_type& func, const std::string& name):
_p(new thread_private())
{
	_p->func = func;
	_p->name = (name.empty() ? "wedo::thread" : name);
}

#if __cplusplus > 201100L
thread::thread (     func_type&& func, const std::string& name):
_p(new thread_private())
{
	_p->func.swap(func);
	_p->name = (name.empty() ? "wedo::thread" : name);
}
#endif // __cplusplus > 201100L

thread::~thread()
{
	if (_p->started)
#if   defined( _WIN32 )
		CloseHandle   (_p->thr_handle);
#elif defined(__unix__)
		pthread_detach(_p->thr_handle);
#endif
}

thread::tid_type thread::id() const
{
	return (_p->tid);
}

void thread::swap(thread& other)
{
	_p.swap(other._p);
}

void thread::start()
{
	if (_p->started) return;
	_p->started = true;

	thread_data * data = new thread_data (_p);
#if   defined( _WIN32 )
	_p->thr_handle = _beginthread (&proc, 0, data);
	if (_p->thr_handle == -1L)
	{
		_p->started = false;
		delete data;
	}
#elif defined(__unix__)
	if (pthread_create(&_p->thr_handle, NULL, &proc, data))
	{
		_p->started = false;
		delete data;
//		LOG << "Failed in pthread_create";
	}
#endif
}

int thread::join()
{
	if (!(_p->started))
		return 0;

	int ret = -1;
#if   defined( _WIN32 )
	ret = ::WaitForSingleObject(_p->thr_handle, INFINITE) == WAIT_FAILED;
#elif defined(__unix__)
	ret = pthread_join(_p->thr_handle, NULL);
#endif
	_p->started = false;
	return ret;
}

}; // namespace wedo
