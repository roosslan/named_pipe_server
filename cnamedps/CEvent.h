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
#include "CHandle.h"

namespace w32
{
    class CEvent : public CHandle
    {
    public:
        CEvent(
            LPSECURITY_ATTRIBUTES lpEventAttributes,
            BOOL bManualReset,
            BOOL bInitialState,
            LPCWSTR lpName);

        CEvent(
            DWORD dwDesiredAccess,
            BOOL bInheritHandle,
            LPCWSTR lpName);

        void SetEvent();

        void ResetEvent();

        static HANDLE w32_CreateEvent(
            LPSECURITY_ATTRIBUTES lpEventAttributes,
            BOOL bManualReset,
            BOOL bInitialState,
            LPCWSTR lpName);

        static HANDLE CreateManualResetEvent(
            BOOL bInitialState = FALSE,
            LPCWSTR lpName = NULL,
            LPSECURITY_ATTRIBUTES lpEventAttributes = NULL);

        static HANDLE CreateAutoResetEvent(
            BOOL bInitialState = FALSE,
            LPCWSTR lpName = NULL,
            LPSECURITY_ATTRIBUTES lpEventAttributes = NULL);

        static HANDLE w32_OpenEvent(
            DWORD dwDesiredAccess,
            BOOL bInheritHandle,
            LPCWSTR lpName);
    };

    class CManualResetEvent : public CEvent
    {
    public:
        CManualResetEvent(
            BOOL bInitialState = FALSE,
            LPCWSTR lpName = NULL,
            LPSECURITY_ATTRIBUTES lpEventAttributes = NULL) :
            CEvent(lpEventAttributes, TRUE, bInitialState, lpName) {}
    };

    class CAutoResetEvent : public CEvent
    {
    public:
        CAutoResetEvent(
            BOOL bInitialState = FALSE,
            LPCWSTR lpName = NULL,
            LPSECURITY_ATTRIBUTES lpEventAttributes = NULL) :
            CEvent(lpEventAttributes, FALSE, bInitialState, lpName) {}
    };
}
