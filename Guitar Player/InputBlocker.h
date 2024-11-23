#include <windows.h>
#include <thread>
#include <atomic>
#include <vector>

BOOLEAN debugCirclesEnabled = FALSE;

struct MouseClick {
    int x;
    int y;
    bool isLeft;
};

class InputBlocker {
private:
    HWND targetWindow;
    HWND overlayWindow;
    std::atomic<bool> active{ false };
    std::atomic<bool> paused{ false };
    std::thread blockingThread;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            MouseClick* clickArray = reinterpret_cast<MouseClick*>(lParam);
            PAINTSTRUCT ps;
            InvalidateRect(hwnd, NULL, TRUE);
            HDC hdc = BeginPaint(hwnd, &ps);

            // Fill with semi-transparent purple
            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH brush = CreateSolidBrush(RGB(147, 112, 219));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);

            HFONT hFont = CreateFontA(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            SelectObject(hdc, hFont);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            const char* text = "Guitar Player is playing.  Input is blocked while playing.";
            TextOutA(hdc, 20, 30, text, (int)strlen(text));
            DeleteObject(hFont);

            // Draw debug circles
            if (debugCirclesEnabled) {
                static const int debugCircleRadius = 10;
                for (int i = 0; i < wParam; i++) {
                    MouseClick click = clickArray[i];
                    int x = click.x;
                    int y = click.y;
                    Ellipse(hdc, x - debugCircleRadius, y - debugCircleRadius, x + debugCircleRadius, y + debugCircleRadius);
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN: {
            // Bring overlay to front
            HWND targetWindow = (HWND)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (targetWindow) {
                // Bring overlay to the front
                SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                // Bring the target window just below the overlay
                SetWindowPos(targetWindow, hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            return 0;
        }
        case WM_MOUSEWHEEL:
        case WM_KEYDOWN:
        case WM_KEYUP:
            // Block any input event
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    void BlockingLoop() {
        WNDCLASSEXA wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "GuitarInputBlocker";
        wc.hbrBackground = CreateSolidBrush(RGB(147, 112, 219));

        RegisterClassExA(&wc);

        RECT targetRect;
        GetWindowRect(targetWindow, &targetRect);

        overlayWindow = CreateWindowExA(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            "GuitarInputBlocker",
            NULL,
            WS_POPUP | WS_VISIBLE,
            targetRect.left, targetRect.top,
            targetRect.right - targetRect.left,
            targetRect.bottom - targetRect.top,
            NULL,   // No parent
            NULL,
            GetModuleHandle(NULL),
            NULL
        );

        if (!overlayWindow) {
            DWORD error = GetLastError();
            char errorMsg[256];
            sprintf_s(errorMsg, "Failed to create overlay window. Error: %lu", error);
            MessageBoxA(NULL, errorMsg, "Error", MB_OK);
            return;
        }

        SetWindowLongPtr(overlayWindow, GWLP_USERDATA, (LONG_PTR)targetWindow);

        SetLayeredWindowAttributes(overlayWindow, 0, 60, LWA_ALPHA); 

        // Ensure the overlay is above the target window
        HWND insertAfter = GetWindow(targetWindow, GW_HWNDPREV);
        SetWindowPos(overlayWindow, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        MSG msg;
        while (active) {
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!paused) {
                // Reposition overlay above the target window
                GetWindowRect(targetWindow, &targetRect);

                insertAfter = GetWindow(targetWindow, GW_HWNDPREV);
                SetWindowPos(overlayWindow, insertAfter,
                    targetRect.left, targetRect.top,
                    targetRect.right - targetRect.left,
                    targetRect.bottom - targetRect.top,
                    SWP_NOACTIVATE);

                if (!IsWindowVisible(overlayWindow)) {
                    ShowWindow(overlayWindow, SW_SHOW);
                }
            }
            else {
                if (IsWindowVisible(overlayWindow)) {
                    ShowWindow(overlayWindow, SW_HIDE);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        if (overlayWindow) {
            DestroyWindow(overlayWindow);
            overlayWindow = NULL;
        }

        UnregisterClassA("GuitarInputBlocker", GetModuleHandle(NULL));
    }

public:
    InputBlocker() : targetWindow(NULL), overlayWindow(NULL) {}

    HWND getOverlayWindow() {
        // Used for receiving updates
        return overlayWindow;
    }

    bool Start(HWND window) {
        if (active) return false;
        if (!IsWindow(window)) {
            MessageBoxA(NULL, "Invalid window handle", "Error", MB_OK);
            return false;
        }

        targetWindow = window;
        active = true;
        paused = false;
        blockingThread = std::thread(&InputBlocker::BlockingLoop, this);
        return true;
    }

    void Stop() {
        if (!active) return;

        active = false;
        if (blockingThread.joinable()) {
            blockingThread.join();
        }
    }

    void Pause() {
        paused = true;
    }

    void Resume() {
        paused = false;
    }

    ~InputBlocker() {
        Stop();
    }
};
