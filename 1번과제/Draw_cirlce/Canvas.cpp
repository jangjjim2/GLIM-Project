#include "pch.h"
#include "Canvas.h"
#include <algorithm>
#include <cmath>
#include "AppWideMsgs.h"   // 프로젝트 전역 메시지(WM_CANVAS_RANDOM_STEP 등)

// ====== 메시지 맵 ======
BEGIN_MESSAGE_MAP(CCanvasWnd, CWnd)
    ON_WM_PAINT()                                // WM_PAINT 처리
    ON_WM_ERASEBKGND()                           // 배경 지우기 처리
    ON_WM_LBUTTONDOWN()                          // 마우스 왼쪽 버튼 눌렀을 때
    ON_WM_LBUTTONUP()                            // 마우스 왼쪽 버튼 뗐을 때
    ON_WM_MOUSEMOVE()                            // 마우스 이동
    ON_WM_SIZE()                                 // 윈도우 크기 변경
    ON_MESSAGE(WM_CANVAS_RANDOM_STEP, OnRandomStep) // 랜덤 위치 재배치
    ON_MESSAGE(WM_CANVAS_RESET, OnReset)            // 초기화
END_MESSAGE_MAP()

// ====== 초기화/파라미터 ======

// 다이얼로그 안의 static control(IDC_CANVAS)을 이 윈도우 클래스로 서브클래싱
BOOL CCanvasWnd::SubclassDlgItem(UINT nID, CWnd* pParent)
{
    BOOL ok = CWnd::SubclassDlgItem(nID, pParent);
    m_pts.clear();       // 점 초기화
    m_allowPlace = true; // 점 추가 가능 상태로
    m_dragIdx = -1;      // 드래그 중인 점 없음
    return ok;
}

// 점 반지름, 외곽선 두께 설정
void CCanvasWnd::SetParams(int clickCircleRadius, int outerStrokeThickness)
{
    m_clickRadius = std::max(1, clickCircleRadius);
    m_outlineThickness = std::max(1, outerStrokeThickness);
    Invalidate(FALSE);   // 다시 그리기 요청
}

// 전체 초기화
void CCanvasWnd::ResetAll() {
    m_pts.clear();       // 점 목록 비움
    m_allowPlace = true;
    m_dragIdx = -1;
    Invalidate(FALSE);
}

// 현재 점 목록 반환
std::vector<CPoint> CCanvasWnd::GetPoints() {
    return m_pts;
}

// ====== DIB 준비/렌더 ======

// 화면 크기에 맞는 32bpp top-down DIB 생성
bool CCanvasWnd::EnsureDIB(int w, int h, CDC* pDC) {
    // 크기가 같고 버퍼 유효하면 그대로 사용
    if (w == m_w && h == m_h && m_bits) return true;

    // 기존 DIB 있으면 해제
    if (m_dib.GetSafeHandle()) {
        m_dib.DeleteObject();
        m_bits = nullptr;
    }
    m_w = w;
    m_h = h;

    // DIB 섹션 정보 구조체 초기화
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = m_w;
    bi.bmiHeader.biHeight = -m_h;  // top-down 방식 (0행이 위쪽)
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;  // 32bpp BGRA
    bi.bmiHeader.biCompression = BI_RGB;

    // DIB 섹션 생성
    void* bits = nullptr;
    HBITMAP hbmp = CreateDIBSection(pDC ? pDC->GetSafeHdc() : nullptr, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!hbmp || !bits) {
        m_bits = nullptr;
        m_pitch = 0;
        m_w = m_h = 0;
        ASSERT(FALSE && "CreateDIBSection failed");
        return false;
    }

    // CBitmap 객체에 HBITMAP 붙이기
    m_dib.Attach(hbmp);
    // 픽셀 데이터 시작 주소 저장
    m_bits = static_cast<BYTE*>(bits);
    // 한 행 바이트 수 (32bpp → width*4)
    m_pitch = m_w * 4;
    return true;
}

// 화면 지우기 (r,g,b 값으로 배경 채움)
void CCanvasWnd::Clear(BYTE r, BYTE g, BYTE b) {
    if (!m_bits || m_w <= 0 || m_h <= 0 || m_pitch <= 0) return;
    for (int y = 0; y < m_h; ++y) {
        BYTE* row = m_bits + y * m_pitch;
        for (int x = 0; x < m_w; ++x) {
            BYTE* p = row + x * 4;
            p[0] = b; p[1] = g; p[2] = r; p[3] = 0xFF;
        }
    }
}

// 준비된 DIB → 실제 화면 DC로 복사
void CCanvasWnd::Blit(CDC* pDC) {
    if (!pDC || !m_dib.GetSafeHandle()) return;
    CDC mem;
    mem.CreateCompatibleDC(pDC);
    auto old = mem.SelectObject(&m_dib);
    if (old) {
        pDC->BitBlt(0, 0, m_w, m_h, &mem, 0, 0, SRCCOPY);
        mem.SelectObject(old);
    }
}

// ====== 저수준 픽셀/도형 ======

// 한 픽셀 찍기
void CCanvasWnd::PutPixel(int x, int y, COLORREF c) {
    if (!m_bits) return;
    if (x < 0 || y < 0 || x >= m_w || y >= m_h) return;
    BYTE* p = m_bits + y * m_pitch + x * 4;
    p[0] = GetBValue(c); p[1] = GetGValue(c); p[2] = GetRValue(c); p[3] = 0xFF;
}

// 수평선 그리기
void CCanvasWnd::HLine(int x1, int x2, int y, COLORREF c) {
    if (!m_bits) return;
    if (y < 0 || y >= m_h) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = std::max(0, x1);
    x2 = std::min(m_w - 1, x2);

    BYTE* row = m_bits + y * m_pitch;
    for (int x = x1; x <= x2; ++x) {
        BYTE* p = row + x * 4;
        p[0] = GetBValue(c);
        p[1] = GetGValue(c);
        p[2] = GetRValue(c);
        p[3] = 0xFF;
    }
}

// ====== 스캔라인 기반 원 그리기 ======

// 채워진 원 그리기
void CCanvasWnd::DrawFilledCircle(int cx, int cy, int r, COLORREF c) {
    if (!m_bits || r <= 0) return;

    const int r2 = r * r;
    const int y0 = std::max(0, cy - r);
    const int y1 = std::min(m_h - 1, cy + r);

    for (int py = y0; py <= y1; ++py) {
        const int dy = py - cy;
        const int dy2 = dy * dy;
        int dx2 = r2 - dy2;
        if (dx2 < 0) continue;

        int dx = (int)std::floor(std::sqrt((double)dx2));
        int sx = std::max(0, cx - dx);
        int ex = std::min(m_w - 1, cx + dx);

        HLine(sx, ex, py, c);
    }
}

// 두께가 있는 외곽 원 그리기 (링 모양)
void CCanvasWnd::DrawCircleOutlineThick(int cx, int cy, int r, int outlineThickness, COLORREF c) {
    if (!m_bits || r <= 0 || outlineThickness <= 0) return;

    const int rOut = r + (outlineThickness - 1) / 2;
    const int rIn = std::max(0, r - (outlineThickness / 2));
    const int rOut2 = rOut * rOut;
    const int rIn2 = rIn * rIn;

    const int y0 = std::max(0, cy - rOut);
    const int y1 = std::min(m_h - 1, cy + rOut);

    for (int py = y0; py <= y1; ++py) {
        const int dy = py - cy;
        const int dy2 = dy * dy;

        int dxOut2 = rOut2 - dy2;
        if (dxOut2 < 0) continue;
        int dxOut = (int)std::floor(std::sqrt((double)dxOut2));
        int sx = std::max(0, cx - dxOut);
        int ex = std::min(m_w - 1, cx + dxOut);

        if (rIn <= 0) {
            HLine(sx, ex, py, c);
            continue;
        }

        int dxIn2 = rIn2 - dy2;
        if (dxIn2 <= 0) {
            HLine(sx, ex, py, c);
            continue;
        }
        int dxIn = (int)std::floor(std::sqrt((double)dxIn2));
        int sxIn = std::max(0, cx - dxIn);
        int exIn = std::min(m_w - 1, cx + dxIn);

        if (sx < sxIn) HLine(sx, sxIn - 1, py, c);
        if (exIn + 1 <= ex) HLine(exIn + 1, ex, py, c);
    }
}

// ====== 외접원 계산 ======
// 세 점을 지나는 원(외접원) 계산: 크래머 공식 사용
bool CCanvasWnd::ComputeCircumcircle(POINT p1, POINT p2, POINT p3, double& cx, double& cy, double& r) {
    auto det3 = [](double a11, double a12, double a13,
        double a21, double a22, double a23,
        double a31, double a32, double a33)->double {
            return a11 * (a22 * a33 - a23 * a32)
                - a12 * (a21 * a33 - a23 * a31)
                + a13 * (a21 * a32 - a22 * a31);
        };

    const double x1 = p1.x, y1 = p1.y;
    const double x2 = p2.x, y2 = p2.y;
    const double x3 = p3.x, y3 = p3.y;

    const double s1 = x1 * x1 + y1 * y1;
    const double s2 = x2 * x2 + y2 * y2;
    const double s3 = x3 * x3 + y3 * y3;

    const double Delta = det3(x1, y1, 1, x2, y2, 1, x3, y3, 1);
    if (std::abs(Delta) <= 0.0) return false; // 세 점이 거의 일직선이면 실패

    const double Dx = det3(s1, y1, 1, s2, y2, 1, s3, y3, 1);
    const double Dy = det3(x1, s1, 1, x2, s2, 1, x3, s3, 1);
    const double Dc = det3(x1, y1, s1, x2, y2, s2, x3, y3, s3);

    cx = 0.5 * (Dx / Delta);
    cy = 0.5 * (Dy / Delta);

    const double r2 = (Dx * Dx + Dy * Dy) / (4.0 * Delta * Delta) + (Dc / Delta);
    if (r2 <= 0.0) return false;
    r = std::sqrt(r2);
    return true;
}

// ====== 렌더/이벤트 ======

// 전체 렌더링
void CCanvasWnd::RenderAll(CDC* pDC) {
    if (!::IsWindow(m_hWnd) || !pDC) return;

    CRect rc; GetClientRect(&rc);
    if (!EnsureDIB(rc.Width(), rc.Height(), pDC)) return;

    Clear(255, 255, 255); // 배경 흰색

    std::vector<CPoint> points = m_pts;
    const int pointRadius = m_clickRadius;
    const int outlineThickness = m_outlineThickness;

    COLORREF pointOutline = RGB(0, 0, 0);
    COLORREF pointFill = RGB(0, 0, 0);
    COLORREF outerCol = RGB(0, 0, 0);

    // 각 클릭된 점을 작은 원으로 표시
    for (size_t i = 0; i < points.size() && i < 3; ++i) {
        DrawFilledCircle(points[i].x, points[i].y, (std::max)(1, pointRadius - 1), pointFill);
        DrawCircleOutlineThick(points[i].x, points[i].y, pointRadius, 2, pointOutline);
    }

    // 세 점이 있으면 외접원 그리기
    if (points.size() == 3) {
        double cx, cy, r;
        if (ComputeCircumcircle(points[0], points[1], points[2], cx, cy, r)) {
            DrawCircleOutlineThick((int)std::lround(cx), (int)std::lround(cy),
                (int)std::lround(r), outlineThickness, outerCol);
        }
    }

    Blit(pDC); // 최종 화면에 출력
}

// WM_PAINT 메시지 처리
void CCanvasWnd::OnPaint() {
    CPaintDC dc(this);
    RenderAll(&dc);
}

// 배경 지우기를 막아서 깜빡임 방지
BOOL CCanvasWnd::OnEraseBkgnd(CDC* /*pDC*/) {
    return TRUE;
}

// 윈도우 크기 변경 시 다시 그리기 요청
void CCanvasWnd::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    Invalidate(FALSE);
}

// 마우스 왼쪽 버튼 누르면 점 추가 또는 드래그 시작
void CCanvasWnd::OnLButtonDown(UINT nFlags, CPoint pt) {
    (void)nFlags;
    SetCapture();

    // 기존 점 근처 클릭 시 → 드래그로 처리
    for (int i = (int)m_pts.size() - 1; i >= 0; --i) {
        const CPoint p = m_pts[i];
        const int dx = pt.x - p.x, dy = pt.y - p.y;
        if (dx * dx + dy * dy <= (m_clickRadius + 4) * (m_clickRadius + 4)) {
            m_dragIdx = i;
            return;
        }
    }
    // 새 점 추가 (최대 3개)
    if (m_allowPlace && (int)m_pts.size() < 3) {
        m_pts.push_back(pt);
        if ((int)m_pts.size() == 3) m_allowPlace = false;
        Invalidate(FALSE);
    }
}

// 마우스 버튼 떼면 드래그 종료
void CCanvasWnd::OnLButtonUp(UINT nFlags, CPoint pt) {
    (void)nFlags;
    if (GetCapture() == this) ReleaseCapture();

    if (m_dragIdx >= 0) {
        m_pts[m_dragIdx] = pt;
        m_dragIdx = -1;
        Invalidate(FALSE);
    }
}

// 드래그 중 마우스 이동 → 점 위치 갱신
void CCanvasWnd::OnMouseMove(UINT nFlags, CPoint pt) {
    if ((nFlags & MK_LBUTTON) && m_dragIdx >= 0) {
        m_pts[m_dragIdx] = pt;
        Invalidate(FALSE);
    }
}

// 세 점이 거의 일직선인지 판정
static bool NearlyCollinear(int w, int h, const POINT& a, const POINT& b, const POINT& c) {
    const long long x1 = (long long)b.x - a.x, y1 = (long long)b.y - a.y;
    const long long x2 = (long long)c.x - a.x, y2 = (long long)c.y - a.y;
    const long long cross = x1 * y2 - y1 * x2; // 외적
    const double diag = std::hypot((double)w, (double)h);
    const double tol = diag; // 허용치
    return std::llabs(cross) < tol;
}

// 랜덤 위치 재배치 메시지 처리
LRESULT CCanvasWnd::OnRandomStep(WPARAM, LPARAM) {
    CRect rc; GetClientRect(&rc);

    if (m_pts.size() == 3) {
        const int margin = 5;
        std::uniform_int_distribution<int> ux(rc.left + margin, rc.right - margin);
        std::uniform_int_distribution<int> uy(rc.top + margin, rc.bottom - margin);

        constexpr int kMaxTries = 128;
        for (int t = 0; t < kMaxTries; ++t) {
            POINT a{ ux(m_rng), uy(m_rng) };
            POINT b{ ux(m_rng), uy(m_rng) };
            POINT c{ ux(m_rng), uy(m_rng) };
            if (!NearlyCollinear(m_w, m_h, a, b, c)) {
                m_pts[0] = a; 
                m_pts[1] = b; 
                m_pts[2] = c;
                Invalidate(FALSE);
                break;
            }
        }
    }
    return 0;
}

// 초기화 메시지 처리
LRESULT CCanvasWnd::OnReset(WPARAM, LPARAM) {
    ResetAll();
    return 0;
}
