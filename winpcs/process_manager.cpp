#pragma once
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
	process_mgr.cpp for process's manage.
*/
#pragma once

#include "process_manager.h"

exec_runner::~exec_runner(void)
{
    this->stop();
}

unsigned long exec_runner::exit_code()
{
    return this->exit_code_;
}

process_config& exec_runner::get_info()
{
    return info_;
}

unsigned long exec_runner::process_id() {
    return process_id_;
}

bool exec_runner::init()
{
    process_utils::kill_last_processes(this->info_.process_name);
    return true;
}


bool exec_runner::start()
{
    this->timer_handler_ = this->timer_.set_timer(boost::bind(&exec_runner::timer_delay, this), this->info_.autostart_delay_second);
    return true;
}

void exec_runner::stop()
{
    this->_kill_timer();
    this->_stop_process();
}

void exec_runner::timer_delay()
{
    SCOPE_EXIT(WRITE_LOG(trace) << "[timer_delay][end]" << this->info_.name);

    WRITE_LOG(trace) << "[timer_delay][begin]" << this->info_.name;

    this->timer_.kill_timer(this->timer_handler_);
    this->timer_handler_ = this->timer_.set_timer(boost::bind(&exec_runner::timer_run_exe, this), this->info_.startsecs, true);
}

void exec_runner::timer_run_exe()
{
    SCOPE_EXIT(WRITE_LOG(trace) << "[timer_run_exe][end]" << this->info_.name);

    WRITE_LOG(trace) << "[timer_run_exe][begin]" << this->info_.name;

    if (this->_check_stop_flag() == true)
    {
        return;
    }

    this->_flush_exit_code();
    if (this->_check_process_running())
    {
        return;
    }

    WRITE_LOG(trace) << "timer_run_exe pass check! >> " << this->info_.name;


    bool success = process_utils::create_process(
        this->info_.process_name,
        this->info_.command,
        this->info_.directory,
        this->process_id_,
        this->process_handle_);

    this->_flush_exit_code();
}

void exec_runner::_flush_exit_code()
{
    if (this->process_handle_ == 0)
    {
        return;
    }

    unsigned long code = 0;
    GetExitCodeProcess(this->process_handle_, &code);
    this->exit_code_ = code;
    if (code != STILL_ACTIVE)
    {
        this->_close_handle();
    }
}

void exec_runner::_kill_timer()
{
    if (this->timer_handler_ != 0)
    {
        this->timer_.kill_timer(this->timer_handler_);
        this->timer_handler_ = 0;
    }
}

bool exec_runner::_check_stop_flag()
{
    return this->stop_flag_;
}

void exec_runner::_set_stop(bool flag)
{
    this->stop_flag_ = flag;
}

void exec_runner::_stop_process()
{
    SCOPE_EXIT(WRITE_LOG(trace) << "stop process [end] >> " << this->info_.name; );
    WRITE_LOG(trace) << "stop process [begin] >> " << this->info_.name;

    if (strcmp(this->info_.stopsignal.c_str(), "%kill%") == 0)
    {
        if (this->_check_process_running() == false)
        {
            return;
        }

        this->_kill_process();
    }
    else if (strcmp(this->info_.stopsignal.c_str(), "%kill_tree%") == 0)
    {
        if (this->_check_process_running() == false)
        {
            return;
        }

        if (this->process_handle_ != NULL) {
            process_utils::kill_processes(this->process_handle_, this->process_id_);
            this->_close_handle();
        }
    }
}



bool exec_runner::_check_process_running()
{
    auto ret = false;

    SCOPE_EXIT(WRITE_LOG(trace) << "check process running >> " << this->info_.name << " | running >> " << ret; );

    if (this->process_handle_ == 0)
    {
        return ret;
    }

    ret = true;
    return ret;
}

void exec_runner::_close_handle()
{
    if (this->process_handle_ == 0)
    {
        return;
    }

    CloseHandle(this->process_handle_);
    this->process_handle_ = 0;
    this->process_id_ = 0;
}


void exec_runner::_kill_process()
{
    if (this->process_handle_ != 0)
    {
        WRITE_LOG(trace) << "kill process >> " << this->info_.name << " finished.";
        TerminateProcess(this->process_handle_, 0);
        this->_close_handle();
    }
}


void process_manager::start(std::vector<process_config>& process_info, timer_generator& timer, boost::system::error_code& ec)
{
    std::for_each(process_info.begin(), process_info.end(),
        [&](process_config& info) {
        auto tmp = boost::make_shared<exec_runner>(info, timer);
        tmp->init();
        this->runners_.push_back(tmp);
    }
    );

    std::for_each(this->runners_.begin(), this->runners_.end(),
        [&](boost::shared_ptr<exec_runner>& runner) {
        runner->start();
    }
    );
}

void process_manager::stop()
{
    std::for_each(this->runners_.begin(), this->runners_.end(),
        [&](boost::shared_ptr<exec_runner>& runner) {
        runner->stop();
    }
    );
}

std::vector<process_status> process_manager::status(unsigned long pid)
{
    std::vector<process_status> res;

    std::for_each(this->runners_.begin(), this->runners_.end(), [&](boost::shared_ptr<exec_runner>& runner)
    {
        process_status ps;
        ps.command = runner->get_info().command;
        ps.directory = runner->get_info().directory;
        ps.exit_code = runner->exit_code();
        ps.name = runner->get_info().name;
        ps.process_name = runner->get_info().process_name;
        ps.pid = runner->process_id();
        ps.environment = runner->get_info().environment;

        res.push_back(ps);
    });

    return res;
}


