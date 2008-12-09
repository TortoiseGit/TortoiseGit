//
// TortoiseSVN Diff script for Powerpoint files
//
// Copyright (C) 2004-2008 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// Last commit by:
// $Author: luebbe $
// $Date: 2008-06-13 21:52:53 +0800 (Fri, 13 Jun 2008) $
// $Rev: 13247 $
//
// Authors:
// Arne Moor, 2006
//

/*
This script starts PowerPoint and compares the two given presentations.
To better see the changes and get the highlighting feature of PowerPoint
click on "Apply all changes to the presentation" on the reviewing toolbar.
*/
var objArgs,num,sBasePpt,sNewPpt,objScript,powerpoint,source;

objArgs = WScript.Arguments;
num = objArgs.length;
if (num < 2)
{
   WScript.Echo("Usage: [CScript | WScript] diff-ppt.js base.ppt new.ppt");
   WScript.Quit(1);
}

sBasePpt = objArgs(0);
sNewPpt = objArgs(1);

objScript = new ActiveXObject("Scripting.FileSystemObject");
if ( ! objScript.FileExists(sBasePpt))
{
    WScript.Echo("File " + sBasePpt + " does not exist.  Cannot compare the presentations.");
    WScript.Quit(1);
}
if ( ! objScript.FileExists(sNewPpt))
{
    WScript.Echo("File " + sNewPpt +" does not exist.  Cannot compare the presentations.");
    WScript.Quit(1);
}

objScript = null;

try
{
   powerpoint = WScript.CreateObject("Powerpoint.Application");
}
catch(e)
{
   WScript.Echo("You must have Microsoft Powerpoint installed to perform this operation.");
   WScript.Quit(1);
}

powerpoint.visible = true;

// Open the original (base) document
source = powerpoint.Presentations.Open(sBasePpt);
    
// Merge the new document, to show the changes
source.Merge(sNewPpt);
    
// Mark the comparison presentation as saved to prevent the annoying
// "Save as" dialog from appearing.
powerpoint.ActivePresentation.Saved = 1;
