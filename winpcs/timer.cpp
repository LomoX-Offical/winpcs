/*
  Copyright (c) 2015, Ben Cheung (zqzjz1982@gmail.com)
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of winpcs nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES AND SHANE GRANT BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
	timer.cpp for timer generator.
*/

#include "timer.h"


time_duration_t timer_item::interval() const
{
    return interval_;
}

void timer_item::start()
{
    if (!this->do_once_immediately_)
    {
        this->timer_.reset(new timer_t(asio_service_, interval_));
    }
    else
    {
        this->timer_.reset(new timer_t(asio_service_, second_t(0)));
    }

    this->timer_->async_wait(boost::bind(&timer_item::run, shared_from_this(), boost::asio::placeholders::error));
}

functor_type timer_item::func()
{
    return func_;
}

void timer_item::cancel()
{
    this->timer_->cancel();
}

void timer_item::run(const boost::system::error_code &error)
{
    if (error != 0) {
        return;
    }

    this->timer_->expires_at(this->timer_->expires_at() + this->interval_);
    this->timer_->async_wait(boost::bind(&timer_item::run, shared_from_this(), boost::asio::placeholders::error));
    this->func_();
}


timer_generator::timer_generator() :
    max_timer_id_(1),
    stopped_(0)
{
    asio_work_.reset(new boost::asio::io_service::work(asio_service_));
}

timer_generator::~timer_generator(void)
{
    stop();
}

int timer_generator::start()
{
    thread_.reset(new boost::thread(boost::bind(&timer_generator::thread_loop, this)));
    return 0;
}

void timer_generator::stop()
{
    if (stopped_.exchange(1, boost::memory_order_relaxed) == 1)
    {
        return;
    }

    for (timers_container::iterator iter = timers_.begin(); iter != timers_.end(); ++iter)
    {
        iter->second->cancel();
    }

    asio_service_.stop();
    asio_work_.reset();
    this->wait();

    // clean timers when stopping
    this->timers_.clear();
}

void timer_generator::wait()
{
    if (thread_ != 0)
    {
        thread_->join();
    }
}

void timer_generator::kill_timer(unsigned long id)
{
    if (stopped_ == 1)
    {
        return;
    }

    LOCK lock(mutex_);

    timers_container::iterator iter = timers_.find(id);
    if (iter != timers_.end())
    {
        iter->second->cancel();
        timers_.erase(iter);
    }
}

// thread function
void timer_generator::thread_loop()
{
    asio_service_.run();
}
