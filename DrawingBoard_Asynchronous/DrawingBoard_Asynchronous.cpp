#include "framework.h"
#include "DrawingBoard_Asynchronous.h"
#include "resource.h"
#include <vector>

HINSTANCE hInst;
WCHAR szTitle[100];
WCHAR szWindowClass[100];

HWND gWnd[2];
int gWndCount = 0;

enum SHAPE_TYPE { SHAPE_NONE, SHAPE_LINE, SHAPE_RECT, SHAPE_ELLIPSE, SHAPE_FREE };
struct DrawAttr { int thickness; COLORREF color; };

bool gTransparent = true;

class Shape {
public:
    DrawAttr attr;
    bool transparent;
    Shape(const DrawAttr& a, bool t) : attr(a), transparent(t) {}
    virtual ~Shape() {}
    virtual void Draw(HDC hdc) = 0;
};

class Line : public Shape {
public:
    POINT s, e;
    Line(POINT a, POINT b, DrawAttr d, bool t) : Shape(d, t), s(a), e(b) {}
    void Draw(HDC hdc) override {
        HPEN p = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HPEN op = (HPEN)SelectObject(hdc, p);
        MoveToEx(hdc, s.x, s.y, NULL);
        LineTo(hdc, e.x, e.y);
        SelectObject(hdc, op);
        DeleteObject(p);
    }
};

class RectShape : public Shape {
public:
    POINT s, e;
    RectShape(POINT a, POINT b, DrawAttr d, bool t) : Shape(d, t), s(a), e(b) {}
    void Draw(HDC hdc) override {
        HPEN p = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HBRUSH b = transparent ? (HBRUSH)GetStockObject(NULL_BRUSH)
            : CreateSolidBrush(attr.color);
        HPEN op = (HPEN)SelectObject(hdc, p);
        HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
        Rectangle(hdc, s.x, s.y, e.x, e.y);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        if (!transparent) DeleteObject(b);
        DeleteObject(p);
    }
};

class EllipseShape : public Shape {
public:
    POINT s, e;
    EllipseShape(POINT a, POINT b, DrawAttr d, bool t) : Shape(d, t), s(a), e(b) {}
    void Draw(HDC hdc) override {
        HPEN p = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HBRUSH b = transparent ? (HBRUSH)GetStockObject(NULL_BRUSH)
            : CreateSolidBrush(attr.color);
        HPEN op = (HPEN)SelectObject(hdc, p);
        HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
        Ellipse(hdc, s.x, s.y, e.x, e.y);
        SelectObject(hdc, ob);
        SelectObject(hdc, op);
        if (!transparent) DeleteObject(b);
        DeleteObject(p);
    }
};

class FreeLine : public Shape {
public:
    std::vector<POINT> pts;
    FreeLine(DrawAttr d, bool t) : Shape(d, t) {}
    void Draw(HDC hdc) override {
        if (pts.size() < 2) return;
        HPEN p = CreatePen(PS_SOLID, attr.thickness, attr.color);
        HPEN op = (HPEN)SelectObject(hdc, p);
        MoveToEx(hdc, pts[0].x, pts[0].y, NULL);
        for (size_t i = 1; i < pts.size(); i++) LineTo(hdc, pts[i].x, pts[i].y);
        SelectObject(hdc, op);
        DeleteObject(p);
    }
};

std::vector<Shape*> shapes;
SHAPE_TYPE gShape = SHAPE_FREE;
DrawAttr gAttr = { 1, RGB(0,0,0) };
bool gDraw = false;
POINT gStart, gTemp;
FreeLine* gFree = nullptr;

void SyncInvalidate() {
    for (int i = 0; i < gWndCount; i++) InvalidateRect(gWnd[i], NULL, TRUE);
}
void SyncInvalidateFast() {
    for (int i = 0; i < gWndCount; i++) InvalidateRect(gWnd[i], NULL, FALSE);
}

void DrawAll(HDC hdc)
{
    RECT rc; GetClientRect(WindowFromDC(hdc), &rc);
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP ob = (HBITMAP)SelectObject(mem, bmp);

    HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(mem, &rc, wb);
    DeleteObject(wb);

    for (auto p : shapes) p->Draw(mem);

    if (gDraw) {
        if (gShape == SHAPE_FREE && gFree) gFree->Draw(mem);
        else if (gShape == SHAPE_LINE) Line(gStart, gTemp, gAttr, gTransparent).Draw(mem);
        else if (gShape == SHAPE_RECT) RectShape(gStart, gTemp, gAttr, gTransparent).Draw(mem);
        else if (gShape == SHAPE_ELLIPSE) EllipseShape(gStart, gTemp, gAttr, gTransparent).Draw(mem);
    }

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);

    SelectObject(mem, ob);
    DeleteObject(bmp);
    DeleteDC(mem);
}

ATOM MyRegisterClass(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    wcscpy_s(szTitle, L"DrawingBoard_Asynchronous");
    LoadStringW(hInstance, IDC_MY20250920DRAWINGBOARD, szWindowClass, 100);

    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW w{};
    w.cbSize = sizeof(WNDCLASSEXW);
    w.style = CS_HREDRAW | CS_VREDRAW;
    w.lpfnWndProc = WndProc;
    w.hInstance = hInstance;
    w.hCursor = LoadCursor(NULL, IDC_ARROW);
    w.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    w.lpszMenuName = MAKEINTRESOURCE(IDC_MY20250920DRAWINGBOARD);
    w.lpszClassName = szWindowClass;
    return RegisterClassExW(&w);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    int w = 900, h = 700;

    HWND left = CreateWindowW(
        szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW,
        0, 0, w, h,
        NULL, NULL, hInstance, NULL);

    HWND right = CreateWindowW(
        szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW,
        w + 20, 0, w, h,
        NULL, NULL, hInstance, NULL);

    if (!left || !right) return FALSE;

    gWnd[0] = left;
    gWnd[1] = right;
    gWndCount = 2;

    ShowWindow(left, nCmdShow);
    UpdateWindow(left);
    ShowWindow(right, nCmdShow);
    UpdateWindow(right);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_COMMAND:
    {
        int id = LOWORD(wp);

        if (id == IDM_EXIT) DestroyWindow(hWnd);
        else if (id == IDM_ABOUT) MessageBox(hWnd, L"DrawingBoard_Asynchronous", L"About", MB_OK);

        else if (id == ID_SHAPE_LINE) gShape = SHAPE_LINE;
        else if (id == ID_SHAPE_RECT) gShape = SHAPE_RECT;
        else if (id == ID_SHAPE_ELLIPSE) gShape = SHAPE_ELLIPSE;
        else if (id == ID_SHAPE_FREE) gShape = SHAPE_FREE;

        else if (id == ID_THICKNESS_1) gAttr.thickness = 1;
        else if (id == ID_THICKNESS_5) gAttr.thickness = 5;
        else if (id == ID_THICKNESS_10) gAttr.thickness = 10;

        else if (id == ID_COLOR_BLACK) { gTransparent = false; gAttr.color = RGB(0, 0, 0); }
        else if (id == ID_COLOR_WHITE) { gTransparent = false; gAttr.color = RGB(255, 255, 255); }
        else if (id == ID_COLOR_RED) { gTransparent = false; gAttr.color = RGB(200, 100, 100); }
        else if (id == ID_COLOR_TRANSPARENT) gTransparent = true;

        SyncInvalidate();
    }
    break;

    case WM_LBUTTONDOWN:
        gDraw = true;
        gStart = { LOWORD(lp),HIWORD(lp) };
        gTemp = gStart;

        if (gShape == SHAPE_FREE) {
            if (gFree) delete gFree;
            gFree = new FreeLine(gAttr, gTransparent);
            gFree->pts.push_back(gStart);
        }
        SetCapture(hWnd);
        break;

    case WM_MOUSEMOVE:
        if (gDraw && (wp & MK_LBUTTON)) {
            POINT p = { LOWORD(lp),HIWORD(lp) };
            if (gShape == SHAPE_FREE) gFree->pts.push_back(p);
            else gTemp = p;
            SyncInvalidateFast();
        }
        break;

    case WM_LBUTTONUP:
    {
        if (!gDraw) break;
        gDraw = false;
        ReleaseCapture();
        POINT e = { LOWORD(lp),HIWORD(lp) };
        Shape* s = nullptr;

        if (gShape == SHAPE_LINE) s = new Line(gStart, e, gAttr, gTransparent);
        else if (gShape == SHAPE_RECT) s = new RectShape(gStart, e, gAttr, gTransparent);
        else if (gShape == SHAPE_ELLIPSE) s = new EllipseShape(gStart, e, gAttr, gTransparent);
        else if (gShape == SHAPE_FREE) { s = gFree; gFree = nullptr; }

        if (s) shapes.push_back(s);
        SyncInvalidate();
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawAll(hdc);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        for (auto p : shapes) delete p;
        shapes.clear();
        if (gFree) delete gFree;
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wp, lp);
    }
    return 0;
}
