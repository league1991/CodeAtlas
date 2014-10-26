#pragma once

#include "Delaunay.h"
#include "DelaunayRoute.h"

using namespace DelaunayCore;
class CChildView : public CWnd
{
public:
	CChildView();
	virtual ~CChildView();

protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnFileOptions();
	void NewVertices(void);

	vertexSet m_Vertices;
	triangleSet m_Triangles;
	int m_nVertices;
	REAL m_hScale;
	REAL m_vScale;
	int m_hMargin;
	int m_vMargin;
	int m_yText;
	int m_nTime;

	DelaunayRouter m_router;
	DECLARE_MESSAGE_MAP()
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	void OnMouseMove(UINT nFlags, CPoint point);
public:
	afx_msg void OnUpdate();
	afx_msg void OnUpdatepoint();
};

class COptionsDlg : public CDialog
{
public:
	enum { IDD = IDD_OPTIONS };

	COptionsDlg(CWnd* pParent = NULL);
	virtual ~COptionsDlg();

	int m_nVertices;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	CSpinButtonCtrl c_spinVertices;
	DECLARE_DYNAMIC(COptionsDlg)
	DECLARE_MESSAGE_MAP()
};
