
#ifndef _WEDO_MUTEX_H_
#define _WEDO_MUTEX_H_


// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
#    pragma once
#endif

#if   defined( _WIN32 )
    #include <windows.h>
#elif defined(__unix__)
    #include <pthread.h>
#endif


namespace wedo {

class mutex
{
	friend class condition;
public:
	 mutex();
	~mutex();

	void lock  ();
	void unlock();

private:
	mutex(const mutex& );
	mutex& operator= (const mutex& );

#if __cplusplus > 201100L
	mutex(const mutex&&);
	mutex& operator= (const mutex&&);
#endif

#if   defined( _WIN32 )
	typedef HANDLE          type;
#elif defined(__unix__)
	typedef pthread_mutex_t type;

	inline  pthread_mutex_t * unix_instance_ptr () { return &_mutex; }
#endif

	type _mutex;
};

class lock_guard
{
public:
	explicit lock_guard(wedo::mutex& mx);
	~lock_guard();

private:
	lock_guard();
	lock_guard(const lock_guard& );
	lock_guard& operator= (const lock_guard& );

#if __cplusplus > 201100L
	lock_guard(const lock_guard&&);
	lock_guard& operator= (const lock_guard&&);
#endif

	wedo::mutex& _mutex;
};

class condition
{
public:
	explicit condition(wedo::mutex& mx);
	~condition();
	
	void wait      ();
	void notify    ();
	void notify_all();

private:
	condition();
	condition(const condition& );
	condition& operator= (const condition& );
	
#if __cplusplus > 201100L
	condition(const condition&&);
	condition& operator= (const condition&&);
#endif

#if   defined( _WIN32 )
	typedef HANDLE         type;
#elif defined(__unix__)
	typedef pthread_cond_t type;
#endif
	mutex& _mutex;
	type   _cond ;
};

}; // namespace wedo;

#endif //_WEDO_MUTEX_H_