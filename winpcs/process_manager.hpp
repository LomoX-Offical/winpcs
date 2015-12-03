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
		info_(info), timer_(timer), stop_flag_(false),
		exclude_names_({ "csrss.exe" , "lsass.exe" , "smss.exe" , "services.exe" , "svchost.exe" , "wininit.exe" , "winlogon.exe" })
	{

	}

	bool init()
	{
		this->_kill_last_processes();
		return true;
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
		this->timer_handler_ = this->timer_.set_timer(boost::bind(&exec_runner::timer_run_exe, this), this->info_.autostart_delay_second, true);
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


	void _kill_processes()
	{
		SCOPE_EXIT( WRITE_LOG(trace) << "kill tree end >> " << this->info_.name; );

		WRITE_LOG(trace) << "kill tree begin >> " << this->info_.name;

		if (this->process_handle_ != 0)
		{
			DWORD pid = ::GetProcessId(this->process_handle_);
			std::vector<DWORD> pids = this->_find_child_process(pid);

			TerminateProcess(this->process_handle_, 0);
			this->_close_handle();

			for (std::vector<DWORD>::iterator iter = pids.begin(); iter != pids.end(); ++iter)
			{
				this->_terminate_process(*iter, 0);
			}
		}
	}

	bool _create_process(bool wait = false)
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

		exe_name = this->info_.process_name;

		cmd_line_char.reset(new char[this->info_.command.length() + 4]);
		sprintf_s(cmd_line_char.get(), this->info_.command.length() + 3, "%s", this->info_.command.c_str());

		BOOL process_created = CreateProcess(exe_name.c_str(), cmd_line_char.get(),
			NULL, NULL, FALSE, 0, NULL, this->info_.directory.c_str(), &si, &pi);

		if (!process_created)
			return ret;

		ret = true;

		CloseHandle(pi.hThread);

		this->process_handle_ = pi.hProcess;

		if (wait == true) {
			WaitForSingleObject(pi.hProcess, INFINITE);
		}

		return ret;
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


	std::vector<DWORD> _find_child_process(DWORD pid)
	{
		SCOPE_EXIT( WRITE_LOG(trace) << "find child processes end >> " << this->info_.name; );

		std::vector<DWORD> pids;
		WRITE_LOG(trace) << "find child processes begin >> " <<  this->info_.name;

		// Take a snapshot of all processes in the system.
		HANDLE process_snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (process_snap == INVALID_HANDLE_VALUE)
		{
			WRITE_LOG(error) << "CreateToolhelp32Snapshot failed >> " << this->info_.name << "| error :" << GetLastError();
			return pids;
		}

		PROCESSENTRY32 pe32 = { 0 };
		pe32.dwSize = sizeof(PROCESSENTRY32);

		BOOL success = ::Process32First(process_snap, &pe32);

		if (success == FALSE)
		{
			WRITE_LOG(error) << "Process32First failed >> " << this->info_.name << "| error :" << GetLastError();
			return pids;
		}


		while (success)
		{
			do
			{
				if (pe32.th32ParentProcessID != pid)
				{
					break;
				}

				std::string pe32_name(pe32.szExeFile);
				std::vector<std::string>::iterator iter =
					std::find_if(this->exclude_names_.begin(), this->exclude_names_.end(),
						boost::bind(boost::contains<std::string, std::string>, pe32_name, _1));

				if (iter != this->exclude_names_.end())
				{   // protect the system processes
					WRITE_LOG(trace) << "process exclude >> " << iter->c_str() << " | parent id >> " << pid << " | pid >> " << pe32.th32ProcessID << " | name >> " << this->info_.name;
					break;
				}

				pids.push_back(pe32.th32ProcessID);
				WRITE_LOG(trace) << "find child >> " << pe32.szExeFile << " | pid >> " << pe32.th32ProcessID << " | name >> " << this->info_.name;
			} while (false);

			success = ::Process32Next(process_snap, &pe32);
		}
		::CloseHandle(process_snap);
		return pids;
	}



	std::vector<DWORD> _find_last_process()
	{
		SCOPE_EXIT(WRITE_LOG(trace) << "find last processes end >>" << this->info_.name; );

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

				std::string strName = _dos_device_path2logical_path(szName);

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

				std::string pe32_name(pe32.szExeFile);
				auto iter = std::find_if(exclude_names_.begin(), exclude_names_.end(),
						boost::bind(boost::contains<std::string, std::string>, pe32_name, _1));

				if (iter != exclude_names_.end())
				{   // protect the system processes
					WRITE_LOG(trace) << "process exclude >> " << iter->c_str() << " | process name >> " << this->info_.process_name << " | pid >> " << pe32.th32ProcessID << " | name >> " << this->info_.name;
					break;
				}

				pids.push_back(pe32.th32ProcessID);

			} while (false);

			success = ::Process32Next(process_snap, &pe32);
		}
		::CloseHandle(process_snap);

		return pids;
	}


	std::string _dos_device_path2logical_path(const TCHAR* lpszDosPath)
	{
		std::string strResult;

		// Translate path with device name to drive letters.

		char szTemp[MAX_PATH];
		szTemp[0] = '\0';

		if (lpszDosPath == NULL || !GetLogicalDriveStrings(_countof(szTemp) - 1, szTemp))
		{
			return strResult;
		}

		char szName[MAX_PATH];
		char szDrive[3] = " :";
		BOOL  bFound = FALSE;
		char* p = szTemp;

		do {
			// Copy the drive letter to the template string
			*szDrive = *p;

			// Look up each device name
			if (QueryDosDevice(szDrive, szName, _countof(szName)))
			{
				UINT uNameLen = (UINT)strlen(szName);
				if (uNameLen < MAX_PATH)
				{

					bFound = _strnicmp(lpszDosPath, szName, uNameLen) == 0;
					if (bFound) {

						// Reconstruct pszFilename using szTemp
						// Replace device path with DOS path
						char szTempFile[MAX_PATH];
						sprintf_s(szTempFile, "%s%s", szDrive, lpszDosPath + uNameLen);
						strResult = szTempFile;
					}
				}
			}

			// Go to the next NULL character.
			while (*p++);
		} while (!bFound && *p); // end of string

		return strResult;
	}


	bool stop_flag_;

	process_config info_;
	HANDLE process_handle_;
	timer_generator& timer_;
	unsigned long timer_handler_;
	std::vector<std::string> exclude_names_;
};

class process_manager : noncopyable
{
public:
	void start(std::vector<process_config>& process_info, timer_generator& timer, boost::system::error_code& ec)
	{
		std::for_each(process_info.begin(), process_info.end(), 
			[&] (process_config& info) {
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

	std::vector<boost::shared_ptr<exec_runner> > runners_;
};