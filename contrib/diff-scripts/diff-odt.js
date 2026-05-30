// extensions: odt;ods;sxw
//
// TortoiseGit Diff script for OpenDocument Text/Spreadsheet files
//
// Copyright (C) 2026 the TortoiseGit team
// Copyright (C) 2004-2009, 2012-2014 the TortoiseSVN team
// This file is distributed under the same license as TortoiseGit
//
// Authors:
// Jonathan Ashley, 2007
// Stefan Küng, 2006, 2009
// Sven Strickroth, 2026
//

var vbCritical = 16;
var vbExclamation = 48;
var vbInformation = 64;

var scriptname = "diff-odt.js"

var objArgs = WScript.Arguments;
var num = objArgs.Count();

if (num < 2) {
    MsgBox("Usage: [CScript | WScript] " + scriptname + " base.odt new.odt", vbInformation, scriptname + ": Invalid arguments");
    WScript.Quit(1);
}

var sBaseDoc = objArgs.Item(0);
var sNewDoc  = objArgs.Item(1);

var objFSO = WScript.CreateObject("Scripting.FileSystemObject");

if (!objFSO.FileExists(sBaseDoc)) {
    MsgBox("File \"" + sBaseDoc + "\" does not exist. Cannot compare the documents.", vbExclamation, scriptname + ": File not found");
    WScript.Quit(1);
}

if (!objFSO.FileExists(sNewDoc)) {
    MsgBox("File \"" + sNewDoc + "\" does not exist. Cannot compare the documents.", vbExclamation, scriptname + ": File not found");
    WScript.Quit(1);
}

// Remove read-only attribute if set
removeReadOnly(objFSO, sBaseDoc);
removeReadOnly(objFSO, sNewDoc);

var objServiceManager;

try {
    // The service manager is always the starting point.
    // If there is no Office running, an Office instance is started.
    objServiceManager = WScript.CreateObject("com.sun.star.ServiceManager");
} catch (e) {
    MsgBox("You must have OpenOffice or LibreOffice installed to perform this operation.", vbExclamation, scriptname);
    WScript.Quit(1);
}

var objDesktop = objServiceManager.createInstance("com.sun.star.frame.Desktop");
var objUriTranslator = objServiceManager.createInstance("com.sun.star.uri.ExternalUriReferenceTranslator");

var sBaseDocUrl = toOfficeFileUrl(sBaseDoc, objUriTranslator);
var sNewDocUrl  = toOfficeFileUrl(sNewDoc, objUriTranslator);

// Open the new document
var oPropertyValue = new Array(1);
oPropertyValue[0] = objServiceManager.Bridge_GetStruct("com.sun.star.beans.PropertyValue");
oPropertyValue[0].Name = "ShowTrackedChanges";
oPropertyValue[0].Value = true;

var objDocument = objDesktop.loadComponentFromURL(sNewDocUrl, "_blank", 0, oPropertyValue);

if (objDocument == null) {
    MsgBox("Could not open document: " + sNewDoc, vbExclamation, scriptname);
    WScript.Quit(1);
}

// Set the frame
var frame = objDesktop.getCurrentFrame();

var dispatcher = objServiceManager.createInstance("com.sun.star.frame.DispatchHelper");

// Show tracked changes
dispatcher.executeDispatch(frame, ".uno:ShowTrackedChanges", "", 0, oPropertyValue);

// Execute the comparison
oPropertyValue[0].Name = "URL";
oPropertyValue[0].Value = sBaseDocUrl;

dispatcher.executeDispatch(frame, ".uno:CompareDocuments", "", 0, oPropertyValue);


// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
function removeReadOnly(fso, path) {
    var file = fso.GetFile(path);

    // Attribute 1 = ReadOnly
    if ((file.Attributes & 1) === 1) {
        file.Attributes = file.Attributes & ~1;
    }
}

function toOfficeFileUrl(path, uriTranslator) {
    var p = path;

    // Convert Windows path for OpenOffice/LibreOffice URL format
    p = p.replace(/\\/g, "/");
    p = p.replace(/:/g, "|");

    // Important: encode % first
    p = p.replace(/%/g, "%25");
    p = p.replace(/ /g, "%20");
    p = p.replace(/#/g, "%23");

    p = "file:///" + p;

    return uriTranslator.translateToInternal(p);
}

// Show a message to the user
function MsgBox(message, flags, title) {
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
