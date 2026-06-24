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
#include "CNamedPipeServer.h"
#include "CNamedPipeWorkerPool.h"


namespace w32
{
	CNamedPipeServer::CNamedPipeServer(
		wstring const& lpName,
		PipeWorkerMessageHandler messageHandler,
		void* messageContext,
		DWORD nOutBufferSize,
		DWORD nInBufferSize,
		DWORD nMaxInstances,
		DWORD nNumThreads) :
		m_Name(lpName),
		m_MaxInstances(nMaxInstances),
		m_InBufferSize(nInBufferSize),
		m_OutBufferSize(nOutBufferSize),
		m_ConnectEvent(),
		m_StartEvent(),
		m_ShutdownEvent(),
		m_WorkerPool(nNumThreads, messageHandler, messageContext, nInBufferSize, nOutBufferSize)
	{
		memset(&m_ConnectOverlap, 0, sizeof(m_ConnectOverlap));
		m_ConnectOverlap.hEvent = m_ConnectEvent;

		//Create a named pipe server endpoint once.
		//This is just to validate the supplied parameters. If they are invalid it's
		//better to find out now than when this is offloaded into a worker thread.
		CHandle pipeHandle = CreateNamedPipeHandle();

		//check the reason for failure
		if (pipeHandle == INVALID_HANDLE_VALUE) {
			throw ExWin32Error(L"CreateNamedPipeW " + lpName);
		}

		m_ConnectionHandlerThread = CreateThread(NULL, 0, ConnectionHandlerFunc, this, 0, NULL);
		if (!m_ConnectionHandlerThread) {
			throw ExWin32Error(L"CreateThread ConnectionHandlerFunc");
		}
	}

	CNamedPipeServer::~CNamedPipeServer()
	{
		//All cleanup is taken care of by the destructors of the members. However we cannot
		//allow this object to be deleted if the worker thread is still running because
		//that would lead to a crash when invalid memory is accessed.
		//If this is the case, then all solutions are problematic.
		//either we wait until the thread has ended, or we end it forcefully.
		//Ending it forcefully is usually the least preferred option because it stops the thread
		//dead in its tracks. No constructors will be called and handles may leak.
		//In this specific case, there is nothing important that will leak.
		//The worst case is that a client connection handle may leak.
		//However, the worker pool cannot just be terminated.
		//It is essentially unknowable what the message handler is doing. If the handler is using
		//synchronization primitives, terminating those threads could lead to bad things.

		m_ShutdownEvent.SetEvent();
		WaitUntilFinished(INFINITE);

	}

	HANDLE CNamedPipeServer::CreateNamedPipeHandle()
	{
		return CreateNamedPipeW(m_Name.c_str(),
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
			m_MaxInstances,
			m_OutBufferSize,
			m_InBufferSize,
			0,
			NULL);
	}

	DWORD WINAPI CNamedPipeServer::ConnectionHandlerFunc(LPVOID lpThreadParameter) // void* ptr)
	{
		CNamedPipeServer* server = (CNamedPipeServer*) lpThreadParameter;

		HANDLE initialWaitObjects[] = { server->m_StartEvent, server->m_ShutdownEvent };
		DWORD StartEventIndex = 0;
		DWORD ShutdownEventIndex = 1;

		//Use a waiting gate until we're allowed to start.
		//This is to give the hosting application the opportunity to initialize itself and be
		//in control of when it starts processing.
		DWORD retVal = WaitForMultipleObjects(2, initialWaitObjects, FALSE, INFINITE);
		if (retVal == StartEventIndex) {
			; //Do nothing. Connection processing is allowed to start
		}
		else {
			//any other wait result indicates that we are not allowed to start
			server->m_WorkerPool.Shutdown();
			return 0;
		}

		HANDLE waitObjects[] = { server->m_ConnectOverlap.hEvent, server->m_ShutdownEvent };
		DWORD ConnectEventIndex = 0;

		while (!server->IsShuttingDown())
		{
			//Create a new server-side endpoint.
			CHandle pipeHandle = server->CreateNamedPipeHandle();

			//check the reason for failure
			if (pipeHandle == INVALID_HANDLE_VALUE) {
				DWORD error = GetLastError();
				//If all possible connections are in use, we cannot create new endpoints.
				//We wait for a bit before returning because otherwise we'll be in a CPU
				//consuming loop until a connection is free.
				//Implementing this with a semaphore would be more elegant, but also
				//add more complexity to deal with various scenarios.
				if (ERROR_PIPE_BUSY == error) {
					Sleep(1000);
					continue;
				}
				else {
					server->Shutdown();
					retVal = error;
				}

			}

			if(!ConnectNamedPipe(pipeHandle, &server->m_ConnectOverlap)) {
				DWORD error = GetLastError();
				if (error == ERROR_IO_PENDING)
					; //normal. The wait operation has started.
				else if (error == ERROR_PIPE_CONNECTED) {
					//a client has already connected between creating the pipe and
					//waiting for a client to connect. This is rare but can happen.
					//When it does, the overlapped wait does not begin, so the overlapped
					//event would not be signalled. We trigger it manually so that the normal
					//workflow continues
					SetEvent(server->m_ConnectOverlap.hEvent);
				}
				else {
					server->Shutdown();
					retVal = error;
				}
			}

			//Enter the wait operation. We wait until either shutdown has been signalled
			//or a client has connected. Note that there is an inherent race condition
			//here. It's perfectly possible that the shutdown event is triggered right
			//after a client connects. That is not a problem if normal workflow is followed.
			//The worker threds have their own lifecycle which is controlled by the shutdown
			//event as well. If the condition happens, we start a new worker. It will shut down
			//immediately. Meanwhile, we loop back around, start another wait, which will then
			//immediately fall through, detect the shutdown, and perform the shutdown sequence.

			DWORD retVal = WaitForMultipleObjects(2, waitObjects, FALSE, INFINITE);
			if (retVal == ConnectEventIndex) {
				//A client connected. Offload the request to a worker thread and get ready
				//to wait for another client
				try
				{
					std::cout << "CNamedPipeServer::CreateNamedPipeHandle() CONNECTED!!";
					server->m_WorkerPool.AddClientConnection(pipeHandle);
				}
				catch (exception& ex)
				{
					//the pipehandle is not added. The only remaining handle will
					//be cleaned up automatically when the local variable goes out of scope
				}
			}
			else if (retVal == ShutdownEventIndex) {
				//We don't have to do anything special here.
				//The loop will terminate because IsShuttingDown is true
			}
			else {
				//Wait failed. Since we wait indefinitely for either shutdown
				//or connect, the only logical conclusion is the application itself is
				//experiencing a critical failure
				server->Shutdown();
				retVal = GetLastError();
			}
		}
		server->m_WorkerPool.Shutdown();
		server->m_WorkerPool.WaitUntilFinished(INFINITE);
		server->m_WorkerPool.CloseAllConnections();

		return retVal;
	}

	bool CNamedPipeServer::IsShuttingDown()
	{
		return (0 == WaitForSingleObject(m_ShutdownEvent, 0));
	}

	//Trigger shutdown of the conenction handler and the IO worker pool
	void CNamedPipeServer::Shutdown()
	{
		m_ShutdownEvent.SetEvent();
	}

	//Notify the connection handler that it's OK to start accepting incoming
	//connection requests
	void CNamedPipeServer::StartServing()
	{
		m_StartEvent.SetEvent();
	}

	//Wait until all threads have finished executing
	//This is needed because those threads are accessing memory that belongs to
	//this object. If the object is deleted while those threads are still running
	//there will be an access violation
	bool CNamedPipeServer::WaitUntilFinished(DWORD dwMilliSeconds)
	{
		DWORD error = WaitForSingleObject(m_ConnectionHandlerThread, dwMilliSeconds);
		if (error == WAIT_OBJECT_0)
			return m_WorkerPool.WaitUntilFinished(dwMilliSeconds);
		else if (error == WAIT_TIMEOUT)
			return false;

		throw ExWin32Error("CNamedPipeServer::WaitUntilFinished");
	}
}