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
#include "CNamedPipeWorkerPool.h"
#include "Exception.h"
#include "CBuffer.h"

namespace w32
{
	/// <summary>
	/// Create a new pool threads for handling IO connections for a named pipe.
	/// </summary>
	/// <param name="shutdownEventHandle">Manual reset event which will trigger shutdown of all worker threads</param>
	/// <param name="numWorkerThreads">number of worker threads that perform the IO</param>
	/// <param name="messageHandler">function which will be called for each IO request</param>
	/// <param name="context">context parameter which will be passed to the message handler</param>
	CNamedPipeWorkerPool::CNamedPipeWorkerPool(
		DWORD numWorkerThreads,
		PipeWorkerMessageHandler messageHandler,
		void* context,
		DWORD nInBufferSize,
		DWORD nOutBufferSize) :
		m_ShutdownEvent(),
		m_MessageHandler(messageHandler),
		m_NumThreads(numWorkerThreads),
		m_Context(context),
		m_IOContext()

	{
		//input validation
		if (m_NumThreads > MAX_THREADS || m_NumThreads < 1)
			throw AppException("Invalid number of threads specified");

		if (!m_MessageHandler)
			throw AppException("Invalid messageHandler specified");

		//The final position in the array of events and wait objects is the shutdown event.
		//this makes most sense because then there is a 1-1 relationship between the indices
		//of the handle arrays, event / overlapped arrays, buffer arrays
		m_WaitObjects[SHUTDOWN_EVT_INDEX] = m_ShutdownEvent;

		//Initialize all slots in the arrays that can be occupied by an actual connection.
		for (DWORD i = 0; i < MAX_INSTANCES; i++)
		{
			m_WaitObjects[i] = m_IOContext[i].IOEvent;
			m_IOContext[i].InputBuffer.Reset(nInBufferSize);
			m_IOContext[i].OutputBuffer.Reset(nInBufferSize);
		}


		try
		{
			//Create the worker threads. These threads start running. There is no point in
			//suspending them because the named pipe server will be in control of accepting
			//connections and only when connections are accepted will workers be triggered
			for (DWORD i = 0; i < m_NumThreads; i++) {
				m_ServerThread[i] = CreateThread(NULL, 0, WorkerThreadFunc, this, 0, NULL);
				if (m_ServerThread == NULL) {
					throw ExWin32Error(L"CNamedPipeWorker::CNamedPipeWorker CreateThread");
				}
			}

		}
		catch (...)
		{
			//if for some reason an exception is thrown we need to terminate the threads
			//because when the stack is being unwound, the named pipe server constructor
			//has not finished, which means the shutdown event will not be triggered and
			//leaving some threads lurking around, waiting on an event for which the handle
			//will be closed is bad form
			//The handles would be closed via RAII but that doesn't stop the threads from running.
			for (DWORD i = 0; i < MAX_THREADS; i++) {
				if (m_ServerThread[i].IsValid())
#pragma warning( push)
#pragma warning( disable : 6258) 
					TerminateThread(m_ServerThread[i], -1);
#pragma warning (pop)
			}
			throw;
		}

	}

	CNamedPipeWorkerPool::~CNamedPipeWorkerPool()
	{
		//cause threads to shutdown if they aren't already
		Shutdown();

		//wait until all threads are shut down.
		WaitUntilFinished(INFINITE);

		//guarantee that all IO is cancelled
		CloseAllConnections();
	}

	//Send the shutdown signal
	void CNamedPipeWorkerPool::Shutdown()
	{
		m_ShutdownEvent.SetEvent();
	}


	bool CNamedPipeWorkerPool::IsShuttingDown()
	{
		return (0 == WaitForSingleObject(m_ShutdownEvent, 0));
	}

	void CNamedPipeWorkerPool::CloseAllConnections()
	{
		for (DWORD i = 0; i < MAX_INSTANCES; i++)
		{
			if (m_IOContext[i].IOHandle.IsValid()) {
				m_IOContext[i].IOHandle = NULL;
			}
		}
	}

	//Wait until all worker threads have ended their run.
	//Only when all threads are finished is it safe to destroy this object
	bool CNamedPipeWorkerPool::WaitUntilFinished(DWORD Timeout)
	{
		HANDLE threads[MAX_THREADS];
		for (DWORD i = 0; i < m_NumThreads; i++) {
			threads[i] = m_ServerThread[i];
		}

		DWORD retVal = WaitForMultipleObjects(m_NumThreads, threads, TRUE, Timeout);
		if (retVal == WAIT_FAILED)
			throw ExWin32Error("CNamedPipeWorker::WaitUntilFinished WaitForMultipleObjects");

		return retVal == WAIT_TIMEOUT;
	}

	//Add an active connection to the list of active connections and initiate
	// the first read operation. If the read initiation fails, the client
	// is disconnected and the handle is removed again.
	void CNamedPipeWorkerPool::AddClientConnection(CHandle connection)
	{
		CIOContext* ioContext = NULL;
		for (DWORD i = 0; i < MAX_INSTANCES; i++)
		{
			if (!m_IOContext[i].IOHandle.IsValid()) {
				ioContext = &m_IOContext[i];
				break;
			}
		}

		if (ioContext == NULL)
			throw AppException("CNamedPipeWorker::AddClientConnection no more empty slots");

		//we MUST set the handle in the handle list before initiating the read because the
		//read may complete on another thread before the InitiateRead function returns and it
		//is vital that when the IO completeion happens, the handle that was used for the overlapped
		//operation is available.
		//However if we hit an exception we must also remove it before allowing the exception to
		//travel up the stack.
		ioContext->IOHandle = connection;
		try
		{
			InitiateRead(connection, ioContext->InputBuffer, &ioContext->Overlapped);
		}
		catch (exception& ex)
		{
			//an exception at this point means that the IO operation was never started and the
			//connection is not operational. Remove the handle because from the worker's point
			//of view the connection is being terminated and will be cleaned up.
			DisconnectNamedPipe(ioContext->IOHandle);
			ioContext->IOHandle = NULL;
			throw;
		}
	}

	/// <summary>
	/// This function initiates a read request on the pipe. We use overlapped IO so that
	/// we can offload the processing to a pool of worker threads.
	/// If data is already waiting at this point, the read operation is performed
	/// in a synchronous manner but we don't do any follow up processing here of any
	/// kind, because the overlapped completion will still be triggered and anything
	/// we'd do here would interfere with processing that will start on another thread.
	/// </summary>
	void CNamedPipeWorkerPool::InitiateRead(HANDLE pipe, CIOBuffer& buffer, LPOVERLAPPED overlapped)
	{
		DWORD bytesRead = 0;
		buffer.Reset();
		if (ReadFile(pipe, buffer.WritePtr(), buffer.SizeRemaining(), &bytesRead, overlapped))
		{
			//ReadFile was a success. The data was retrieved synchronous
			//This is not a problem. The overlapped event will still be triggered, and
			//any data handling will be taken care of in the thread pool. where the
			//wait will fall through. If this happens inside the threadpool, the
			//read operation may be completed in a different thread than the one where
			//the read request originated.
			//If no error was returned, this also means that the entire message was
			//received in the read operation.
		}
		else {
			if (GetLastError() == ERROR_IO_PENDING) {
				//There was no data waiting for the read operation. This is normal.
				//When data arrives, it will trigger the overlapped event and the data
				//will be read in the thread pool by one of the worker threads.
			}
			else if (GetLastError() == ERROR_MORE_DATA) {
				//Data arrived synchronous but not all data was received.
				//This is also not a problem. It just means that when the worker
				//is processing the overlapped event it will detect that it needs to
				//retrieve more data
			}
			else {
				throw ExWin32Error(L"Read named pipe error");
			}
		}
	}

	/// <summary>
	/// This function finishes the read completion that was started earlier.
	/// If the read was incomplete due to buffer size limitation, it will
	/// be completed here so the incoming message can be processed
	/// </summary>
	void CNamedPipeWorkerPool::ProcessReadCompletion(HANDLE pipe, CIOBuffer & ioBuffer, LPOVERLAPPED overlapped)
	{
		DWORD bytesRead = 0;
		if (GetOverlappedResult(pipe, overlapped, &bytesRead, FALSE))
		{
			//The overlapped read was successful, which means all data in the message was read.
			//nothing remains to be done here except update the total number of bytes read in
			//the IO request.
			ioBuffer.SetOffset(
				ioBuffer.Offset() + bytesRead);
		}
		else
		{
			//Failure may indicate that there is more data to read. The pipe can tell us how much
			//data is waiting in the message that is currently being read.
			if (GetLastError() == ERROR_MORE_DATA) {
				//There was more data. Messages are always written / read whole, which means that
				//if we read an incomplete message because of the buffersize limitation, we can
				//check how much data is remaining and read it synchronously. There will be no wait
				//because the data is already there.
				ioBuffer.SetOffset(
					ioBuffer.Offset() + bytesRead);

				DWORD bytesRemainingInMessage = 0;
				if (!PeekNamedPipe(pipe, NULL, 0, NULL, NULL, &bytesRemainingInMessage)) {
					throw ExWin32Error("PeekNamedPipe failed after GetLastError() == ERROR_MORE_DATA");
				}

				bytesRead = 0;
				ioBuffer.AddSize(bytesRemainingInMessage);
				//perform a read for the remaining data. Do not use overlapped IO so no other threads
				//get triggered. The data is already available so this doesn't cost extra time here.
				if (!ReadFile(pipe, ioBuffer.WritePtr(), ioBuffer.SizeRemaining(), &bytesRead, NULL)) {
					throw ExWin32Error("ReadFile failed for remaining bytes in pipe");
				}
			}
			else {
				throw ExWin32Error("Overlapped ReadFile failed.");
			}
		}
	}

	//Send a return message after the incoming message was processed.
	//This is performed synchronously because the entire message is ready
	//to be sent and the minute performance difference isn't worth the complexity
	//of implementing the asynchronous write.
	void CNamedPipeWorkerPool::SendReturnMessage(HANDLE pipe, CIOBuffer& buffer)
	{
		if (!WriteFile(pipe, buffer.Ptr(), buffer.Offset(), NULL, NULL)) {
			throw ExWin32Error("CNamedPipeWorker::SendReturnMessage WriteFile");
		}
	}

	//Perform work. This function will be executed concurrently on as many threads
	//as were started. Each thread waits until one of the connections is triggered
	//either by an incoming message or a shutdown event.
	//All messages get processed, and a reply is sent.
	 DWORD WINAPI  CNamedPipeWorkerPool::WorkerThreadFunc(LPVOID lpThreadParameter)
	{
		CNamedPipeWorkerPool* worker = (CNamedPipeWorkerPool*)lpThreadParameter;
		while (!worker->IsShuttingDown())
		{
			DWORD eventIndex = WaitForMultipleObjects(NUM_WAIT_OBJECTS, worker->m_WaitObjects, FALSE, INFINITE);

			//If a shutdown was signalled, the thread must end
			if (eventIndex == SHUTDOWN_EVT_INDEX)
				return 0;

			//return value if an event was triggered is the index of the event. If the return value
			//is not a valid index, something is catastrophically wrong and we exit.
			if (eventIndex >= MAX_INSTANCES) {
				DWORD error = GetLastError();
				return -1;
			}

			CIOContext* ioContext = &worker->m_IOContext[eventIndex];

			try
			{
				//verify that there is an active handle associated with the event
				//otherwise do nothing because this was a spurious event
				if (!ioContext->IOHandle.IsValid())
					continue;

				worker->ProcessReadCompletion(
					ioContext->IOHandle,
					ioContext->InputBuffer,
					&ioContext->Overlapped);

				worker->m_MessageHandler(
					worker->m_Context,
					ioContext->IOHandle,
					ioContext->InputBuffer,
					ioContext->OutputBuffer );
				
				if (GetCurrentThreadToken() != NULL) {
					if (!RevertToSelf()) {
						SetEvent(worker->m_ShutdownEvent);
						return GetLastError();
					}
				}

				//if data was written we send a replay. Otherwise we assume that no response was required
				if (ioContext->OutputBuffer.Offset() > 0) {
					worker->SendReturnMessage(
						ioContext->IOHandle,
						ioContext->OutputBuffer);
				}

				//The previous operation is completely finished. Get ready for the next
				//request which may come at any time (or already be there).
				worker->InitiateRead(
					ioContext->IOHandle,
					ioContext->InputBuffer,
					&ioContext->Overlapped);

			}
			catch (exception& ex)
			{
				//Something went catastrophically wrong with the communication.
				//It is essentially unknowable what exactly. If it is related to the handle
				//we may not be able to send back anything. If it is related to the message
				//handling, we don't know what to send back to the client.
				//If error handling 'can' be done, it will have to be implemented in the message
				//handler which can format a reply. If things progress to this place
				//we terminate the connection cleanly and free up the connection slot.
				//No locking is required because this is the only place where the handle
				//array is set back to NULL when the threads are running.

				DisconnectNamedPipe(ioContext->IOHandle);

				//at this point it doesn't even matter if IO is in progress or not.
				//We just have to make sure it is cancelled if it is, to make sure that an
				//outstanding IO request would not touch the buffer pointer at some future
				//time when the buffer has been invalidated.
				CancelIo(ioContext->IOHandle);

				//will cause the handle to be closed
				ioContext->IOHandle = NULL;
			}
		}

		return 0;
	}
}