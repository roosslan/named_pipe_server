
#include "..\stdafx.h"
#include "String.h"


namespace w32
{
    //convert a wstring to a string
    string WStringToString(wstring const& ws) {
        wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(ws.c_str());
    }

    //convert a string to wstring
    wstring StringToWString(string const& s) {
        wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(s.c_str());
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