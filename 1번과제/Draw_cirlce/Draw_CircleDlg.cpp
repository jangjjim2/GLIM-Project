// Draw_cirlceDlg.cpp: 구현 파일

#include "pch.h"
#include "framework.h"
#include "Draw_Circle.h"
#include "Draw_CircleDlg.h"
#include "afxdialogex.h"

#include <algorithm> // std::max
#include <random>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ----- About 대화상자 -----
class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    DECLARE_MESSAGE_MAP()
};
CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) { 
    CDialogEx::DoDataExchange(pDX); 
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)

END_MESSAGE_MAP()

// ----- 메인 대화상자 -----
CDrawcircleDlg::CDrawcircleDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DRAW_CIRLCE_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDrawcircleDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDrawcircleDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_RESET, &CDrawcircleDlg::OnBnClickedBtnReset)
    ON_BN_CLICKED(IDC_BTN_RANDOM, &CDrawcircleDlg::OnBnClickedBtnRandom)
    ON_EN_CHANGE(IDC_EDIT_RADIUS, &CDrawcircleDlg::OnEnChangeRadius)
    ON_EN_CHANGE(IDC_EDIT_THICK, &CDrawcircleDlg::OnEnChangeThick)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_RANDOM_DONE, &CDrawcircleDlg::OnRandomDone)
END_MESSAGE_MAP()

BOOL CDrawcircleDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // ----- 시스템 메뉴 "정보..." 항목 추가(기본 템플릿) -----
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    if (CMenu* pSysMenu = GetSystemMenu(FALSE)) {
        CString strAboutMenu; BOOL b = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(b);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // 아이콘 설정
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // ----- 컨트롤 서브클래싱 -----
    BOOL ok = TRUE;
    ok &= m_canvas.SubclassDlgItem(IDC_CANVAS, this);
    ok &= m_lblP1.SubclassDlgItem(IDC_LBL_P1, this);
    ok &= m_lblP2.SubclassDlgItem(IDC_LBL_P2, this);
    ok &= m_lblP3.SubclassDlgItem(IDC_LBL_P3, this);
    ok &= m_editRadius.SubclassDlgItem(IDC_EDIT_RADIUS, this);
    ok &= m_editThick.SubclassDlgItem(IDC_EDIT_THICK, this);
    ok &= m_btnReset.SubclassDlgItem(IDC_BTN_RESET, this);
    ok &= m_btnRandom.SubclassDlgItem(IDC_BTN_RANDOM, this);
    ASSERT(ok);

    m_canvas.ModifyStyle(0, SS_NOTIFY);
    m_canvas.EnableWindow(TRUE);
    m_canvas.ShowWindow(SW_SHOW);

    // ----- 초기값/표시 -----
    m_editRadius.SetWindowTextW(L"8");
    m_editThick.SetWindowTextW(L"3");
    m_canvas.SetParams(8, 3);

    // 좌표 라벨 초기 표시
    m_lblP1.SetWindowTextW(L"(-,-)");
    m_lblP2.SetWindowTextW(L"(-,-)");
    m_lblP3.SetWindowTextW(L"(-,-)");

    // 라벨 업데이트용 타이머
    SetTimer(1, 100, nullptr);

    return TRUE;
}

void CDrawcircleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout; dlgAbout.DoModal();
    }
    else {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

void CDrawcircleDlg::OnPaint() {
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect; GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CDrawcircleDlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}

// ---------------- 랜덤 애니메이션 스레드 ----------------

// 버튼 클릭 → 중복 실행 방지, 버튼 비활성화, 스레드 시작
void CDrawcircleDlg::OnBnClickedBtnRandom() {
    if (m_canvas.m_pts.size() < 3) return;

    // 이미 실행 중이면 무시
    if (m_randomRunning.load()) return;

    m_randomRunning.store(true);
    if (CWnd* p = GetDlgItem(IDC_BTN_RANDOM)) p->EnableWindow(FALSE);

    // 워커 스레드 시작
    m_pRandomThread = AfxBeginThread(&CDrawcircleDlg::OnBnClickedBtnRandomThread, this);
}

// 워커 스레드 본체: 캔버스에 랜덤 이동 메시지를 10 회 0.5초 간격으로 Post
UINT __cdecl CDrawcircleDlg::OnBnClickedBtnRandomThread(LPVOID pParam)
{
    auto* dlg = static_cast<CDrawcircleDlg*>(pParam);
    
    for (int i = 0; i < 10 && dlg->m_randomRunning.load(); ++i) {
        if (dlg->m_canvas.GetSafeHwnd()) {
            dlg->m_canvas.PostMessage(WM_CANVAS_RANDOM_STEP, 0, 0);
        }
        ::Sleep(500);
    }
    ::PostMessage(dlg->GetSafeHwnd(), WM_RANDOM_DONE, 0, 0);
    return 0;
}

// 완료 메시지 → 플래그 해제, 버튼 재활성화
LRESULT CDrawcircleDlg::OnRandomDone(WPARAM, LPARAM) {
    m_randomRunning.store(false);
    m_pRandomThread = nullptr;
    if (CWnd* p = GetDlgItem(IDC_BTN_RANDOM)) p->EnableWindow(TRUE);
    return 0;
}

// ---------------- 기타 컨트롤 핸들러 ----------------

void CDrawcircleDlg::OnBnClickedBtnReset() {
    // 애니메이션 도중 Reset 눌렀으면 중단
    m_randomRunning.store(false);
    m_canvas.PostMessage(WM_CANVAS_RESET, 0, 0);
}

void CDrawcircleDlg::OnEnChangeRadius() {
    CString rs, ts;
    m_editRadius.GetWindowTextW(rs);
    m_editThick.GetWindowTextW(ts);
    int r = _wtoi(rs);
    int th = _wtoi(ts);
    m_canvas.SetParams((std::max)(1, r), (std::max)(1, th));
}

void CDrawcircleDlg::OnEnChangeThick() {
    OnEnChangeRadius();
}

void CDrawcircleDlg::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == 1) {
        auto pts = m_canvas.GetPoints();
        auto fmt = [](const CPoint& p) {
            CString s; s.Format(L"(%d, %d)", p.x, p.y); 
            return s;
        };
        auto safeSet = [](CStatic& h, const CString& t) {
            if (h.GetSafeHwnd()) h.SetWindowTextW(t);
        };

        safeSet(m_lblP1, pts.size() > 0 ? fmt(pts[0]) : L"(-,-)");
        safeSet(m_lblP2, pts.size() > 1 ? fmt(pts[1]) : L"(-,-)");
        safeSet(m_lblP3, pts.size() > 2 ? fmt(pts[2]) : L"(-,-)");
    }
    CDialogEx::OnTimer(nIDEvent);
}

void CDrawcircleDlg::OnDestroy() {
    CDialogEx::OnDestroy();

    // 안전 종료
    m_randomRunning.store(false);
    if (m_pRandomThread) {
        WaitForSingleObject(m_pRandomThread->m_hThread, 500);
        m_pRandomThread = nullptr;
    }
}
