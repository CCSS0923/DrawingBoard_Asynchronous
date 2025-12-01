// 20250920_DrawingBoard.cpp : Win32 API를 이용한 그림판 프로그램
//
#include "framework.h"
#include "20250920_DrawingBoard.h"
#include <windows.h>
#include <vector>

#ifndef ID_SHAPE_LINE
#define ID_SHAPE_LINE     40001
#endif
#ifndef ID_SHAPE_RECT
#define ID_SHAPE_RECT     40002
#endif
#ifndef ID_SHAPE_ELLIPSE
#define ID_SHAPE_ELLIPSE  40003
#endif
#ifndef ID_SHAPE_FREE
#define ID_SHAPE_FREE     40004
#endif

#ifndef ID_THICKNESS_1
#define ID_THICKNESS_1    40101
#endif
#ifndef ID_THICKNESS_5
#define ID_THICKNESS_5    40102
#endif
#ifndef ID_THICKNESS_10
#define ID_THICKNESS_10   40103
#endif

#ifndef ID_COLOR_BLACK
#define ID_COLOR_BLACK    40201
#endif
#ifndef ID_COLOR_WHITE
#define ID_COLOR_WHITE    40202
#endif
#ifndef ID_COLOR_RED
#define ID_COLOR_RED      40203
#endif

HINSTANCE hInst;
WCHAR szTitle[100] = L"그림판";
WCHAR szWindowClass[100] = L"DrawingBoardClass";

enum SHAPE_TYPE { SHAPE_NONE, SHAPE_LINE, SHAPE_RECT, SHAPE_ELLIPSE, SHAPE_FREE };

struct DrawAttr {
    int thickness;
    COLORREF color;
};

class Shape {
public:
    DrawAttr attr;
    Shape(const DrawAttr& a) : attr(a) {}
    virtual ~Shape() {}
    virtual void Draw(HDC hdc) = 0;
};

class Line : public Shape {
public:
    POINT start, end;
    Line(POINT s, POINT e, DrawAttr a) : Shape(a), start(s), end(e) {}
    void Draw(HDC hdc) override {
        HPEN pen = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, start.x, start.y, NULL);
        LineTo(hdc, end.x, end.y);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
};

class RectShape : public Shape {
public:
    POINT start, end;
    RectShape(POINT s, POINT e, DrawAttr a) : Shape(a), start(s), end(e) {}
    void Draw(HDC hdc) override {
        HPEN pen = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HBRUSH brush = CreateSolidBrush(attr.color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        Rectangle(hdc, start.x, start.y, end.x, end.y);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }
};

class EllipseShape : public Shape {
public:
    POINT start, end;
    EllipseShape(POINT s, POINT e, DrawAttr a) : Shape(a), start(s), end(e) {}
    void Draw(HDC hdc) override {
        HPEN pen = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HBRUSH brush = CreateSolidBrush(attr.color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        Ellipse(hdc, start.x, start.y, end.x, end.y);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }
};

class FreeLine : public Shape {
public:
    std::vector<POINT> pts;
    FreeLine(DrawAttr a) : Shape(a) {}
    void Draw(HDC hdc) override {
        if (pts.size() < 2)
            return;
        HPEN pen = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, pts[0].x, pts[0].y, NULL);
        for (size_t i = 1; i < pts.size(); i++) {
            LineTo(hdc, pts[i].x, pts[i].y);
        }
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
};

std::vector<Shape*> shapes;
SHAPE_TYPE g_shapeFlag = SHAPE_NONE;
DrawAttr g_curAttr = { 1, RGB(0, 0, 0) };
bool g_bDrawing = false;
POINT g_startPt;
FreeLine* g_curFreeLine = nullptr;

ATOM MyRegisterClass(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void CreateAppMenu(HWND);
void DrawAll(HDC);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (auto p : shapes) delete p;
    if (g_curFreeLine) delete g_curFreeLine;
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    if (!hWnd) return FALSE;

    CreateAppMenu(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

void CreateAppMenu(HWND hWnd) {
    HMENU hMenu = CreateMenu();
    HMENU drawMenu = CreateMenu();
    AppendMenu(drawMenu, MF_STRING, ID_SHAPE_LINE, L"직선");
    AppendMenu(drawMenu, MF_STRING, ID_SHAPE_RECT, L"사각형");
    AppendMenu(drawMenu, MF_STRING, ID_SHAPE_ELLIPSE, L"타원");
    AppendMenu(drawMenu, MF_STRING, ID_SHAPE_FREE, L"자유선");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)drawMenu, L"그리기");

    HMENU thickMenu = CreateMenu();
    AppendMenu(thickMenu, MF_STRING, ID_THICKNESS_1, L"1px");
    AppendMenu(thickMenu, MF_STRING, ID_THICKNESS_5, L"5px");
    AppendMenu(thickMenu, MF_STRING, ID_THICKNESS_10, L"10px");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)thickMenu, L"두께");

    HMENU colorMenu = CreateMenu();
    AppendMenu(colorMenu, MF_STRING, ID_COLOR_BLACK, L"검은색");
    AppendMenu(colorMenu, MF_STRING, ID_COLOR_WHITE, L"하얀색");
    AppendMenu(colorMenu, MF_STRING, ID_COLOR_RED, L"다홍색");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)colorMenu, L"색상");

    SetMenu(hWnd, hMenu);
}

void DrawAll(HDC hdc) {
    RECT rc;
    GetClientRect(WindowFromDC(hdc), &rc);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
    HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(memDC, &rc, hbr);
    DeleteObject(hbr);
    for (auto p : shapes) p->Draw(memDC);
    if (g_bDrawing && g_shapeFlag == SHAPE_FREE && g_curFreeLine)
        g_curFreeLine->Draw(memDC);
    BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static POINT ptCurrent = { 0, 0 };
    switch (message) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_SHAPE_LINE:
            g_shapeFlag = SHAPE_LINE;
            break;
        case ID_SHAPE_RECT:
            g_shapeFlag = SHAPE_RECT;
            break;
        case ID_SHAPE_ELLIPSE:
            g_shapeFlag = SHAPE_ELLIPSE;
            break;
        case ID_SHAPE_FREE:
            g_shapeFlag = SHAPE_FREE;
            break;
        case ID_THICKNESS_1:
            g_curAttr.thickness = 1;
            break;
        case ID_THICKNESS_5:
            g_curAttr.thickness = 5;
            break;
        case ID_THICKNESS_10:
            g_curAttr.thickness = 10;
            break;
        case ID_COLOR_BLACK:
            g_curAttr.color = RGB(0, 0, 0);
            break;
        case ID_COLOR_WHITE:
            g_curAttr.color = RGB(255, 255, 255);
            break;
        case ID_COLOR_RED:
            g_curAttr.color = RGB(200, 100, 100);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    }
    case WM_LBUTTONDOWN:
        if (g_shapeFlag == SHAPE_NONE)
            break;
        g_bDrawing = true;
        g_startPt.x = LOWORD(lParam);
        g_startPt.y = HIWORD(lParam);
        ptCurrent = g_startPt;
        if (g_shapeFlag == SHAPE_FREE) {
            if (g_curFreeLine)
                delete g_curFreeLine;
            g_curFreeLine = new FreeLine(g_curAttr);
            g_curFreeLine->pts.push_back(g_startPt);
        }
        SetCapture(hWnd);
        break;
    case WM_MOUSEMOVE:
        if (g_bDrawing && g_shapeFlag == SHAPE_FREE && (wParam & MK_LBUTTON)) {
            POINT pt = { (LONG)LOWORD(lParam), (LONG)HIWORD(lParam) };
            g_curFreeLine->pts.push_back(pt);
            ptCurrent = pt;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_LBUTTONUP: {
        if (!g_bDrawing)
            break;
        g_bDrawing = false;
        ReleaseCapture();
        POINT ptEnd = { (LONG)LOWORD(lParam), (LONG)HIWORD(lParam) };
        Shape* shape = nullptr;
        if (g_shapeFlag == SHAPE_LINE)
            shape = new Line(g_startPt, ptEnd, g_curAttr);
        else if (g_shapeFlag == SHAPE_RECT)
            shape = new RectShape(g_startPt, ptEnd, g_curAttr);
        else if (g_shapeFlag == SHAPE_ELLIPSE)
            shape = new EllipseShape(g_startPt, ptEnd, g_curAttr);
        else if (g_shapeFlag == SHAPE_FREE) {
            shape = g_curFreeLine;
            g_curFreeLine = nullptr;
        }
        if (shape)
            shapes.push_back(shape);
        InvalidateRect(hWnd, NULL, TRUE);
    } break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawAll(hdc);
        EndPaint(hWnd, &ps);
    } break;
    case WM_ERASEBKGND:
        // 배경 자동 지우기 방지 (깜빡임 최소화)
        return 1;
    case WM_DESTROY:
        for (auto p : shapes)
            delete p;
        shapes.clear();
        if (g_curFreeLine)
            delete g_curFreeLine;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
