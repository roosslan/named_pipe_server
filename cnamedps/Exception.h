//Copyright (c) 2022 Bruno van Dooren
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.


#pragma once

#include "PlatformIncludes.h"


namespace w32
{
	//Base class for regular application exceptions
	//Because applications can use both string and wstring
	//we support both types.
	class AppException : public exception
	{
	protected:
		string m_What;
	public:
		AppException();
		AppException(string const& message);
		AppException(wstring const& message);

		const char* what() const noexcept override;
	};

	//Base class for exceptions raised in interaction with the Windows
	//subsystem.
	class Win32Exception : public AppException {
	protected:
		DWORD _value = 0;
	public:
		Win32Exception();
		Win32Exception(DWORD value);
		virtual DWORD Value();
	};

	//Class for errors that return as NTSTATUS codes which are
	//different from HRESULT or error codes
	class ExNtStatus : public Win32Exception {
	public:
		ExNtStatus(DWORD value);
	};

	//Class for errors that are returned as error codes
	//There are a bunch of different constructors because
	//either we supply the value or it is fetched from GetLastError
	//and then we can supply a message or no, as string or wstring.
	class ExWin32Error : public Win32Exception {
	public:
		ExWin32Error();
		ExWin32Error(wstring const& message);
		ExWin32Error(string const& message);

		ExWin32Error(DWORD value);
		ExWin32Error(DWORD value, wstring const& message);
		ExWin32Error(DWORD value, string const& message);
	};

	//Class for errors that are returned as HRESULT
	//There is no default constructor because we always need to supply the HRESULT
	class ExHResult : public Win32Exception {
	public:
		ExHResult(DWORD value);
		ExHResult(DWORD value, wstring const& message);
		ExHResult(DWORD value, string const& message);
	};
}