#pragma once

#include <vector>
#include <random>
#include <afxwin.h>     // MFC CWnd, CDC, etc.

class CCanvasWnd : public CWnd
{
public:
    CCanvasWnd() = default;
    virtual ~CCanvasWnd() = default;

    // �ʱ�ȭ/�Ķ����
    BOOL SubclassDlgItem(UINT nID, CWnd* pParent);
    void SetParams(int clickCircleRadius, int outerStrokeThickness);
    void ResetAll();

    // ���� ���
    std::vector<CPoint> GetPoints();

    // ���ͷ��� ����
    std::vector<CPoint> m_pts;
    bool  m_allowPlace = true;
    int   m_dragIdx = -1;

protected:
    // ���� ����������
    void RenderAll(CDC* pDC);
    bool EnsureDIB(int w, int h, CDC* pDC);
    void Clear(BYTE r, BYTE g, BYTE b);
    void Blit(CDC* pDC);

    // ������ �ȼ�/����
    void PutPixel(int x, int y, COLORREF c);
    void HLine(int x1, int x2, int y, COLORREF c);

    // ��ĵ���� ��� �� �׸���(ä��/��)
    void DrawFilledCircle(int cx, int cy, int r, COLORREF c);
    void DrawCircleOutlineThick(int cx, int cy, int r, int outlineThickness, COLORREF c);

    // ���� ��ƿ (�� ���� ������)
    bool ComputeCircumcircle(POINT p1, POINT p2, POINT p3, double& cx, double& cy, double& r);

    // �޽��� �ڵ鷯
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
    // ĵ����(������ۿ� DIB)
    CBitmap m_dib;
    BYTE* m_bits = nullptr;
    int     m_w = 0, m_h = 0, m_pitch = 0; // pitch = m_w*4 (32bpp)

    // �׸��� �Ķ����
    int m_clickRadius = 6;
    int m_outlineThickness = 2;

    // RNG
    std::mt19937 m_rng{ std::random_device{}() };
};
