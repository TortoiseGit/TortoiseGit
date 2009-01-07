// TortoiseBlame - a Viewer for Subversion Blames

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#include "stdafx.h"

// disable "dead code eleminated" warning
#pragma warning(disable:4505)

#include "TortoiseBlame.h"

void TortoiseBlame::SetupLexer(LPCSTR filename)
{
	char line[20];
	const char * lineptr = _tcsrchr(filename, '.');

	if (lineptr)
	{
		_tcscpy_s(line, 20, lineptr+1);
		_tcslwr_s(line, 20);
		if ((_tcscmp(line, _T("py"))==0)||
			(_tcscmp(line, _T("pyw"))==0)||
			(_tcscmp(line, _T("pyw"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PYTHON);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield"));
			SetAStyle(SCE_P_DEFAULT, black);
			SetAStyle(SCE_P_COMMENTLINE, darkGreen);
			SetAStyle(SCE_P_NUMBER, RGB(0, 0x80, 0x80));
			SetAStyle(SCE_P_STRING, RGB(0, 0, 0x80));
			SetAStyle(SCE_P_CHARACTER, RGB(0, 0, 0x80));
			SetAStyle(SCE_P_WORD, RGB(0x80, 0, 0x80));
			SetAStyle(SCE_P_TRIPLE, black);
			SetAStyle(SCE_P_TRIPLEDOUBLE, black);
			SetAStyle(SCE_P_CLASSNAME, darkBlue);
			SetAStyle(SCE_P_DEFNAME, darkBlue);
			SetAStyle(SCE_P_OPERATOR, darkBlue);
			SetAStyle(SCE_P_IDENTIFIER, darkBlue);
			SetAStyle(SCE_P_COMMENTBLOCK, darkGreen);
			SetAStyle(SCE_P_STRINGEOL, red);
		}
		if ((_tcscmp(line, _T("c"))==0)||
			(_tcscmp(line, _T("cc"))==0)||
			(_tcscmp(line, _T("cpp"))==0)||
			(_tcscmp(line, _T("cxx"))==0)||
			(_tcscmp(line, _T("h"))==0)||
			(_tcscmp(line, _T("hh"))==0)||
			(_tcscmp(line, _T("hpp"))==0)||
			(_tcscmp(line, _T("hxx"))==0)||
			(_tcscmp(line, _T("dlg"))==0)||
			(_tcscmp(line, _T("mak"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("and and_eq asm auto bitand bitor bool break \
case catch char class compl const const_cast continue \
default delete do double dynamic_cast else enum explicit export extern false float for \
friend goto if inline int long mutable namespace new not not_eq \
operator or or_eq private protected public \
register reinterpret_cast return short signed sizeof static static_cast struct switch \
template this throw true try typedef typeid typename union unsigned using \
virtual void volatile wchar_t while xor xor_eq"));
			SendEditor(SCI_SETKEYWORDS, 3, (LPARAM)_T("a addindex addtogroup anchor arg attention \
author b brief bug c class code date def defgroup deprecated dontinclude \
e em endcode endhtmlonly endif endlatexonly endlink endverbatim enum example exception \
f$ f[ f] file fn hideinitializer htmlinclude htmlonly \
if image include ingroup internal invariant interface latexonly li line link \
mainpage name namespace nosubgrouping note overload \
p page par param post pre ref relates remarks return retval \
sa section see showinitializer since skip skipline struct subsection \
test throw todo typedef union until \
var verbatim verbinclude version warning weakgroup $ @ \\ & < > # { }"));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("cs"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("abstract as base bool break byte case catch char checked class \
const continue decimal default delegate do double else enum \
event explicit extern false finally fixed float for foreach goto if \
implicit in int interface internal is lock long namespace new null \
object operator out override params private protected public \
readonly ref return sbyte sealed short sizeof stackalloc static \
string struct switch this throw true try typeof uint ulong \
unchecked unsafe ushort using virtual void while"));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("rc"))==0)||
			(_tcscmp(line, _T("rc2"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("ACCELERATORS ALT AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON \
BEGIN BITMAP BLOCK BUTTON CAPTION CHARACTERISTICS CHECKBOX CLASS \
COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG DIALOGEX DISCARDABLE \
EDITTEXT END EXSTYLE FONT GROUPBOX ICON LANGUAGE LISTBOX LTEXT \
MENU MENUEX MENUITEM MESSAGETABLE POPUP \
PUSHBUTTON RADIOBUTTON RCDATA RTEXT SCROLLBAR SEPARATOR SHIFT STATE3 \
STRINGTABLE STYLE TEXTINCLUDE VALUE VERSION VERSIONINFO VIRTKEY"));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("idl"))==0)||
			(_tcscmp(line, _T("odl"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("aggregatable allocate appobject arrays async async_uuid \
auto_handle \
bindable boolean broadcast byte byte_count \
call_as callback char coclass code comm_status \
const context_handle context_handle_noserialize \
context_handle_serialize control cpp_quote custom \
decode default defaultbind defaultcollelem \
defaultvalue defaultvtable dispinterface displaybind dllname \
double dual \
enable_allocate encode endpoint entry enum error_status_t \
explicit_handle \
fault_status first_is float \
handle_t heap helpcontext helpfile helpstring \
helpstringcontext helpstringdll hidden hyper \
id idempotent ignore iid_as iid_is immediatebind implicit_handle \
import importlib in include in_line int __int64 __int3264 interface \
last_is lcid length_is library licensed local long \
max_is maybe message methods midl_pragma \
midl_user_allocate midl_user_free min_is module ms_union \
ncacn_at_dsp ncacn_dnet_nsp ncacn_http ncacn_ip_tcp \
ncacn_nb_ipx ncacn_nb_nb ncacn_nb_tcp ncacn_np \
ncacn_spx ncacn_vns_spp ncadg_ip_udp ncadg_ipx ncadg_mq \
ncalrpc nocode nonbrowsable noncreatable nonextensible notify \
object odl oleautomation optimize optional out out_of_line \
pipe pointer_default pragma properties propget propput propputref \
ptr public \
range readonly ref represent_as requestedit restricted retval \
shape short signed size_is small source strict_context_handle \
string struct switch switch_is switch_type \
transmit_as typedef \
uidefault union unique unsigned user_marshal usesgetlasterror uuid \
v1_enum vararg version void wchar_t wire_marshal"));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("java"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("abstract assert boolean break byte case catch char class \
const continue default do double else extends final finally float for future \
generic goto if implements import inner instanceof int interface long \
native new null outer package private protected public rest \
return short static super switch synchronized this throw throws \
transient try var void volatile while"));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("js"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("abstract boolean break byte case catch char class \
const continue debugger default delete do double else enum export extends \
final finally float for function goto if implements import in instanceof \
int interface long native new package private protected public \
return short static super switch synchronized this throw throws \
transient try typeof var void volatile while with"));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("pas"))==0)||
			(_tcscmp(line, _T("dpr"))==0)||
			(_tcscmp(line, _T("pp"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PASCAL);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("and array as begin case class const constructor \
destructor div do downto else end except file finally \
for function goto if implementation in inherited \
interface is mod not object of on or packed \
procedure program property raise record repeat \
set shl shr then threadvar to try type unit \
until uses var while with xor"));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("as"))==0)||
			(_tcscmp(line, _T("asc"))==0)||
			(_tcscmp(line, _T("jsfl"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("add and break case catch class continue default delete do \
dynamic else eq extends false finally for function ge get gt if implements import in \
instanceof interface intrinsic le lt ne new not null or private public return \
set static super switch this throw true try typeof undefined var void while with"));
			SendEditor(SCI_SETKEYWORDS, 1, (LPARAM)_T("Array Arguments Accessibility Boolean Button Camera Color \
ContextMenu ContextMenuItem Date Error Function Key LoadVars LocalConnection Math \
Microphone Mouse MovieClip MovieClipLoader NetConnection NetStream Number Object \
PrintJob Selection SharedObject Sound Stage String StyleSheet System TextField \
TextFormat TextSnapshot Video Void XML XMLNode XMLSocket \
_accProps _focusrect _global _highquality _parent _quality _root _soundbuftime \
arguments asfunction call capabilities chr clearInterval duplicateMovieClip \
escape eval fscommand getProperty getTimer getURL getVersion gotoAndPlay gotoAndStop \
ifFrameLoaded Infinity -Infinity int isFinite isNaN length loadMovie loadMovieNum \
loadVariables loadVariablesNum maxscroll mbchr mblength mbord mbsubstring MMExecute \
NaN newline nextFrame nextScene on onClipEvent onUpdate ord parseFloat parseInt play \
prevFrame prevScene print printAsBitmap printAsBitmapNum printNum random removeMovieClip \
scroll set setInterval setProperty startDrag stop stopAllSounds stopDrag substring \
targetPath tellTarget toggleHighQuality trace unescape unloadMovie unLoadMovieNum updateAfterEvent"));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("html"))==0)||
			(_tcscmp(line, _T("htm"))==0)||
			(_tcscmp(line, _T("shtml"))==0)||
			(_tcscmp(line, _T("htt"))==0)||
			(_tcscmp(line, _T("xml"))==0)||
			(_tcscmp(line, _T("asp"))==0)||
			(_tcscmp(line, _T("xsl"))==0)||
			(_tcscmp(line, _T("php"))==0)||
			(_tcscmp(line, _T("xhtml"))==0)||
			(_tcscmp(line, _T("phtml"))==0)||
			(_tcscmp(line, _T("cfm"))==0)||
			(_tcscmp(line, _T("tpl"))==0)||
			(_tcscmp(line, _T("dtd"))==0)||
			(_tcscmp(line, _T("hta"))==0)||
			(_tcscmp(line, _T("htd"))==0)||
			(_tcscmp(line, _T("wxs"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_HTML);
			SendEditor(SCI_SETSTYLEBITS, 7);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)_T("a abbr acronym address applet area b base basefont \
bdo big blockquote body br button caption center \
cite code col colgroup dd del dfn dir div dl dt em \
fieldset font form frame frameset h1 h2 h3 h4 h5 h6 \
head hr html i iframe img input ins isindex kbd label \
legend li link map menu meta noframes noscript \
object ol optgroup option p param pre q s samp \
script select small span strike strong style sub sup \
table tbody td textarea tfoot th thead title tr tt u ul \
var xml xmlns abbr accept-charset accept accesskey action align alink \
alt archive axis background bgcolor border \
cellpadding cellspacing char charoff charset checked cite \
class classid clear codebase codetype color cols colspan \
compact content coords \
data datafld dataformatas datapagesize datasrc datetime \
declare defer dir disabled enctype event \
face for frame frameborder \
headers height href hreflang hspace http-equiv \
id ismap label lang language leftmargin link longdesc \
marginwidth marginheight maxlength media method multiple \
name nohref noresize noshade nowrap \
object onblur onchange onclick ondblclick onfocus \
onkeydown onkeypress onkeyup onload onmousedown \
onmousemove onmouseover onmouseout onmouseup \
onreset onselect onsubmit onunload \
profile prompt readonly rel rev rows rowspan rules \
scheme scope selected shape size span src standby start style \
summary tabindex target text title topmargin type usemap \
valign value valuetype version vlink vspace width \
text password checkbox radio submit reset \
file hidden image"));
			SendEditor(SCI_SETKEYWORDS, 1, (LPARAM)_T("assign audio block break catch choice clear disconnect else elseif \
emphasis enumerate error exit field filled form goto grammar help \
if initial link log menu meta noinput nomatch object option p paragraph \
param phoneme prompt property prosody record reprompt return s say-as \
script sentence subdialog submit throw transfer value var voice vxml"));
			SendEditor(SCI_SETKEYWORDS, 2, (LPARAM)_T("accept age alphabet anchor application base beep bridge category charset \
classid cond connecttimeout content contour count dest destexpr dtmf dtmfterm \
duration enctype event eventexpr expr expritem fetchtimeout finalsilence \
gender http-equiv id level maxage maxstale maxtime message messageexpr \
method mime modal mode name namelist next nextitem ph pitch range rate \
scope size sizeexpr skiplist slot src srcexpr sub time timeexpr timeout \
transferaudio type value variant version volume xml:lang"));
			SendEditor(SCI_SETKEYWORDS, 3, (LPARAM)_T("and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield"));
			SendEditor(SCI_SETKEYWORDS, 4, (LPARAM)_T("and argv as argc break case cfunction class continue declare default do \
die echo else elseif empty enddeclare endfor endforeach endif endswitch \
endwhile e_all e_parse e_error e_warning eval exit extends false for \
foreach function global http_cookie_vars http_get_vars http_post_vars \
http_post_files http_env_vars http_server_vars if include include_once \
list new not null old_function or parent php_os php_self php_version \
print require require_once return static switch stdclass this true var \
xor virtual while __file__ __line__ __sleep __wakeup"));

			SetAStyle(SCE_H_TAG, darkBlue);
			SetAStyle(SCE_H_TAGUNKNOWN, red);
			SetAStyle(SCE_H_ATTRIBUTE, darkBlue);
			SetAStyle(SCE_H_ATTRIBUTEUNKNOWN, red);
			SetAStyle(SCE_H_NUMBER, RGB(0x80,0,0x80));
			SetAStyle(SCE_H_DOUBLESTRING, RGB(0,0x80,0));
			SetAStyle(SCE_H_SINGLESTRING, RGB(0,0x80,0));
			SetAStyle(SCE_H_OTHER, RGB(0x80,0,0x80));
			SetAStyle(SCE_H_COMMENT, RGB(0x80,0x80,0));
			SetAStyle(SCE_H_ENTITY, RGB(0x80,0,0x80));

			SetAStyle(SCE_H_TAGEND, darkBlue);
			SetAStyle(SCE_H_XMLSTART, darkBlue);	// <?
			SetAStyle(SCE_H_QUESTION, darkBlue);	// <?
			SetAStyle(SCE_H_XMLEND, darkBlue);		// ?>
			SetAStyle(SCE_H_SCRIPT, darkBlue);		// <script
			SetAStyle(SCE_H_ASP, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <% ... %>
			SetAStyle(SCE_H_ASPAT, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <%@ ... %>

			SetAStyle(SCE_HB_DEFAULT, black);
			SetAStyle(SCE_HB_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HB_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HB_WORD, darkBlue);
			SendEditor(SCI_STYLESETBOLD, SCE_HB_WORD, 1);
			SetAStyle(SCE_HB_STRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HB_IDENTIFIER, black);

			// This light blue is found in the windows system palette so is safe to use even in 256 colour modes.
			// Show the whole section of VBScript with light blue background
			for (int bstyle=SCE_HB_DEFAULT; bstyle<=SCE_HB_STRINGEOL; bstyle++) {
				SendEditor(SCI_STYLESETFONT, bstyle, 
					reinterpret_cast<LPARAM>("Lucida Console"));
				SendEditor(SCI_STYLESETBACK, bstyle, lightBlue);
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HB_STRINGEOL, RGB(0x7F,0x7F,0xFF));
			SendEditor(SCI_STYLESETFONT, SCE_HB_COMMENTLINE, 
				reinterpret_cast<LPARAM>("Lucida Console"));

			SetAStyle(SCE_HBA_DEFAULT, black);
			SetAStyle(SCE_HBA_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HBA_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HBA_WORD, darkBlue);
			SendEditor(SCI_STYLESETBOLD, SCE_HBA_WORD, 1);
			SetAStyle(SCE_HBA_STRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HBA_IDENTIFIER, black);

			// Show the whole section of ASP VBScript with bright yellow background
			for (int bastyle=SCE_HBA_DEFAULT; bastyle<=SCE_HBA_STRINGEOL; bastyle++) {
				SendEditor(SCI_STYLESETFONT, bastyle, 
					reinterpret_cast<LPARAM>("Lucida Console"));
				SendEditor(SCI_STYLESETBACK, bastyle, RGB(0xFF, 0xFF, 0));
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bastyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HBA_STRINGEOL, RGB(0xCF,0xCF,0x7F));
			SendEditor(SCI_STYLESETFONT, SCE_HBA_COMMENTLINE, 
				reinterpret_cast<LPARAM>("Lucida Console"));

			// If there is no need to support embedded Javascript, the following code can be dropped.
			// Javascript will still be correctly processed but will be displayed in just the default style.

			SetAStyle(SCE_HJ_START, RGB(0x80,0x80,0));
			SetAStyle(SCE_HJ_DEFAULT, black);
			SetAStyle(SCE_HJ_COMMENT, darkGreen);
			SetAStyle(SCE_HJ_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HJ_COMMENTDOC, darkGreen);
			SetAStyle(SCE_HJ_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HJ_WORD, black);
			SetAStyle(SCE_HJ_KEYWORD, darkBlue);
			SetAStyle(SCE_HJ_DOUBLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJ_SINGLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJ_SYMBOLS, black);

			SetAStyle(SCE_HJA_START, RGB(0x80,0x80,0));
			SetAStyle(SCE_HJA_DEFAULT, black);
			SetAStyle(SCE_HJA_COMMENT, darkGreen);
			SetAStyle(SCE_HJA_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HJA_COMMENTDOC, darkGreen);
			SetAStyle(SCE_HJA_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HJA_WORD, black);
			SetAStyle(SCE_HJA_KEYWORD, darkBlue);
			SetAStyle(SCE_HJA_DOUBLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJA_SINGLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJA_SYMBOLS, black);

			SetAStyle(SCE_HPHP_DEFAULT, black);
			SetAStyle(SCE_HPHP_HSTRING,  RGB(0x80,0,0x80));
			SetAStyle(SCE_HPHP_SIMPLESTRING,  RGB(0x80,0,0x80));
			SetAStyle(SCE_HPHP_WORD, darkBlue);
			SetAStyle(SCE_HPHP_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HPHP_VARIABLE, red);
			SetAStyle(SCE_HPHP_HSTRING_VARIABLE, red);
			SetAStyle(SCE_HPHP_COMPLEX_VARIABLE, red);
			SetAStyle(SCE_HPHP_COMMENT, darkGreen);
			SetAStyle(SCE_HPHP_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HPHP_OPERATOR, darkBlue);

			// Show the whole section of Javascript with off white background
			for (int jstyle=SCE_HJ_DEFAULT; jstyle<=SCE_HJ_SYMBOLS; jstyle++) {
				SendEditor(SCI_STYLESETFONT, jstyle, 
					reinterpret_cast<LPARAM>("Lucida Console"));
				SendEditor(SCI_STYLESETBACK, jstyle, offWhite);
				SendEditor(SCI_STYLESETEOLFILLED, jstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HJ_STRINGEOL, RGB(0xDF, 0xDF, 0x7F));
			SendEditor(SCI_STYLESETEOLFILLED, SCE_HJ_STRINGEOL, 1);

			// Show the whole section of Javascript with brown background
			for (int jastyle=SCE_HJA_DEFAULT; jastyle<=SCE_HJA_SYMBOLS; jastyle++) {
				SendEditor(SCI_STYLESETFONT, jastyle, 
					reinterpret_cast<LPARAM>("Lucida Console"));
				SendEditor(SCI_STYLESETBACK, jastyle, RGB(0xDF, 0xDF, 0x7F));
				SendEditor(SCI_STYLESETEOLFILLED, jastyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HJA_STRINGEOL, RGB(0x0,0xAF,0x5F));
			SendEditor(SCI_STYLESETEOLFILLED, SCE_HJA_STRINGEOL, 1);
		}
	}
	else
	{
		SendEditor(SCI_SETLEXER, SCLEX_CPP);
		SetupCppLexer();
	}
	SendEditor(SCI_COLOURISE, 0, -1);
}

void TortoiseBlame::SetupCppLexer()
{
	SetAStyle(SCE_C_DEFAULT, RGB(0, 0, 0));
	SetAStyle(SCE_C_COMMENT, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTLINE, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOC, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTLINEDOC, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOCKEYWORD, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOCKEYWORDERROR, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_NUMBER, RGB(0, 0x80, 0x80));
	SetAStyle(SCE_C_WORD, RGB(0, 0, 0x80));
	SendEditor(SCE_C_WORD, 1);
	SetAStyle(SCE_C_STRING, RGB(0x80, 0, 0x80));
	SetAStyle(SCE_C_IDENTIFIER, RGB(0, 0, 0));
	SetAStyle(SCE_C_PREPROCESSOR, RGB(0x80, 0, 0));
	SetAStyle(SCE_C_OPERATOR, RGB(0x80, 0x80, 0));
}
