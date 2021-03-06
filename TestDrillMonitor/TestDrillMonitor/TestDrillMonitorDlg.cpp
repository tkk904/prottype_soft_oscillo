
// TestDrillMonitorDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "TestDrillMonitor.h"
#include "TestDrillMonitorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CNullDrillMonitor : public IDrillMonitor
{
public:
	virtual int  GetData(double* ptr, int count);
	virtual int  GetError();
	virtual bool Start(const char* spindle, const char* sensor);
	virtual void Stop();
	virtual bool Initialize();
	virtual void Finalize();
	CNullDrillMonitor();
};
CNullDrillMonitor::CNullDrillMonitor()
{
}
int  CNullDrillMonitor::GetData(double* ptr, int count)
{
	for (int i = 0; i < count; i++) {
		*ptr++ = 0.0;
	}
	return 0;
}
int  CNullDrillMonitor::GetError()
{
	return 0;
}
bool CNullDrillMonitor::Start(const char* spindle, const char* sensor)
{
	return false;
}
void CNullDrillMonitor::Stop()
{
}
bool CNullDrillMonitor::Initialize()
{
	return false;
}
void CNullDrillMonitor::Finalize()
{
}

// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTestDrillMonitorDlg ダイアログ



CTestDrillMonitorDlg::CTestDrillMonitorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_TESTDRILLMONITOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//
	m_hDrillMonitorDLL = LoadLibrary(_T("Primado2Monitor.dll"));
	FARPROC GetDrillMonitor = GetProcAddress(m_hDrillMonitorDLL, "DrillMonitor");
	//
	m_pDrillMonitor = NULL;
	if (GetDrillMonitor != NULL) {
		m_pDrillMonitor = (IDrillMonitor*)GetDrillMonitor();
	}
	if (m_pDrillMonitor == NULL) {
		static CNullDrillMonitor l_nullmonitor;
		m_pDrillMonitor = &l_nullmonitor;
	}
}

void CTestDrillMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTestDrillMonitorDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_CBN_DROPDOWN(IDC_PRIMADO2_PORT_LIST, &CTestDrillMonitorDlg::OnCbnDropdownPrimado2PortList)
	ON_CBN_DROPDOWN(IDC_TSND121_PORT_LIST, &CTestDrillMonitorDlg::OnCbnDropdownTsnd121PortList)
	ON_BN_CLICKED(IDC_START, &CTestDrillMonitorDlg::OnBnClickedStart)
END_MESSAGE_MAP()


// CTestDrillMonitorDlg メッセージ ハンドラー

BOOL CTestDrillMonitorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	m_pDrillMonitor->Initialize();

	m_timerID = SetTimer(1, 100, NULL);

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CTestDrillMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CTestDrillMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CTestDrillMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CTestDrillMonitorDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: ここに特定なコードを追加するか、もしくは基底クラスを呼び出してください。
	if (pMsg->message == WM_KEYDOWN) {
		switch (pMsg->wParam) {
		case VK_RETURN :
		case VK_ESCAPE :
			return TRUE;
		default :
			break;
		}
	}
	return CDialog::PreTranslateMessage(pMsg);
}
void CTestDrillMonitorDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	m_pDrillMonitor->Finalize();
	if (m_hDrillMonitorDLL != NULL) {
		FreeLibrary(m_hDrillMonitorDLL);
	}
}
void CTestDrillMonitorDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (m_pDrillMonitor == NULL) {
		return;
	}
	double data[8];
	int n = m_pDrillMonitor->GetData(data, sizeof(data) / sizeof(data[0]));
	CString s;
	s.Format("%.0f", data[0]);
	SetDlgItemText(IDC_RPM_TEXT, s);
	s.Format("%.3f", data[1]);
	SetDlgItemText(IDC_AMPERE_TEXT, s);

	s.Format("%.3f", data[2]);
	SetDlgItemText(IDC_ACCEL_X, s);
	s.Format("%.3f", data[3]);
	SetDlgItemText(IDC_ACCEL_Y, s);
	s.Format("%.3f", data[4]);
	SetDlgItemText(IDC_ACCEL_Z, s);

	s.Format("%.3f", data[5]);
	SetDlgItemText(IDC_ANGULAR_X, s);
	s.Format("%.3f", data[6]);
	SetDlgItemText(IDC_ANGULAR_Y, s);
	s.Format("%.3f", data[7]);
	SetDlgItemText(IDC_ANGULAR_Z, s);

	CDialog::OnTimer(nIDEvent);
}

extern void ListupPort(CComboBox& combobox);
static CString PortName(CWnd* pWnd)
{
	CComboBox* pCombo = reinterpret_cast<CComboBox*>(pWnd);
	CString s;
	int index = pCombo->GetCurSel();
	if (0 <= index) {
		pCombo->GetLBText(index, s);
	}
	return s;
}
void CTestDrillMonitorDlg::OnCbnDropdownPrimado2PortList()
{
	ListupPort(*reinterpret_cast<CComboBox*>(GetDlgItem(IDC_PRIMADO2_PORT_LIST)));
}
void CTestDrillMonitorDlg::OnCbnDropdownTsnd121PortList()
{
	ListupPort(*reinterpret_cast<CComboBox*>(GetDlgItem(IDC_TSND121_PORT_LIST)));
}

void CTestDrillMonitorDlg::OnBnClickedStart()
{
	if (m_pDrillMonitor == NULL) {
		return;
	}
	m_pDrillMonitor->Stop();
	CString spindle = PortName(GetDlgItem(IDC_PRIMADO2_PORT_LIST));
	CString tsnd121 = PortName(GetDlgItem(IDC_TSND121_PORT_LIST));
	m_pDrillMonitor->Start(spindle, tsnd121);
}
