#pragma once

#include <atomic>
#include <afxwin.h>
#include "Canvas.h"        // CCanvasWnd
#include "resource.h"      // IDC/IDD 등
#include "AppWideMsgs.h"   // WM_CANVAS_RANDOM_STEP, WM_CANVAS_RESET 등

class CDrawcircleDlg : public CDialogEx
{
public:
    CDrawcircleDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DRAW_CIRLCE_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    // 메시지 맵
    DECLARE_MESSAGE_MAP()

    // 핸들러
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedBtnReset();
    afx_msg void OnBnClickedBtnRandom();
    afx_msg void OnEnChangeRadius();
    afx_msg void OnEnChangeThick();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();

    // 랜덤 애니메이션 완료 통지용 사용자 메시지
    enum { WM_RANDOM_DONE = WM_APP + 200 };
    afx_msg LRESULT OnRandomDone(WPARAM, LPARAM);

    // 워커 스레드 프로시저 (static)
    static UINT __cdecl OnBnClickedBtnRandomThread(LPVOID pParam);

private:
    // 아이콘
    HICON m_hIcon = nullptr;

public:
    // 캔버스 및 컨트롤들
    CCanvasWnd m_canvas;
    CStatic    m_lblP1, m_lblP2, m_lblP3;
    CEdit      m_editRadius, m_editThick;
    CButton    m_btnReset, m_btnRandom;

    // 랜덤 스레드 실행 상태
    std::atomic_bool m_randomRunning{ false };
    CWinThread* m_pRandomThread = nullptr;
};
