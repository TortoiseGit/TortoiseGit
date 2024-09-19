// extensions: zip
//
// TortoiseSVN Diff script for ZIP and other containers handled by File Explorer
//
// Last commit by:
// $Author: danielsahlberg $
// $Date: 2024-09-14 14:36:34 +0200 (Sa, 14 Sep 2024) $
// $Rev: 29709 $
//
// Authors:
// Daniel Sahlberg, 2024
//

var ShellApplication, WScriptShell, FileSystemObject, Arguments, BaseFile, NewFile;

ShellApplication = new ActiveXObject("Shell.Application");
WScriptShell = new ActiveXObject("WScript.Shell");
FileSystemObject = new ActiveXObject("Scripting.FileSystemObject");

// Show a message to the user
function MsgBox(message, flags, title)
{
	if (FileSystemObject.GetFileName( WScript.FullName ) == "wscript.exe")
	{
		// In Windows
		// Use WScript.Shell.Popup here to be able to set the title and icon.
		// WScript.Echo only gives an anonymous message box.
		WScriptShell.Popup(message, 0, title, flags);
	}
	else
	{
		// In console (or unknown) fall back to WScript.Echo
		WScript.Echo(title + ": " + message);
	}
}

// Basic sanity checks
Arguments = WScript.Arguments;
if (Arguments.length < 2)
{
    MsgBox("Usage: [CScript | WScript] diff-zip.js base.zip new.zip", 64, "diff-zip.js");
    WScript.Quit(1);
}

BaseFile = Arguments(0);
NewFile = Arguments(1);

if (!FileSystemObject.FileExists(BaseFile))
{
    Msgbox("File " + BaseFile + " does not exist. Cannot compare the files.", 48, "diff-zip.js");
    WScript.Quit(1);
}

if (!FileSystemObject.FileExists(NewFile))
{
    MsgBox("File " + NewFile + " does not exist. Cannot compare the files.", 48, "diff-zip.js");
    WScript.Quit(1);
}

// Read a zip file contents and write to a temporary file.
// Return the file name, or empty if an error occured.
function ReadZip(pathToZipFile)
{
    var Folder;
    Folder = ShellApplication.NameSpace(pathToZipFile);
    if (!Folder)
    {
        MsgBox("Cannot read " + pathToZipFile, 48, "diff-zip.js");
        return "";
    }
    else
    {
        var Items, File;
        Items = Folder.Items();
        FileName = FileSystemObject.GetSpecialFolder(2) + "\\" + FileSystemObject.GetTempName();
        // 2 = ForWriting
        // -1 = Opens the file as Unicode
        File = FileSystemObject.OpenTextFile(FileName, 2, true, -1);
        if (!File)
        {
            MsgBox("Cannot open " + File + " for writing", 48, "diff-zip.js");
            return "";
        }

        File.WriteLine(pathToZipFile);
        File.WriteLine("");
        var MaxLength = 20;
        for (var i = 0; i < Items.Count; i++)
        {
            if (Items.Item(i).Name.length > MaxLength)
            {
                MaxLength = Items.Item(i).Name.length;
            }
        }
        for (var i = 0; i < Items.Count; i++)
        {
            var Item = Items.Item(i);
            File.WriteLine(Item.Name + new Array(MaxLength-Item.Name.length+2).join(" ") + Item.ModifyDate + new Array(11-Item.Size.toString().length).join(" ") + Item.Size);
        }
        File.WriteLine("");
        File.WriteLine(Items.Count + " items");
        File.Close();
        return FileName;
    }
}

// Read the two input zip files
BaseFile = ReadZip(BaseFile)
NewFile = ReadZip(NewFile)

// Bail out if any of the files failed to read
if (BaseFile == "" || NewFile == "")
{
    WScript.Quit(1);
}

// Run TortoiseMerge to display the diff. Wait for it to complete.
WScriptShell.Run("\"TortoiseGitMerge.exe\" /readonly /base:\"" + BaseFile + "\" /basename:\"" + Arguments(0) + "\" /mine:\"" + NewFile + "\" /minename:\"" + Arguments(1) + "\"", 0, true)

// Delete temporary files
FileSystemObject.DeleteFile(BaseFile)
FileSystemObject.DeleteFile(NewFile)
