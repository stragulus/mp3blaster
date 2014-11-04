#include "config.h"

#include <string.h>

#include "cyclicbuffer.h"

CyclicBuffer::CyclicBuffer(const unsigned int size)
{
	this->buffer = new unsigned char[size];
	this->bufferSize = size;
	this->readIndex = 0;
	this->writeIndex = 0;
	this->isFull = 0;
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
	} else {
		/* #bytes to read >= amount */
		bytesRead = amount;
	}

	if (bytesRead > 0) {
		readData(buf, bytesRead);
		isFull = 0; //since we just read data, there's always space to write now
	}
	unlock();
	return (int)bytesRead;
}

int
CyclicBuffer::write(const unsigned char * const buf,
	const unsigned int amount, const int partial)
{
	unsigned int emptySpace;
	unsigned int bytesWritten = 0;
	
	lock();
	emptySpace = this->bufferSize - this->contentSizeInternal();

	if (emptySpace == 0) {
		unlock();
		return 0; //no space to write to
	}

	if (emptySpace < amount) {
		if (!partial) {
			unlock();
			return 0; //not enough space to write to
		}

		//partial write
		bytesWritten = emptySpace;
	} else {
		bytesWritten = amount;
	}

	writeData(buf, bytesWritten);

	/* If we filled all of the buffer, toggle isFull. This is required since
	 * it's otherwise impossible to tell if the buffer is full or empty when
	 * the R/W pointers are equal!
	 */
	if (bytesWritten > 0 && this->readIndex == this->writeIndex) {
		isFull = 1;
	}

	unlock();
	return (int)bytesWritten;
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

/**
 * Returns the number of bytes in the buffer that contain data. The
 * empty space would then be the total bufer size minus the return value.
 * This function does not perform locking.
 */
unsigned int
CyclicBuffer::contentSizeInternal()
{
	if (this->readIndex == this->writeIndex) {
		// ____RW_____, either completely empty or completely filled
		if (isFull) {
			return this->bufferSize;
		} 
		return 0; //no content
	} else if (this->readIndex < this->writeIndex) {
		// ___R******W___
		return this->writeIndex - this->readIndex;
	}
	
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
 * Transfers data from the internal buffer into the supplied 'buf'
 * parameter.
 *
 * 'amount' must always be equal to, or less than, the contents in the
 * internal buffer.
 *
 * Adjusts internal buffer's read pointer.
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
