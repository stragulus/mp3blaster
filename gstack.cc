#include "mp3blaster.h"
#include <stdlib.h>
#include "gstack.h"

gStack::~gStack()
{
	if (!cstack) // nothing to delete!
		return; 
	while (cstack->prev)
		cstack = cstack->prev;

	/* now we're in the first stacked window.. */

	while (cstack)
	{
		struct stack *nxt = cstack->next;
		delete cstack->win;
		delete[] cstack;
		if (nxt)
			cstack = nxt;
		else
			cstack = NULL;
	}
}

/* Function Name: gStack::insert
 * Description  : Inserts an element to the stack, which will be put behind
 *              : the current cstack (and in front of the previously following
 *              : stack-elements if any).
 * Arguments    : a: scrollWin to be added to the stack. This variable must
 *              : be allocated with new!
 * Yet to be added to the list of functions..
void gStack::insert(scrollWin *a)
{
	struct stack
		*new_elm;

	new_elm = new stack[1];
	new_elm->win = a;
	
	if (cstack)
	{
		struct stack *oldnext;

		new_elm->prev = cstack;
		if ( (oldnext = cstack->next) )
			new_elm->next = oldnext;
		else
			new_elm->next = NULL;

		cstack->next = new_elm;
		new_elm->index = cstack->index + 1;

		// updates indices
		struct stack *tmp_stack = new_elm;
		
		while (tmp_stack->next)
		{
			int old_index = tmp_stack->index;

			tmp_stack = tmp_stack->next;
			tmp_stack->index = old_index + 1;
		}
	}
	else
	{
		new_elm->prev = NULL;
		new_elm->next = NULL;
		new_elm->index = 0;
		cstack = new_elm;
	}
}
*/

/* adds a new entry on top of the stack (top of the stack has highest index
 * number), containing scrollWin a. next/previous-pointers are updated. If
 * the stack was empty before the addition (cstack == NULL), then cstack will
 * point to the newly added stack-entry.
 */
void gStack::add(scrollWin* a)
{
	struct stack
		*tmpstack = cstack,
		*oldstack = NULL,
		*newstack = NULL;

	while (tmpstack)
	{
		oldstack = tmpstack;
		tmpstack = tmpstack->next;
	}
	/* oldstack is now the last entry on the stack, or NULL if the stack's
	   empty */
	newstack = new stack[1];
	newstack->win = a;
	newstack->index = (oldstack ? oldstack->index + 1 : 0);
	newstack->next = NULL;
	newstack->prev = (struct stack*)(oldstack ? oldstack : NULL);

	if (oldstack)
		oldstack->next = newstack;
	else
		oldstack = newstack;

	if (!cstack)
		cstack = newstack;

	++ssize;
}

/* Function Name: gStack::del
 * Description  : deletes current stack-entry from the stack. If deletion was
 *              : successful (entry existed and could be deleted) non-zero will
 *              : be returned. The new current stack-entry will be the one
 *              : following the deleted one (if that entry exists), otherwise
 *              : it will be the previous entry. If that entry also doesn't 
 *              : exist, the stack is empty.
 * Depricated: Function is undesired!
short gStack::del()
{
}
 */

/* Deletes stack-entry with index 'stack_index'. If this entry existed and
 * is deleted, non-zero will be returned. The stack also is updated. If the
 * current stack-entry was the deleted entry, it will point to the next item
 * on the stack. If that item doesn't exist, it will point to the previous
 * item and if that doesn't exist either the stack is empty and cstack will be
 * NULL.
 */
short gStack::del(unsigned int stack_index)
{
	stack
		*tmpstack = cstack;

	while (tmpstack->index != stack_index)
	{
		if (tmpstack->index > stack_index)
			tmpstack = tmpstack->prev;
		else if (tmpstack->index < stack_index)
			tmpstack = tmpstack->next;
		if (!tmpstack) /* stack_index out of range */
			return 0;
	}

	struct stack
		*oldnext = tmpstack->next,
		*oldprev = tmpstack->prev;

	if (cstack == tmpstack) /* deleting cstack, so cstack must be updated! */
	{
		if (tmpstack->next)
			cstack = tmpstack->next;
		else if (tmpstack->prev)
			cstack = tmpstack->prev;
		else
			cstack = NULL;
	}

	/* delete desired entry */
	delete tmpstack->win; 
	delete[] tmpstack;

	/* update links. Both oldprev/oldnext can be NULL which means there was no
	 * previous/next entry. With these 2 statements, the links will always be
	 * updated as desired. */
	if (oldnext)
		oldnext->prev = oldprev;

	if (oldprev)
		oldprev->next = oldnext; 

	/* update indices */
	while (oldnext)
	{
		oldnext->index = (oldnext->index) - 1;
		oldnext = oldnext->next;
	}

	--ssize; /* decease stack-size */

	return 1; /* successful */

}

/* The current scrollwin will be the next scrollwin in the stack. Returns
 * non-zero when there is a next scrollwin in stack.
 */
short gStack::next()
{
	if (!cstack || !cstack->next)
		return 0;
	cstack = cstack->next;
	return 1;
}

short gStack::previous()
{
	if (!cstack || !cstack->prev)
		return 0;

	cstack = cstack->prev;
	return 1;
}

scrollWin* gStack::first()
{
	stack
		*tmpstack = cstack,
		*firststack = NULL;

	while (tmpstack)
	{
		firststack = tmpstack;
		tmpstack = tmpstack->prev;
	}
	
	if (firststack)
		return firststack->win;
	else
		return NULL;
}

scrollWin* gStack::current()
{
	if (cstack)
		return cstack->win;
	
	return NULL;
}

scrollWin* gStack::last()
{
	stack
		*tmpstack = cstack,
		*laststack = NULL;

	while (tmpstack)
	{
		laststack = tmpstack;
		tmpstack = tmpstack->next;
	}
	
	if (laststack)
		return laststack->win;
	else
		return NULL;
}

/* returns stack-entry [stack_index] or NULL if stack_index is out of bounds.
 */
scrollWin* gStack::entry(unsigned int stack_index)
{
	struct stack
		*tmpentry = cstack;

	if (!tmpentry)
		return NULL;
	
	while (tmpentry->index != stack_index)
	{
		if (tmpentry->index > stack_index)
			tmpentry = tmpentry->prev;
		else if (tmpentry->index < stack_index)
			tmpentry = tmpentry->next;

		if (!tmpentry)
			return NULL;
	}

	return tmpentry->win;
}

unsigned int gStack::stackSize()
{
	return ssize;
}
