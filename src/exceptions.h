#ifndef MP3BLASTER_EXCEPTIONS_H
#define MP3BLASTER_EXCEPTIONS_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

class IllegalArgumentsException {
public:
	IllegalArgumentsException() { message = ""; }
	IllegalArgumentsException(const char * message) {
		this->message = message;
	}

	const char * getMessage() { return message; }
private:
	const char * message;
};

#endif /* MP3BLASTER_EXCEPTIONS_H */
