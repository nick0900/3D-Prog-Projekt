#include "WindowHelper.h"
#include <iostream>

namespace Static
{
	UINT movementInput;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 0x57: //w
			Static::movementInput |= 0x01;
			break;

		case 0x41: //a
			Static::movementInput |= 0x02;
			break;

		case 0x53: //s
			Static::movementInput |= 0x04;
			break;

		case 0x44: //d
			Static::movementInput |= 0x08;
			break;

		case VK_UP: //up arrow
			Static::movementInput |= 0x10;
			break;

		case VK_LEFT: //left arrow
			Static::movementInput |= 0x20;
			break;

		case VK_DOWN: //down arrow
			Static::movementInput |= 0x40;
			break;

		case VK_RIGHT: //right arrow
			Static::movementInput |= 0x80;
			break;

		case 0x31: //1 key
			Static::movementInput |= 0x100;
			break;

		case 0x32: //2 key
			Static::movementInput |= 0x200;
			break;
		}
		return 0;

	case WM_KEYUP:
		switch (wParam)
		{
		case 0x57: //w
			Static::movementInput &= 0xffe;
			break;

		case 0x41: //a
			Static::movementInput &= 0xffd;
			break;

		case 0x53: //s
			Static::movementInput &= 0xffb;
			break;

		case 0x44: //d
			Static::movementInput &= 0xff7;
			break;

		case VK_UP: //up arrow
			Static::movementInput &= 0xfef;
			break;

		case VK_LEFT: //left arrow
			Static::movementInput &= 0xfdf;
			break;

		case VK_DOWN: //down arrow
			Static::movementInput &= 0xfbf;
			break;

		case VK_RIGHT: //right arrow
			Static::movementInput &= 0xf7f;
			break;

		case 0x31: //1 key
			Static::movementInput &= 0xeff;
			break;

		case 0x32: //2 key
			Static::movementInput &= 0xdff;
			break;
		}
		return 0;

	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

bool SetupWindow(HINSTANCE instance, UINT width, UINT height, int nCmdShow, HWND& window)
{
	const wchar_t CLASS_NAME[] = L"Demo Window Class";

	WNDCLASS wc = {};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	window = CreateWindowEx(0, CLASS_NAME, L"Labb 2 Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, width, height, nullptr, nullptr, instance, nullptr);
	if (window == nullptr)
	{
		std::cerr << "HWND was nullptr, last error: " << GetLastError() << std::endl;
		return false;
	}

	ShowWindow(window, nCmdShow);

	return true;
}

UINT GetInputTracker()
{
	return Static::movementInput;
}
