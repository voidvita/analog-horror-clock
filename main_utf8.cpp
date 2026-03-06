#define NOMINMAX
#include <windows.h>
#include <ctime>
#include <cstdlib>

// 全局变量
int g_seconds = 45;          // 从45秒开始
int g_phase = 0;             // 0:正常 1:冻结 2:扫描效果 3:黑屏 4:快逃
bool g_blink = true;         // 闪烁控制
int g_freezeCount = 0;       // 冻结计数器
int g_offsetX = 0;           // 抖动X
int g_offsetY = 0;           // 抖动Y
int g_blinkCounter = 0;      // 闪烁计数器
int g_scanProgress = 0;      // 扫描进度

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    srand((unsigned)time(NULL));

    const wchar_t CLASS_NAME[] = L"HorrorClockWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"时钟校准系统",
        WS_OVERLAPPEDWINDOW,
        100, 100, 900, 700,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    SetTimer(hwnd, 1, 30, NULL);
    SetTimer(hwnd, 2, 1000, NULL);
    SetTimer(hwnd, 3, 20, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        KillTimer(hwnd, 2);
        KillTimer(hwnd, 3);
        PostQuitMessage(0);
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            if (g_phase == 0 || g_phase == 1) {
                g_offsetX = (rand() % 3) - 1;
                g_offsetY = (rand() % 3) - 1;
            }
            else if (g_phase == 3) {
                g_offsetX = (rand() % 7) - 3;
                g_offsetY = (rand() % 7) - 3;
            }

            if (g_phase == 3) {
                g_blinkCounter++;
                if (g_blinkCounter >= 10) {
                    g_blink = !g_blink;
                    g_blinkCounter = 0;
                }
            }

            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (wParam == 2) {
            if (g_phase == 0) {
                g_blink = !g_blink;
                if (g_seconds < 59) {
                    g_seconds++;
                }
                else {
                    g_phase = 1;
                    g_freezeCount = 0;
                }
            }
            else if (g_phase == 1) {
                g_freezeCount++;
                if (g_freezeCount >= 2) {  // 冻结2秒后开始扫描
                    g_phase = 2;
                    g_scanProgress = 0;
                }
            }
            else if (g_phase == 3) {
                if (g_seconds < 15) {
                    g_seconds++;
                }
                else {
                    g_phase = 4;
                }
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (wParam == 3) {
            if (g_phase == 2) {
                g_scanProgress += 1;  // 减慢速度
                if (g_scanProgress >= 100) {
                    g_phase = 3;
                    g_seconds = 0;
                    g_blinkCounter = 0;
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        HBRUSH hBrush;
        if (g_phase < 3) {
            hBrush = CreateSolidBrush(RGB(0, 0, 170));
        }
        else {
            hBrush = CreateSolidBrush(RGB(0, 0, 0));
        }
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        SetBkMode(hdc, TRANSPARENT);

        if (g_phase == 0 || g_phase == 1) {
            SetTextColor(hdc, RGB(255, 255, 255));

            // "请校准时钟" - 闪烁，不偏移
            if (g_blink || g_phase == 1) {
                HFONT hFont = CreateFontW(48, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, GB2312_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
                SelectObject(hdc, hFont);

                const wchar_t* text = L"请校准时钟";
                SIZE size;
                GetTextExtentPoint32W(hdc, text, wcslen(text), &size);
                TextOutW(hdc,
                    rect.right / 2 - size.cx / 2 + g_offsetX,
                    rect.bottom / 3 - 80 + g_offsetY,
                    text, wcslen(text));
                DeleteObject(hFont);
            }

            // 日期显示，不偏移
            HFONT hFontDate = CreateFontW(28, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFontDate);

            const wchar_t* dateText = L"现在是2025年12月31日";
            SIZE sizeDate;
            GetTextExtentPoint32W(hdc, dateText, wcslen(dateText), &sizeDate);
            TextOutW(hdc,
                rect.right / 2 - sizeDate.cx / 2 + g_offsetX,
                rect.bottom / 2 - 80 + g_offsetY,
                dateText, wcslen(dateText));
            DeleteObject(hFontDate);

            // 时钟显示 - 细长宋体，带腰斩效果
            HFONT hFont2 = CreateFontW(120, 35, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFont2);

            wchar_t timeStr[32];
            wsprintfW(timeStr, L"23:59:%02d", g_seconds);
            SIZE size2;
            GetTextExtentPoint32W(hdc, timeStr, wcslen(timeStr), &size2);

            int baseX = rect.right / 2 - size2.cx / 2 + g_offsetX;
            int baseY = rect.bottom / 2 + 20 + g_offsetY;

            // 判断是否需要腰斩（冻结第2秒开始）
            if (g_phase == 1 && g_freezeCount >= 1) {
                int splitOffset = 50;  // 上半部分向右偏移
                int middleY = baseY + size2.cy / 2;

                // 绘制上半部分（向右偏移）
                int saved = SaveDC(hdc);
                IntersectClipRect(hdc, 0, 0, rect.right, middleY);
                TextOutW(hdc, baseX + splitOffset, baseY, timeStr, wcslen(timeStr));
                RestoreDC(hdc, saved);

                // 绘制下半部分（不偏移）
                saved = SaveDC(hdc);
                IntersectClipRect(hdc, 0, middleY, rect.right, rect.bottom);
                TextOutW(hdc, baseX, baseY, timeStr, wcslen(timeStr));
                RestoreDC(hdc, saved);
            }
            else {
                // 正常绘制
                TextOutW(hdc, baseX, baseY, timeStr, wcslen(timeStr));
            }

            DeleteObject(hFont2);
        }
        else if (g_phase == 2) {
            // 先绘制完整的phase 1内容
            SetTextColor(hdc, RGB(255, 255, 255));

            // "请校准时钟"
            HFONT hFont = CreateFontW(48, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFont);
            const wchar_t* text = L"请校准时钟";
            SIZE size;
            GetTextExtentPoint32W(hdc, text, wcslen(text), &size);
            TextOutW(hdc, rect.right / 2 - size.cx / 2, rect.bottom / 3 - 80, text, wcslen(text));
            DeleteObject(hFont);

            // 日期
            HFONT hFontDate = CreateFontW(28, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFontDate);
            const wchar_t* dateText = L"现在是2025年12月31日";
            SIZE sizeDate;
            GetTextExtentPoint32W(hdc, dateText, wcslen(dateText), &sizeDate);
            TextOutW(hdc, rect.right / 2 - sizeDate.cx / 2, rect.bottom / 2 - 80, dateText, wcslen(dateText));
            DeleteObject(hFontDate);

            // 时钟（保留腰斩效果）
            HFONT hFont2 = CreateFontW(120, 35, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFont2);
            wchar_t timeStr[32];
            wsprintfW(timeStr, L"23:59:%02d", g_seconds);
            SIZE size2;
            GetTextExtentPoint32W(hdc, timeStr, wcslen(timeStr), &size2);
            int baseX = rect.right / 2 - size2.cx / 2;
            int baseY = rect.bottom / 2 + 20;

            int splitOffset = 50;
            int middleY = baseY + size2.cy / 2;
            int saved = SaveDC(hdc);
            IntersectClipRect(hdc, 0, 0, rect.right, middleY);
            TextOutW(hdc, baseX + splitOffset, baseY, timeStr, wcslen(timeStr));
            RestoreDC(hdc, saved);
            saved = SaveDC(hdc);
            IntersectClipRect(hdc, 0, middleY, rect.right, rect.bottom);
            TextOutW(hdc, baseX, baseY, timeStr, wcslen(timeStr));
            RestoreDC(hdc, saved);
            DeleteObject(hFont2);

            // DOS扫描覆盖效果
            int blockSize = 16;
            int cols = (rect.right + blockSize - 1) / blockSize;
            int rows = (rect.bottom + blockSize - 1) / blockSize;
            int totalBlocks = cols * rows;
            int filledBlocks = (totalBlocks * g_scanProgress) / 100;

            HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
            int blockCount = 0;
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    if (blockCount < filledBlocks) {
                        RECT blockRect;
                        blockRect.left = x * blockSize;
                        blockRect.top = y * blockSize;
                        blockRect.right = (x + 1) * blockSize;
                        if (blockRect.right > rect.right) blockRect.right = rect.right;
                        blockRect.bottom = (y + 1) * blockSize;
                        if (blockRect.bottom > rect.bottom) blockRect.bottom = rect.bottom;
                        FillRect(hdc, &blockRect, blackBrush);
                    }
                    blockCount++;
                }
            }
            DeleteObject(blackBrush);
        }
        else if (g_phase == 3) {
            SetTextColor(hdc, RGB(255, 0, 0));

            if (g_blink) {
                HFONT hFont = CreateFontW(48, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, GB2312_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
                SelectObject(hdc, hFont);

                const wchar_t* text = L"跨年失败";
                SIZE size;
                GetTextExtentPoint32W(hdc, text, wcslen(text), &size);
                TextOutW(hdc,
                    rect.right / 2 - size.cx / 2 + g_offsetX,
                    rect.bottom / 3 - 80 + g_offsetY,
                    text, wcslen(text));
                DeleteObject(hFont);
            }

            HFONT hFontDate = CreateFontW(28, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFontDate);

            const wchar_t* dateText = L"现在是2025年12月32日??";
            SIZE sizeDate;
            GetTextExtentPoint32W(hdc, dateText, wcslen(dateText), &sizeDate);
            TextOutW(hdc,
                rect.right / 2 - sizeDate.cx / 2 + g_offsetX,
                rect.bottom / 2 - 80 + g_offsetY,
                dateText, wcslen(dateText));
            DeleteObject(hFontDate);

            HFONT hFont2 = CreateFontW(120, 35, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFont2);

            wchar_t timeStr[32];
            wsprintfW(timeStr, L"23:60:%02d", g_seconds);
            SIZE size2;
            GetTextExtentPoint32W(hdc, timeStr, wcslen(timeStr), &size2);
            TextOutW(hdc,
                rect.right / 2 - size2.cx / 2 + g_offsetX,
                rect.bottom / 2 + 20 + g_offsetY,
                timeStr, wcslen(timeStr));
            DeleteObject(hFont2);
        }
        else if (g_phase == 4) {
            SetTextColor(hdc, RGB(255, 0, 0));

            HFONT hFont = CreateFontW(250, 0, 0, 0, FW_BOLD,
                FALSE, FALSE, FALSE, GB2312_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"宋体");
            SelectObject(hdc, hFont);

            const wchar_t* text = L"快逃";
            SIZE size;
            GetTextExtentPoint32W(hdc, text, wcslen(text), &size);
            TextOutW(hdc,
                rect.right / 2 - size.cx / 2,
                rect.bottom / 2 - size.cy / 2,
                text, wcslen(text));
            DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}