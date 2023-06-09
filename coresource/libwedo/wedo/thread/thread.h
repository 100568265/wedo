
#ifndef WEDO_THREAD_H
#define WEDO_THREAD_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#if   defined( _WIN32 )
# include <windows.h>
# include <process.h>
#elif defined(__unix__)
# include <pthread.h>
#endif

#include <string>

namespace wedo {

/**
* Encapsulates system API functions.
*/
class thread
{
	thread();
	thread(const thread&);
	thread& operator= (const thread&);

public:
	typedef boost::function<void ()> func_type;

#if   defined( _WIN32 )
	typedef DWORD tid_type;
#elif defined(__unix__)
	typedef pid_t tid_type;
#endif

#if __cplusplus > 201100L
	explicit thread(	 func_type&& func, const std::string& name = std::string());
#endif // __cplusplus > 201100L

/**
* The default name is wedo::thread.
*/
	explicit thread(const func_type& func, const std::string& name = std::string());

/**
* If the thread does not end, the detached thread to the destructor to continue.
* Note: Detaching a thread means that the thread is no longer needed by the notification system,
*		allowing the system to reclaim the resources allocated to it.
*/
	~thread();

	// void set_joined_callback ();
	// void set_error_callback ();
/**
* Start the thread.
*/
	void start();

/**
* Block waiting for the end of the thread.
*/
	int  join ();

/**
* Returns the thread ID, or 0 if no thread was created.
*/
	tid_type id() const;

/**
* Swap objects.
*/
	void swap (thread& other);

private:
	boost::shared_ptr<struct thread_private> _p;
};

}; // namespace wedo

#endif // WEDO_THREAD_H
