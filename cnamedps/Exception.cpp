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

#include "..\stdafx.h"
#include "Exception.h"

#include "StringHandling.h"

namespace w32
{
	/////////////////////////////////////////////////////////////
	//AppException
	/////////////////////////////////////////////////////////////
	AppException::AppException() : exception(){
	}

	AppException::AppException(wstring const& message) {
		m_What = WStringToString(message);
	}

	AppException::AppException(string const& message) : m_What(message) {
	}

	const char* AppException::what() const noexcept {
		return m_What.c_str();
	}

	/////////////////////////////////////////////////////////////
	//Win32Exception
	/////////////////////////////////////////////////////////////
	Win32Exception::Win32Exception() {
	}

	Win32Exception::Win32Exception(DWORD value) :_value(value) {
	}

	DWORD Win32Exception::Value() {
		return _value;
	}


	/////////////////////////////////////////////////////////////
	//ExNtStatus
	/////////////////////////////////////////////////////////////
	ExNtStatus::ExNtStatus(DWORD value) : Win32Exception(value) {
		stringstream stream;
		stream << "Function returned NTSTATUS " << value << ".";
		m_What = stream.str();
	}

	/////////////////////////////////////////////////////////////
	//ExWin32Error
	/////////////////////////////////////////////////////////////
	ExWin32Error::ExWin32Error() {
		_value = GetLastError();
		stringstream stream;
		stream << "Function returned error code: " << _value << ".";
		m_What = stream.str();
	}

	ExWin32Error::ExWin32Error(DWORD value) : Win32Exception(value) {
		stringstream stream;
		stream << "Function returned error code: " << value << ".";
		m_What = stream.str();
	}

	ExWin32Error::ExWin32Error(wstring const& message) {
		_value = GetLastError();
		stringstream stream;
		stream << "Function returned error code: " << _value << ". " << WStringToString( message);
		m_What = stream.str();
	}

	ExWin32Error::ExWin32Error(string const& message) {
		_value = GetLastError();
		stringstream stream;
		stream << "Function returned error code: " << _value << ". " << message;
		m_What = stream.str();
	}

	ExWin32Error::ExWin32Error(DWORD value, wstring const& message) : Win32Exception(value) {
		stringstream stream;
		stream << "Function returned error code: " << value << ". " << WStringToString( message);
		m_What = stream.str();
	}

	ExWin32Error::ExWin32Error(DWORD value, string const& message) : Win32Exception(value) {
		stringstream stream;
		stream << "Function returned error code: " << value << ". " << message;
		m_What = stream.str();
	}

	/////////////////////////////////////////////////////////////
	//ExHResult
	/////////////////////////////////////////////////////////////

	ExHResult::ExHResult(DWORD value) : Win32Exception(value) {
		stringstream stream;
		stream << "Function returned hresult 0x" << hex << value << ".";
		m_What = stream.str();
	}

	ExHResult::ExHResult(DWORD value, wstring const& message) : Win32Exception(value) {
		stringstream stream;
		stream << "Function returned hresult 0x" <<hex << value << ". ";
		stream << WStringToString(message);
		m_What = stream.str();
	}
	ExHResult::ExHResult(DWORD value, string const& message) : Win32Exception(value) {
		stringstream stream;
		stream << "Function returned hresult 0x" << hex << value << ". " << message;
		m_What = stream.str();
	}
}