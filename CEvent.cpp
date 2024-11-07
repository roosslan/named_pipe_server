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

#include "stdafx.h"
#include "CEvent.h"
#include "Exception.h"

namespace w32
{

    CEvent::CEvent(
        LPSECURITY_ATTRIBUTES lpEventAttributes,
        BOOL bManualReset,
        BOOL bInitialState,
        LPCWSTR lpName) :
        CHandle(w32_CreateEvent(
            lpEventAttributes,
            bManualReset,
            bInitialState,
            lpName))
    {

    }

    CEvent::CEvent(
        DWORD dwDesiredAccess,
        BOOL bInheritHandle,
        LPCWSTR lpName) :
        CHandle(w32_OpenEvent(
            dwDesiredAccess,
            bInheritHandle,
            lpName))
    {

    }

    void CEvent::SetEvent()
    {
        if (!::SetEvent(m_handle))
            throw ExWin32Error("CEvent::SetEvent()");
    }

    void CEvent::ResetEvent()
    {
        if (!::ResetEvent(m_handle))
            throw ExWin32Error("CEvent::ResetEvent()");
    }

    HANDLE CEvent::w32_CreateEvent(
        LPSECURITY_ATTRIBUTES lpEventAttributes,
        BOOL bManualReset,
        BOOL bInitialState,
        LPCWSTR lpName)
    {
        HANDLE newHandle = CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
        if (NULL == newHandle) {
            throw ExWin32Error("CreateEventW");
        }
        return newHandle;
    }

    HANDLE CEvent::CreateManualResetEvent(
        BOOL bInitialState,
        LPCWSTR lpName,
        LPSECURITY_ATTRIBUTES lpEventAttributes)
    {
        return w32_CreateEvent(lpEventAttributes, TRUE, bInitialState, lpName);
    }

    HANDLE CEvent::CreateAutoResetEvent(
        BOOL bInitialState,
        LPCWSTR lpName,
        LPSECURITY_ATTRIBUTES lpEventAttributes)
    {
        return w32_CreateEvent(lpEventAttributes, FALSE, bInitialState, lpName);
    }

    HANDLE CEvent::w32_OpenEvent(
        DWORD dwDesiredAccess,
        BOOL bInheritHandle,
        LPCWSTR lpName)
    {
        HANDLE newHandle = OpenEventW(dwDesiredAccess, bInheritHandle, lpName);
        if (NULL == newHandle) {
            throw ExWin32Error("OpenEventW");
        }
        return newHandle;
    }
}
