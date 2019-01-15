Translations
============

TortoiseGit is still in development so translations of all resources is
also a work 'in progress'.

General
-------
The TortoiseGit project uses the web-based Transifex platform for
managing translations:
https://www.transifex.com/projects/p/tortoisegit/

This should make it easier for new translators to get started. If you
want to help to translate TortoiseGit, please register an account a
transifex.com (you can also login using your google account), then
request membership of one of our translation teams or request to create
a new team, if your language is not yet listed. If you want to
translate into a language that has different locales and you *really*
need the second locale, add the two letter ISO-3166 locale code. If
you're not sure about your country code, look here:
http://www.iso.ch/iso/en/prods-services/iso3166ma/02iso-3166-code-lists/list-en1.html.
Please keep in mind that we would like to have as few translations as
possible that cover as many languages as possible (in order to reduce
the work needed for keeping all translations up to date).

For most (unlisted) languages you do not need to start all over. A large
amount of translated strings can be reused from the TortoiseSVN project:
Just request a new language team and contact the TortoiseGit developers!

Translating
-----------
There are some notes and hints for translating:

When translating please try to adapt the naming conventions used by the
Git team. Their translations can be found here:
https://github.com/gitster/git/tree/master/po

When translating on transifex.com there you might see red symbols in the
original text such as "⇥", "⏎", "↩" and "»" (you need an UTF-8 aware
editor to see the the three different symbols here). DO NOT COPY these
into the translated text as this might break certain semantics. "»" and
"⇥" mean that a tabulator is needed at that position. "⏎" and "↩" indicate
that a new line should be inserted there. For "⏎" just press enter, "↩"
needs special attention as well as the tabulator character.
ATTENTION: Right now there is a bug on transifex so that the tabulator
character and the "↩" new line character cannot be inserted using the
keyboard. Workaround: Click on "Copy source string" and then carefully
replace the text before and after the tabulator resp. the "↩" new line
character.

Character sequences such as "%i", "%d", "%ld", "%I64d", "%u" and "%.1f"
(will be substituted by a number each) and "%s" (will be substituted by
a string/text) MUST be included in the SAME ORDER as in the original
text (otherwise the program will crash at run-time). These sequences are
called "format specifiers" (see
https://msdn.microsoft.com/en-us/library/75w45ekt.aspx for more
information).
There is only one exception: Only if a number, an exclamation mark, a
format specifier and another exclamation mark (such as "1!s!") is after
the per cent sign (such as "%!1!s" or "%!3!d") then the order can be
exchanged (the number before the first exclamation mark indicates which
input will be inserted at run-time).

An ampercent (&) before a letter such as "&Open" indicates that "O" is
an accelerator key in the current context (e.g., in a menu or in a
window) which can be accessed by pressing the key or ALT+key (depending
on the context; open a menu with ALT+key choosing an option within the
menu just press the key). These should be unique in their contexts
otherwise they won't work - try to use the same letters as in the
original text or test which other keys don't conflict (and are common in
your language).
In order to see them, just press the ALT key then the respecting
characters will be shown underlined.

There is also a second type of accelerator keys (such as CTRL+F). These
are represented as specially formatted strings which look like
"ID:xxxxxx:VACS+X" and are designed for localized accelerator keys.
Format is: "ID:xxxxxx:VACS+X" where:
   ID:xxxxx = the menu ID corresponding to the accelerator
   V = Virtual key (or blank if not used) - nearly always set!
   A = Alt key     (or blank if not used)
   C = Ctrl key    (or blank if not used)
   S = Shift key   (or blank if not used)
   X = upper case character
e.g. "V CS+Q" == Ctrl + Shift + 'Q'
ONLY Accelerator Keys with corresponding alphanumeric characters can be
updated, i.e. function keys (F2), special keys (Delete, HoMe) etc. will
not.
Below the translation textbox, there is a field "Developer note:". There
you can find further information for accelerator keys, e.g.:
"Accelerator Entry for Menu ID:57636; 'Find' Accelerator Entry for Menu
ID:57636; '&Find\tCtrl+F'". This means that this accelerator is used
for the menu entry "&Find" (which also has a local accelerator key "F"
within the menu; there is no need that both accelerator keys include
the key - e.g. it's common to use 'x' for "E&xit" within the "File"
menu and CTRL+W as global accelerator key).

We hope that these hints will answer most questions. If you have any
further question, feel free to ask (e.g., on the mailing lists)!

What now?
---------
That's all you have to do. We'll take care of creating the resource
dlls and installers for the different language packs.

Our goal is to provide language packs starting with the next releases
as soon as more than 50 % of the strings are translated on the offical
TortoiseGit download page (https://tortoisegit.org/download).

Technical details
-----------------
The TortoiseGit project uses gettext PO (portable object) files for its
translation. PO files are plain text, so you can even translate using a
normal text editor. The only thing you have to take care of is that the
editor can handle the UTF-8 charset. There is a wide range of specialized
tools out there that can handle and manipulate PO files (such as poEdit,
http://poedit.sourceforge.net).

You can also create your personal translations. Just use the
"Languages/Tortoise.pot" file as a template for your own "Tortoise_CC.po"
file and start translating it.

Building your own dlls
----------------------
If you want to test the translations you made yourself, you need three things:

- The "raw" resource dlls for your version of TortoiseGit. Other dlls
  won't work. The TortoiseGit teams provides those for every stable release.
  You can find them in the release folder on https://download.tortoisegit.org/tgit/
  named "TortoiseGit-LanguageDlls-X.X.X.X-(32|64)bit.7z" (you might need 7-Zip for unpacking)
- A po file containing the translations for your language.
- The ResText utility that is used to insert the data from the po file
  into your resource dlls. This tool is also included in the archive in which the "raw"
  resource dlls are included.

Once you got all needed files, you run:

    restext apply TortoiseProcLang.dll TortoiseProcNNNN.dll Tortoise_CC.po
    restext apply TortoiseMergeLang.dll TortoiseMergeNNNN.dll Tortoise_CC.po
    restext apply TortoiseIDiffLang.dll TortoiseIDiffNNNN.dll Tortoise_CC.po
    restext apply TortoiseGitUDiffLang.dll TortoiseGitUDiffNNNN.dll Tortoise_CC.po
    restext apply TortoiseGitBlameLang.dll TortoiseGitBlameNNNN.dll Tortoise_CC.po

where NNNN is your four digit country code (1031 for germany)
and CC is your 2 letter country code (de for germany)

You can find the four digit country code at
https://msdn.microsoft.com/goglobal/bb964664.aspx
under the column LCID dec or on
https://docs.microsoft.com/windows/desktop/intl/language-identifier-constants-and-strings
unter the column "Language identifier". In both cases it has to be
converted to decimal first (e.g., 0x0407 -> 1031 for de-DE).

After successfully creating your dlls, copy them into
"C:\Program Files\TortoiseGit\Languages".
Now you should be able to select your language from the combobox on the
TortoiseGit main setting page.
