// extensions: doc;docx;docm
//
// TortoiseSVN Merge script for Word Doc files
//
// Copyright (C) 2004-2008, 2011 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// Last commit by:
// $Author$
// $Date$
// $Rev$
//
// Authors:
// Dan Sheridan, 2008
// Davide Orlandi and Hans-Emil Skogh, 2005
// Richard Horton, 2011
// Paolo Nesti Poggi, 2017
//

var objArgs, num, sTheirDoc, sMyDoc, sBaseDoc, sMergedDoc,
    objScript, word, baseDoc, myDoc, theirDoc, WSHShell;

// Microsoft Office versions for Microsoft Windows OS
var vOffice2000 = 9;
var vOffice2002 = 10;
//var vOffice2003 = 11;
var vOffice2007 = 12;
var vOffice2010 = 14;
// WdCompareTarget
var wdCompareTargetSelected = 0;
//var wdCompareTargetCurrent = 1;
var wdCompareTargetNew = 2;
var wdMergeTargetCurrent = 1;

objArgs = WScript.Arguments;
num = objArgs.length;
if (num < 4)
{
    WScript.Echo("Usage: [CScript | WScript] merge-doc.js merged.doc theirs.doc mine.doc base.doc");
    WScript.Quit(1);
}

sMergedDoc = objArgs(0);
sTheirDoc = objArgs(1);
sMyDoc = objArgs(2);
sBaseDoc = objArgs(3);

objScript = new ActiveXObject("Scripting.FileSystemObject");

if (!objScript.FileExists(sTheirDoc))
{
    WScript.Echo("File " + sTheirDoc + " does not exist.  Cannot compare the documents.", vbExclamation, "File not found");
    WScript.Quit(1);
}

if (!objScript.FileExists(sMergedDoc))
{
    WScript.Echo("File " + sMergedDoc + " does not exist.  Cannot compare the documents.", vbExclamation, "File not found");
    WScript.Quit(1);
}

objScript = null;

try
{
    word = WScript.CreateObject("Word.Application");
}
catch (e)
{
    WScript.Echo("You must have Microsoft Word installed to perform this operation.");
    WScript.Quit(1);
}

word.visible = true;

// Anticipate creation of shell object
WSHShell = WScript.CreateObject("WScript.Shell");

// Show usage hint message
WSHShell.Popup("After you click 'OK' we'll create a merge document. Please wait.\nThen reject or accept changes and save the document renaming it to the original conflicting filename.", 0, "TortoiseGit Word Merge", 64);

// Open the base document
baseDoc = word.Documents.Open(sTheirDoc);

// Merge into the "My" document
if (parseInt(word.Version, 10) < vOffice2000)
{
    baseDoc.Compare(sMergedDoc);
}
else if (parseInt(word.Version, 10) < vOffice2007)
{
    baseDoc.Compare(sMergedDoc, "Comparison", wdCompareTargetNew, true, true);
}
else
{
    // this implements the three-way comparison for versions >= 2007
    theirDoc = baseDoc;
    baseDoc = word.Documents.Open(sBaseDoc);
    myDoc = word.Documents.Open(sMyDoc);

    baseDoc.Activate(); //required otherwise it compares the wrong docs !!!
    baseDoc.Compare(sTheirDoc, "theirs", wdCompareTargetSelected, true, true);

    baseDoc.Activate(); //required otherwise it compares the wrong docs !!!
    baseDoc.Compare(sMyDoc, "mine", wdCompareTargetSelected, true, true);

    myDoc.Activate(); //required? just in case
    myDoc.Merge(sTheirDoc, wdMergeTargetCurrent);
    theirDoc.close();

    // bring focus to the window, for accept/reject buttons to be active.
    WSHShell.AppActivate(word.windows.Item(1).caption);
}

// Show the merge result
if (parseInt(word.Version, 10) < vOffice2007)
{
    word.ActiveDocument.Windows(1).Visible = 1;
}

// Close the first document
if (parseInt(word.Version, 10) >= vOffice2002)
{
    baseDoc.Close();
}

