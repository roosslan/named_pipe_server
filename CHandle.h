#pragma once

#include "PlatformIncludes.h"


namespace w32
{

    /// <summary>
    /// This handle wraps Windows HANDLE values for lifecycle management
    /// </summary>
    class CHandle
    {
    protected:
        HANDLE m_handle = NULL;
    public:
        //Create an invalid handle
        CHandle();

        //Initialize a handle object and assume ownership of the handle
        CHandle(HANDLE handle);

        //duplicate the handle
        CHandle(CHandle const& handle);
        virtual ~CHandle();

        //Close the internal handle
        virtual void CloseHandle();

        //Is the internal HANDLE a valid one?
        bool IsValid();

        //Assign a HANDLE and assume ownership
        void operator = (HANDLE handle);

        //Assign a CHandle and duplicate it
        void operator = (CHandle const& handle);

        //cast to a HANDLE for use with Windows APIs
        operator HANDLE ();

    };


}