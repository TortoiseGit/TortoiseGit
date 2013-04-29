/******************************************************************
 *    LexHaskell.cxx
 *
 *    A haskell lexer for the scintilla code control.
 *    Some stuff "lended" from LexPython.cxx and LexCPP.cxx.
 *    External lexer stuff inspired from the caml external lexer.
 *    Folder copied from Python's.
 *
 *    Written by Tobias Engvall - tumm at dtek dot chalmers dot se
 *
 *    Several bug fixes by Krasimir Angelov - kr.angelov at gmail.com
 *
 *    Improvements by kudah <kudahkukarek@gmail.com>
 *
 *    TODO:
 *    * Implement a folder :)
 *    * Nice Character-lexing (stuff inside '\''), LexPython has
 *      this.
 *
 *
 *****************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "PropSetSimple.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

#ifdef BUILD_AS_EXTERNAL_LEXER

#include "ExternalLexer.h"
#include "WindowAccessor.h"

#define BUILD_EXTERNAL_LEXER 0

#endif

#define HA_MODE_DEFAULT     0
#define HA_MODE_IMPORT1     1
#define HA_MODE_IMPORT2     2
#define HA_MODE_IMPORT3     3
#define HA_MODE_MODULE      4
#define HA_MODE_FFI         5
#define HA_MODE_TYPE        6

static inline bool IsAWordStart(const int ch) {
   return (IsLowerCase(ch) || IsUpperCase(ch) || ch == '_');
}

static inline bool IsAWordChar(const int ch) {
   return (  IsAlphaNumeric(ch)
          || ch == '_'
          || ch == '\'');
}

static inline bool IsAnOperatorChar(const int ch) {
   return
      (  ch == '!' || ch == '#' || ch == '$' || ch == '%'
      || ch == '&' || ch == '*' || ch == '+' || ch == '-'
      || ch == '.' || ch == '/' || ch == ':' || ch == '<'
      || ch == '=' || ch == '>' || ch == '?' || ch == '@'
      || ch == '^' || ch == '|' || ch == '~' || ch == '\\');
}

static inline void skipMagicHash(StyleContext &sc, const bool magicHash, const bool twoHashes) {
   if (magicHash && sc.ch == '#') {
      sc.Forward();
      if (twoHashes && sc.ch == '#') {
         sc.Forward();
      }
   }
}

static void ColorizeHaskellDoc(unsigned int startPos, int length, int initStyle,
                               WordList *keywordlists[], Accessor &styler) {

   WordList &keywords = *keywordlists[0];
   WordList &ffi      = *keywordlists[1];

   // property lexer.haskell.allow.hash
   //  Set to 1 to allow the # character in identifiers and literals with the
   //  haskell lexer.
   //  (GHC -XMagicHash extension)
   const bool magicHash = styler.GetPropertyInt("lexer.haskell.allow.hash") != 0;
   // property lexer.haskell.allow.quotes
   //  Set to 1 to enable highlighting of Template Haskell name quotations
   //  and promoted constructors
   //  (GHC -XTemplateHaskell and -XDataKinds extensions)
   const bool allowQuotes = styler.GetPropertyInt("lexer.haskell.allow.quotes") != 0;
   // property lexer.haskell.import.safe
   //  Set to 1 to allow keyword "safe" in imports
   //  (GHC SafeHaskell extensions)
   const bool highlightSafe = styler.GetPropertyInt("lexer.haskell.import.safe") != 0;
   const bool stylingWithinPreprocessor = styler.GetPropertyInt("styling.within.preprocessor") != 0;

   StyleContext sc(startPos, length, initStyle, styler);

   int lineCurrent = styler.GetLine(startPos);

   int state = lineCurrent ? styler.GetLineState(lineCurrent-1) : 0;
   int mode  = state & 0xF;
   int nestLevel = state >> 4;

   int base = 10;
   bool inDashes = false;

   while (sc.More()) {
      // Check for state end

      // For line numbering (and by extension, nested comments) to work,
      // states should either only forward one character at a time, or check
      // that characters they're skipping are not newlines. If states match on
      // line end, they should skip it, to prevent double counting.
      if (sc.atLineEnd) {
         // Remember the line state for future incremental lexing
         styler.SetLineState(lineCurrent, (nestLevel << 4) | mode);
         lineCurrent++;
      }

      // Handle line continuation generically.
      if (sc.ch == '\\' &&
         (  sc.state == SCE_HA_STRING
         || sc.state == SCE_HA_PREPROCESSOR)) {
         if (sc.chNext == '\n' || sc.chNext == '\r') {
            // Remember the line state for future incremental lexing
            styler.SetLineState(lineCurrent, (nestLevel << 4) | mode);
            lineCurrent++;

            sc.Forward();
            if (sc.ch == '\r' && sc.chNext == '\n') {
               sc.Forward();
            }
            sc.Forward();
            continue;
         }
      }

         // Operator
      if (sc.state == SCE_HA_OPERATOR) {
         int style = SCE_HA_OPERATOR;

         if (sc.ch == ':' &&
            // except "::"
            !(sc.chNext == ':' && !IsAnOperatorChar(sc.GetRelative(2)))) {
            style = SCE_HA_CAPITAL;
         }

         while (IsAnOperatorChar(sc.ch))
               sc.Forward();

         styler.ColourTo(sc.currentPos - 1, style);
         sc.ChangeState(SCE_HA_DEFAULT);
      }
         // String
      else if (sc.state == SCE_HA_STRING) {
         if (sc.ch == '\"') {
            sc.Forward();
            skipMagicHash(sc, magicHash, false);
            sc.SetState(SCE_HA_DEFAULT);
         } else if (sc.ch == '\\') {
            sc.Forward(2);
         } else if (sc.atLineEnd) {
            sc.SetState(SCE_HA_DEFAULT);
            sc.Forward(); // prevent double counting a line
         } else {
            sc.Forward();
         }
      }
         // Char
      else if (sc.state == SCE_HA_CHARACTER) {
         if (sc.ch == '\'') {
            sc.Forward();
            skipMagicHash(sc, magicHash, false);
            sc.SetState(SCE_HA_DEFAULT);
         } else if (sc.ch == '\\') {
            sc.Forward(2);
         } else if (sc.atLineEnd) {
            sc.SetState(SCE_HA_DEFAULT);
            sc.Forward(); // prevent double counting a line
         } else {
            sc.Forward();
         }
      }
         // Number
      else if (sc.state == SCE_HA_NUMBER) {
         if (IsADigit(sc.ch, base) ||
            (sc.ch=='.' && IsADigit(sc.chNext, base))) {
            sc.Forward();
         } else if ((base == 10) &&
                    (sc.ch == 'e' || sc.ch == 'E') &&
                    (IsADigit(sc.chNext) || sc.chNext == '+' || sc.chNext == '-')) {
            sc.Forward();
            if (sc.ch == '+' || sc.ch == '-')
                sc.Forward();
         } else {
            skipMagicHash(sc, magicHash, true);
            sc.SetState(SCE_HA_DEFAULT);
         }
      }
         // Keyword or Identifier
      else if (sc.state == SCE_HA_IDENTIFIER) {
         int style = isupper(sc.ch) ? SCE_HA_CAPITAL : SCE_HA_IDENTIFIER;

         sc.Forward();

         while (sc.More()) {
            if (IsAWordChar(sc.ch)) {
               sc.Forward();
            } else if (sc.ch == '#' && magicHash) {
               sc.Forward();
               break;
            } else if (style == SCE_HA_CAPITAL && sc.ch=='.') {
               if (isupper(sc.chNext)) {
                  sc.Forward();
                  style = SCE_HA_CAPITAL;
               } else if (IsAWordStart(sc.chNext)) {
                  sc.Forward();
                  style = SCE_HA_IDENTIFIER;
               } else if (IsAnOperatorChar(sc.chNext)) {
                  sc.Forward();
                  style = sc.ch == ':' ? SCE_HA_CAPITAL : SCE_HA_OPERATOR;
                  while (IsAnOperatorChar(sc.ch))
                     sc.Forward();
                  break;
               } else {
                  break;
               }
            } else {
               break;
            }
         }

         char s[100];
         sc.GetCurrent(s, sizeof(s));

         int new_mode = HA_MODE_DEFAULT;

         if (keywords.InList(s)) {
            style = SCE_HA_KEYWORD;
         } else if (isupper(s[0])) {
            if (mode == HA_MODE_IMPORT1 || mode == HA_MODE_IMPORT3) {
               style    = SCE_HA_MODULE;
               new_mode = HA_MODE_IMPORT2;
            } else if (mode == HA_MODE_MODULE) {
               style = SCE_HA_MODULE;
            }
         } else if (mode == HA_MODE_IMPORT1 &&
                    strcmp(s,"qualified") == 0) {
             style    = SCE_HA_KEYWORD;
             new_mode = HA_MODE_IMPORT1;
         } else if (highlightSafe &&
                    mode == HA_MODE_IMPORT1 &&
                    strcmp(s,"safe") == 0) {
             style    = SCE_HA_KEYWORD;
             new_mode = HA_MODE_IMPORT1;
         } else if (mode == HA_MODE_IMPORT2) {
             if (strcmp(s,"as") == 0) {
                style    = SCE_HA_KEYWORD;
                new_mode = HA_MODE_IMPORT3;
            } else if (strcmp(s,"hiding") == 0) {
                style     = SCE_HA_KEYWORD;
            }
         } else if (mode == HA_MODE_TYPE) {
            if (strcmp(s,"family") == 0)
               style    = SCE_HA_KEYWORD;
         }

         if (mode == HA_MODE_FFI) {
            if (ffi.InList(s)) {
               style = SCE_HA_KEYWORD;
               new_mode = HA_MODE_FFI;
            }
         }

         styler.ColourTo(sc.currentPos - 1, style);

         if (strcmp(s,"import") == 0 && mode != HA_MODE_FFI)
            new_mode = HA_MODE_IMPORT1;
         else if (strcmp(s,"module") == 0)
            new_mode = HA_MODE_MODULE;
         else if (strcmp(s,"foreign") == 0)
            new_mode = HA_MODE_FFI;
         else if (strcmp(s,"type") == 0
               || strcmp(s,"data") == 0)
            new_mode = HA_MODE_TYPE;

         sc.ChangeState(SCE_HA_DEFAULT);
         mode = new_mode;
      }

         // Comments
            // Oneliner
      else if (sc.state == SCE_HA_COMMENTLINE) {
         if (inDashes && sc.ch != '-') {
            inDashes = false;
            if (IsAnOperatorChar(sc.ch))
               sc.ChangeState(SCE_HA_OPERATOR);
         } else if (sc.atLineEnd) {
            sc.SetState(SCE_HA_DEFAULT);
            sc.Forward(); // prevent double counting a line
         } else {
            sc.Forward();
         }
      }
            // Nested
      else if (sc.state == SCE_HA_COMMENTBLOCK) {
         if (sc.Match('{','-')) {
            sc.Forward(2);
            nestLevel++;
         }
         else if (sc.Match('-','}')) {
            sc.Forward(2);
            nestLevel--;
            if (nestLevel == 0) {
               sc.SetState(SCE_HA_DEFAULT);
            }
         } else {
            sc.Forward();
         }
      }
            // Pragma
      else if (sc.state == SCE_HA_PRAGMA) {
         // GHC pragma end should always be indented further than it's start.
         if (sc.Match("#-}") && !sc.atLineStart) {
            sc.Forward(3);
            sc.SetState(SCE_HA_DEFAULT);
         } else {
            sc.Forward();
         }
      }
            // Preprocessor
      else if (sc.state == SCE_HA_PREPROCESSOR) {
         if (stylingWithinPreprocessor && !IsAWordStart(sc.ch)) {
            sc.SetState(SCE_HA_DEFAULT);
         } else if (sc.atLineEnd) {
            sc.SetState(SCE_HA_DEFAULT);
            sc.Forward(); // prevent double counting a line
         } else {
            sc.Forward();
         }
      }
            // New state?
      else if (sc.state == SCE_HA_DEFAULT) {
         // Digit
         if (IsADigit(sc.ch)) {
            sc.SetState(SCE_HA_NUMBER);
            if (sc.ch == '0' && (sc.chNext == 'X' || sc.chNext == 'x')) {
               // Match anything starting with "0x" or "0X", too
               sc.Forward(2);
               base = 16;
            } else if (sc.ch == '0' && (sc.chNext == 'O' || sc.chNext == 'o')) {
               // Match anything starting with "0x" or "0X", too
               sc.Forward(2);
               base = 8;
            } else {
               sc.Forward();
               base = 10;
            }
            mode = HA_MODE_DEFAULT;
         }
         // Pragma
         else if (sc.Match("{-#")) {
            sc.SetState(SCE_HA_PRAGMA);
            sc.Forward(3);
         }
         // Comment line
         else if (sc.Match('-','-')) {
            sc.SetState(SCE_HA_COMMENTLINE);
            sc.Forward(2);
            inDashes = true;
         }
         // Comment block
         else if (sc.Match('{','-')) {
            sc.SetState(SCE_HA_COMMENTBLOCK);
            sc.Forward(2);
            nestLevel++;
         }
         // String
         else if (sc.ch == '\"') {
            sc.SetState(SCE_HA_STRING);
            sc.Forward();
         }
         // Character or quoted name
         else if (sc.ch == '\'') {
            styler.ColourTo(sc.currentPos - 1, state);
            sc.Forward();

            int style = SCE_HA_CHARACTER;

            if (allowQuotes) {
               // Quoted type ''T
               if (sc.ch=='\'' && IsAWordStart(sc.chNext)) {
                  sc.Forward();
                  style=SCE_HA_IDENTIFIER;
               } else if (sc.chNext != '\'') {
                  // Quoted value or promoted constructor 'N
                  if (IsAWordStart(sc.ch)) {
                     style=SCE_HA_IDENTIFIER;
                  // Promoted constructor operator ':~>
                  } else if (sc.ch == ':') {
                     style=SCE_HA_OPERATOR;
                  // Promoted list or tuple '[T]
                  } else if (sc.ch == '[' || sc.ch== '(') {
                     styler.ColourTo(sc.currentPos - 1, SCE_HA_OPERATOR);
                     style=SCE_HA_DEFAULT;
                  }
               }
            }

            sc.ChangeState(style);
         }
         // Preprocessor
         else if (sc.atLineStart && sc.ch == '#') {
            mode = HA_MODE_DEFAULT;
            sc.SetState(SCE_HA_PREPROCESSOR);
            sc.Forward();
         }
         // Operator
         else if (IsAnOperatorChar(sc.ch)) {
            mode = HA_MODE_DEFAULT;
            sc.SetState(SCE_HA_OPERATOR);
         }
         // Braces and punctuation
         else if (sc.ch == ',' || sc.ch == ';'
               || sc.ch == '(' || sc.ch == ')'
               || sc.ch == '[' || sc.ch == ']'
               || sc.ch == '{' || sc.ch == '}') {
            sc.SetState(SCE_HA_OPERATOR);
            sc.Forward();
            sc.SetState(SCE_HA_DEFAULT);
         }
         // Keyword or Identifier
         else if (IsAWordStart(sc.ch)) {
            sc.SetState(SCE_HA_IDENTIFIER);
         // Something we don't care about
         } else {
            sc.Forward();
         }
      }
   }
   sc.Complete();
}

static bool IsCommentLine(int line, Accessor &styler) {
   int pos = styler.LineStart(line);
   int eol_pos = styler.LineStart(line + 1) - 1;

   for (int i = pos; i < eol_pos; i++) {
      int ch = styler[i];
      int style = styler.StyleAt(i);

      if ((style < SCE_HA_COMMENTLINE || style > SCE_HA_COMMENTBLOCK3)
         && ch != ' '
         && ch != '\t') {
         return false;
      }
   }
   return true;
}

static void FoldHaskellDoc(unsigned int startPos, int length, int // initStyle
                          ,WordList *[], Accessor &styler) {
   const int maxPos = startPos + length;
   const int maxLines =
      maxPos == styler.Length()
         ? styler.GetLine(maxPos)
         : styler.GetLine(maxPos - 1);  // Requested last line
   const int docLines = styler.GetLine(styler.Length()); // Available last line

   const bool foldCompact = styler.GetPropertyInt("fold.compact") != 0;
   // const bool foldComment = styler.GetPropertyInt("fold.comment") != 0;

   // Backtrack to previous non-blank line so we can determine indent level
   // for any white space lines
   // and so we can fix any preceding fold level (which is why we go back
   // at least one line in all cases)
   int spaceFlags = 0;
   int lineCurrent = styler.GetLine(startPos);
   int indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, NULL);
   while (lineCurrent > 0) {
      lineCurrent--;
      indentCurrent = styler.IndentAmount(lineCurrent, &spaceFlags, NULL);
      if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG) &&
               !IsCommentLine(lineCurrent, styler))
         break;
   }
   int indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;

   // Set up initial loop state
   startPos = styler.LineStart(lineCurrent);

   // Process all characters to end of requested range
   //that hangs over the end of the range.  Cap processing in all cases
   // to end of document.
   while (lineCurrent <= docLines && lineCurrent <= maxLines) {

      // Gather info
      int lev = indentCurrent;
      int lineNext = lineCurrent + 1;
      int indentNext = indentCurrent;
      if (lineNext <= docLines) {
         // Information about next line is only available if not at end of document
         indentNext = styler.IndentAmount(lineNext, &spaceFlags, NULL);
      }
      if (indentNext & SC_FOLDLEVELWHITEFLAG)
         indentNext = SC_FOLDLEVELWHITEFLAG | indentCurrentLevel;

      // Skip past any blank lines for next indent level info; we skip also
      // comments (all comments, not just those starting in column 0)
      // which effectively folds them into surrounding code rather
      // than screwing up folding.

      while ((lineNext < docLines) &&
            ((indentNext & SC_FOLDLEVELWHITEFLAG) ||
             (lineNext <= docLines && IsCommentLine(lineNext, styler)))) {
         lineNext++;
         indentNext = styler.IndentAmount(lineNext, &spaceFlags, NULL);
      }

      const int levelAfterComments = indentNext & SC_FOLDLEVELNUMBERMASK;
      const int levelBeforeComments = Maximum(indentCurrentLevel,levelAfterComments);

      // Now set all the indent levels on the lines we skipped
      // Do this from end to start.  Once we encounter one line
      // which is indented more than the line after the end of
      // the comment-block, use the level of the block before

      int skipLine = lineNext;
      int skipLevel = levelAfterComments;

      while (--skipLine > lineCurrent) {
         int skipLineIndent = styler.IndentAmount(skipLine, &spaceFlags, NULL);

         if (foldCompact) {
            if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > levelAfterComments) {
               skipLevel = levelBeforeComments;
            }

            int whiteFlag = skipLineIndent & SC_FOLDLEVELWHITEFLAG;

            styler.SetLevel(skipLine, skipLevel | whiteFlag);
         } else {
            if (  (skipLineIndent & SC_FOLDLEVELNUMBERMASK) > levelAfterComments
               && !(skipLineIndent & SC_FOLDLEVELWHITEFLAG)
               && !IsCommentLine(skipLine, styler)) {
               skipLevel = levelBeforeComments;
            }

            styler.SetLevel(skipLine, skipLevel);
         }
      }

      if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG)) {
         if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK))
            lev |= SC_FOLDLEVELHEADERFLAG;
      }

      // Set fold level for this line and move to next line
      styler.SetLevel(lineCurrent, foldCompact ? lev : lev & ~SC_FOLDLEVELWHITEFLAG);
      indentCurrent = indentNext;
      lineCurrent = lineNext;
   }

   // NOTE: Cannot set level of last line here because indentCurrent doesn't have
   // header flag set; the loop above is crafted to take care of this case!
   //styler.SetLevel(lineCurrent, indentCurrent);
}

static const char * const haskellWordListDesc[] = {
   "Keywords",
   "FFI",
   0
};

// External stuff - used for dynamic-loading, not implemented in wxStyledTextCtrl yet.
// Inspired by the caml external lexer - Credits to Robert Roessler - http://www.rftp.com
#ifdef BUILD_EXTERNAL_LEXER
static const char* LexerName = "haskell";

void EXT_LEXER_DECL Lex(unsigned int lexer, unsigned int startPos, int length, int initStyle,
                        char *words[], WindowID window, char *props)
{
   PropSetSimple ps;
   ps.SetMultiple(props);
   WindowAccessor wa(window, ps);

   int nWL = 0;
   for (; words[nWL]; nWL++) ;
   WordList** wl = new WordList* [nWL + 1];
   int i = 0;
   for (; i<nWL; i++)
   {
      wl[i] = new WordList();
      wl[i]->Set(words[i]);
   }
   wl[i] = 0;

   ColorizeHaskellDoc(startPos, length, initStyle, wl, wa);
   wa.Flush();
   for (i=nWL-1;i>=0;i--)
      delete wl[i];
   delete [] wl;
}

void EXT_LEXER_DECL Fold (unsigned int lexer, unsigned int startPos, int length, int initStyle,
                        char *words[], WindowID window, char *props)
{

}

int EXT_LEXER_DECL GetLexerCount()
{
   return 1;
}

void EXT_LEXER_DECL GetLexerName(unsigned int Index, char *name, int buflength)
{
   if (buflength > 0) {
      buflength--;
      int n = strlen(LexerName);
      if (n > buflength)
         n = buflength;
      memcpy(name, LexerName, n), name[n] = '\0';
   }
}
#endif

LexerModule lmHaskell(SCLEX_HASKELL, ColorizeHaskellDoc, "haskell", FoldHaskellDoc, haskellWordListDesc);
