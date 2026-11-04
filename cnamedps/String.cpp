
#include "stdafx.h"
#include "String.h"


namespace w32
{
    //convert a wstring to a string (UTF-8)
    //std::wstring_convert устарел в C++17, поэтому преобразование через WinAPI
    string WStringToString(wstring const& ws) {
        if (ws.empty())
            return {};
        const int size = WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
        string result(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()), result.data(), size, nullptr, nullptr);
        return result;
    }

    //convert a string (UTF-8) to wstring
    wstring StringToWString(string const& s) {
        if (s.empty())
            return {};
        const int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
        wstring result(static_cast<size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), size);
        return result;
    }

    //convert to lowercase
    void ToLowerInPlace(string& s) {
        transform(s.begin(), s.end(), s.begin(), ::tolower);
    }

    //convert to uppercase
    void ToUpperInPlace(string& s) {
        transform(s.begin(), s.end(), s.begin(), ::toupper);
    }

    //convert to lowercase
    void ToWLowerInPlace(wstring& ws) {
        transform(ws.begin(), ws.end(), ws.begin(), towlower);
    }

    //convert to uppercase
    void ToWUpperInPlace(wstring& ws) {
        transform(ws.begin(), ws.end(), ws.begin(), towupper);
    }

    //surround an input string with marks
    wstring SurroundWith(wstring val, wstring mark) {
        wstringstream wss;
        wss << mark << val << mark;
        return wss.str();
    }
}