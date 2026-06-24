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
#include "CIOBuffer.h"
#include "CHandle.h"
#include "CEvent.h"

namespace w32
{
	//This class is a logical grouping of the various elements that go into
	//an asynchronous IO operation
	class CIOContext
	{
	public:
		CIOContext();
		CHandle IOHandle;			//The handle for the object on which the IO is done
		CAutoResetEvent IOEvent;	//completion event for the object
		OVERLAPPED Overlapped;		//overlapped completion structure
		CIOBuffer InputBuffer;		//buffer for input IO
		CIOBuffer OutputBuffer;		//buffer for output IO
	};
}