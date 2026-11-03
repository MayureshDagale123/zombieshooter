#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <chrono>
#include <string>
#include "CG_Algorithms.hpp"
#include "Game.hpp"

// Global constants
constexpr int WINDOW_WIDTH = 960;
constexpr int WINDOW_HEIGHT = 600;

// Global Game and Framebuffer instances
static Framebuffer g_framebuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
static Game g_game(WINDOW_WIDTH, WINDOW_HEIGHT);
static BITMAPINFO g_bmi;

// Key state tracker
static bool g_key_up = false;
static bool g_key_down = false;
static bool g_key_aim_up = false;
static bool g_key_aim_down = false;

// Window Procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Setup standard 32-bit top-down DIB header for blitting our software framebuffer
            ZeroMemory(&g_bmi, sizeof(BITMAPINFO));
            g_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            g_bmi.bmiHeader.biWidth = WINDOW_WIDTH;
            g_bmi.bmiHeader.biHeight = -WINDOW_HEIGHT; // Negative height indicates top-down bitmap
            g_bmi.bmiHeader.biPlanes = 1;
            g_bmi.bmiHeader.biBitCount = 32;
            g_bmi.bmiHeader.biCompression = BI_RGB;
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Blit our software pixel buffer to the window screen
            SetDIBitsToDevice(
                hdc,
                0, 0,
                WINDOW_WIDTH, WINDOW_HEIGHT,
                0, 0,
                0, WINDOW_HEIGHT,
                g_framebuffer.pixels.data(),
                &g_bmi,
                DIB_RGB_COLORS
            );

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_RETURN:
                case VK_SPACE:
                    if (g_game.state == Game::MENU) {
                        g_game.state = Game::PLAYING;
                    } else if (g_game.state == Game::PLAYING) {
                        g_game.shoot_bullet();
                    } else if (g_game.state == Game::GAME_OVER) {
                        g_game.restart();
                    }
                    break;

                case 'W':
                case VK_UP:
                    g_key_up = true;
                    break;

                case 'S':
                case VK_DOWN:
                    g_key_down = true;
                    break;

                case 'A':
                case VK_LEFT:
                    g_key_aim_up = true;
                    break;

                case 'D':
                case VK_RIGHT:
                    g_key_aim_down = true;
                    break;

                case 'R':
                    g_game.restart();
                    break;

                case VK_ESCAPE:
                    PostQuitMessage(0);
                    break;
            }
            return 0;
        }

        case WM_KEYUP: {
            switch (wParam) {
                case 'W':
                case VK_UP:
                    g_key_up = false;
                    break;

                case 'S':
                case VK_DOWN:
                    g_key_down = false;
                    break;

                case 'A':
                case VK_LEFT:
                    g_key_aim_up = false;
                    break;

                case 'D':
                case VK_RIGHT:
                    g_key_aim_down = false;
                    break;
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // Prevent flickering

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"ZombieShooterWindowClass";

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 1;
    }

    // Adjust window rect so client area is exactly WINDOW_WIDTH x WINDOW_HEIGHT
    RECT wr = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Zombie Line Defense - Computer Graphics Mini-Project (C++)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // High resolution frame timing
    using clock = std::chrono::high_resolution_clock;
    auto prev_time = clock::now();

    MSG msg = {};
    bool running = true;

    while (running) {
        // Process Windows message queue
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!running) break;

        // Calculate delta time
        auto current_time = clock::now();
        std::chrono::duration<float> elapsed = current_time - prev_time;
        prev_time = current_time;
        float dt = elapsed.count();
        if (dt > 0.05f) dt = 0.05f; // Clamp to avoid huge jumps on window drag

        // Handle continuous movement input
        if (g_game.state == Game::PLAYING) {
            if (g_key_up) {
                g_game.player.move_up(dt);
            }
            if (g_key_down) {
                g_game.player.move_down(dt);
            }
            if (g_key_aim_up) {
                g_game.player.aim_up(dt);
            }
            if (g_key_aim_down) {
                g_game.player.aim_down(dt);
            }
        }

        // Update Game Physics & CG Entities
        g_game.update(dt, WINDOW_WIDTH, WINDOW_HEIGHT);

        // Rasterize frame using our custom Computer Graphics Algorithms into g_framebuffer
        g_game.draw(g_framebuffer);

        // Request redraw
        HDC hdc = GetDC(hwnd);
        SetDIBitsToDevice(
            hdc,
            0, 0,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            0, 0,
            0, WINDOW_HEIGHT,
            g_framebuffer.pixels.data(),
            &g_bmi,
            DIB_RGB_COLORS
        );
        ReleaseDC(hwnd, hdc);

        // Target ~60 FPS
        Sleep(14);
    }

    return 0;
}

// Console main fallback if compiled as console application
int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
