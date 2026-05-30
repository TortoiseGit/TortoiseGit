// extensions: dll;exe
//
// TortoiseGit Diff script for binary files
//
// Copyright (C) 2026 the TortoiseGit team
// Copyright (C) 2010-2014 the TortoiseSVN team
// This file is distributed under the same license as TortoiseGit
//
// Authors:
// Casey Barton, 2010
// Hans-Emil Skogh, 2011
// Sven Strickroth, 2026
//

var vbCritical = 16;
var vbExclamation = 48;
var vbInformation = 64;

var scriptname = "diff-dll.js"

var objArgs = WScript.Arguments;
var num = objArgs.Count();

if (num < 2) {
    MsgBox("Usage: [CScript | WScript] " + scriptname + " base.[dll|exe] new.[dll|exe]", vbInformation, scriptname + ": Invalid arguments");
    WScript.Quit(1);
}

var sBaseFile = objArgs.Item(0);
var sNewFile = objArgs.Item(1);

var objFileSystem = WScript.CreateObject("Scripting.FileSystemObject");

if (!objFileSystem.FileExists(sBaseFile)) {
    MsgBox("File \"" + sBaseFile + "\" does not exist. Cannot compare the files.", vbExclamation, scriptname + ": File not found");
    WScript.Quit(1);
}

if (!objFileSystem.FileExists(sNewFile)) {
    MsgBox("File \"" + sNewFile + "\" does not exist. Cannot compare the files.", vbExclamation, scriptname + ": File not found");
    WScript.Quit(1);
}

// Compare file size
var fBaseFile = objFileSystem.GetFile(sBaseFile);
var fNewFile = objFileSystem.GetFile(sNewFile);

var sMessage = "";
var sBaseMessage = "";
var sNewMessage = "";
var bDiffers = false;
if (fBaseFile.Size !== fNewFile.Size) {
    bDiffers = true;

    sBaseMessage += "  Size: " + fBaseFile.Size + " bytes\r\n";
    sNewMessage += "  Size: " + fNewFile.Size + " bytes\r\n";
} else {
    sMessage += "File sizes: " + fNewFile.Size + " bytes\r\n";
}

// Compare files using fc.exe
if (!bDiffers) {
    var WshShell = WScript.CreateObject("WScript.Shell");

    var command =
        'fc.exe "' +
        sBaseFile.replace(/"/g, '""') +
        '" "' +
        sNewFile.replace(/"/g, '""') +
        '"';

    var exitStatus = WshShell.Run(command, 0, true);

    if (exitStatus === 1) {
        bDiffers = true;
        sMessage += "File content differs!\r\n";
    } else if (exitStatus > 1) {
        sMessage += "Error while comparing file content. fc.exe returned exit code " + exitStatus + ".\r\n";
    }
}

// Only compare versions if we are comparing exe or dll files
if (isExeOrDll(sBaseFile) || isExeOrDll(sNewFile)) {
    var sBaseVer = objFileSystem.GetFileVersion(sBaseFile);
    var sNewVer = objFileSystem.GetFileVersion(sNewFile);

    if (sBaseVer.length === 0 && sNewVer.length === 0) {
        sMessage += "No version information available.";
    } else if (sBaseVer === sNewVer) {
        sMessage += "Version: " + sBaseVer;
    } else {
        bDiffers = true;

        sBaseMessage += "  Version: " + sBaseVer + "\r\n";
        sNewMessage += "  Version: " + sNewVer + "\r\n";
    }
}

// Generate result message
sBaseMessage =
    "Base\r\n" +
    "  File: " + sBaseFile + "\r\n" +
    sBaseMessage;

sNewMessage =
    "New\r\n" +
    "  File: " + sNewFile + "\r\n" +
    sNewMessage;

if (bDiffers) {
    sMessage =
        "Files differ!\r\n" +
        "\r\n" +
        sBaseMessage + "\r\n" +
        sNewMessage + "\r\n" +
        sMessage;

    MsgBox(sMessage, vbExclamation, scriptname + ": File Comparison - Differs");

    WScript.Quit(1);
} else {
    sMessage =
        "Files are identical\r\n" +
        "\r\n" +
        sMessage;

    MsgBox(sMessage, vbInformation, scriptname + ": File Comparison - Identical");

    WScript.Quit(0);
}

function isExeOrDll(path) {
    var lower = path.toLowerCase();
    return lower.slice(-4) === ".exe" || lower.slice(-4) === ".dll";
}

// Show a message to the user
function MsgBox(message, flags, title)
{
    if (/wscript\.exe$/i.test(WScript.FullName)) {
        // In Windows
        // Use WScript.Shell.Popup here to be able to set the title and icon.
        // WScript.Echo only gives an anonymous message box.
        WScript.CreateObject("WScript.Shell").Popup(message, 0, title, flags);
    } else {
        // In console (or unknown) fall back to WScript.Echo
        WScript.Echo(title + ": " + message);
    }
}
