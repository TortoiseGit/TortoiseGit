// extensions: xls;xlsx;xlsm;xlsb;xlam
//
// TortoiseSVN Diff script for Excel files
//
// Copyright (C) 2004-2008, 2012-2015 the TortoiseSVN team
// This file is distributed under the same license as TortoiseSVN
//
// Last commit by:
// $Author: steveking $
// $Date: 2015-10-26 20:01:19 +0100 (Mo, 26 Okt 2015) $
// $Rev: 26930 $
//
// Authors:
// Hiroki Najima <h.najima at gmail.com>, 2013
// Michael Joras <michael@joras.net>, 2008
// Suraj Barkale, 2006
//

// ----- configuration -----
// Fast mode switch
// Fast mode does not copy Worksheets but require opened base document at the same time.
var bFastMode = false;

// ----- constants -----
var vbCritical = 0x10;
var vbExclamation = 0x30;
//var vbInformation = 0x40;

var xlNone = -4142;
var xlMaximized = -4137;
var xlArrangeStyleHorizontal = -4128;
var xlCellValue = 1;
//var xlExpression = 2;
//var xlEqual = 3;
var xlNotEqual = 4;

//var vOffice95 = 7;
//var vOffice97 = 8;
//var vOffice2000 = 9;
//var vOffice2002 = 10;
var vOffice2003 = 11;
//var vOffice2007 = 12;
//var vOffice2010 = 14;
//var vOffice2013 = 15;

// ----- main -----

var aWarningMessages = [];

var objArgs = WScript.Arguments;
if (objArgs.length < 2)
{
    Abort("Usage: [CScript | WScript] diff-xls.js base.xls new.xls", "Invalid arguments");
}

var sBaseDoc = objArgs(0);
var sNewDoc = objArgs(1);

var objScript = new ActiveXObject("Scripting.FileSystemObject");

if (objScript.GetBaseName(sBaseDoc) === objScript.GetBaseName(sNewDoc))
{
    Abort("File '" + sBaseDoc + "' and '" + sNewDoc + "' is same file name.\nCannot compare the documents.", "Same file name");
}

if (!objScript.FileExists(sBaseDoc))
{
    Abort("File '" + sBaseDoc + "' does not exist.\nCannot compare the documents.", "File not found");
}

if (!objScript.FileExists(sNewDoc))
{
    Abort("File '" + sNewDoc + "' does not exist.\nCannot compare the documents.", "File not found");
}

sBaseDoc = objScript.GetAbsolutePathName(sBaseDoc);
sNewDoc = objScript.GetAbsolutePathName(sNewDoc);
objScript = null;

var objExcelApp;
try
{
    objExcelApp = WScript.CreateObject("Excel.Application");
}
catch (e)
{
    Abort("You must have Excel installed to perform this operation.", "Excel Instantiation Failed");
}
var fExcelVersion = parseInt(objExcelApp.Version, 10);

// Open base Excel book
var objBaseWorkbook;
try
{
    objBaseWorkbook = objExcelApp.Workbooks.Open(sBaseDoc, null, true);
}
catch (e)
{
    Abort("Failed to open '" + sBaseDoc + "'\nIt might not be a valid Excel file.", "File open error");
}

// Open new Excel book
var objNewWorkbook;
try
{
    objNewWorkbook = objExcelApp.Workbooks.Open(sNewDoc, null, true);
}
catch (e)
{
    Abort("Failed to open '" + sNewDoc + "'\nIt might not be a valid Excel file.", "File open error");
}

// Show Excel window
objExcelApp.Visible = true;

// Arrange windows
if (objBaseWorkbook.ProtectWindows || objNewWorkbook.ProtectWindows)
{
    StoreWarning("Unable to arrange windows because one or both Workbooks are protected.");
}
else
{
    // Make windows a compare side by side view
    if (fExcelVersion >= vOffice2003)
    {
        objExcelApp.Windows.CompareSideBySideWith(objExcelApp.Windows(2).Caption);
    }
    objExcelApp.Application.WindowState = xlMaximized;
    objExcelApp.Windows.Arrange(xlArrangeStyleHorizontal);
}

if (!bFastMode && objNewWorkbook.ProtectWindows)
{
    StoreWarning("Fall back to fast mode because " + objNewWorkbook.Name + " is protected.");
    bFastMode = true;
}

// Create a special workbook for formula conversion.
var objSpecialWorkbook = objExcelApp.Workbooks.Add;

// Mark differences in sNewDoc red
var length = objNewWorkbook.Worksheets.Count;
for (var i = 1; i <= length; i++)
{
    var objBaseWorksheet = null;
    if (i <= objBaseWorkbook.Worksheets.Count)
        objBaseWorksheet = objBaseWorkbook.Worksheets(i);
    var objNewWorksheet = objNewWorkbook.Worksheets(i);

    if (objBaseWorksheet != null)
        UnhideWorksheet(objBaseWorksheet);
    UnhideWorksheet(objNewWorksheet);

    if (!bFastMode && (objBaseWorksheet != null))
    {
        objBaseWorkbook.Sheets(i).Copy(null, objNewWorkbook.Sheets(objNewWorkbook.Sheets.Count));
        var objDummyWorksheet = objNewWorkbook.Sheets(objNewWorkbook.Sheets.Count);
        objDummyWorksheet.Name = "Dummy_for_Comparison" + i;
        objDummyWorksheet.Visible = true;
        if (fExcelVersion >= vOffice2003)
        {
            objDummyWorksheet.Tab.ColorIndex = 16;  // 16:Dark gray RGB(128,128,128)
        }
    }

    if (objNewWorksheet.ProtectContents)
    {
        StoreWarning("Unable to mark differences to " +
            ToAbsoluteReference(objNewWorksheet) +
            " because the Worksheet is protected.");
    }
    else
    {
        objNewWorksheet.Cells.FormatConditions.Delete();
        var sFormula;
        if (bFastMode && (objBaseWorksheet != null))
        {
            sFormula = "=INDIRECT(\"" + ToAbsoluteReference(objBaseWorksheet) + "!\"&ADDRESS(ROW(),COLUMN()))";
        }
        else
        {
            sFormula = "=INDIRECT(\"Dummy_for_Comparison" + i + "!\"&ADDRESS(ROW(),COLUMN()))";
        }
        sFormula = convertFormula(sFormula);
        objNewWorksheet.Cells.FormatConditions.Add(xlCellValue, xlNotEqual, sFormula);
        objNewWorksheet.Cells.FormatConditions(1).Interior.ColorIndex = 3;  // 3:Red RGB(128,0,0)
    }
}

// Close the special workbook quietly
objSpecialWorkbook.Saved = true;
objSpecialWorkbook.Close();

// Activate first Worksheet
objBaseWorkbook.Sheets(1).Activate();
objNewWorkbook.Sheets(1).Activate();

// Suppress save dialog if nothing changed
objBaseWorkbook.Saved = true;
objNewWorkbook.Saved = true;

// Show warnings if exist
ShowWarning();

WScript.Quit(0);


// ----- functions -----

// Show Message Dialog
// VBcript's MsgBox emulation
function MsgBox(sMessage, iButtons, sTitle)
{
    var objShell = new ActiveXObject("WScript.Shell");
    objShell.popup(sMessage, 0, sTitle, iButtons);
}

// Show an error message and quit script with cleanup Excel Application Object.
function Abort(sMessage, sTitle)
{
    MsgBox(sMessage, vbCritical, sTitle);
    if (objExcelApp)
    {
        objExcelApp.Quit();
    }
    WScript.Quit(1);
}

// Unhide the Worksheet if it is hidden.
// This also sets color to the tab, if Office2003 or later.
//  - 46(Orange)      : Hidden Worksheet
//  - xlNone(default) : Not hidden Worksheet
function UnhideWorksheet(objWorksheet)
{
    if (objWorksheet.Visible)
    {
        if (fExcelVersion >= vOffice2003)
        {
            if (objWorksheet.Tab.ColorIndex !== xlNone)
            {
                if (objWorksheet.Parent.ProtectStructure)
                {
                    StoreWarning("Unable to set tab color to " +
                        ToAbsoluteReference(objWorksheet) +
                        " because the Workbook's structure is protected.");
                }
                else
                {
                    objWorksheet.Tab.ColorIndex = xlNone;
                }
            }
        }
    }
    else if (objWorksheet.Parent.ProtectStructure)
    {
        StoreWarning("Unable to unhide " +
            ToAbsoluteReference(objWorksheet) +
            " because the Workbook's structure is protected.");
    }
    else
    {
        objWorksheet.Visible = true;
        if (fExcelVersion >= vOffice2003)
        {
            objWorksheet.Tab.ColorIndex = 10;   // 10:Green RGB(0,128,0)
        }
    }

}

// Generate Absolute Reference Formula of Worksheet.
function ToAbsoluteReference(objWorksheet)
{
    return "[" + objWorksheet.Parent.Name + "]" + objWorksheet.Name;
}

// Convert a formula for workaround in some situation.
// Actually I don't know what will be changed between sFormula and FormulaLocal.
function convertFormula(sFormula)
{
    var worksheet = objSpecialWorkbook.Sheets(1);
    var originalContent = worksheet.Cells(1, 1).Formula;
    worksheet.Cells(1, 1).Formula = sFormula;
    sFormula = worksheet.Cells(1, 1).FormulaLocal;
    worksheet.Cells(1, 1).Formula = originalContent;
    return sFormula;
}

// Accumulate a warning message.
function StoreWarning(sMessage)
{
    aWarningMessages[aWarningMessages.length] = sMessage;
}

// Show accumulated warning messages if exist.
// To avoid make huge message dialog, this limits message count to show.
function ShowWarning()
{
    if (aWarningMessages.length === 0)
    {
        return;
    }
    var sMessage = "The following warnings occurred while processing.\n";
    for (var j = 0; j < aWarningMessages.length; j++)
    {
        if (j >= 10)
        {
            sMessage += "... And more " + (aWarningMessages.length - j) + " messages";
            break;
        }
        sMessage += "[" + (j + 1) + "] " + aWarningMessages[j] + "\n";
    }
    MsgBox(sMessage, vbExclamation, "Warning");
}
