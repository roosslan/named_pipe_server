#include "..\stdafx.h"
#include "CIOBuffer.h"

namespace w32
{

	CIOBuffer::CIOBuffer(DWORD defaultSize) :
		CBuffer<BYTE>(defaultSize),
		m_DefaultSize(defaultSize),
		m_WriteOffset(0)
	{

	}

	CIOBuffer::CIOBuffer() :
		CBuffer<BYTE>(0),
		m_DefaultSize(0),
		m_WriteOffset(0)
	{

	}

	void CIOBuffer::Reset()
	{
		Resize(m_DefaultSize);
		m_WriteOffset = 0;
	}

	void CIOBuffer::Reset(DWORD defaultSize)
	{
		m_DefaultSize = defaultSize;
		Reset();
	}

	void CIOBuffer::AddSize(DWORD additionalSize)
	{
		Resize(Size() + additionalSize);
	}

	DWORD CIOBuffer::Offset()
	{
		return m_WriteOffset;
	}

	void CIOBuffer::SetOffset(DWORD newOffset)
	{
		m_WriteOffset = newOffset;
	}

	void* CIOBuffer::WritePtr()
	{
		return (BYTE*) m_buffer + m_WriteOffset;
	}

	DWORD CIOBuffer::SizeRemaining()
	{
		return Size() - m_WriteOffset;
	}

	void* CIOBuffer::Ptr()
	{
		return m_buffer;
	}

	DWORD CIOBuffer::Size()
	{
		return CBuffer<BYTE>::Size();
	}

}