
// TestDrillMonitor.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CTestDrillMonitorApp:
// このクラスの実装については、TestDrillMonitor.cpp を参照してください
//

class CTestDrillMonitorApp : public CWinApp
{
public:
	CTestDrillMonitorApp();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CTestDrillMonitorApp theApp;
