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

#if !defined(AFX_COLORBAR_H__D1243F40_D4D2_11D2_8DDF_00104B6FD9E3__INCLUDED_)
#define AFX_COLORBAR_H__D1243F40_D4D2_11D2_8DDF_00104B6FD9E3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once

#include <cstdint>
#endif // _MSC_VER >= 1000
// ColorBar.h : header file
//

#ifdef WWCTRL_LIB
	#define LINKAGE_SPEC	__declspec (dllexport) 
#else
	#define LINKAGE_SPEC
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Constants
//
const int MAX_COLOR_POINTS		= 15;

//
//	Window styles
//
#define		CBRS_SUNKEN			0x00000001
#define		CBRS_RAISED			0x00000002
#define		CBRS_FRAME			0x00000004
#define		CBRS_FRAME_MASK	CBRS_FRAME | CBRS_RAISED | CBRS_SUNKEN
#define		CBRS_HORZ			0x00000008
#define		CBRS_VERT			0x00000010
#define		CBRS_HAS_SEL		0x00000020
#define		CBRS_SHOW_FRAMES	0x00000040
#define		CBRS_PAINT_GRAPH	0x00000080

//
//	Window messages and notifications
//
#define		CBRN_MOVED_POINT		0x0001
#define		CBRN_MOVING_POINT		0x0002
#define		CBRN_DBLCLK_POINT		0x0003
#define		CBRN_SEL_CHANGED		0x0004
#define		CBRN_DEL_POINT			0x0005
#define		CBRN_DELETED_POINT	0x0006
#define		CBRN_INSERTED_POINT	0x0007
#define		CBRM_GETCOLOR			(WM_USER+101)
#define		CBRM_SETCOLOR		(	WM_USER+102)

//
//	Point styles
//
#define		POINT_VISIBLE		0x00000001
#define		POINT_CAN_MOVE		0x00000002

//
//	Return values for WM_NOTIFY
//
#define		STOP_EVENT			0x00000077


/////////////////////////////////////////////////////////////////////////////
//
// Structures
//

// Structure used to send notifications via WM_NOTIFY
typedef struct
{
	NMHDR		hdr;
	int		key_index;
	float		red;
	float		green;
	float		blue;
	float		position;
} CBR_NMHDR;


/////////////////////////////////////////////////////////////////////////////
//
// ColorBarClass
//
class LINKAGE_SPEC ColorBarClass : public CWnd
{
// Construction
public:
	ColorBarClass();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ColorBarClass)
	public:
	virtual int32_t Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, uint32_t dwStyle, const RECT& rect, CWnd* pParentWnd, uint32_t nID, CCreateContext* pContext = NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~ColorBarClass();

	// Generated message map functions
protected:
	//{{AFX_MSG(ColorBarClass)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(uint32_t nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(uint32_t nFlags, CPoint point);
	afx_msg void OnLButtonUp(uint32_t nFlags, CPoint point);
	afx_msg void OnMouseMove(uint32_t nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKeyDown(uint32_t nChar, uint32_t nRepCnt, uint32_t nFlags);
	afx_msg void OnLButtonDblClk(uint32_t nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	friend intptr_t WINAPI fnColorBarProc (HWND hwnd, uint32_t message, uintptr_t wparam, intptr_t lparam);

	public:
		
		/////////////////////////////////////////////////////////////////////////
		//
		//	Public methods
		//
		bool				Insert_Point (int index, float position, float red, float green, float blue, uint32_t flags = POINT_VISIBLE | POINT_CAN_MOVE);
		bool				Insert_Point (CPoint point, uint32_t flags = POINT_VISIBLE | POINT_CAN_MOVE);
		bool				Modify_Point (int index, float position, float red, float green, float blue, uint32_t flags = POINT_VISIBLE | POINT_CAN_MOVE);
		bool				Set_User_Data (int index, uint32_t data);
		uint32_t				Get_User_Data (int index);
		bool				Set_Graph_Percent (int index, float percent);
		float				Get_Graph_Percent (int index);
		bool				Delete_Point (int index);
		void				Clear_Points (void);

		int				Get_Point_Count (void) const { return m_iColorPoints; }
		bool				Get_Point (int index, float *position, float *red, float *green, float *blue);

		int				Marker_From_Point (CPoint point);
		void				Set_Selection_Pos (float pos);
		float				Get_Selection_Pos (void) const { return m_SelectionPos; }
		void				Get_Color (float position, float *red, float *green, float *blue);

		void				Get_Range (float &min, float &max) const	{ min = m_MinPos; max = m_MaxPos; }
		void				Set_Range (float min, float max);

		void				Set_Redraw (bool redraw = true);
		intptr_t			Send_Notification (int code, int key);

		//////////////////////////////////////////////////////////////////////////
		//	Static members
		//////////////////////////////////////////////////////////////////////////
		static ColorBarClass *Get_Color_Bar (HWND window_handle)	{  return (ColorBarClass *)::GetProp (window_handle, "CLASSPOINTER"); }

	protected:

		/////////////////////////////////////////////////////////////////////////
		//
		//	Protected data types
		//
		typedef struct
		{
			float	PosPercent;			
			int	StartPos;
			int	EndPos;

			float	StartGraphPercent;
			float	GraphPercentInc;

			float StartRed;
			float StartGreen;
			float StartBlue;			

			float	RedInc;
			float	GreenInc;
			float	BlueInc;

			uint32_t user_data;
			int	flags;

		} COLOR_POINT;

		/////////////////////////////////////////////////////////////////////////
		//
		//	Protected methods
		//
		void				Paint_DIB (void);
		void				Create_Bitmap (void);
		void				Free_Bitmap (void);
		void				Free_Marker_Bitmap (void);
		void				Paint_Bar_Horz (int x_pos, int y_pos, int width, int height, uint8_t *pbits);
		void				Paint_Bar_Vert (int x_pos, int y_pos, int width, int height, uint8_t *pbits);
		void				Update_Point_Info (void);
		void				Load_Key_Frame_BMP (void);
		void				Paint_Key_Frame (int x_pos, int y_pos);
		void				Paint_Screen (HDC hwnd_dc);
		void				Get_Selection_Rectangle (CRect &rect);
		void				Move_Selection (CPoint point, bool send_notify = true);
		void				Move_Selection (float new_pos, bool send_notify = true);
		void				Repaint (void);

	private:

		/////////////////////////////////////////////////////////////////////////
		//
		//	Private member data
		//
		HBITMAP			m_hBitmap;
		HBITMAP			m_KeyFrameDIB;
		HDC				m_hMemDC;
		uint8_t	*			m_pBits;
		uint8_t	*			m_pKeyFrameBits;
		int				m_iColorWidth;
		int				m_iColorHeight;
		int				m_iBMPWidth;
		int				m_iBMPHeight;
		int				m_iMarkerWidth;
		int				m_iMarkerHeight;
		int				m_iScanlineSize;
		int				m_iColorPoints;
		float				m_MinPos;
		float				m_MaxPos;
		COLOR_POINT		m_ColorPoints[MAX_COLOR_POINTS];
		CRect				m_ColorArea;
		int				m_iCurrentKey;
		bool				m_bMoving;
		bool				m_bMoved;
		bool				m_bRedraw;
		float				m_SelectionPos;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORBAR_H__D1243F40_D4D2_11D2_8DDF_00104B6FD9E3__INCLUDED_)
