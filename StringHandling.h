#pragma once
#include "PlatformIncludes.h"

namespace w32
{
    /// <summary>
/// Get the minimum required size for the string + termination
/// </summary>
/// <param name="charStr">input string</param>
/// <returns>required buffer size</returns>
    inline size_t GetReqBufSize(PCHAR charStr) {
        return strlen(charStr) + 1;
    }

    /// <summary>
    /// Get the minimum required size for the string + termination
    /// </summary>
    /// <param name="charStr">input string</param>
    /// <returns>required buffer size</returns>
    inline size_t GetReqBufSize(PWCHAR charStr) {
        return (lstrlenW(charStr) + 1) * sizeof(WCHAR);
    }

    /// <summary>
    /// Get the minimum required size for the string + termination
    /// </summary>
    /// <param name="charStr">input string</param>
    /// <returns>required buffer size</returns>
    inline size_t GetReqBufSize(string const& str) {
        return str.length() + 1;
    }

    /// <summary>
    /// Get the minimum required size for the string + termination
    /// </summary>
    /// <param name="charStr">input string</param>
    /// <returns>required buffer size</returns>
    inline size_t GetReqBufSize(wstring const& str) {
        return (str.length() + 1) * sizeof(WCHAR);
    }

    //Convert a wstring to a string
    string WStringToString(wstring const& ws);

    //Convert a string to a wstring
    wstring StringToWString(string const& s);

    //convert a string to all lowercase
    void ToLowerInPlace(string& s);

    //convert a string to all uppercase
    void ToUpperInPlace(string& s);

    //convert a wstring to all lowercase
    void ToWLowerInPlace(wstring& ws);

    //convert a wstring to all uppercase
    void ToWUpperInPlace(wstring& ws);

    //surround an input string with marks
    wstring SurroundWith(wstring val, wstring mark);
}