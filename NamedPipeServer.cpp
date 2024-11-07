// NamedPipeServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <iostream>
#include <mutex>
#include <sstream>
using namespace std;

void pipeWorkerMessageHandler(
    void* context,
    w32::CHandle& handle,
    CIOBuffer& input,
    CIOBuffer& output)
{
    std::mutex* pmutex = (std::mutex*) context;
    DWORD threadId = GetCurrentThreadId();
    Sleep(1000); //simulate work
    LPWSTR text = (LPWSTR)input.Ptr();
    {
        lock_guard< std::mutex> lock(*pmutex);
        std::wcout << L"Thread " << threadId << L" recieved: " << text << std::endl;
    }

    wstringstream stream;
    stream << L"Message answered by thread " << threadId;
    wstring message = stream.str();
    DWORD bytes = (message.size() + 1 ) * sizeof(wstring::value_type);
    memcpy(
        output.Ptr(),
        (void*)message.c_str(),
        bytes);
    output.SetOffset(bytes);
}

int main()
{
    try
    {
        std::mutex g_mutex;
        wstring name(L"\\\\.\\Pipe\\Named2Pipe");

        CNamedPipeServer server(name,
            pipeWorkerMessageHandler,
            &g_mutex,
            256,
            256,
            10);

        cout << "Make a choice:" << endl;
        cout << "==============" << endl;
        cout << "s: start serving" << endl;
        cout << "q: quit" << endl << endl;
        cout << "Choice: ";

        char choice = 0;
        do
        {
            choice = getchar();
            if (choice == 'q') {
                cout << "Initiating shutdown" << endl;
                server.Shutdown();
                cout << "Waiting until everything is shutdown" << endl;
                server.WaitUntilFinished(INFINITE);
                cout << "Shutdown finished" << endl;
            }
            else if (choice == 's') {
                cout << "Starting server connections" << endl;
                server.StartServing();
            }
        } while (choice != 'q');

    }
    catch (exception& ex)
    {
        cout << ex.what() << endl;
    }
}

