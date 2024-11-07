
#include "stdafx.h"
#include "CHandle.h"
#include "Exception.h"

namespace w32
{
    //Initialize an empty handle
    CHandle::CHandle() {}

    //Initialize a CHandle and take ownership
    CHandle::CHandle(HANDLE handle) {
        m_handle = handle;
    }

    //Initialize copy a CHandle
    CHandle::CHandle(CHandle const& other) {
        if (!DuplicateHandle(
            GetCurrentProcess(),
            other.m_handle,
            GetCurrentProcess(),
            &m_handle,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS))
            throw ExWin32Error("CHandle::CHandle(CHandle const& other)");
    }

    //Destroy the CHandle
    CHandle::~CHandle()
    {
        CloseHandle();
    }

    //Close the handle if it was valid
    void CHandle::CloseHandle() {
        if (IsValid()) {
            ::CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

    //Is the internal handle a valid one?
    bool CHandle::IsValid() {
        return (m_handle != NULL && m_handle != INVALID_HANDLE_VALUE);
    }

    //assign a handle and take ownership
    void CHandle::operator = (HANDLE handle) {
        CloseHandle();
        m_handle = handle;
    }

    //assign a handle and take ownership
    void CHandle::operator = (CHandle const& other) {
        if (this != &other) {
            CloseHandle();
            if (!DuplicateHandle(
                GetCurrentProcess(),
                other.m_handle,
                GetCurrentProcess(),
                &m_handle,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS))
                throw ExWin32Error("CHandle::CHandle(CHandle const& other)");
        }
    }

    //cast the class to a handle value for interaction with the win32 api
    CHandle::operator HANDLE () {
        return m_handle;
    }


}