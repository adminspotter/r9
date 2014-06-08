/* thread_pool.h                                           -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 22 Nov 2009, 12:18:31 trinity
 *
 * Revision IX game server
 * Copyright (C) 2009  Trinity Annabelle Quirk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * This file contains a basic thread pool template class.  The template
 * parameter is the type of requests that go into the queue.
 *
 * This thread pool is able to resize on the fly.  We start up new
 * threads if the size grows, or stop threads if the size shrinks.
 * Basically, if we're using some arbitrary amount of machines or
 * processors, and that number changes, we want to be able to adjust
 * how much we're trying to do.  This is with an eye to using
 * something like openmosix.
 *
 * Interface:
 *   ThreadPool(char *name, int size)
 *       creates a thread pool "<name>" with <size> threads
 *   ~ThreadPool(void)
 *       destroys the thread pool
 *
 *   start(void *(*func)(void *))
 *       starts up all the threads with worker function <func>
 *   stop(void)
 *       stops all the threads
 *
 *   push(T& req)
 *       pushes a request <req> onto the work queue
 *   pop(T *buf)
 *       pops the head of the work queue into <buf>
 *
 *   pool_size(void)
 *       returns the number of threads in the pool
 *   queue_size(void)
 *       returns the length of the work queue
 *   resize(int size)
 *       resizes the thread pool to have <size> threads
 *
 * Changes
 *   01 Jul 2006 TAQ - Created the file.
 *   04 Jul 2006 TAQ - Added pop method, to encapsulate mutex handling.
 *                     Added name member, so our log entries mean something.
 *                     The thread_pool member is now a pointer, as it should
 *                     have been in the first place.  We now throw int
 *                     exceptions, which contain errno values, out of the
 *                     constructor.
 *   06 Jul 2006 TAQ - Added the C++ tag at the top to get emacs to use the
 *                     right mode.
 *   12 Jul 2006 TAQ - Fixed a couple syntax errors.
 *   27 Jul 2006 TAQ - Added a crapload of syslog debugging output, since
 *                     we're running into some real big problems here.  Turns
 *                     out that if we sleep on entry to our worker routine,
 *                     the seg-faults we're seeing go away - some sort of
 *                     race condition?
 *   30 Jul 2006 TAQ - Removed some debugging syslog output.  We seem to be
 *                     working pretty well now.  Moved the allocations before
 *                     the mutex initialization, to attempt to combat the
 *                     apparent race condition we're seeing.  Moved the startup
 *                     into its own start() routine, which completely removes
 *                     any chance of that race condition.
 *   16 Aug 2006 TAQ - Added clean_on_pop member, and a conditional to do a
 *                     memset to 0 of the last item popped from the queue.
 *                     Basically it's for cleaning up passwords from memory
 *                     when we're done with them.
 *   12 Jun 2007 TAQ - Minor tweaks.
 *   21 Jun 2007 TAQ - Added a wrapper function to the start so that we can
 *                     set thread cancel reactions the way we want.  Added the
 *                     resize method.  Added pool_size and queue_size
 *                     accessor functions, for reporting purposes.
 *   22 Jun 2007 TAQ - Added a 0-length sleep to the destructor before
 *                     killing each thread, so that we'll give up our slice
 *                     and give the children a chance to get on the CPU.
 *                     That way, they'll actually die in the correct way.
 *                     Every place we cancel a thread, we now do a zero
 *                     sleep, in case it's important.
 *   05 Sep 2007 TAQ - Added an exit_flag member, since we can't depend on
 *                     main_loop_exit_flag to be set when we really need
 *                     child threads to exit.  Added a stop() method.  Added
 *                     a startup_arg, so that we could actually pass something
 *                     into the worker function.
 *   06 Sep 2007 TAQ - Removed startup_wrapper, since it was redundant, and
 *                     got in the way of making startup_arg available.
 *   08 Sep 2007 TAQ - Added string.h and unistd.h includes, which were
 *                     strangely absent.
 *   17 Sep 2007 TAQ - Removed a pthread_cancel, which does not return if
 *                     the thread in question has already exited.
 *   23 Sep 2007 TAQ - STLized the thread_pool (vector) and the request_
 *                     queue (queue).  Lots of complexity went away.  The
 *                     constructor only takes two args now, name and pool
 *                     size.
 *   22 Nov 2009 TAQ - Added const to constructor's string argument to get
 *                     rid of a compiler warning.
 *
 * Things to do
 *   - Might we need a way to gun a stuck thread?
 *
 * $Id: thread_pool.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_THREAD_POOL_H__
#define __INC_THREAD_POOL_H__

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>
#include <vector>
#include <queue>
#include <exception>

template <class T>
class ThreadPool
{
  private:
    char *name;
    std::vector<pthread_t> thread_pool;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_not_empty;
    void *(*startup_func)(void *);
    std::queue<T> request_queue;
    unsigned int thread_count;
    bool exit_flag;
  public:
    bool clean_on_pop;
    void *startup_arg;

  public:
    ThreadPool(const char *pool_name, unsigned int pool_size)
	throw (int)
	: thread_pool(), request_queue()
	{
	    int ret;

	    /* Copy our name into local storage, to a maximum of 8 chars */
	    if ((this->name = strndup(pool_name, 8)) == NULL)
		throw ENOMEM;

	    /* Initialize the elements of our pool */
	    this->thread_count = pool_size;
	    this->clean_on_pop = false;
	    this->exit_flag = false;
	    this->startup_arg = NULL;

	    /* Initialize the mutex and condition variables */
	    if ((ret = pthread_mutex_init(&(this->queue_lock), NULL)) != 0)
	    {
		syslog(LOG_ERR,
		       "couldn't init %s queue mutex: %s",
		       this->name, strerror(ret));
		free(this->name);
		throw ret;
	    }
	    if ((ret = pthread_cond_init(&(this->queue_not_empty), NULL)) != 0)
	    {
		syslog(LOG_ERR,
		       "couldn't init %s queue not-empty cond: %s",
		       this->name, strerror(ret));
		pthread_mutex_destroy(&(this->queue_lock));
		free(this->name);
		throw ret;
	    }
	    /* We've moved the starting of the threads into the start() call */
	};

    ~ThreadPool()
	{
	    this->stop();
	    pthread_cond_destroy(&(this->queue_not_empty));
	    pthread_mutex_destroy(&(this->queue_lock));
	    syslog(LOG_DEBUG, "destroyed the %s mutexes", this->name);
	    free(this->name);
	};

    void start(void *(*func)(void *))
	throw (int)
	{
	    int ret;
	    pthread_t thread;

	    pthread_mutex_lock(&(this->queue_lock));
	    this->thread_pool.reserve(this->thread_count);
	    this->startup_func = func;
	    /* Make sure we're ready to start */
	    this->exit_flag = false;
	    /* Start up the actual threads */
	    while (this->thread_pool.size() < this->thread_count)
	    {
		if ((ret = pthread_create(&thread,
					  NULL,
					  this->startup_func,
					  this->startup_arg)) != 0)
		{
		    /* Error! */
		    syslog(LOG_ERR,
			   "couldn't start a %s thread: %s",
			   this->name, strerror(ret));
		    /* Something's messed up; stop all the threads */
		    this->stop();
		    throw ret;
		}
		thread_pool.push_back(thread);
	    }
	    pthread_mutex_unlock(&(this->queue_lock));
	};

    void stop(void)
	{
	    this->exit_flag = true;
	    /* Wake everybody up, in case they're sleeping */
	    pthread_cond_broadcast(&(this->queue_not_empty));
	    /* Reap all our child threads */
	    while (this->thread_pool.size() > 0)
	    {
		/* Give up our slice, so the children can die */
		sleep(0);
		pthread_join(this->thread_pool.back(), NULL);
		this->thread_pool.pop_back();
	    }
	};

    unsigned int pool_size(void)
	{
	    return this->thread_pool.size();
	};

    unsigned int queue_size(void)
	{
	    return this->request_queue.size();
	};

    /* Resize the pool */
    void resize(unsigned int new_count)
	throw (int)
	{
	    pthread_mutex_lock(&(this->queue_lock));
	    this->thread_count = new_count;
	    if (this->thread_count < this->thread_pool.size())
	    {
		/* We gotta shrink the pool; cancel and delete the
		 * threads at the end of the array
		 */
		while (this->thread_pool.size() > this->thread_count)
		{
		    pthread_cancel(this->thread_pool.back());
		    /* Give up our slice, just in case it's needed */
		    sleep(0);
		    pthread_join(this->thread_pool.back(), NULL);
		    this->thread_pool.pop_back();
		}
		pthread_mutex_unlock(&(this->queue_lock));
	    }
	    else if (this->thread_count > this->thread_pool.size())
	    {
		/* Grow the pool */
		/* We need to unlock here, because start() grabs the lock */
		pthread_mutex_unlock(&(this->queue_lock));
		/* If there's an error, just keep throwing it */
		try { this->start(this->startup_func); }
		catch (int err) { throw err; }
	    }
	};

    void push(T& req)
	{
	    pthread_mutex_lock(&(this->queue_lock));
	    request_queue.push(req);
	    pthread_cond_signal(&(this->queue_not_empty));
	    pthread_mutex_unlock(&(this->queue_lock));
	};

    /* The worker threads will call into this function, and just wait
     * until it returns, so that's why we have the exit_flag processing
     * taking place here.
     */
    void pop(T *buffer)
	{
	    /* Just in case */
	    if (buffer == NULL)
		return;

	    /* Lock the giant lock for all the thread pool activity */
	    pthread_mutex_lock(&(this->queue_lock));

	    /* If there's nothing to do, wait until there is */
	    while (this->request_queue.empty() && !this->exit_flag)
		pthread_cond_wait(&(this->queue_not_empty),
				  &(this->queue_lock));

	    /* If it's time to exit, go ahead and exit */
	    if (this->exit_flag)
	    {
		syslog(LOG_DEBUG,
		       "%s thread into the exit routine", this->name);
		pthread_mutex_unlock(&(this->queue_lock));
		pthread_exit(NULL);
	    }

	    /* Grab what's at the head of the queue */
	    memcpy(buffer, &(this->request_queue.front()), sizeof(T));
	    /* If we need to clean on pop, do it */
	    if (this->clean_on_pop)
		memset(&(this->request_queue.front()), 0, sizeof(T));
	    this->request_queue.pop();

	    /* We're done mangling the queue, so drop the giant lock */
	    pthread_mutex_unlock(&(this->queue_lock));
	};
};

#endif /* __INC_THREAD_POOL_H__ */
