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
#include <VersionHelpers.h>
#include <tlhelp32.h>
#include <Psapi.h>

#include "timer.hpp"
#include "parse_config.hpp"

struct exec_runner : noncopyable
{
	exec_runner(process_config& info, timer_generator& timer) :
		info_(info), timer_(timer), stop_flag_(false)
	{

	}

	bool init()
	{
		this->_kill_last_processes();
	}


	bool start()
	{
		this->timer_handler_ = this->timer_.set_timer(boost::bind(&exec_runner::timer_delay, this), this->info_.autostart_delay_second);
		return true;
	}

	void stop()
	{
		if (this->timer_handler_ != 0)
		{
			this->timer_.kill_timer(this->timer_handler_);
			this->timer_handler_ = 0;
		}
	}

	void timer_delay()
	{
		SCOPE_EXIT( WRITE_LOG(trace) << "[timer_delay][end]" << this->info_.name );

		WRITE_LOG(trace) << "[timer_delay][begin]" << this->info_.name;

		this->timer_.kill_timer(this->timer_handler_);
		this->timer_handler_ = this->timer_.set_timer(boost::bind(exec_runner::timer_run_exe, this), this->info_.autostart_delay_second, true);
	}

	void timer_run_exe()
	{
		SCOPE_EXIT(WRITE_LOG(trace) << "[timer_run_exe][end]" << this->info_.name );

		WRITE_LOG(trace) << "[timer_run_exe][begin]" << this->info_.name;

		if (this->_check_stop_flag() == true)
		{
			return;
		}

		if (this->_check_process_running())
		{
			return;
		}

		WRITE_LOG(trace) << "timer_run_exe pass check! >> " << this->info_.name;

		bool success = this->_create_process();
	}

private:

	bool _check_stop_flag()
	{
		return this->stop_flag_;
	}

	void _set_stop(bool flag)
	{
		this->stop_flag_ = flag;
	}

	void _stop_process()
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

			this->_kill_processes();
		}
	}

	bool _create_process(bool start, bool wait)
	{
		auto ret = false;

		SCOPE_EXIT(WRITE_LOG(error) << "create process >> " << this->info_.name << " | result >> " << ret;);

		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };

		ZeroMemory(&si, sizeof(si));
		ZeroMemory(&pi, sizeof(pi));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		boost::scoped_array<char> cmd_line_char;
		std::string exe_name;

		if (start == true)
		{
			exe_name = this->info_.process_name;

			cmd_line_char.reset(new char[this->info_.command.length() + 4]);
			sprintf(cmd_line_char.get(), "%s", this->info_.command.c_str());
		}

		BOOL process_created = CreateProcess(exe_name.c_str(), cmd_line_char.get(),
			NULL, NULL, FALSE, 0, NULL, this->info_.directory.c_str(), &si, &pi);

		if (!process_created)
			return false;

		CloseHandle(pi.hThread);

		this->process_handle_ = pi.hProcess;

		if (wait == true) {
			WaitForSingleObject(pi.hProcess, INFINITE);
		}

		return true;
	}

	bool _check_process_running()
	{
		auto ret = false;

		SCOPE_EXIT(WRITE_LOG(trace) << "check process running [" << this->info_.name << "]" << ret; );

		if (this->process_handle_ == 0)
		{
			return ret;
		}

		DWORD exit_code = 0;
		GetExitCodeProcess(this->process_handle_, &exit_code);

		if (exit_code == STILL_ACTIVE)
		{
			ret = true;
			return ret;
		}

		this->_close_handle();
		return ret;
	}

	void _close_handle()
	{
		if (this->process_handle_ == 0)
		{
			return;
		}

		CloseHandle(this->process_handle_);
		this->process_handle_ = 0;
	}

	void _kill_last_processes()
	{
		SCOPE_EXIT(WRITE_LOG(error) << "kill last processes finished >> " << this->info_.name; );

		std::vector<DWORD> pids = this->_find_last_process();
		for (std::vector<DWORD>::iterator iter = pids.begin(); iter != pids.end(); ++iter)
		{
			this->_terminate_process(*iter, 0);
		}
	}


	void _kill_process()
	{
		if (this->process_handle_ != 0)
		{
			WRITE_LOG(trace) << "kill process >> " << this->info_.name << " finished.";
			TerminateProcess(this->process_handle_, 0);
			this->_close_handle();
		}
	}

	void _terminate_process(DWORD pid, UINT exit_code)
	{
		SCOPE_EXIT(WRITE_LOG(trace) << "kill last processes [" << this->info_.name << "] finished.");

		HANDLE process_handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (process_handle == NULL)
		{
			return;
		}

		WRITE_LOG(trace) << "terminate process >>" << this->info_.name << " finished.";
		TerminateProcess(process_handle, exit_code);
		CloseHandle(process_handle);
	}

	std::vector<DWORD> _find_last_process()
	{
		WRITE_LOG(trace) << "find last processes begin >> " << this->info_.name;

		std::vector<DWORD> pids;

		// Take a snapshot of all processes in the system.
		HANDLE process_snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (process_snap == INVALID_HANDLE_VALUE)
		{
			WRITE_LOG(error) << "CreateToolhelp32Snapshot failed!error >> [" << GetLastError() << "], process: >>" << this->info_.name;
			return pids;
		}

		PROCESSENTRY32 pe32 = { 0 };
		pe32.dwSize = sizeof(PROCESSENTRY32);

		BOOL success = ::Process32First(process_snap, &pe32);

		if (success == FALSE)
		{
			WRITE_LOG(error) << "Process32First failed!error >> [" << GetLastError() << "], process: >>" << this->info_.name;
			return pids;
		}

		while (success)
		{
			do
			{
				using boost::filesystem::equivalent;
				using boost::filesystem::path;

				TCHAR szName[2048] = { 0 };
				DWORD nSize = 2048;
				DWORD dwDesiredAccess = PROCESS_QUERY_INFORMATION;

				if (IsWindowsVistaOrGreater())
				{
					dwDesiredAccess = PROCESS_QUERY_LIMITED_INFORMATION;
				}

				HANDLE hProcess = OpenProcess(dwDesiredAccess, FALSE, pe32.th32ProcessID);
				if (!hProcess)
				{
					continue;
				}

				DWORD ret = ::GetProcessImageFileName(hProcess, szName, nSize);
				CloseHandle(hProcess);
				if (ret == FALSE)
				{
					continue;
				}

				std::string strName = DosDevicePath2LogicalPath(szName);

				try
				{
					if (equivalent(path(this->info_.process_name.c_str()), path(strName.c_str())) == false)
					{
						break;
					}
				}
				catch (...)
				{
					// this->exe_name_.c_str() file has not fould
					break;
				}

				std::wstring pe32_name(pe32.szExeFile);
				std::vector<std::wstring>::iterator iter =
					std::find_if(exclude_names_.begin(), exclude_names_.end(),
						boost::bind(boost::contains<std::wstring, std::wstring>, pe32_name, _1));

				if (iter != exclude_names_.end())
				{   // protect the system processes
					logger_->write_error_log(FILE_LINE_FORMAT("process exclude [%s], pid:[%d]"), CW2T(iter->c_str()), pe32.th32ProcessID);
					break;
				}

				pids.push_back(pe32.th32ProcessID);
				logger_->write_error_log(FILE_LINE_FORMAT("kill last [%s][%d] pushed into list."), pe32.szExeFile, pe32.th32ProcessID);

			} while (false);

			success = ::Process32Next(process_snap, &pe32);
		}
		::CloseHandle(process_snap);
		return pids;
	}

	bool stop_flag_;

	process_config info_;
	HANDLE process_handle_;
	timer_generator& timer_;
	unsigned long timer_handler_;
};

class process_manager : noncopyable
{
	void start(std::vector<process_config>& process_info, timer_generator& timer, boost::system::error_code& ec)
	{
		std::for_each(process_info.begin(), process_info.end(), 
			[&] (process_config& info) {

			}
		);

	}
};