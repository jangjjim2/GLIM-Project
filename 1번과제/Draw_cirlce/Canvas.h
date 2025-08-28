#pragma once

#include <vector>
#include <random>
#include <afxwin.h>     // MFC CWnd, CDC, etc.

class CCanvasWnd : public CWnd
{
public:
    CCanvasWnd() = default;
    virtual ~CCanvasWnd() = default;

    // 초기화/파라미터
    BOOL SubclassDlgItem(UINT nID, CWnd* pParent);
    void SetParams(int clickCircleRadius, int outerStrokeThickness);
    void ResetAll();

    // 상태 얻기
    std::vector<CPoint> GetPoints();

    // 인터랙션 상태
    std::vector<CPoint> m_pts;
    bool  m_allowPlace = true;
    int   m_dragIdx = -1;

protected:
    // 렌더 파이프라인
    void RenderAll(CDC* pDC);
    bool EnsureDIB(int w, int h, CDC* pDC);
    void Clear(BYTE r, BYTE g, BYTE b);
    void Blit(CDC* pDC);

    // 저수준 픽셀/도형
    void PutPixel(int x, int y, COLORREF c);
    void HLine(int x1, int x2, int y, COLORREF c);

    // 스캔라인 기반 원 그리기(채움/링)
    void DrawFilledCircle(int cx, int cy, int r, COLORREF c);
    void DrawCircleOutlineThick(int cx, int cy, int r, int outlineThickness, COLORREF c);

    // 기하 유틸 (세 점의 외접원)
    bool ComputeCircumcircle(POINT p1, POINT p2, POINT p3, double& cx, double& cy, double& r);

    // 메시지 핸들러
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* /*pDC*/);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);

    afx_msg LRESULT OnRandomStep(WPARAM, LPARAM);
    afx_msg LRESULT OnReset(WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

private:
    // 캔버스(더블버퍼용 DIB)
    CBitmap m_dib;
    BYTE* m_bits = nullptr;
    int     m_w = 0, m_h = 0, m_pitch = 0; // pitch = m_w*4 (32bpp)

    // 그리기 파라미터
    int m_clickRadius = 6;
    int m_outlineThickness = 2;

    // RNG
    std::mt19937 m_rng{ std::random_device{}() };
};
