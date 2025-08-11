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
#include "CIOBuffer.h"
#include "CIOContext.h"


namespace w32
{
	typedef void (*PipeWorkerMessageHandler)(
		void* context,
		CHandle & pipeHandle,
		CIOBuffer& input,
		CIOBuffer& output);

	/// <summary>
	/// This class manages a pool of worker threads for a named pipe server.
	/// The number of worker threads is variable. However it is not needed to
	/// implement dynamically sized collections for this purpose because there
	/// cannot be more than PIPE_UNLIMITED_INSTANCES (255) named pipe clients 
	/// connected at any given timeso we foresee that as a maximum array capacity.
	/// This reduces the overall complexity of error handling.
	/// While the number of worker threads is not fixed, it would be nonsensical
	/// to allow more threads than there are possible connected clients so we use
	/// that as a maximum array size.
	/// We keep 2 arrays of waitable objects. The HANDLE array is used for working
	/// with WaitMultipleObjects. The CHandle array is used to manage lifecycle
	/// so that they are deleted by the destructor.
	/// There is 1 more waitable object than there are possible connections. This is
	/// because all threads need to be listening to the shutdown even which will
	/// cause all threads to terminate.
	/// 
	/// That said, we further reduce the number of possible threads / pipe connections
	/// because the maximum number of waitable objects in a single WaitMultipleObjects
	/// is MAXIMUM_WAIT_OBJECTS (64). This is still a very respectable number so we use
	/// that as total. This allows a full pool of X number of threads to serve all
	/// connections. To go beyond that would drastically increase the complexity
	/// because we would need to maintain different pools of threads to server different
	/// sets of connections.
	/// </summary>
	class CNamedPipeWorkerPool
	{

	public:
		static const DWORD MAX_INSTANCES = MAXIMUM_WAIT_OBJECTS - 1;

	private:
		static const DWORD NUM_WAIT_OBJECTS = MAXIMUM_WAIT_OBJECTS;
		static const DWORD MAX_THREADS = 16;
	
		CHandle m_ServerThread[MAX_THREADS];

		//Each connection will have an input buffer, which in turn requires an
		//overlapped structure and a notification event
		CIOContext m_IOContext[MAX_INSTANCES];
		CManualResetEvent m_ShutdownEvent;

		//1 more event than there are possible instances
		HANDLE m_WaitObjects[NUM_WAIT_OBJECTS];

		//the index of the shutdown event is the index of the last element in the 
		//the array.
		static const DWORD SHUTDOWN_EVT_INDEX = MAX_INSTANCES;

		//the externally supplied message handler with context parameter
		PipeWorkerMessageHandler m_MessageHandler;
		void* m_Context;

		DWORD m_NumThreads;
		static DWORD WINAPI WorkerThreadFunc(void* ptr);

		//Initiate an asynchronous read request
		void InitiateRead(HANDLE pipe, CIOBuffer &buffer, LPOVERLAPPED overlapped);

		//Process the read request upon overlapped notification
		void ProcessReadCompletion(HANDLE pipe, CIOBuffer& buffer, LPOVERLAPPED overlapped);

		//reply to the caller
		void SendReturnMessage(HANDLE pipe, CIOBuffer& buffer);

	public:
		CNamedPipeWorkerPool(
			DWORD numWorkerThreads,
			PipeWorkerMessageHandler messageHandler,
			void* context,
			DWORD nInBufferSize,
			DWORD nOutBufferSize);
		~CNamedPipeWorkerPool();

		//Add a new connection and activate message processing
		void AddClientConnection(CHandle connection);

		//Shutdown the IO pool
		void Shutdown();

		bool IsShuttingDown();

		//Wait until all process threads have finished
		bool WaitUntilFinished(DWORD Timeout);

		//Shutdown the IO pool
		void CloseAllConnections();
	};
}
