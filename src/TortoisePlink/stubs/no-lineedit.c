/*
 * Stubs of functions in lineedit.c, for use in programs that don't
 * have any use for line editing (e.g. because they don't have a
 * terminal either).
 */

#include "putty.h"
#include "terminal.h"

TermLineEditor *lineedit_new(Terminal *term, unsigned flags,
                             TermLineEditorCallbackReceiver *receiver)
{
    return NULL;
}
void lineedit_free(TermLineEditor *le) {}
void lineedit_input(TermLineEditor *le, char ch, bool dedicated) {}
void lineedit_modify_flags(TermLineEditor *le, unsigned clr, unsigned flip) {}
void lineedit_send_line(TermLineEditor *le) {}
