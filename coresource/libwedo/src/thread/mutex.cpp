
#include "../../wedo/thread/mutex.h"

namespace wedo {

mutex::mutex()
{
#if   defined( _WIN32 )
	_mutex = CreateMutex(NULL, FALSE, NULL);
#elif defined(__unix__)
    pthread_mutex_init(&_mutex, NULL);
#endif
}

mutex::~mutex()
{
#if   defined( _WIN32 )
	CloseHandle(_mutex);
#elif defined(__unix__)
    pthread_mutex_destroy(&_mutex);
#endif
}

void mutex::lock()
{
#if   defined( _WIN32 )
//	DWORD d =
	::WaitForSingleObject(_mutex, INFINITE);
#elif defined(__unix__)
    pthread_mutex_lock(&_mutex);
#endif
}

void mutex::unlock()
{
#if   defined( _WIN32 )
	::ReleaseMutex(_mutex);
#elif defined(__unix__)
    pthread_mutex_unlock(&_mutex);
#endif
}

//============================================================

lock_guard::lock_guard(wedo::mutex& mx)
: _mutex(mx)
{ _mutex.lock(); }

lock_guard::~lock_guard()
{ _mutex.unlock(); }

//============================================================

condition::condition(wedo::mutex& mx)
: _mutex(mx)
{
#if   defined( _WIN32 )
	_cond = CreateEvent(NULL, FALSE, FALSE, NULL );
#elif defined(__unix__)
    pthread_cond_init(&_cond, NULL);
#endif
}

condition::~condition()
{
#if   defined( _WIN32 )
	::CloseHandle(_cond);
#elif defined(__unix__)
    pthread_cond_destroy(&_cond);
#endif
}

void condition::wait()
{
#if   defined( _WIN32 )
	::WaitForSingleObject(_cond, INFINITE);
#elif defined(__unix__)
    pthread_cond_wait(&_cond, _mutex.unix_instance_ptr());
#endif
}

void condition::notify()
{
#if   defined( _WIN32 )
	::SetEvent(_cond);
#elif defined(__unix__)
    pthread_cond_signal(&_cond);
#endif
}

#if   defined(__unix__)
void condition::notify_all()
{
	pthread_cond_broadcast(&_cond);
}
#endif

}; // namespace wedo;
