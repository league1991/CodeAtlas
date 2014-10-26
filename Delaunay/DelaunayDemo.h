// DelaunayDemo.h : main header file for the DelaunayDemo application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// CDelaunayDemoApp:
// See DelaunayDemo.cpp for the implementation of this class
//

class CDelaunayDemoApp : public CWinApp
{
public:
	CDelaunayDemoApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CDelaunayDemoApp theApp;