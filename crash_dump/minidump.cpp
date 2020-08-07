#include "Minidump.h"
#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#include <Dbghelp.h>

// MiniDumpWriteDump() function declaration (so we can just get the function directly from windows)
typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)
(
	HANDLE hProcess,
	DWORD dwPid,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

std::string gBinaryModule;
namespace died
{
	namespace Minidump
	{
		// Purpose: Creates a new file and dumps the exception info into it
		bool writeDump(PEXCEPTION_POINTERS pExceptionInfo, MINIDUMP_TYPE minidumpType)
		{
			// get the function pointer directly so that we don't have to include the .lib, and that
			// we can easily change it to using our own dll when this code is used on win98/ME/2K machines
			HMODULE hDbgHelpDll = ::LoadLibraryA("DbgHelp.dll");
			if (!hDbgHelpDll) {
				return false;
			}

			BOOL ret = FALSE;
			MINIDUMPWRITEDUMP pfnMiniDumpWrite = (MINIDUMPWRITEDUMP) ::GetProcAddress(hDbgHelpDll, "MiniDumpWriteDump");

			if (pfnMiniDumpWrite)
			{
				std::string outFilename{ gBinaryModule + CRASH_DUMP_FILENAME };
				HANDLE hFile = ::CreateFileA(outFilename.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile)
				{
					// dump the exception information into the file
					PMINIDUMP_EXCEPTION_INFORMATION pExInfo = new MINIDUMP_EXCEPTION_INFORMATION;
					pExInfo->ThreadId = ::GetCurrentThreadId();
					pExInfo->ExceptionPointers = pExceptionInfo;
					pExInfo->ClientPointers = FALSE;

					ret = (*pfnMiniDumpWrite)(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, minidumpType, pExInfo, NULL, NULL);
					::CloseHandle(hFile);
					delete pExInfo;
				}
			}

			::FreeLibrary(hDbgHelpDll);
			return (ret == TRUE);
		}

		long WINAPI unhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
		{
			// First try to write it with all the indirectly referenced memory (ie: a large file).
			// If that doesn't work, then write a smaller one.
			int iType = MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory;
			writeDump(pExceptionInfo, (MINIDUMP_TYPE)iType);
			return 0;
		}

		void startMonitering(const std::string& binDir)
		{
			gBinaryModule = binDir;
			::SetUnhandledExceptionFilter(unhandledExceptionFilter);
		}
	}
}