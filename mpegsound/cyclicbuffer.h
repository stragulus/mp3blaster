#ifndef _MPEGSOUND_CYCLICBUFFER_
#define _MPEGSOUND_CYCLICBUFFER_

#ifdef LIBPTH
# include <pth.h>
#elif defined(PTHREADEDMPEG)
# include <pthread.h>
#endif

class CyclicBuffer
{
public:
	CyclicBuffer(const unsigned int size);
	~CyclicBuffer();

	/* Function   : read
	 * Description: Read bytes from the buffer
	 * Parameters : buf
	 *            :  Buffer to store read data in. Must be large enough.
	 *            : amount
	 *            :  Amount of data to read
	 *            : partial
	 *            :  If non-zero, allow partial reads (e.g. less than requested)
	 * Returns    : The number of bytes read into the buffer
	 * SideEffects: None.
	 */
	int read(unsigned char * const buf, const unsigned int amount,
		const int partial = 0);

	/* Function   : write
	 * Description: Writes data into the buffer.
	 * Parameters : buf
	 *            :  Buffer containing the data to be written
	 *            : amount
	 *            :  Number of bytes to write
	 *            : partial
	 *            :  If non-zero, allow partial writes (e.g. less than requested)
	 * Returns    : The number of bytes written into the buffer
	 * SideEffects: None.
	 */
	int write(const unsigned char * const buf, const unsigned int amount,
		const int partial = 0);
	
	/* Function   : setEmpty
	 * Description: Empty the contents of the buffer. The size of the buffer
	 *            : is unaffected.
	 * Parameters : None.
	 * Returns    : Nothing.
	 * SideEffects: None.
	 */
	void setEmpty(void);

	/* Function   : contentSize
	 * Description: Calculates the size of the contents of the buffer
	 * Parameters : None.
	 * Returns    : The number of stored, non-read bytes in the buffer.
	 * SideEffects: None.
	 */
	unsigned int contentSize(void);

	/* total size - contentsize */
	unsigned int emptySpace(void);

	/* Function   : size
	 * Description: Size of the entire buffer.
	 * Parameters : None.
	 * Returns    : Number of bytes this buffer can contain
	 * SideEffects: None.
	 */
	unsigned int size(void);

private:
	void readData(unsigned char * const buf, const unsigned int amount);
	void writeData(const unsigned char * const buf, const unsigned int amount);
	void lock(void);
	void unlock(void);
	unsigned int contentSizeInternal(void); //does not lock/unlock
	unsigned int emptySpaceInternal(void); //does not lock/unlock

	unsigned char *buffer;
	int readIndex; //where are we reading
	int writeIndex; //where are we writing
	int isFull; //if readIndex==writeIndex, this indicates the filled status
	unsigned int bufferSize; //size of buffer

#ifdef LIBPTH
	pth_mutex_t rw_mutex;
#elif defined(PTHREADEDMPEG)
	pthread_mutex_t rw_mutex;
#endif
};

#endif /* _MPEGSOUND_CYCLICBUFFER_ */
