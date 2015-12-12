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
	process_mgr.hpp for process's manage.
*/
#pragma once

#include "config.hpp"

#include <Windows.h>

#include "process_utils.h"
#include "timer.h"
#include "parse_config.h"
#include "http_struct.hpp"


struct exec_runner : boost::noncopyable
{
    exec_runner(process_config& info, timer_generator& timer) :
        info_(info), timer_(timer), stop_flag_(false),
        exit_code_(0), process_id_(0), process_handle_(0)
    {

    }

    ~exec_runner(void);

    bool init();
    bool start();
    void stop();

    void timer_delay();
    void timer_run_exe();

    unsigned long exit_code();
    process_config& get_info();
    unsigned long process_id();

private:

    void _flush_exit_code();
    void _stop_process();
    bool _check_process_running();
    void _close_handle();
    void _kill_process();

    void _kill_timer();
    bool _check_stop_flag();
    void _set_stop(bool flag);


    bool stop_flag_;

    process_config info_;
    HANDLE process_handle_;
    unsigned long process_id_;
    unsigned long exit_code_;
    timer_generator& timer_;
    unsigned long timer_handler_;
};

class process_manager : boost::noncopyable
{
public:
    void start(std::vector<process_config>& process_info, 
        timer_generator& timer, boost::system::error_code& ec);
    void stop();
    std::vector<process_status> status(unsigned long pid);

private:
	std::vector<boost::shared_ptr<exec_runner> > runners_;
};

