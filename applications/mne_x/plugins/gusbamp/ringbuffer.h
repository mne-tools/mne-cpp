//Copyright (c) 2011 by g.tec medical engineering GmbH

#pragma once

#include <Windows.h>
#include "gusbamp_global.h"

/*
 * Class representing a ring buffer with elements of type float.
 */
template <typename T> class GUSBAMPSHARED_EXPORT CRingBuffer
{
public:

	//Constructor. Creates an empty buffer with an initial capacity of zero.
	CRingBuffer(void)
		: _buffer(NULL), _capacity(0), _start(0), _end(0), _isEmpty(true)
	{
	}

	//Destructor. Frees the allocated buffer.
	~CRingBuffer(void)
	{
		if (_buffer != NULL)
			VirtualFree(_buffer, 0, MEM_RELEASE);

		_buffer = NULL;
	}

	/* 
	 * Initializes the buffer with the specified capacity representing the number of elements that the buffer can contain.
	 * Returns false if the memory couldn't be allocated (e.g. because of not enough free disk space); true, if the call succeeded.
	 */
	bool Initialize(unsigned int capacity)
	{
		//if the buffer has been allocated before, release this memory first
		if (_buffer != NULL)
		{
			VirtualFree(_buffer, 0, MEM_RELEASE);
			_buffer = NULL;
		}

		if (capacity > 0)
		{
			//allocate memory for the buffer
			_buffer = (T*) VirtualAlloc(NULL, capacity * sizeof(T), MEM_COMMIT, PAGE_READWRITE);

			//check if allocation succeeded
			if (_buffer == NULL)
				return false;

			_capacity = capacity;
		}

		//reset the buffer positions
		Reset();

		return true;
	}

	//Clears the buffer by resetting both the start and end position to zero.
	void Reset()
	{
		_start = 0;
		_end = 0;
		_isEmpty = true;
	}

	//Returns the buffer's capacity it has been initialized to, i.e. the number of elements the buffer can contain.
	int GetCapacity()
	{
		return _capacity;
	}

	//Returns the free space of the buffer, i.e. the number of new elements that can be enqueued before the buffer will overrun.
	int GetFreeSize()
	{
		return _capacity - GetSize();
	}

	//Returns the number of elements that the buffer currently contains (don't confuse the size (number of ACTUALLY contained elements) with the capacity (maximum number of elements that the buffer CAN contain)!).
	int GetSize()
	{
		if (_isEmpty)
			return 0;
		else if (_start < _end)
			return _end - _start;
		else
			return _capacity - (_start - _end);
	}

	/*
	 * Writes the specified number of elements from the specified source array into the ring buffer. If the number of elements to copy exceeds the free buffer space, only the free buffer space will be written, existing elements will NOT be overwritten.
	 * float* source:		pointer to the first element of the source array whose elements should be stored into the ring buffer.
	 * unsigned int length:	the number of elements from the source array that should be copied into the ring buffer.
	 */
	void Write(T *source, unsigned int length)
	{
		//if buffer is full or no elements should be written, no elements can be written
		if ((!_isEmpty && _start == _end) || length <= 0)
			return;

		//if _start <= _end, split the free buffer space into two parts 
		unsigned int firstPartCapacity = (_start <= _end) ? _capacity - _end : _start - _end;
		unsigned int secondPartCapacity = (_start <= _end) ? _start : 0;

		//copy first part
		CopyMemory(&_buffer[_end], source, min(firstPartCapacity, length) * sizeof(T));

		//if a second part exists, copy second part
		if (length > firstPartCapacity)
			CopyMemory(&_buffer[0], &source[firstPartCapacity], min(secondPartCapacity, length - firstPartCapacity) * sizeof(T));

		//update buffer positions
		_end = (_end + min(length, firstPartCapacity + secondPartCapacity)) % _capacity;
		_isEmpty = false;
	}

	/*
	 * Copys the specified number of elements from the ring buffer into the specified destination array.
	 * If there are less elements in the buffer than the to read, only available elements will be copied.
	 * float *destination:	The array where to copy the elements from the ring buffer to.
	 * unsigned int length: The number of elements to copy from the ring buffer into the destination array.
	 */
	void Read(T *destination, unsigned int length)
	{
		if (length <= 0)
			return;

		//if _start >= _end, split the read operation into two parts 
		int firstPartSize = (_start < _end) ? min(length, _end - _start) : min(length, _capacity - _start);
		int secondPartSize = (_start < _end) ? 0 : min(_end, length - firstPartSize);

		//copy first part
		CopyMemory(destination, &_buffer[_start], firstPartSize * sizeof(T));

		//if a second part exists, copy second part
		if (secondPartSize > 0)
			CopyMemory(&destination[firstPartSize], &_buffer[0], secondPartSize * sizeof(T));

		//update the buffer positions
		_start = (_start + (firstPartSize + secondPartSize)) % _capacity;

		if (_start == _end)
			_isEmpty = true;
	}

protected:
	//the buffer array
	T* _buffer;

	//the number of elements the buffer can contain
	unsigned int _capacity;

	//the position of the first contained element of the buffer in the internal array
	unsigned int _start;

	//the position of the first free element of the buffer in the internal array (this position - 1 equals the position of the last contained element of the buffer)
	unsigned int _end;

	//flag indicating if the buffer is empty. Necessary because when _start == _end it is undefined if the buffer is full or empty.
	bool _isEmpty;
};
