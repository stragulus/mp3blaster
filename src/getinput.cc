#include "getinput.h"
#include <string.h>
#include <ctype.h>

getInput::getInput(WINDOW *win, const char *prefill, unsigned int size,
	unsigned int maxlen, int y, int x)
{
	this->win = win;
	this->size = size;
	this->maxlen = maxlen;
	this->y = y;
	this->x = x;
	this->input = new char[maxlen + 1];
	func = NULL;
	func_args = NULL;

	curlen = cursor_pos = view_start = 0;
	insert_mode = 0;
	memset(input, 0, maxlen + 1);

	if (prefill)
	{
		strncpy(input, prefill, maxlen);
		curlen = strlen(prefill);
		mvwaddnstr(win, y, x, input, size);
		if (strlen(prefill) > maxlen)
			curlen = maxlen;
	}

	noecho();
	cbreak();
	leaveok(win, FALSE);
	nodelay(win, FALSE);
	wmove(win, y,x);
	wrefresh(win);
}

getInput::~getInput()
{
	if (input)
		delete[] input;
}

void
getInput::moveLeft(short count)
{
	while (cursor_pos > 0 && count--)
		cursor_pos--;
	updateCursor();
}

void
getInput::moveRight(short count)
{
	while (cursor_pos < curlen && count--)
		cursor_pos++;
	updateCursor();
}

void
getInput::moveToEnd()
{
	while (cursor_pos < curlen)
		cursor_pos++;
	updateCursor();
}

void
getInput::moveToBegin()
{
	cursor_pos = 0;
	updateCursor();
}

/* left is amount of cursor-positions left from the current cursor position
 * to delete a char (and move the chars behind it 1 pos to the left)
 */
void
getInput::deleteChar(unsigned short left)
{
	unsigned int i = 0;

	/* changes to occur within range of string? */
	if (cursor_pos >= left && cursor_pos < (curlen + left))
	{
		for (i = cursor_pos - left; i < curlen - 1; i++)
		{
			input[i] = input[i + 1];
			//mvwaddch(win, y, x + i, input[i]);
		}
		input[--curlen] = '\0';
		//mvwaddch(win, y, x + curlen, ' ');
		cursor_pos -= left;
		updateCursor(1);
	}
}

void
getInput::replaceChar(int c)
{
	if (cursor_pos < maxlen && isprint(c))
	{
		//assume cursor is *always* in the input box.
		mvwaddch(win, y, x + (cursor_pos - view_start), c);
		input[cursor_pos] = c;
		if (cursor_pos == curlen)
			curlen++;
		if (cursor_pos < maxlen - 1)
			cursor_pos++;
		updateCursor();
	}
}

void
getInput::insertChar(int c)
{
	short must_redraw = 0;
	if (curlen == maxlen || !isprint(c))
		return;
	
	int i = (curlen > 0 ? curlen - 1 : 0);
	for (; i >= (int)cursor_pos; i--)
	{
		if (input[i])
		{
			input[i + 1] = input[i];
			must_redraw = 1;
			//mvwaddch(win, y, x + i + 1, input[i + 1]);
		}
	}
	input[cursor_pos] = c;
	//assume cursor is *always* in the input box.
	mvwaddch(win, y, x + (cursor_pos - view_start), c);
	curlen++;
	if (cursor_pos < maxlen - 1)
		cursor_pos++;
	updateCursor(must_redraw);
}

void
getInput::updateCursor(short redraw)
{
	short must_redraw = redraw;
	unsigned int view_end = view_start + (size - 1);

	leaveok(stdscr, FALSE);

	/* does the view over the string shift back or forward? */
	if (cursor_pos < view_start)
	{
		view_start = cursor_pos;
		must_redraw = 1;
	}
	else if (cursor_pos > view_end)
	{
		view_start += (cursor_pos - view_end);
		must_redraw = 1;
	}
	
	if (must_redraw)
	{
		unsigned int string_index;
		char char_to_draw;
		/* redraw entire view */
		for (unsigned int i = 0; i < size; i++)
		{
			char_to_draw = ' ';
			string_index = view_start + i;
			if (string_index < curlen) //character in input string
				char_to_draw = input[string_index];
			mvwaddch(win, y, x + i, char_to_draw);
		}
	}
	wmove(win, y, x + (cursor_pos - view_start));
	wrefresh(win);
}

void
getInput::setCallbackFunction(void (*f)(const char *,void *), void *args)
{
	//TODO: finish..
	func = f;
	if (args)
		func_args = args;
}

short
getInput::setInsertMode(short yesno)
{
	insert_mode = (yesno ? 1 : 0);
	return insert_mode;
}

void
getInput::addChar(int c)
{
	if (insert_mode)
		return insertChar(c);

	return replaceChar(c);
}
