Const ForReading = 1    
Const ForWriting = 2

strFileName = Wscript.Arguments(0)

Set objFSO = CreateObject("Scripting.FileSystemObject")
Set objFile = objFSO.OpenTextFile(strFileName, ForReading)
strText = objFile.ReadAll
objFile.Close

strNewText = Replace(strText, "_USCORE", "_")
strNewText = Replace(strNewText, "_x002e", "_")
Set objFile = objFSO.OpenTextFile(strFileName, ForWriting)
objFile.Write strNewText  'WriteLine adds extra CR/LF
objFile.Close