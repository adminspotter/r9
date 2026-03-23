/* thread_pool.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2006-2026  Trinity Annabelle Quirk
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
 * hosts, we can expand our number of proceses to fit.  A shrink
 * operation is going to require a large amount of scaffolding, so we
 * won't be doing that at this time.
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

#include <vector>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <system_error>

#include "log.h"

template <class T>
class ThreadPool
{
  private:
    std::string name;
    std::vector<std::thread> thread_pool;
    std::mutex queue_lock;
    std::condition_variable queue_not_empty;
    void (*startup_func)(void *);
    std::queue<T> request_queue;
    unsigned int thread_count;
    bool exit_flag;
  public:
    bool clean_on_pop;
    void *startup_arg;

  public:
    ThreadPool(const char *pool_name, unsigned int pool_size)
        : name(pool_name, 0, 8), thread_pool(), queue_lock(),
          queue_not_empty(), request_queue()
        {
            int ret;

            this->thread_count = pool_size;
            this->clean_on_pop = false;
            this->exit_flag = false;
            this->startup_arg = NULL;
        };

    virtual ~ThreadPool() { this->stop(); };

    virtual void start(void (*func)(void *), void *arg = NULL)
        {
            std::scoped_lock lock(this->queue_lock);
            this->thread_pool.reserve(this->thread_count);
            if (arg != NULL)
                this->startup_arg = arg;
            this->startup_func = func;
            this->exit_flag = false;
            while (this->thread_pool.size() < this->thread_count)
            {
                try
                {
                    this->thread_pool.push_back(
                        std::thread(this->startup_func, this->startup_arg)
                    );
                }
                catch (std::system_error& e)
                {
                    /* Something's messed up; stop all the threads */
                    this->stop();
                    throw;
                }
            }
        };

    void stop(void)
        {
            this->exit_flag = true;
            this->queue_not_empty.notify_all();
            while (this->thread_pool.size() > 0)
            {
                /* Give up our slice, so the children can die */
                sleep(0);
                this->thread_pool.back().join();
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

    void grow(unsigned int new_count)
        {
            if (new_count > this->thread_pool.size())
            {
                this->thread_count = new_count;
                this->start(this->startup_func);
            }
        };

    virtual void push(T& req)
        {
            {
                std::scoped_lock lock(this->queue_lock);
                this->request_queue.push(req);
            }
            this->queue_not_empty.notify_one();
        };

    virtual bool pop(T *buffer)
        {
            if (buffer == NULL)
                return false;

            std::unique_lock lock(this->queue_lock);

            while (this->request_queue.empty() && !this->exit_flag)
                this->queue_not_empty.wait(lock);

            if (this->exit_flag)
                return false;

            memcpy(buffer, &(this->request_queue.front()), sizeof(T));
            if (this->clean_on_pop)
                memset(&(this->request_queue.front()), 0, sizeof(T));
            this->request_queue.pop();
            return true;
        };
};

#endif /* __INC_THREAD_POOL_H__ */
