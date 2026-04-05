/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : LevelEdit                                                    *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Tools/LevelEdit/excel8.h        $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 7/26/01 2:06p                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#if defined(_MSC_VER)
#pragma once

#include <cstdint>
#endif


#ifndef __EXCEL8_H
#define __EXCEL8_H

#include <afxdisp.h>

class Workbooks : public COleDispatchDriver
{
public:
	Workbooks() {}		// Calls COleDispatchDriver default constructor
	Workbooks(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	Workbooks(const Workbooks& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	LPDISPATCH Add(const VARIANT& Template);
	void Close();
	int32_t GetCount();
	LPDISPATCH GetItem(const VARIANT& Index);
	LPUNKNOWN Get_NewEnum();
	LPDISPATCH Open(LPCTSTR Filename, const VARIANT& UpdateLinks, const VARIANT& ReadOnly, const VARIANT& Format, const VARIANT& Password, const VARIANT& WriteResPassword, const VARIANT& IgnoreReadOnlyRecommended, const VARIANT& Origin, 
		const VARIANT& Delimiter, const VARIANT& Editable, const VARIANT& Notify, const VARIANT& Converter, const VARIANT& AddToMru);
	void OpenText(LPCTSTR Filename, const VARIANT& Origin, const VARIANT& StartRow, const VARIANT& DataType, int32_t TextQualifier, const VARIANT& ConsecutiveDelimiter, const VARIANT& Tab, const VARIANT& Semicolon, const VARIANT& Comma, 
		const VARIANT& Space, const VARIANT& Other, const VARIANT& OtherChar, const VARIANT& FieldInfo, const VARIANT& TextVisualLayout);
	LPDISPATCH Get_Default(const VARIANT& Index);
};
/////////////////////////////////////////////////////////////////////////////
// _Application wrapper class

class _Application : public COleDispatchDriver
{
public:
	_Application() {}		// Calls COleDispatchDriver default constructor
	_Application(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_Application(const _Application& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	LPDISPATCH GetActiveCell();
	LPDISPATCH GetActiveChart();
	CString GetActivePrinter();
	void SetActivePrinter(LPCTSTR lpszNewValue);
	LPDISPATCH GetActiveSheet();
	LPDISPATCH GetActiveWindow();
	LPDISPATCH GetActiveWorkbook();
	LPDISPATCH GetAddIns();
	LPDISPATCH GetAssistant();
	void Calculate();
	LPDISPATCH GetCells();
	LPDISPATCH GetCharts();
	LPDISPATCH GetColumns();
	LPDISPATCH GetCommandBars();
	int32_t GetDDEAppReturnCode();
	void DDEExecute(int32_t Channel, LPCTSTR String);
	int32_t DDEInitiate(LPCTSTR App, LPCTSTR Topic);
	void DDEPoke(int32_t Channel, const VARIANT& Item, const VARIANT& Data);
	VARIANT DDERequest(int32_t Channel, LPCTSTR Item);
	void DDETerminate(int32_t Channel);
	VARIANT Evaluate(const VARIANT& Name);
	VARIANT _Evaluate(const VARIANT& Name);
	VARIANT ExecuteExcel4Macro(LPCTSTR String);
	LPDISPATCH Intersect(LPDISPATCH Arg1, LPDISPATCH Arg2, const VARIANT& Arg3, const VARIANT& Arg4, const VARIANT& Arg5, const VARIANT& Arg6, const VARIANT& Arg7, const VARIANT& Arg8, const VARIANT& Arg9, const VARIANT& Arg10, 
		const VARIANT& Arg11, const VARIANT& Arg12, const VARIANT& Arg13, const VARIANT& Arg14, const VARIANT& Arg15, const VARIANT& Arg16, const VARIANT& Arg17, const VARIANT& Arg18, const VARIANT& Arg19, const VARIANT& Arg20, 
		const VARIANT& Arg21, const VARIANT& Arg22, const VARIANT& Arg23, const VARIANT& Arg24, const VARIANT& Arg25, const VARIANT& Arg26, const VARIANT& Arg27, const VARIANT& Arg28, const VARIANT& Arg29, const VARIANT& Arg30);
	LPDISPATCH GetNames();
	LPDISPATCH GetRange(const VARIANT& Cell1, const VARIANT& Cell2);
	LPDISPATCH GetRows();
	VARIANT Run(const VARIANT& Macro, const VARIANT& Arg1, const VARIANT& Arg2, const VARIANT& Arg3, const VARIANT& Arg4, const VARIANT& Arg5, const VARIANT& Arg6, const VARIANT& Arg7, const VARIANT& Arg8, const VARIANT& Arg9, 
		const VARIANT& Arg10, const VARIANT& Arg11, const VARIANT& Arg12, const VARIANT& Arg13, const VARIANT& Arg14, const VARIANT& Arg15, const VARIANT& Arg16, const VARIANT& Arg17, const VARIANT& Arg18, const VARIANT& Arg19, 
		const VARIANT& Arg20, const VARIANT& Arg21, const VARIANT& Arg22, const VARIANT& Arg23, const VARIANT& Arg24, const VARIANT& Arg25, const VARIANT& Arg26, const VARIANT& Arg27, const VARIANT& Arg28, const VARIANT& Arg29, 
		const VARIANT& Arg30);
	VARIANT _Run2(const VARIANT& Macro, const VARIANT& Arg1, const VARIANT& Arg2, const VARIANT& Arg3, const VARIANT& Arg4, const VARIANT& Arg5, const VARIANT& Arg6, const VARIANT& Arg7, const VARIANT& Arg8, const VARIANT& Arg9, 
		const VARIANT& Arg10, const VARIANT& Arg11, const VARIANT& Arg12, const VARIANT& Arg13, const VARIANT& Arg14, const VARIANT& Arg15, const VARIANT& Arg16, const VARIANT& Arg17, const VARIANT& Arg18, const VARIANT& Arg19, 
		const VARIANT& Arg20, const VARIANT& Arg21, const VARIANT& Arg22, const VARIANT& Arg23, const VARIANT& Arg24, const VARIANT& Arg25, const VARIANT& Arg26, const VARIANT& Arg27, const VARIANT& Arg28, const VARIANT& Arg29, 
		const VARIANT& Arg30);
	LPDISPATCH GetSelection();
	void SendKeys(const VARIANT& Keys, const VARIANT& Wait);
	LPDISPATCH GetSheets();
	LPDISPATCH GetThisWorkbook();
	LPDISPATCH Union(LPDISPATCH Arg1, LPDISPATCH Arg2, const VARIANT& Arg3, const VARIANT& Arg4, const VARIANT& Arg5, const VARIANT& Arg6, const VARIANT& Arg7, const VARIANT& Arg8, const VARIANT& Arg9, const VARIANT& Arg10, const VARIANT& Arg11, 
		const VARIANT& Arg12, const VARIANT& Arg13, const VARIANT& Arg14, const VARIANT& Arg15, const VARIANT& Arg16, const VARIANT& Arg17, const VARIANT& Arg18, const VARIANT& Arg19, const VARIANT& Arg20, const VARIANT& Arg21, 
		const VARIANT& Arg22, const VARIANT& Arg23, const VARIANT& Arg24, const VARIANT& Arg25, const VARIANT& Arg26, const VARIANT& Arg27, const VARIANT& Arg28, const VARIANT& Arg29, const VARIANT& Arg30);
	LPDISPATCH GetWindows();
	LPDISPATCH GetWorkbooks();
	LPDISPATCH GetWorksheetFunction();
	LPDISPATCH GetWorksheets();
	LPDISPATCH GetExcel4IntlMacroSheets();
	LPDISPATCH GetExcel4MacroSheets();
	void ActivateMicrosoftApp(int32_t Index);
	void AddChartAutoFormat(const VARIANT& Chart, LPCTSTR Name, const VARIANT& Description);
	void AddCustomList(const VARIANT& ListArray, const VARIANT& ByRow);
	int32_t GetAlertBeforeOverwriting();
	void SetAlertBeforeOverwriting(int32_t bNewValue);
	CString GetAltStartupPath();
	void SetAltStartupPath(LPCTSTR lpszNewValue);
	int32_t GetAskToUpdateLinks();
	void SetAskToUpdateLinks(int32_t bNewValue);
	int32_t GetEnableAnimations();
	void SetEnableAnimations(int32_t bNewValue);
	LPDISPATCH GetAutoCorrect();
	int32_t GetBuild();
	int32_t GetCalculateBeforeSave();
	void SetCalculateBeforeSave(int32_t bNewValue);
	int32_t GetCalculation();
	void SetCalculation(int32_t nNewValue);
	VARIANT GetCaller(const VARIANT& Index);
	int32_t GetCanPlaySounds();
	int32_t GetCanRecordSounds();
	CString GetCaption();
	void SetCaption(LPCTSTR lpszNewValue);
	int32_t GetCellDragAndDrop();
	void SetCellDragAndDrop(int32_t bNewValue);
	double CentimetersToPoints(double Centimeters);
	int32_t CheckSpelling(LPCTSTR Word, const VARIANT& CustomDictionary, const VARIANT& IgnoreUppercase);
	VARIANT GetClipboardFormats(const VARIANT& Index);
	int32_t GetDisplayClipboardWindow();
	void SetDisplayClipboardWindow(int32_t bNewValue);
	int32_t GetCommandUnderlines();
	void SetCommandUnderlines(int32_t nNewValue);
	int32_t GetConstrainNumeric();
	void SetConstrainNumeric(int32_t bNewValue);
	VARIANT ConvertFormula(const VARIANT& Formula, int32_t FromReferenceStyle, const VARIANT& ToReferenceStyle, const VARIANT& ToAbsolute, const VARIANT& RelativeTo);
	int32_t GetCopyObjectsWithCells();
	void SetCopyObjectsWithCells(int32_t bNewValue);
	int32_t GetCursor();
	void SetCursor(int32_t nNewValue);
	int32_t GetCustomListCount();
	int32_t GetCutCopyMode();
	void SetCutCopyMode(int32_t nNewValue);
	int32_t GetDataEntryMode();
	void SetDataEntryMode(int32_t nNewValue);
	CString Get_Default();
	CString GetDefaultFilePath();
	void SetDefaultFilePath(LPCTSTR lpszNewValue);
	void DeleteChartAutoFormat(LPCTSTR Name);
	void DeleteCustomList(int32_t ListNum);
	LPDISPATCH GetDialogs();
	int32_t GetDisplayAlerts();
	void SetDisplayAlerts(int32_t bNewValue);
	int32_t GetDisplayFormulaBar();
	void SetDisplayFormulaBar(int32_t bNewValue);
	int32_t GetDisplayFullScreen();
	void SetDisplayFullScreen(int32_t bNewValue);
	int32_t GetDisplayNoteIndicator();
	void SetDisplayNoteIndicator(int32_t bNewValue);
	int32_t GetDisplayCommentIndicator();
	void SetDisplayCommentIndicator(int32_t nNewValue);
	int32_t GetDisplayExcel4Menus();
	void SetDisplayExcel4Menus(int32_t bNewValue);
	int32_t GetDisplayRecentFiles();
	void SetDisplayRecentFiles(int32_t bNewValue);
	int32_t GetDisplayScrollBars();
	void SetDisplayScrollBars(int32_t bNewValue);
	int32_t GetDisplayStatusBar();
	void SetDisplayStatusBar(int32_t bNewValue);
	void DoubleClick();
	int32_t GetEditDirectlyInCell();
	void SetEditDirectlyInCell(int32_t bNewValue);
	int32_t GetEnableAutoComplete();
	void SetEnableAutoComplete(int32_t bNewValue);
	int32_t GetEnableCancelKey();
	void SetEnableCancelKey(int32_t nNewValue);
	int32_t GetEnableSound();
	void SetEnableSound(int32_t bNewValue);
	VARIANT GetFileConverters(const VARIANT& Index1, const VARIANT& Index2);
	LPDISPATCH GetFileSearch();
	LPDISPATCH GetFileFind();
	void FindFile();
	int32_t GetFixedDecimal();
	void SetFixedDecimal(int32_t bNewValue);
	int32_t GetFixedDecimalPlaces();
	void SetFixedDecimalPlaces(int32_t nNewValue);
	VARIANT GetCustomListContents(int32_t ListNum);
	int32_t GetCustomListNum(const VARIANT& ListArray);
	VARIANT GetOpenFilename(const VARIANT& FileFilter, const VARIANT& FilterIndex, const VARIANT& Title, const VARIANT& ButtonText, const VARIANT& MultiSelect);
	VARIANT GetSaveAsFilename(const VARIANT& InitialFilename, const VARIANT& FileFilter, const VARIANT& FilterIndex, const VARIANT& Title, const VARIANT& ButtonText);
	void Goto(const VARIANT& Reference, const VARIANT& Scroll);
	double GetHeight();
	void SetHeight(double newValue);
	void Help(const VARIANT& HelpFile, const VARIANT& HelpContextID);
	int32_t GetIgnoreRemoteRequests();
	void SetIgnoreRemoteRequests(int32_t bNewValue);
	double InchesToPoints(double Inches);
	VARIANT InputBox(LPCTSTR Prompt, const VARIANT& Title, const VARIANT& Default, const VARIANT& Left, const VARIANT& Top, const VARIANT& HelpFile, const VARIANT& HelpContextID, const VARIANT& Type);
	int32_t GetInteractive();
	void SetInteractive(int32_t bNewValue);
	VARIANT GetInternational(const VARIANT& Index);
	int32_t GetIteration();
	void SetIteration(int32_t bNewValue);
	double GetLeft();
	void SetLeft(double newValue);
	CString GetLibraryPath();
	void MacroOptions(const VARIANT& Macro, const VARIANT& Description, const VARIANT& HasMenu, const VARIANT& MenuText, const VARIANT& HasShortcutKey, const VARIANT& ShortcutKey, const VARIANT& Category, const VARIANT& StatusBar, 
		const VARIANT& HelpContextID, const VARIANT& HelpFile);
	void MailLogoff();
	void MailLogon(const VARIANT& Name, const VARIANT& Password, const VARIANT& DownloadNewMail);
	VARIANT GetMailSession();
	int32_t GetMailSystem();
	int32_t GetMathCoprocessorAvailable();
	double GetMaxChange();
	void SetMaxChange(double newValue);
	int32_t GetMaxIterations();
	void SetMaxIterations(int32_t nNewValue);
	int32_t GetMemoryFree();
	int32_t GetMemoryTotal();
	int32_t GetMemoryUsed();
	int32_t GetMouseAvailable();
	int32_t GetMoveAfterReturn();
	void SetMoveAfterReturn(int32_t bNewValue);
	int32_t GetMoveAfterReturnDirection();
	void SetMoveAfterReturnDirection(int32_t nNewValue);
	LPDISPATCH GetRecentFiles();
	CString GetName();
	LPDISPATCH NextLetter();
	CString GetNetworkTemplatesPath();
	LPDISPATCH GetODBCErrors();
	int32_t GetODBCTimeout();
	void SetODBCTimeout(int32_t nNewValue);
	void OnKey(LPCTSTR Key, const VARIANT& Procedure);
	void OnRepeat(LPCTSTR Text, LPCTSTR Procedure);
	void OnTime(const VARIANT& EarliestTime, LPCTSTR Procedure, const VARIANT& LatestTime, const VARIANT& Schedule);
	void OnUndo(LPCTSTR Text, LPCTSTR Procedure);
	CString GetOnWindow();
	void SetOnWindow(LPCTSTR lpszNewValue);
	CString GetOperatingSystem();
	CString GetOrganizationName();
	CString GetPath();
	CString GetPathSeparator();
	VARIANT GetPreviousSelections(const VARIANT& Index);
	int32_t GetPivotTableSelection();
	void SetPivotTableSelection(int32_t bNewValue);
	int32_t GetPromptForSummaryInfo();
	void SetPromptForSummaryInfo(int32_t bNewValue);
	void Quit();
	void RecordMacro(const VARIANT& BasicCode, const VARIANT& XlmCode);
	int32_t GetRecordRelative();
	int32_t GetReferenceStyle();
	void SetReferenceStyle(int32_t nNewValue);
	VARIANT GetRegisteredFunctions(const VARIANT& Index1, const VARIANT& Index2);
	int32_t RegisterXLL(LPCTSTR Filename);
	void Repeat();
	int32_t GetRollZoom();
	void SetRollZoom(int32_t bNewValue);
	void SaveWorkspace(const VARIANT& Filename);
	int32_t GetScreenUpdating();
	void SetScreenUpdating(int32_t bNewValue);
	void SetDefaultChart(const VARIANT& FormatName, const VARIANT& Gallery);
	int32_t GetSheetsInNewWorkbook();
	void SetSheetsInNewWorkbook(int32_t nNewValue);
	int32_t GetShowChartTipNames();
	void SetShowChartTipNames(int32_t bNewValue);
	int32_t GetShowChartTipValues();
	void SetShowChartTipValues(int32_t bNewValue);
	CString GetStandardFont();
	void SetStandardFont(LPCTSTR lpszNewValue);
	double GetStandardFontSize();
	void SetStandardFontSize(double newValue);
	CString GetStartupPath();
	VARIANT GetStatusBar();
	void SetStatusBar(const VARIANT& newValue);
	CString GetTemplatesPath();
	int32_t GetShowToolTips();
	void SetShowToolTips(int32_t bNewValue);
	double GetTop();
	void SetTop(double newValue);
	int32_t GetDefaultSaveFormat();
	void SetDefaultSaveFormat(int32_t nNewValue);
	CString GetTransitionMenuKey();
	void SetTransitionMenuKey(LPCTSTR lpszNewValue);
	int32_t GetTransitionMenuKeyAction();
	void SetTransitionMenuKeyAction(int32_t nNewValue);
	int32_t GetTransitionNavigKeys();
	void SetTransitionNavigKeys(int32_t bNewValue);
	void Undo();
	double GetUsableHeight();
	double GetUsableWidth();
	int32_t GetUserControl();
	void SetUserControl(int32_t bNewValue);
	CString GetUserName_();
	void SetUserName(LPCTSTR lpszNewValue);
	CString GetValue();
	LPDISPATCH GetVbe();
	CString GetVersion();
	int32_t GetVisible();
	void SetVisible(int32_t bNewValue);
	void Volatile(const VARIANT& Volatile);
	void Wait(const VARIANT& Time);
	double GetWidth();
	void SetWidth(double newValue);
	int32_t GetWindowsForPens();
	int32_t GetWindowState();
	void SetWindowState(int32_t nNewValue);
	int32_t GetUILanguage();
	void SetUILanguage(int32_t nNewValue);
	int32_t GetDefaultSheetDirection();
	void SetDefaultSheetDirection(int32_t nNewValue);
	int32_t GetCursorMovement();
	void SetCursorMovement(int32_t nNewValue);
	int32_t GetControlCharacters();
	void SetControlCharacters(int32_t nNewValue);
	int32_t GetEnableEvents();
	void SetEnableEvents(int32_t bNewValue);
};
/////////////////////////////////////////////////////////////////////////////
// _Workbook wrapper class

class _Workbook : public COleDispatchDriver
{
public:
	_Workbook() {}		// Calls COleDispatchDriver default constructor
	_Workbook(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_Workbook(const _Workbook& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	int32_t GetAcceptLabelsInFormulas();
	void SetAcceptLabelsInFormulas(int32_t bNewValue);
	void Activate();
	LPDISPATCH GetActiveChart();
	LPDISPATCH GetActiveSheet();
	int32_t GetAutoUpdateFrequency();
	void SetAutoUpdateFrequency(int32_t nNewValue);
	int32_t GetAutoUpdateSaveChanges();
	void SetAutoUpdateSaveChanges(int32_t bNewValue);
	int32_t GetChangeHistoryDuration();
	void SetChangeHistoryDuration(int32_t nNewValue);
	LPDISPATCH GetBuiltinDocumentProperties();
	void ChangeFileAccess(int32_t Mode, const VARIANT& WritePassword, const VARIANT& Notify);
	void ChangeLink(LPCTSTR Name, LPCTSTR NewName, int32_t Type);
	LPDISPATCH GetCharts();
	void Close(const VARIANT& SaveChanges, const VARIANT& Filename, const VARIANT& RouteWorkbook);
	CString GetCodeName();
	CString Get_CodeName();
	void Set_CodeName(LPCTSTR lpszNewValue);
	VARIANT GetColors(const VARIANT& Index);
	void SetColors(const VARIANT& Index, const VARIANT& newValue);
	LPDISPATCH GetCommandBars();
	int32_t GetConflictResolution();
	void SetConflictResolution(int32_t nNewValue);
	LPDISPATCH GetContainer();
	int32_t GetCreateBackup();
	LPDISPATCH GetCustomDocumentProperties();
	int32_t GetDate1904();
	void SetDate1904(int32_t bNewValue);
	void DeleteNumberFormat(LPCTSTR NumberFormat);
	int32_t GetDisplayDrawingObjects();
	void SetDisplayDrawingObjects(int32_t nNewValue);
	int32_t ExclusiveAccess();
	int32_t GetFileFormat();
	void ForwardMailer();
	CString GetFullName();
	int32_t GetHasPassword();
	int32_t GetHasRoutingSlip();
	void SetHasRoutingSlip(int32_t bNewValue);
	int32_t GetIsAddin();
	void SetIsAddin(int32_t bNewValue);
	VARIANT LinkInfo(LPCTSTR Name, int32_t LinkInfo, const VARIANT& Type, const VARIANT& EditionRef);
	VARIANT LinkSources(const VARIANT& Type);
	LPDISPATCH GetMailer();
	void MergeWorkbook(const VARIANT& Filename);
	int32_t GetMultiUserEditing();
	CString GetName();
	LPDISPATCH GetNames();
	LPDISPATCH NewWindow();
	void OpenLinks(LPCTSTR Name, const VARIANT& ReadOnly, const VARIANT& Type);
	CString GetPath();
	int32_t GetPersonalViewListSettings();
	void SetPersonalViewListSettings(int32_t bNewValue);
	int32_t GetPersonalViewPrintSettings();
	void SetPersonalViewPrintSettings(int32_t bNewValue);
	LPDISPATCH PivotCaches();
	void Post(const VARIANT& DestName);
	int32_t GetPrecisionAsDisplayed();
	void SetPrecisionAsDisplayed(int32_t bNewValue);
	void PrintOut(const VARIANT& From, const VARIANT& To, const VARIANT& Copies, const VARIANT& Preview, const VARIANT& ActivePrinter, const VARIANT& PrintToFile, const VARIANT& Collate);
	void PrintPreview(const VARIANT& EnableChanges);
	void Protect(const VARIANT& Password, const VARIANT& Structure, const VARIANT& Windows);
	void ProtectSharing(const VARIANT& Filename, const VARIANT& Password, const VARIANT& WriteResPassword, const VARIANT& ReadOnlyRecommended, const VARIANT& CreateBackup, const VARIANT& SharingPassword);
	int32_t GetProtectStructure();
	int32_t GetProtectWindows();
	int32_t GetReadOnly();
	int32_t GetReadOnlyRecommended();
	void RefreshAll();
	void Reply();
	void ReplyAll();
	void RemoveUser(int32_t Index);
	int32_t GetRevisionNumber();
	void Route();
	int32_t GetRouted();
	LPDISPATCH GetRoutingSlip();
	void RunAutoMacros(int32_t Which);
	void Save();
	void SaveAs(const VARIANT& Filename, const VARIANT& FileFormat, const VARIANT& Password, const VARIANT& WriteResPassword, const VARIANT& ReadOnlyRecommended, const VARIANT& CreateBackup, int32_t AccessMode, const VARIANT& ConflictResolution, 
		const VARIANT& AddToMru, const VARIANT& TextCodepage, const VARIANT& TextVisualLayout);
	void SaveCopyAs(const VARIANT& Filename);
	int32_t GetSaved();
	void SetSaved(int32_t bNewValue);
	int32_t GetSaveLinkValues();
	void SetSaveLinkValues(int32_t bNewValue);
	void SendMail(const VARIANT& Recipients, const VARIANT& Subject, const VARIANT& ReturnReceipt);
	void SendMailer(const VARIANT& FileFormat, int32_t Priority);
	void SetLinkOnData(LPCTSTR Name, const VARIANT& Procedure);
	LPDISPATCH GetSheets();
	int32_t GetShowConflictHistory();
	void SetShowConflictHistory(int32_t bNewValue);
	LPDISPATCH GetStyles();
	void Unprotect(const VARIANT& Password);
	void UnprotectSharing(const VARIANT& SharingPassword);
	void UpdateFromFile();
	void UpdateLink(const VARIANT& Name, const VARIANT& Type);
	int32_t GetUpdateRemoteReferences();
	void SetUpdateRemoteReferences(int32_t bNewValue);
	VARIANT GetUserStatus();
	LPDISPATCH GetCustomViews();
	LPDISPATCH GetWindows();
	LPDISPATCH GetWorksheets();
	int32_t GetWriteReserved();
	CString GetWriteReservedBy();
	LPDISPATCH GetExcel4IntlMacroSheets();
	LPDISPATCH GetExcel4MacroSheets();
	int32_t GetTemplateRemoveExtData();
	void SetTemplateRemoveExtData(int32_t bNewValue);
	void HighlightChangesOptions(const VARIANT& When, const VARIANT& Who, const VARIANT& Where);
	int32_t GetHighlightChangesOnScreen();
	void SetHighlightChangesOnScreen(int32_t bNewValue);
	int32_t GetKeepChangeHistory();
	void SetKeepChangeHistory(int32_t bNewValue);
	int32_t GetListChangesOnNewSheet();
	void SetListChangesOnNewSheet(int32_t bNewValue);
	void PurgeChangeHistoryNow(int32_t Days, const VARIANT& SharingPassword);
	void AcceptAllChanges(const VARIANT& When, const VARIANT& Who, const VARIANT& Where);
	void RejectAllChanges(const VARIANT& When, const VARIANT& Who, const VARIANT& Where);
	void ResetColors();
	LPDISPATCH GetVBProject();
	void FollowHyperlink(LPCTSTR Address, const VARIANT& SubAddress, const VARIANT& NewWindow, const VARIANT& AddHistory, const VARIANT& ExtraInfo, const VARIANT& Method, const VARIANT& HeaderInfo);
	void AddToFavorites();
	int32_t GetIsInplace();
};
/////////////////////////////////////////////////////////////////////////////
// _Worksheet wrapper class

class _Worksheet : public COleDispatchDriver
{
public:
	_Worksheet() {}		// Calls COleDispatchDriver default constructor
	_Worksheet(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_Worksheet(const _Worksheet& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	void Activate();
	void Copy(const VARIANT& Before, const VARIANT& After);
	void Delete();
	CString GetCodeName();
	CString Get_CodeName();
	void Set_CodeName(LPCTSTR lpszNewValue);
	int32_t GetIndex();
	void Move(const VARIANT& Before, const VARIANT& After);
	CString GetName();
	void SetName(LPCTSTR lpszNewValue);
	LPDISPATCH GetNext();
	LPDISPATCH GetPageSetup();
	LPDISPATCH GetPrevious();
	void PrintOut(const VARIANT& From, const VARIANT& To, const VARIANT& Copies, const VARIANT& Preview, const VARIANT& ActivePrinter, const VARIANT& PrintToFile, const VARIANT& Collate);
	void PrintPreview(const VARIANT& EnableChanges);
	void Protect(const VARIANT& Password, const VARIANT& DrawingObjects, const VARIANT& Contents, const VARIANT& Scenarios, const VARIANT& UserInterfaceOnly);
	int32_t GetProtectContents();
	int32_t GetProtectDrawingObjects();
	int32_t GetProtectionMode();
	int32_t GetProtectScenarios();
	void SaveAs(LPCTSTR Filename, const VARIANT& FileFormat, const VARIANT& Password, const VARIANT& WriteResPassword, const VARIANT& ReadOnlyRecommended, const VARIANT& CreateBackup, const VARIANT& AddToMru, const VARIANT& TextCodepage, 
		const VARIANT& TextVisualLayout);
	void Select(const VARIANT& Replace);
	void Unprotect(const VARIANT& Password);
	int32_t GetVisible();
	void SetVisible(int32_t nNewValue);
	LPDISPATCH GetShapes();
	int32_t GetTransitionExpEval();
	void SetTransitionExpEval(int32_t bNewValue);
	int32_t GetAutoFilterMode();
	void SetAutoFilterMode(int32_t bNewValue);
	void SetBackgroundPicture(LPCTSTR Filename);
	void Calculate();
	int32_t GetEnableCalculation();
	void SetEnableCalculation(int32_t bNewValue);
	LPDISPATCH GetCells();
	LPDISPATCH ChartObjects(const VARIANT& Index);
	void CheckSpelling(const VARIANT& CustomDictionary, const VARIANT& IgnoreUppercase, const VARIANT& AlwaysSuggest, const VARIANT& IgnoreInitialAlefHamza, const VARIANT& IgnoreFinalYaa, const VARIANT& SpellScript);
	LPDISPATCH GetCircularReference();
	void ClearArrows();
	LPDISPATCH GetColumns();
	int32_t GetConsolidationFunction();
	VARIANT GetConsolidationOptions();
	VARIANT GetConsolidationSources();
	int32_t GetEnableAutoFilter();
	void SetEnableAutoFilter(int32_t bNewValue);
	int32_t GetEnableSelection();
	void SetEnableSelection(int32_t nNewValue);
	int32_t GetEnableOutlining();
	void SetEnableOutlining(int32_t bNewValue);
	int32_t GetEnablePivotTable();
	void SetEnablePivotTable(int32_t bNewValue);
	VARIANT Evaluate(const VARIANT& Name);
	VARIANT _Evaluate(const VARIANT& Name);
	int32_t GetFilterMode();
	void ResetAllPageBreaks();
	LPDISPATCH GetNames();
	LPDISPATCH OLEObjects(const VARIANT& Index);
	LPDISPATCH GetOutline();
	void Paste(const VARIANT& Destination, const VARIANT& Link);
	void PasteSpecial(const VARIANT& Format, const VARIANT& Link, const VARIANT& DisplayAsIcon, const VARIANT& IconFileName, const VARIANT& IconIndex, const VARIANT& IconLabel);
	LPDISPATCH PivotTables(const VARIANT& Index);
	LPDISPATCH PivotTableWizard(const VARIANT& SourceType, const VARIANT& SourceData, const VARIANT& TableDestination, const VARIANT& TableName, const VARIANT& RowGrand, const VARIANT& ColumnGrand, const VARIANT& SaveData, 
		const VARIANT& HasAutoFormat, const VARIANT& AutoPage, const VARIANT& Reserved, const VARIANT& BackgroundQuery, const VARIANT& OptimizeCache, const VARIANT& PageFieldOrder, const VARIANT& PageFieldWrapCount, const VARIANT& ReadData, 
		const VARIANT& Connection);
	LPDISPATCH GetRange(const VARIANT& Cell1, const VARIANT& Cell2);
	LPDISPATCH GetRows();
	LPDISPATCH Scenarios(const VARIANT& Index);
	CString GetScrollArea();
	void SetScrollArea(LPCTSTR lpszNewValue);
	void ShowAllData();
	void ShowDataForm();
	double GetStandardHeight();
	double GetStandardWidth();
	void SetStandardWidth(double newValue);
	int32_t GetTransitionFormEntry();
	void SetTransitionFormEntry(int32_t bNewValue);
	int32_t GetType();
	LPDISPATCH GetUsedRange();
	LPDISPATCH GetHPageBreaks();
	LPDISPATCH GetVPageBreaks();
	LPDISPATCH GetQueryTables();
	int32_t GetDisplayPageBreaks();
	void SetDisplayPageBreaks(int32_t bNewValue);
	LPDISPATCH GetComments();
	LPDISPATCH GetHyperlinks();
	void ClearCircles();
	void CircleInvalid();
	LPDISPATCH GetAutoFilter();
};
/////////////////////////////////////////////////////////////////////////////
// Range wrapper class

class Range : public COleDispatchDriver
{
public:
	Range() {}		// Calls COleDispatchDriver default constructor
	Range(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	Range(const Range& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	void Activate();
	VARIANT GetAddIndent();
	void SetAddIndent(const VARIANT& newValue);
	CString GetAddress(const VARIANT& RowAbsolute, const VARIANT& ColumnAbsolute, int32_t ReferenceStyle, const VARIANT& External, const VARIANT& RelativeTo);
	CString GetAddressLocal(const VARIANT& RowAbsolute, const VARIANT& ColumnAbsolute, int32_t ReferenceStyle, const VARIANT& External, const VARIANT& RelativeTo);
	void AdvancedFilter(int32_t Action, const VARIANT& CriteriaRange, const VARIANT& CopyToRange, const VARIANT& Unique);
	void ApplyNames(const VARIANT& Names, const VARIANT& IgnoreRelativeAbsolute, const VARIANT& UseRowColumnNames, const VARIANT& OmitColumn, const VARIANT& OmitRow, int32_t Order, const VARIANT& AppendLast);
	void ApplyOutlineStyles();
	LPDISPATCH GetAreas();
	CString AutoComplete(LPCTSTR String);
	void AutoFill(LPDISPATCH Destination, int32_t Type);
	void AutoFilter(const VARIANT& Field, const VARIANT& Criteria1, int32_t Operator, const VARIANT& Criteria2, const VARIANT& VisibleDropDown);
	void AutoFit();
	void AutoFormat(int32_t Format, const VARIANT& Number, const VARIANT& Font, const VARIANT& Alignment, const VARIANT& Border, const VARIANT& Pattern, const VARIANT& Width);
	void AutoOutline();
	void BorderAround(const VARIANT& LineStyle, int32_t Weight, int32_t ColorIndex, const VARIANT& Color);
	LPDISPATCH GetBorders();
	void Calculate();
	LPDISPATCH GetCells();
	LPDISPATCH GetCharacters(const VARIANT& Start, const VARIANT& Length);
	void CheckSpelling(const VARIANT& CustomDictionary, const VARIANT& IgnoreUppercase, const VARIANT& AlwaysSuggest, const VARIANT& IgnoreInitialAlefHamza, const VARIANT& IgnoreFinalYaa, const VARIANT& SpellScript);
	void Clear();
	void ClearContents();
	void ClearFormats();
	void ClearNotes();
	void ClearOutline();
	int32_t GetColumn();
	LPDISPATCH ColumnDifferences(const VARIANT& Comparison);
	LPDISPATCH GetColumns();
	VARIANT GetColumnWidth();
	void SetColumnWidth(const VARIANT& newValue);
	void Consolidate(const VARIANT& Sources, const VARIANT& Function, const VARIANT& TopRow, const VARIANT& LeftColumn, const VARIANT& CreateLinks);
	void Copy(const VARIANT& Destination);
	int32_t CopyFromRecordset(LPUNKNOWN Data, const VARIANT& MaxRows, const VARIANT& MaxColumns);
	void CopyPicture(int32_t Appearance, int32_t Format);
	int32_t GetCount();
	void CreateNames(const VARIANT& Top, const VARIANT& Left, const VARIANT& Bottom, const VARIANT& Right);
	void CreatePublisher(const VARIANT& Edition, int32_t Appearance, const VARIANT& ContainsPICT, const VARIANT& ContainsBIFF, const VARIANT& ContainsRTF, const VARIANT& ContainsVALU);
	LPDISPATCH GetCurrentArray();
	LPDISPATCH GetCurrentRegion();
	void Cut(const VARIANT& Destination);
	void DataSeries(const VARIANT& Rowcol, int32_t Type, int32_t Date, const VARIANT& Step, const VARIANT& Stop, const VARIANT& Trend);
	VARIANT Get_Default(const VARIANT& RowIndex, const VARIANT& ColumnIndex);
	void Set_Default(const VARIANT& RowIndex, const VARIANT& ColumnIndex, const VARIANT& newValue);
	void Delete(const VARIANT& Shift);
	LPDISPATCH GetDependents();
	VARIANT DialogBox_();
	LPDISPATCH GetDirectDependents();
	LPDISPATCH GetDirectPrecedents();
	VARIANT EditionOptions(int32_t Type, int32_t Option, const VARIANT& Name, const VARIANT& Reference, int32_t Appearance, int32_t ChartSize, const VARIANT& Format);
	LPDISPATCH GetEnd(int32_t Direction);
	LPDISPATCH GetEntireColumn();
	LPDISPATCH GetEntireRow();
	void FillDown();
	void FillLeft();
	void FillRight();
	void FillUp();
	LPDISPATCH Find(const VARIANT& What, const VARIANT& After, const VARIANT& LookIn, const VARIANT& LookAt, const VARIANT& SearchOrder, int32_t SearchDirection, const VARIANT& MatchCase, const VARIANT& MatchByte, 
		const VARIANT& MatchControlCharacters, const VARIANT& MatchDiacritics, const VARIANT& MatchKashida, const VARIANT& MatchAlefHamza);
	LPDISPATCH FindNext(const VARIANT& After);
	LPDISPATCH FindPrevious(const VARIANT& After);
	LPDISPATCH GetFont();
	VARIANT GetFormula();
	void SetFormula(const VARIANT& newValue);
	VARIANT GetFormulaArray();
	void SetFormulaArray(const VARIANT& newValue);
	int32_t GetFormulaLabel();
	void SetFormulaLabel(int32_t nNewValue);
	VARIANT GetFormulaHidden();
	void SetFormulaHidden(const VARIANT& newValue);
	VARIANT GetFormulaLocal();
	void SetFormulaLocal(const VARIANT& newValue);
	VARIANT GetFormulaR1C1();
	void SetFormulaR1C1(const VARIANT& newValue);
	VARIANT GetFormulaR1C1Local();
	void SetFormulaR1C1Local(const VARIANT& newValue);
	void FunctionWizard();
	int32_t GoalSeek(const VARIANT& Goal, LPDISPATCH ChangingCell);
	VARIANT Group(const VARIANT& Start, const VARIANT& End, const VARIANT& By, const VARIANT& Periods);
	VARIANT GetHasArray();
	VARIANT GetHasFormula();
	VARIANT GetHeight();
	VARIANT GetHidden();
	void SetHidden(const VARIANT& newValue);
	VARIANT GetHorizontalAlignment();
	void SetHorizontalAlignment(const VARIANT& newValue);
	VARIANT GetIndentLevel();
	void SetIndentLevel(const VARIANT& newValue);
	void InsertIndent(int32_t InsertAmount);
	void Insert(const VARIANT& Shift);
	LPDISPATCH GetInterior();
	VARIANT GetItem(const VARIANT& RowIndex, const VARIANT& ColumnIndex);
	void SetItem(const VARIANT& RowIndex, const VARIANT& ColumnIndex, const VARIANT& newValue);
	void Justify();
	VARIANT GetLeft();
	int32_t GetListHeaderRows();
	void ListNames();
	int32_t GetLocationInTable();
	VARIANT GetLocked();
	void SetLocked(const VARIANT& newValue);
	void Merge(const VARIANT& Across);
	void UnMerge();
	LPDISPATCH GetMergeArea();
	VARIANT GetMergeCells();
	void SetMergeCells(const VARIANT& newValue);
	VARIANT GetName();
	void SetName(const VARIANT& newValue);
	void NavigateArrow(const VARIANT& TowardPrecedent, const VARIANT& ArrowNumber, const VARIANT& LinkNumber);
	LPUNKNOWN Get_NewEnum();
	LPDISPATCH GetNext();
	CString NoteText(const VARIANT& Text, const VARIANT& Start, const VARIANT& Length);
	VARIANT GetNumberFormat();
	void SetNumberFormat(const VARIANT& newValue);
	VARIANT GetNumberFormatLocal();
	void SetNumberFormatLocal(const VARIANT& newValue);
	LPDISPATCH GetOffset(const VARIANT& RowOffset, const VARIANT& ColumnOffset);
	VARIANT GetOrientation();
	void SetOrientation(const VARIANT& newValue);
	VARIANT GetOutlineLevel();
	void SetOutlineLevel(const VARIANT& newValue);
	int32_t GetPageBreak();
	void SetPageBreak(int32_t nNewValue);
	void Parse(const VARIANT& ParseLine, const VARIANT& Destination);
	void PasteSpecial(int32_t Paste, int32_t Operation, const VARIANT& SkipBlanks, const VARIANT& Transpose);
	LPDISPATCH GetPivotField();
	LPDISPATCH GetPivotItem();
	LPDISPATCH GetPivotTable();
	LPDISPATCH GetPrecedents();
	VARIANT GetPrefixCharacter();
	LPDISPATCH GetPrevious();
	void PrintOut(const VARIANT& From, const VARIANT& To, const VARIANT& Copies, const VARIANT& Preview, const VARIANT& ActivePrinter, const VARIANT& PrintToFile, const VARIANT& Collate);
	void PrintPreview(const VARIANT& EnableChanges);
	LPDISPATCH GetQueryTable();
	LPDISPATCH GetRange(const VARIANT& Cell1, const VARIANT& Cell2);
	void RemoveSubtotal();
	int32_t Replace(const VARIANT& What, const VARIANT& Replacement, const VARIANT& LookAt, const VARIANT& SearchOrder, const VARIANT& MatchCase, const VARIANT& MatchByte, const VARIANT& MatchControlCharacters, const VARIANT& MatchDiacritics, 
		const VARIANT& MatchKashida, const VARIANT& MatchAlefHamza);
	LPDISPATCH GetResize(const VARIANT& RowSize, const VARIANT& ColumnSize);
	int32_t GetRow();
	LPDISPATCH RowDifferences(const VARIANT& Comparison);
	VARIANT GetRowHeight();
	void SetRowHeight(const VARIANT& newValue);
	LPDISPATCH GetRows();
	VARIANT Run(const VARIANT& Arg1, const VARIANT& Arg2, const VARIANT& Arg3, const VARIANT& Arg4, const VARIANT& Arg5, const VARIANT& Arg6, const VARIANT& Arg7, const VARIANT& Arg8, const VARIANT& Arg9, const VARIANT& Arg10, 
		const VARIANT& Arg11, const VARIANT& Arg12, const VARIANT& Arg13, const VARIANT& Arg14, const VARIANT& Arg15, const VARIANT& Arg16, const VARIANT& Arg17, const VARIANT& Arg18, const VARIANT& Arg19, const VARIANT& Arg20, 
		const VARIANT& Arg21, const VARIANT& Arg22, const VARIANT& Arg23, const VARIANT& Arg24, const VARIANT& Arg25, const VARIANT& Arg26, const VARIANT& Arg27, const VARIANT& Arg28, const VARIANT& Arg29, const VARIANT& Arg30);
	void Select();
	void Show();
	void ShowDependents(const VARIANT& Remove);
	VARIANT GetShowDetail();
	void SetShowDetail(const VARIANT& newValue);
	void ShowErrors();
	void ShowPrecedents(const VARIANT& Remove);
	VARIANT GetShrinkToFit();
	void SetShrinkToFit(const VARIANT& newValue);
	void Sort(const VARIANT& Key1, int32_t Order1, const VARIANT& Key2, const VARIANT& Type, int32_t Order2, const VARIANT& Key3, int32_t Order3, int32_t Header, const VARIANT& OrderCustom, const VARIANT& MatchCase, int32_t Orientation, int32_t SortMethod, 
		const VARIANT& IgnoreControlCharacters, const VARIANT& IgnoreDiacritics, const VARIANT& IgnoreKashida);
	void SortSpecial(int32_t SortMethod, const VARIANT& Key1, int32_t Order1, const VARIANT& Type, const VARIANT& Key2, int32_t Order2, const VARIANT& Key3, int32_t Order3, int32_t Header, const VARIANT& OrderCustom, const VARIANT& MatchCase, int32_t Orientation);
	LPDISPATCH GetSoundNote();
	LPDISPATCH SpecialCells(int32_t Type, const VARIANT& Value);
	VARIANT GetStyle();
	void SetStyle(const VARIANT& newValue);
	void SubscribeTo(LPCTSTR Edition, int32_t Format);
	void Subtotal(int32_t GroupBy, int32_t Function, const VARIANT& TotalList, const VARIANT& Replace, const VARIANT& PageBreaks, int32_t SummaryBelowData);
	VARIANT GetSummary();
	void Table(const VARIANT& RowInput, const VARIANT& ColumnInput);
	VARIANT GetText();
	void TextToColumns(const VARIANT& Destination, int32_t DataType, int32_t TextQualifier, const VARIANT& ConsecutiveDelimiter, const VARIANT& Tab, const VARIANT& Semicolon, const VARIANT& Comma, const VARIANT& Space, const VARIANT& Other, 
		const VARIANT& OtherChar, const VARIANT& FieldInfo);
	VARIANT GetTop();
	void Ungroup();
	VARIANT GetUseStandardHeight();
	void SetUseStandardHeight(const VARIANT& newValue);
	VARIANT GetUseStandardWidth();
	void SetUseStandardWidth(const VARIANT& newValue);
	LPDISPATCH GetValidation();
	VARIANT GetValue();
	void SetValue(const VARIANT& newValue);
	VARIANT GetValue2();
	void SetValue2(const VARIANT& newValue);
	VARIANT GetVerticalAlignment();
	void SetVerticalAlignment(const VARIANT& newValue);
	VARIANT GetWidth();
	LPDISPATCH GetWorksheet();
	VARIANT GetWrapText();
	void SetWrapText(const VARIANT& newValue);
	LPDISPATCH AddComment(const VARIANT& Text);
	LPDISPATCH GetComment();
	void ClearComments();
	LPDISPATCH GetPhonetic();
	LPDISPATCH GetFormatConditions();
	int32_t GetReadingOrder();
	void SetReadingOrder(int32_t nNewValue);
	LPDISPATCH GetHyperlinks();
};
/////////////////////////////////////////////////////////////////////////////
// Border wrapper class

class Border : public COleDispatchDriver
{
public:
	Border() {}		// Calls COleDispatchDriver default constructor
	Border(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	Border(const Border& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	VARIANT GetColor();
	void SetColor(const VARIANT& newValue);
	VARIANT GetColorIndex();
	void SetColorIndex(const VARIANT& newValue);
	VARIANT GetLineStyle();
	void SetLineStyle(const VARIANT& newValue);
	VARIANT GetWeight();
	void SetWeight(const VARIANT& newValue);
};
/////////////////////////////////////////////////////////////////////////////
// Borders wrapper class

class Borders : public COleDispatchDriver
{
public:
	Borders() {}		// Calls COleDispatchDriver default constructor
	Borders(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	Borders(const Borders& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	VARIANT GetColor();
	void SetColor(const VARIANT& newValue);
	VARIANT GetColorIndex();
	void SetColorIndex(const VARIANT& newValue);
	int32_t GetCount();
	LPDISPATCH GetItem(int32_t Index);
	VARIANT GetLineStyle();
	void SetLineStyle(const VARIANT& newValue);
	LPUNKNOWN Get_NewEnum();
	VARIANT GetValue();
	void SetValue(const VARIANT& newValue);
	VARIANT GetWeight();
	void SetWeight(const VARIANT& newValue);
	LPDISPATCH Get_Default(int32_t Index);
};
/////////////////////////////////////////////////////////////////////////////
// Interior wrapper class

class Interior : public COleDispatchDriver
{
public:
	Interior() {}		// Calls COleDispatchDriver default constructor
	Interior(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	Interior(const Interior& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH GetApplication();
	int32_t GetCreator();
	LPDISPATCH GetParent();
	VARIANT GetColor();
	void SetColor(const VARIANT& newValue);
	VARIANT GetColorIndex();
	void SetColorIndex(const VARIANT& newValue);
	VARIANT GetInvertIfNegative();
	void SetInvertIfNegative(const VARIANT& newValue);
	VARIANT GetPattern();
	void SetPattern(const VARIANT& newValue);
	VARIANT GetPatternColor();
	void SetPatternColor(const VARIANT& newValue);
	VARIANT GetPatternColorIndex();
	void SetPatternColorIndex(const VARIANT& newValue);
};


#endif //__EXCEL8_H
