#pragma once

#include "Exception.h"

namespace w32
{
    /// <summary>
/// This class is a dynamic buffer for elements of a given type.
/// Memory management is dynamic and managed ont he process heap.
/// Size is byte based, not element count
/// </summary>
/// <typeparam name="T">Type of elements in the buffer</typeparam>
    template<typename T>
    class CBuffer
    {
    protected:
        T* m_buffer = NULL;     //buffer pointer
        size_t m_size = 0;      //number of bytes

        /// <summary>
        /// Allocte a dynamic memory range as buffer range
        /// </summary>
        /// <param name="size">Total size of the buffer</param>
        virtual void Allocate(size_t size) {
            DeAllocate();

            if (size == 0)
                return;

            m_buffer = static_cast<T*> (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size));
            if (!m_buffer) {
                throw bad_alloc();
            }
            memset(m_buffer, 0, m_size);
            m_size = size;
        }

        /// <summary>
        /// Release the previously allocated memory
        /// </summary>
        virtual void DeAllocate() {
            if (m_buffer)
                HeapFree(GetProcessHeap(), 0, m_buffer);
            m_buffer = NULL;
            m_size = 0;
        }

    public:
        /// <summary>
        /// Initialize a new, zero size buffer
        /// </summary>
        CBuffer() {
        }

        /// <summary>
        /// Initialize a new buffer of a given size
        /// </summary>
        /// <param name="size">Total number of bytes</param>
        CBuffer(size_t size) {
            Allocate(size);
        }

        /// <summary>
        /// Move initialization. The rval memory is moved into the new instance
        /// </summary>
        /// <param name="rval"></param>
        CBuffer(CBuffer&& rval) {
            m_buffer = rval.m_buffer;
            m_size = rval.m_size;
            rval.m_buffer = NULL;
            rval.m_size = 0;
        }

        /// <summary>
        /// Copy initialization. 
        /// </summary>
        /// <param name="rval"></param>
        CBuffer(CBuffer& other) {
            Allocate(other.Size());
            memcpy(m_buffer, other.m_buffer, m_size);
        }

        virtual ~CBuffer() {
            DeAllocate();
        }

        /// <summary>
        /// Resize the buffer
        /// </summary>
        /// <param name="newSize">new size of the buffer</param>
        void Resize(size_t newSize) {            
            if (newSize == 0) {
                DeAllocate();
                return;
            }

            if (! m_buffer) {
                Allocate(newSize);
            }
            else {
                T* newBuffer = static_cast<T*> (
                    HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_buffer, newSize));
                if (!newBuffer) {
                    throw bad_alloc();
                }
                //only overwrite if reallocation fails because the original pointer is not
                //freed and if we set m_buffer to NULL, the memory would leak.
                m_buffer = newBuffer;
                m_size = newSize;
            }
        }

        /// <summary>
        /// Get the size in bytes
        /// </summary>
        /// <returns>Size in bytes</returns>
        size_t Size() {
            return m_size;
        }

        /// <summary>
        /// Get the length of the buffer as an array of elements with type T
        /// </summary>
        /// <returns>Number of elements of type T</returns>
        size_t Length() {
            return m_size / sizeof(T);
        }

        /// <summary>
        /// Get the element at index in the array of type T
        /// </summary>
        /// <param name="index">index in the array</param>
        /// <returns>element at the specified index, by reference</returns>
        T& operator [](size_t index) {
            if (index < m_size / sizeof(T))
                return m_buffer[index];
            else {
                throw range_error("index out of bounds");
            }
        }

        /// <summary>
        /// Is the buffer valid?
        /// </summary>
        /// <returns>true if a buffer is allocated</returns>
        bool IsValid() {
            return m_buffer != NULL;
        }

        /// <summary>
        /// Cast the buffer pointer to an array of type T
        /// </summary>
        operator T* () {
            return static_cast<T*>(m_buffer);
        }

        /// <summary>
        /// Cast the buffer pointer to void*
        /// </summary>
        operator void* () {
            return static_cast<void*>(m_buffer);
        }

        /// <summary>
        /// assignment operator
        /// </summary>
        CBuffer& operator = (const CBuffer & other) {
            if (this == &other)
                return *this;

            if (m_size != other.m_size) {
                DeAllocate();
                Allocate(other.m_size);
            }

            memcpy(m_buffer, other.m_buffer, m_size);
            return *this;
        }

        /// <summary>
        /// assignment operator
        /// </summary>
        CBuffer& operator = (CBuffer && other) noexcept {
            if (this == &other)
                return *this;

            if (m_size != other.m_size) {
                std::swap(m_size, other.m_size);
                std::swap(m_buffer, other.m_buffer);
            }

            return *this;
        }

    };

    template<typename T>
    class C_Array : CBuffer<T> { };
}