#include <config.h>

#include NCURSES_HEADER
#include <string.h>

class getInput
{
public:
	/* Function   : getInput
	 * Description: Creates a new instance with the contents of 'defval' as
	 *            : input (or empty input buffer if defval == NULL)
	 * Parameters : win
	 *            :  ncurses window object to get input from
	 *            : prefill
	 *            :  contents of input buffer, or NULL
	 *            : size
	 *            :  size of input box
	 *            : maxlen
	 *            :  maximum length of input. If > size, contents of input box
	 *            :  will scroll if necessary.
	 *            : y, x
	 *            :  coordinate of topleft corner of input buffer
	 * Returns    : Nothing
	 * SideEffects: Sets several ncurses input and output options. After
	 *            : calling methods in this class, you'll want to reset them 
	 *            : back to your old values (which this class can't do for 
	 *            : you, since old values can not be read prior to changing them)
	 */
	getInput(WINDOW *win, const char *prefill, unsigned int size,
		unsigned int maxlen, int y, int x);

	~getInput();

	void moveLeft(short count = 1);
	void moveRight(short count = 1);
	void moveToEnd();
	void moveToBegin();

	/* based on insert_mode, either replace or insert a character under cursor */
	void addChar(int c);

	/* inserts character under cursor, and moves all characters previously at
	 * and after cursor 1 pos to the right */
	void insertChar(int c);

	/* replaces character under cursor with character in 'c', and moves cursor
	 * one pos right. */
	void replaceChar(int c);

	/* deletes character 'left' positions left from cursor, and moves
	 * everything at and after that spot one pos to the left */
	void deleteChar(unsigned short left=1);

	/* returns input at the time of calling. Reference will be invalid as soon
	 * as this object is deleted.
	 */
	const char *getString() { return input; }

	/* sets cursor to the correct position in input box. If redraw is 1,
	 * force a redraw of the characters in input box */
	void updateCursor(short redraw=0);

	/* Set the function that should be called after termination of input.
	 * 'args' are optional function parameters you want to pass along.
	 * The function to be called must accept exactly 2 parameters: The first
	 * being a char-pointer, the second being a void pointer ('args' will be
	 * put in that parameter).
	 */
	void setCallbackFunction(void (*f)(const char *, void *), void *args);
	//void getCallbackFunction(void (**f)(char *, void *)) { f = &func; }
	void (*getCallbackFunction())(const char *, void *) { return func; }
	void *getCallbackData() { return func_args; }

	//sets and returns new insert mode
	short setInsertMode(short yesno);
	short getInsertMode() { return insert_mode; }
private:

	char
		*input;	//stores the input in the buffer
	unsigned int
		curlen, //length of input
		cursor_pos, //cursor position in string
		maxlen, //maximum length of the input string
		size, //size of the view (input field)
		view_start; //index of first character of input string in the view
	int
		y, x;
	short
		insert_mode; //0->replace, 1->insert
	WINDOW
		*win;
	void
		(*func)(const char *, void*), //function to call at end of input
		*func_args;
};
