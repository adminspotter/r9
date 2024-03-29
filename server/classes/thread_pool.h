/* thread_pool.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Oct 2021, 09:02:12 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2021  Trinity Annabelle Quirk
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
 * This thread pool is able to grow on the fly, starting up new
 * threads as necessary.  This is with an eye to using a
 * dynamically-expandable pool of servers; if we expand our pool of
 * hosts, we can expand our number of proceses to fit.  Unfortunately
 * pthreads is unable to kill off a single specific thread without a
 * lot of scaffolding in place, so a shrink operation is out of the
 * question.
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
 *   grow(int size)
 *       resizes the thread pool to have <size> threads
 *
 * Things to do
 *   - Might we need a way to gun a stuck thread?
 *
 */

#ifndef __INC_THREAD_POOL_H__
#define __INC_THREAD_POOL_H__

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <system_error>

#include "log.h"

template <class T>
class ThreadPool
{
  private:
    std::string name;
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
        : name(pool_name, 0, 8), thread_pool(), request_queue()
        {
            int ret;

            this->thread_count = pool_size;
            this->clean_on_pop = false;
            this->exit_flag = false;
            this->startup_arg = NULL;

            if ((ret = pthread_mutex_init(&(this->queue_lock), NULL)) != 0)
            {
                std::ostringstream s;

                s << "couldn't init " << this->name << " queue mutex";
                throw std::system_error(ret, std::generic_category(), s.str());
            }
            if ((ret = pthread_cond_init(&(this->queue_not_empty), NULL)) != 0)
            {
                std::ostringstream s;

                s << "couldn't init " << this->name << " queue not-empty cond";
                pthread_mutex_destroy(&(this->queue_lock));
                throw std::system_error(ret, std::generic_category(), s.str());
            }
        };

    virtual ~ThreadPool()
        {
            this->stop();

            /* We won't throw exceptions out of a destructor, so no
             * point in checking return values here.  There's nothing
             * we can do anyway.
             */
            pthread_cond_destroy(&(this->queue_not_empty));
            pthread_mutex_destroy(&(this->queue_lock));
        };

    virtual void start(void *(*func)(void *))
        {
            int ret;
            pthread_t thread;

            pthread_mutex_lock(&(this->queue_lock));
            this->thread_pool.reserve(this->thread_count);
            this->startup_func = func;
            this->exit_flag = false;
            while (this->thread_pool.size() < this->thread_count)
            {
                if ((ret = pthread_create(&thread,
                                          NULL,
                                          this->startup_func,
                                          this->startup_arg)) != 0)
                {
                    std::ostringstream s;

                    s << "couldn't start a " << this->name << " thread";
                    /* Something's messed up; stop all the threads */
                    pthread_mutex_unlock(&(this->queue_lock));
                    this->stop();
                    throw std::system_error(ret,
                                            std::generic_category(),
                                            s.str());
                }
                this->thread_pool.push_back(thread);
            }
            pthread_mutex_unlock(&(this->queue_lock));
        };

    void stop(void)
        {
            this->exit_flag = true;
            pthread_cond_broadcast(&(this->queue_not_empty));
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
    void grow(unsigned int new_count)
        {
            if (new_count > this->thread_pool.size())
            {
                pthread_mutex_lock(&(this->queue_lock));
                this->thread_count = new_count;
                pthread_mutex_unlock(&(this->queue_lock));
                this->start(this->startup_func);
            }
        };

    virtual void push(T& req)
        {
            pthread_mutex_lock(&(this->queue_lock));
            this->request_queue.push(req);
            pthread_cond_signal(&(this->queue_not_empty));
            pthread_mutex_unlock(&(this->queue_lock));
        };

    /* The worker threads will call into this function, and just wait
     * until it returns, so that's why we have the exit_flag processing
     * taking place here.
     */
    virtual void pop(T *buffer)
        {
            if (buffer == NULL)
                return;

            pthread_mutex_lock(&(this->queue_lock));

            while (this->request_queue.empty() && !this->exit_flag)
                pthread_cond_wait(&(this->queue_not_empty),
                                  &(this->queue_lock));

            if (this->exit_flag)
            {
                pthread_mutex_unlock(&(this->queue_lock));
                pthread_exit(NULL);
            }

            memcpy(buffer, &(this->request_queue.front()), sizeof(T));
            if (this->clean_on_pop)
                memset(&(this->request_queue.front()), 0, sizeof(T));
            this->request_queue.pop();

            pthread_mutex_unlock(&(this->queue_lock));
        };
};

#endif /* __INC_THREAD_POOL_H__ */
