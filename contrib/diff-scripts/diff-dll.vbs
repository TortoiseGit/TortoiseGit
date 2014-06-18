' extensions: dll;exe
'
' TortoiseSVN Diff script for binary files
'
' Copyright (C) 2010-2014 the TortoiseSVN team
' This file is distributed under the same license as TortoiseSVN
'
' Last commit by:
' $Author$
' $Date$
' $Rev$
'
' Authors:
' Casey Barton, 2010
' Hans-Emil Skogh, 2011
'
dim objArgs, objFileSystem, sBaseVer, sNewVer, sMessage, sBaseMessage, sNewMessage, bDiffers

bDiffers = False

Set objArgs = WScript.Arguments
num = objArgs.Count
if num < 2 then
    MsgBox "Usage: [CScript | WScript] diff-dll.vbs base.[dll|exe] new.[dll|exe]", vbCritical, "Invalid arguments"
    WScript.Quit 1
end if

sBaseFile = objArgs(0)
sNewFile = objArgs(1)

Set objFileSystem = CreateObject("Scripting.FileSystemObject")
If objFileSystem.FileExists(sBaseFile) = False Then
    MsgBox "File " + sBaseFile + " does not exist.  Cannot compare the files.", vbCritical, "File not found"
    Wscript.Quit 1
End If
If objFileSystem.FileExists(sNewFile) = False Then
    MsgBox "File " + sNewFile + " does not exist.  Cannot compare the files.", vbCritical, "File not found"
    Wscript.Quit 1
End If

' Compare file size
dim fBaseFile, fNewFile
Set fBaseFile = objFileSystem.GetFile(sBaseFile)
Set fNewFile = objFileSystem.GetFile(sNewFile)

If fBaseFile.size <> fNewFile.size Then
    bDiffers = True
    sBaseMessage = sBaseMessage + "  Size: " + CStr(fBaseFile.Size) + " bytes" + vbCrLf
    sNewMessage = sNewMessage + "  Size: " + CStr(fNewFile.Size) + " bytes" + vbCrLf
Else
    sMessage = sMessage + "File sizes: " + CStr(fNewFile.Size) + " bytes" + vbCrLf
End If

' Compare files using fc.exe
If bDiffers = False Then
    Set WshShell = WScript.CreateObject("WScript.Shell")
    exitStatus = WshShell.Run("fc.exe """ + sBaseFile + """ """ + sNewFile + """", 0, True)
    If exitStatus = 1 Then
        bDiffers = True
        sMessage = sMessage + "File content differs!" + vbCrLf
    ElseIf exitStatus > 1 Then
        ' Todo: Handle error!
    End If
End If

' Only compare versions if we are comparing exe:s or dll:s
If LCase(Right(sBaseFile, 3)) = "exe" or LCase(Right(sNewFile, 3)) = "exe" or _
    LCase(Right(sBaseFile, 3)) = "dll" or LCase(Right(sNewFile, 3)) = "dll" Then

    ' Compare version
    sBaseVer = objFileSystem.GetFileVersion(sBaseFile)
    sNewVer = objFileSystem.GetFileVersion(sNewFile)

    If Len(sBaseVer) = 0 and Len(sNewVer) = 0 Then
          sMessage = sMessage + "No version information available."
    ElseIf sBaseVer = sNewVer Then
        sMessage = sMessage + "Version: " + sBaseVer
    Else
        sBaseMessage = sBaseMessage + "  Version: " + sBaseVer + vbCrLf
        sNewMessage = sNewMessage + "  Version: " + sNewVer + vbCrLf
    End If
End If

' Generate result message
sBaseMessage = "Base" + vbCrLf _
    + "  File: " + sBaseFile + vbCrLf _
    + sBaseMessage
sNewMessage = + "New" + vbCrLf _
    + "  File: " + sNewFile + vbCrLf _
    + sNewMessage

If bDiffers = True Then
    sMessage = "Files differ!" + vbCrLf _
        + vbCrLf _
        + sBaseMessage + vbCrLf _
        + sNewMessage + vbCrLf _
        + sMessage

    MsgBox sMessage, vbExclamation, "File Comparison - Differs"
Else
    sMessage = "Files are identical" + vbCrLf _
        + vbCrLf _
        + sMessage

    MsgBox sMessage, vbInformation, "File Comparison - Identical"
End If

Wscript.Quit
