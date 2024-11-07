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
#include "CEvent.h"
#include "CNamedPipeWorkerPool.h"

namespace w32
{
	class CNamedPipeServer
	{ 
	private:
		wstring m_Name;						//Name of the pipe (\\.\Pipe\Pipename)
		DWORD m_MaxInstances;				//maximum number of parallel pip connections
		DWORD m_OutBufferSize;				//IO buffer length of the pipe object
		DWORD m_InBufferSize;				//IO buffer length of the pipe object

		CHandle m_ConnectionHandlerThread;				//thread which handles connection requests

		OVERLAPPED m_ConnectOverlap;		//for asynchronous handling of the connection request
		CAutoResetEvent m_ConnectEvent;		//event that fires when a client connects
		CAutoResetEvent m_StartEvent;		//event that tells the thread to start accepting clients
		CManualResetEvent m_ShutdownEvent;	//event signalling shutdown

		CNamedPipeWorkerPool m_WorkerPool;	//the pool which handles all connection IO

		HANDLE CreateNamedPipeHandle();

	public:

		//Create a new pipe server object.
		CNamedPipeServer(
			wstring const& lpName,
			PipeWorkerMessageHandler messageHandler,
			void* messageContext,
			DWORD nOutBufferSize,
			DWORD nInBufferSize,
			DWORD nMaxInstances = CNamedPipeWorkerPool::MAX_INSTANCES,
			DWORD nNumThreads = 4 );
		~CNamedPipeServer();

		//Start accepting clients
		void StartServing();

		//Shutdown all client connections and shutdown the thread for new connections
		void Shutdown();

		//are we shutting down?
		bool IsShuttingDown();

		//Wait until all client connections and the connection thread are shut down
		bool WaitUntilFinished(DWORD dwMilliSeconds = INFINITE);

		//worker thread for accpeting new connections
		static DWORD WINAPI ConnectionHandlerFunc(void* ptr);
	};
}
