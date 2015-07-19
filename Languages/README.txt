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
under the column LCID dec.

After successfully creating your dlls, copy them into
"C:\Program Files\TortoiseGit\Languages".
Now you should be able to select your language from the combobox on the
TortoiseGit main setting page.
