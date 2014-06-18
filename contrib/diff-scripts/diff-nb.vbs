'
' TortoiseSVN Diff script for Mathematica notebooks
'
' Last commit by:
' $Author$
' $Date$
' $Rev$
'
' Authors:
' Szabolcs Horvát, 2008
' Chris Rodgers http://rodgers.org.uk/, 2008
' (Based on diff-xlsx.vbs)
'

dim objArgs, objScript, objDiffNotebook

Set objArgs = WScript.Arguments
num = objArgs.Count
if num < 2 then
    MsgBox "Usage: [CScript | WScript] diff-nb.vbs base.nb new.nb", vbExclamation, "Invalid arguments"
    WScript.Quit 1
end if

sBaseFile = objArgs(0)
sNewFile = objArgs(1)

Set objScript = CreateObject("Scripting.FileSystemObject")

If objScript.FileExists(sBaseFile) = False Then
    MsgBox "File " + sBaseFile + " does not exist.  Cannot compare the notebooks.", vbExclamation, "File not found"
    Wscript.Quit 1
Else
    sBaseFile = objScript.GetAbsolutePathName(sBaseFile)
End If

If objScript.FileExists(sNewFile) = False Then
    MsgBox "File " + sNewFile + " does not exist.  Cannot compare the notebooks.", vbExclamation, "File not found"
    Wscript.Quit 1
Else
    sNewFile = objScript.GetAbsolutePathName(sNewFile)
End If

On Error Resume Next
Dim tfolder, tname
Const TemporaryFolder = 2

Set tfolder = objScript.GetSpecialFolder(TemporaryFolder)

tname = objScript.GetTempName + ".nb"
Set objDiffNotebook = tfolder.CreateTextFile(tname)

'Output a Mathematica notebook that will do the diff for us
objDiffNotebook.WriteLine "Notebook[{" + vbCrLf + _
"Cell[BoxData[ButtonBox[""\<\""Compare Notebooks\""\>""," + vbCrLf + _
"ButtonFrame->""DialogBox"", Active->True, ButtonEvaluator->Automatic," + vbCrLf + _
"ButtonFunction:>(Needs[""AuthorTools`""];" + vbCrLf + _
"NotebookPut[Symbol[""NotebookDiff""][" + vbCrLf + _
"""" + Replace(sBaseFile, "\", "\\") + """," + vbCrLf + _
"""" + Replace(sNewFile, "\", "\\") + """" + vbCrLf + _
"]])]], NotebookDefault]" + vbCrLf + _
"}, Saveable->False, Editable->False, Selectable->False, WindowToolbars->{}, WindowFrame->ModelessDialog, WindowElements->{}, WindowFrameElements->CloseBox, WindowTitle->""Diff"", ShowCellBracket->False, WindowSize->{Fit,Fit}]"


objDiffNotebook.Close

Set objShell = CreateObject("WScript.Shell")
objShell.Run tfolder + "\" + tname
