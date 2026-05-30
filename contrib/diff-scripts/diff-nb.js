// extensions: nb
//
// TortoiseGit Diff script for Mathematica notebooks
//
// Copyright (C) 2026 the TortoiseGit team
// Copyright (C) 2004-2009, 2012-2014 the TortoiseSVN team
// This file is distributed under the same license as TortoiseGit
//
// Authors:
// Szabolcs Horvát, 2008
// Chris Rodgers http://rodgers.org.uk/, 2008
// (Based on diff-xlsx.vbs)
// Sven Strickroth, 2026

var vbCritical = 16;
var vbExclamation = 48;
var vbInformation = 64;

var scriptname = "diff-nb.js";

var objArgs = WScript.Arguments;
var num = objArgs.Count();

if (num < 2) {
    MsgBox("Usage: [CScript | WScript] " + scriptname + " base.nb new.nb", vbInformation, scriptname + ": Invalid arguments");
    WScript.Quit(1);
}

var sBaseFile = objArgs.Item(0);
var sNewFile  = objArgs.Item(1);

var objFSO = WScript.CreateObject("Scripting.FileSystemObject");

if (!objFSO.FileExists(sBaseFile)) {
    MsgBox("File \"" + sBaseFile + "\" does not exist. Cannot compare the notebooks.", vbExclamation, scriptname + ": File not found");
    WScript.Quit(1);
} else {
    sBaseFile = objFSO.GetAbsolutePathName(sBaseFile);
}

if (!objFSO.FileExists(sNewFile)) {
    MsgBox("File \"" + sNewFile + "\" does not exist. Cannot compare the notebooks.", vbExclamation, scriptname + ": File not found");
    WScript.Quit(1);
} else {
    sNewFile = objFSO.GetAbsolutePathName(sNewFile);
}

var objShell = WScript.CreateObject("WScript.Shell");

try {
    var TemporaryFolder = 2;
    var tfolder = objFSO.GetSpecialFolder(TemporaryFolder);

    var tname = objFSO.GetTempName() + ".nb";
    var tempPath = objFSO.BuildPath(tfolder.Path, tname);

    var objDiffNotebook = objFSO.CreateTextFile(tempPath, true);

    // Output a Mathematica notebook that will do the diff for us
    objDiffNotebook.WriteLine(
        "Notebook[{\r\n" +
        "Cell[BoxData[ButtonBox[\"\\<\\\"Compare Notebooks\\\"\\>\",\r\n" +
        "ButtonFrame->\"DialogBox\", Active->True, ButtonEvaluator->Automatic,\r\n" +
        "ButtonFunction:>(Needs[\"AuthorTools`\"];\r\n" +
        "NotebookPut[Symbol[\"NotebookDiff\"][\r\n" +
        "\"" + escapeMathematicaPath(sBaseFile) + "\",\r\n" +
        "\"" + escapeMathematicaPath(sNewFile) + "\"\r\n" +
        "]])]], NotebookDefault]\r\n" +
        "}, Saveable->False, Editable->False, Selectable->False, " +
        "WindowToolbars->{}, WindowFrame->ModelessDialog, WindowElements->{}, " +
        "WindowFrameElements->CloseBox, WindowTitle->\"Diff\", " +
        "ShowCellBracket->False, WindowSize->{Fit,Fit}]"
    );

    objDiffNotebook.Close();

    objShell.Run("\"" + tempPath + "\"");

} catch (e) {
    MsgBox("Could not create or open the temporary notebook.\n\n" + e.message, vbExclamation, scriptname);
    WScript.Quit(1);
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

function escapeMathematicaPath(path) {
    // Mathematica string paths need backslashes escaped
    return path.replace(/\\/g, "\\\\");
}

// Show a message to the user
function MsgBox(message, flags, title) {
    if (/wscript\.exe$/i.test(WScript.FullName)) {
        // In Windows
        // Use WScript.Shell.Popup here to be able to set the title and icon.
        // WScript.Echo only gives an anonymous message box.
        WScript.CreateObject("WScript.Shell").Popup(message, 0, title, flags);
    } else {
        // In console
        WScript.Echo(title + ": " + message);
    }
}
