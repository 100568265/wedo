
#ifndef WEDO_THIS_THREAD_H
#define WEDO_THIS_THREAD_H


// Provide a TLS mechanism.
#ifndef WEDO_THREAD_LOCAL
//# if   __cplusplus > 201100L
//#  define WEDO_THREAD_LOCAL thread_local
# if defined(__GNUC__)
#  define WEDO_THREAD_LOCAL __thread
# else
#  define WEDO_THREAD_LOCAL __declspec(thread)
# endif
#endif

namespace wedo {

namespace this_thread {
/**
* Returns the current thread ID.
*/
	int tid ();

/**
* Returns the format string for tid value.
*/
	const char * tid_string ();


	const char * name ();
/**
* wedo::this_thread::is_main();
* Determine whether the main thread.
*/
	bool is_main ();

	void msleep (int msec);

}; // namespace this_thread

}; // namespace wedo

#endif // WEDO_THIS_THREAD_H
