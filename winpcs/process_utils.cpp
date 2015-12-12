
#include "config.hpp"

#include <Windows.h>
#include <VersionHelpers.h>
#include <tlhelp32.h>
#include <Psapi.h>

#include "process_utils.h"

namespace process_utils {


void terminate_process(DWORD pid, UINT exit_code)
{
    SCOPE_EXIT(WRITE_LOG(trace) << "kill last processes [" << pid << "] finished.");

    HANDLE process_handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (process_handle == NULL)
    {
        return;
    }

    WRITE_LOG(trace) << "terminate process >>" << pid << " finished.";
    TerminateProcess(process_handle, exit_code);
    CloseHandle(process_handle);
}


std::vector<DWORD> find_child_process(DWORD pid)
{
    SCOPE_EXIT(WRITE_LOG(trace) << "find child processes end >> " << pid; );

    std::vector<DWORD> pids;
    WRITE_LOG(trace) << "find child processes begin >> " << pid;

    // Take a snapshot of all processes in the system.
    HANDLE process_snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (process_snap == INVALID_HANDLE_VALUE)
    {
        WRITE_LOG(error) << "CreateToolhelp32Snapshot failed >> " << pid << "| error :" << GetLastError();
        return pids;
    }

    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32);

    BOOL success = ::Process32First(process_snap, &pe32);

    if (success == FALSE)
    {
        WRITE_LOG(error) << "Process32First failed >> " << pid << "| error :" << GetLastError();
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

            if (process_utils::is_exclude(std::string(pe32.szExeFile)))
            {
                WRITE_LOG(trace) << "process exclude >> " << pe32.szExeFile << " | parent id >> " << pid << " | pid >> " << pe32.th32ProcessID;
                break;
            }

            pids.push_back(pe32.th32ProcessID);
            WRITE_LOG(trace) << "find child >> " << pe32.szExeFile << " | pid >> " << pe32.th32ProcessID;
        } while (false);

        success = ::Process32Next(process_snap, &pe32);
    }
    ::CloseHandle(process_snap);
    return pids;
}


std::vector<DWORD> find_last_process(const std::string& process_name)
{
    SCOPE_EXIT(WRITE_LOG(trace) << "find last processes end >>" << process_name; );

    WRITE_LOG(trace) << "find last processes begin >> " << process_name;

    std::vector<DWORD> pids;

    // Take a snapshot of all processes in the system.
    HANDLE process_snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (process_snap == INVALID_HANDLE_VALUE)
    {
        WRITE_LOG(error) << "CreateToolhelp32Snapshot failed!error >> [" << GetLastError() << "], process: >>" << process_name;
        return pids;
    }

    PROCESSENTRY32 pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32);

    BOOL success = ::Process32First(process_snap, &pe32);

    if (success == FALSE)
    {
        WRITE_LOG(error) << "Process32First failed!error >> [" << GetLastError() << "], process: >>" << process_name;
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

            std::string strName = dos_device_path2logical_path(szName);

            try
            {
                if (equivalent(path(process_name.c_str()), path(strName.c_str())) == false)
                {
                    break;
                }
            }
            catch (...)
            {
                // process_name.c_str() file has not fould
                break;
            }

            if (process_utils::is_exclude(std::string(pe32.szExeFile)))
            {   // protect the system processes
                WRITE_LOG(trace) << "process exclude >> " << pe32.szExeFile;
                break;
            }

            pids.push_back(pe32.th32ProcessID);

        } while (false);

        success = ::Process32Next(process_snap, &pe32);
    }
    ::CloseHandle(process_snap);

    return pids;
}


std::string dos_device_path2logical_path(const char* lpszDosPath)
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


bool create_process(std::string& process_name, std::string& command, std::string& directory, unsigned long& pid, HANDLE& handle)
{
    auto ret = false;

    SCOPE_EXIT(WRITE_LOG(error) << "create process >> " << process_name << " | result >> " << ret;);

    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    boost::scoped_array<char> cmd_line_char;

    size_t cmd_line_size = command.length() + 1;
    cmd_line_char.reset(new char[cmd_line_size]);
    strcpy_s(cmd_line_char.get(), cmd_line_size, command.c_str());

    BOOL process_created = CreateProcess(process_name.c_str(), cmd_line_char.get(),
        NULL, NULL, FALSE, 0, NULL, directory.c_str(), &si, &pi);

    if (!process_created)
        return ret;

    ret = true;

    CloseHandle(pi.hThread);

    handle = pi.hProcess;
    pid = pi.dwProcessId;

    return ret;
}

bool is_exclude(const std::string& pe32_name)
{
    static std::vector<std::string> exclude_names = { "csrss.exe" , "lsass.exe" , "smss.exe" , "services.exe" , "svchost.exe" , "wininit.exe" , "winlogon.exe" };
    auto iter = std::find_if(exclude_names.begin(), exclude_names.end(),
            boost::bind(boost::contains<std::string, std::string>, pe32_name, _1));

    if (iter != exclude_names.end())
    {
        return true;
    }

    return false;
}


void kill_last_processes(const std::string& process_name)
{
    SCOPE_EXIT(WRITE_LOG(error) << "kill last processes finished >> " << process_name; );

    std::vector<DWORD> pids = process_utils::find_last_process(process_name);
    for (std::vector<DWORD>::iterator iter = pids.begin(); iter != pids.end(); ++iter)
    {
        process_utils::terminate_process(*iter, 0);
    }
}


void kill_processes(HANDLE process_handle, unsigned long pid)
{
    SCOPE_EXIT(WRITE_LOG(trace) << "kill tree end. pid >> " << pid; );

    WRITE_LOG(trace) << "kill tree begin. pid >> " << pid;

    std::vector<DWORD> pids = process_utils::find_child_process(pid);

    TerminateProcess(process_handle, 0);

    for (std::vector<DWORD>::iterator iter = pids.begin(); iter != pids.end(); ++iter)
    {
        process_utils::terminate_process(*iter, 0);
    }
}

}
