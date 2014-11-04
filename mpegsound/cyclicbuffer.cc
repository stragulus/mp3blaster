#include "config.h"

#include <string.h>

#include "cyclicbuffer.h"

CyclicBuffer::CyclicBuffer(const unsigned int size)
{
	this->buffer = new unsigned char[size];
	this->bufferSize = size;
	this->readIndex = 0;
	this->writeIndex = 0;
#ifdef LIBPTH
  (void)pth_mutex_init(&rw_mutex);
#elif defined(PTHREADEDMPEG)
  (void)pthread_mutex_init(&rw_mutex, NULL);
#endif
}

CyclicBuffer::~CyclicBuffer()
{
	delete[] buffer;
}

int
CyclicBuffer::read(unsigned char * const buf, const unsigned int amount,
	const int partial)
{
	unsigned int contentLength;
	unsigned int bytesRead;

	lock();

	contentLength  = this->contentSizeInternal();
	if (contentLength < amount)
	{
		if (!partial)
		{
			//not enough to read.
			unlock();
			return 0;
		}

		//partial read
		bytesRead = contentLength;
		readData(buf, bytesRead);
		unlock();
		return (int)bytesRead;
	}

	/* #bytes to read >= amount */
	readData(buf, amount);
	unlock();
	return (int)amount;
}

int
CyclicBuffer::write(const unsigned char * const buf,
	const unsigned int amount, const int partial)
{
	unsigned int emptySpace;
	
	lock();
	emptySpace = this->bufferSize - this->contentSizeInternal();

	if (emptySpace < amount)
	{
		if (!partial)
		{
			unlock();
			return 0; //not enough space to write to
		}

		//partial write
		writeData(buf, emptySpace);
		unlock();
		return (int)emptySpace;
	}

	writeData(buf, amount);
	unlock();
	return (int)amount;
}

void
CyclicBuffer::setEmpty()
{
	lock();
	this->readIndex = this->writeIndex;
	unlock();
}

unsigned int
CyclicBuffer::contentSize()
{
	unsigned int returnValue;

	lock();
	returnValue = contentSizeInternal();
	unlock();
	return returnValue;
}

unsigned int
CyclicBuffer::contentSizeInternal()
{
	if (this->readIndex == this->writeIndex) // ____RW_____
		return 0; //no content
	else if (this->readIndex < this->writeIndex) // ___R******W___
		return this->writeIndex - this->readIndex;
	
	// ****W_______R**** 
	/* readIndex > writeIndex. Content is from readIndex to end of buffer,
	 * and start of buffer to writeIndex
	 */
	return (this->bufferSize - this->readIndex) + this->writeIndex;
}

unsigned int
CyclicBuffer::emptySpace()
{
	unsigned int returnValue;
	lock();
	returnValue = emptySpaceInternal();
	unlock();

	return returnValue;
}

unsigned int
CyclicBuffer::emptySpaceInternal()
{
	return this->bufferSize - contentSizeInternal();
}

unsigned int
CyclicBuffer::size()
{
	return bufferSize;
}

/*
 * amount must always be equal to, or less than, the contents in the buffer.
 * Adjusts read pointer.
 */
void
CyclicBuffer::readData(unsigned char * const buf, const unsigned int amount)
{
	unsigned int len;
	unsigned int toread = amount;
	unsigned int bytesRead = 0;

	/* area of bytes from read pointer to end of buffer pointer */
	len = this->bufferSize - this->readIndex;

	if (len >= toread)
	{
		/* it fits within this part. */
		memcpy(buf, buffer + readIndex, toread);
		this->readIndex += (int)toread;
		return;
	}
	else if (len > 0)
	{
		memcpy(buf, buffer + readIndex, len);
		toread -= len;
		bytesRead = len;
	}

	/* area of bytes from start of buffer to write pointer, which happens
	 * when content wraps around to start of buffer
	 */
	memcpy(buf + bytesRead, buffer, toread);
	this->readIndex = (int)toread;
}

/*
 * amount must always be equal to, or less than, the empty space in the buffer.
 * Adjusts write pointer.
 */
void
CyclicBuffer::writeData(const unsigned char * const buf, const unsigned int amount)
{
	unsigned int len;
	unsigned int towrite = amount;
	unsigned int bytesWritten = 0;

	/* area of bytes from read pointer to end of buffer pointer */
	len = this->bufferSize - this->writeIndex;

	if (len >= towrite)
	{
		/* it fits within this part. */
		memcpy(buffer + writeIndex, buf, towrite);
		this->writeIndex += (int)towrite;
		return;
	}
	else if (len > 0)
	{
		memcpy(buffer + writeIndex, buf, len);
		towrite -= len;
		bytesWritten = len;
	}

	/* area of bytes from start of buffer to write pointer, which happens
	 * when content wraps around to start of buffer
	 */
	memcpy(buffer, buf + bytesWritten, towrite);
	this->writeIndex = (int)towrite;
}

void
CyclicBuffer::lock()
{
#ifdef LIBPTH
  (void)pth_mutex_acquire(&rw_mutex, FALSE, NULL);
#elif defined(PTHREADEDMPEG)
  (void)pthread_mutex_lock(&rw_mutex);
#endif
}

void CyclicBuffer::unlock()
{
#ifdef LIBPTH
  (void)pth_mutex_release(&rw_mutex);
#elif defined(PTHREADEDMPEG)
  (void)pthread_mutex_unlock(&rw_mutex);
#endif
}
