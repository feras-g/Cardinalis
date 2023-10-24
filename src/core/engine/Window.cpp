#include "Window.h"
#include "core/engine/EngineLogger.h"

///// Window Implementation
#if defined(_WIN32)	

#include <Windowsx.h>
#include <ShellScalingApi.h>
#include "core/engine/Application.h"
#include "backends/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/////////////////////////////////////////////////////////////////////////
struct WindowData
{
	HWND hWnd;
	HINSTANCE hInstance;
};

/////////////////////////////////////////////////////////////////////////
class Window::Impl
{
public:
	Impl(const WindowInfo& info, Application* hApp);
	~Impl();
	void Create();
	void Show();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void UpdateGUI()  const;
	void ShutdownGUI()  const;
	bool IsClosed()  const;
	int  GetHeight() const;
	int  GetWidth()	const;
	float GetAspectRatio() const;
	const WindowData*  GetData()	const;
	void HandleEvents();

	double GetTime() const;

	void OnClose();
	void OnResize(unsigned int width, unsigned int height);


	void OnLeftMouseButtonUp(MouseEvent event);
	void OnLeftMouseButtonDown(MouseEvent event);
	void OnMouseMove(MouseEvent event);
	void OnKeyEvent(KeyEvent event);
	bool AsyncKeyState(Key key) const;

	bool is_in_focus() const;

	WindowInfo m_WinInfo;
	WindowState m_WinState;
	WindowData m_Data;

	Application* hApplication;

	HWND hWnd;
	HINSTANCE hInstance;

	double m_PerfCounterFreq;

	bool b_is_in_focus;
};

/////////////////////////////////////////////////////////////////////////
Window::Impl::Impl(const WindowInfo& info, Application* hApp) : m_WinInfo(info), m_WinState({}), hWnd(NULL), hInstance(NULL) , hApplication(hApp), b_is_in_focus(false)
{
}

Window::Impl::~Impl()
{
}

/////////////////////////////////////////////////////////////////////////
void Window::Impl::Create()
{
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	// Get the DPI of the primary display
	HDC hdc = GetDC(NULL);
	int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
	int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
	ReleaseDC(NULL, hdc);


	int h = m_WinInfo.height;
	int w = m_WinInfo.width;
	
	const char* title = m_WinInfo.title;
	
	// Registration
	WNDCLASSA wca
	{
		.style = 0,
		.lpfnWndProc = WndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = nullptr,
		.hIcon = LoadIcon(NULL, IDI_APPLICATION),
		.hCursor = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = NULL,
		.lpszMenuName = NULL,
		.lpszClassName = title
	};

	if (!RegisterClassA(&wca))
	{
		EXIT_ON_ERROR("Failed to register window");
	}

	m_Data.hInstance = GetModuleHandle(nullptr);

	// Creation
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX;

	// Coordinates of the top-left corner in a centered window
	int top  = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
	int left = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;

	m_WinInfo.aspect_ratio = (float)w / h;

	CreateWindowA(title, title, style, left, top, w, h, nullptr, nullptr, m_Data.hInstance, this);

	SetWindowLongA(hWnd, GWL_STYLE, style);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	// Init performance counter
	LARGE_INTEGER perfCountFreq;
	QueryPerformanceFrequency(&perfCountFreq);
	m_PerfCounterFreq = (double)perfCountFreq.QuadPart;

	// Initialize ImGui
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	if (!ImGui_ImplWin32_Init(m_Data.hWnd))
	{
		assert(false);
	}
}

void Window::Impl::Show()
{
	ShowWindow(m_Data.hWnd, SW_SHOW);
	SetForegroundWindow(m_Data.hWnd);
	SetFocus(m_Data.hWnd);
}


/////////////////////////////////////////////////////////////////////////
// https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992
LRESULT CALLBACK Window::Impl::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Window::Impl* self = nullptr;

	// Set window handle as soon as we receive the creation message
	if (uMsg == WM_NCCREATE) 
	{
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		self = static_cast<Window::Impl*>(lpcs->lpCreateParams);
		self->m_Data.hWnd = hWnd;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
	}
	else
	{
		self = reinterpret_cast<Window::Impl*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	// Handle msgs for ImGui
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	// Handle messages
	if (nullptr != self)
	{
		switch (uMsg)
		{
			case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
				lpMMI->ptMinTrackSize.x = 100;
				lpMMI->ptMinTrackSize.y = 100;
			}
			break;
			/* Window events */
			case WM_CLOSE:
			{
				self->OnClose();
				return 0;
			}
			case WM_SIZE:
			{
				// Don't change window size on minimization
				if (wParam != SIZE_MINIMIZED)
				{
					UINT width  = LOWORD(lParam);
					UINT height = HIWORD(lParam);

					if (self->m_WinInfo.width != width || self->m_WinInfo.height != height)
					{
						self->m_WinInfo.width  = width;
						self->m_WinInfo.height = height;
						self->m_WinInfo.aspect_ratio = width / (float)height;

						self->OnResize(width, height);
					}
				}
			}
			break;
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);
				FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 6));
				EndPaint(hWnd, &ps);
			}
			break;
			case WM_SETFOCUS:
			{
				self->b_is_in_focus = true;
			}
			break;
			case WM_KILLFOCUS:
			{
				self->b_is_in_focus = false;
			}
			break;
			/* Mouse events */
			case WM_MOUSEMOVE:
			{
				MouseEvent& event = self->hApplication->m_event_manager->mouse_event;

				event.px = GET_X_LPARAM(lParam);
				event.py = GET_Y_LPARAM(lParam);

				self->OnMouseMove(event);
			}
			break;

			case WM_LBUTTONDOWN:
			{
				MouseEvent& event = self->hApplication->m_event_manager->mouse_event;

				event.b_lmb_click = true;
				event.b_first_lmb_click = true;

				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				ScreenToClient(self->hWnd, &pt);

				event.px = pt.x;
				event.py = pt.y;
				self->OnLeftMouseButtonDown(event);
			}
			break;

			case WM_LBUTTONUP:
			{
				MouseEvent& event = self->hApplication->m_event_manager->mouse_event;

				event.b_lmb_click = false;
				event.b_first_lmb_click = false;
				event.px = GET_X_LPARAM(lParam);
				event.py = GET_Y_LPARAM(lParam);

				self->OnLeftMouseButtonUp(event);
			}
			break;

			/* Keyboard events */
			case WM_KEYDOWN:
			{
				KeyEvent& event = self->hApplication->m_event_manager->key_event;

				if(uMsg == WM_KEYDOWN) event.pressed = true;

				if (wParam == 'Z')		event.append(Key::Z);
				if (wParam == 'Q')		event.append(Key::Q);
				if (wParam == 'S')		event.append(Key::S);
				if (wParam == 'D')		event.append(Key::D);
				if (wParam == VK_LEFT)	event.append(Key::LEFT);
				if (wParam == VK_RIGHT)	event.append(Key::RIGHT);
				if (wParam == VK_UP)	event.append(Key::UP);
				if (wParam == VK_DOWN)	event.append(Key::DOWN);

				self->OnKeyEvent(event);
			}
			break;
			case WM_KEYUP:
			{
				KeyEvent& event = self->hApplication->m_event_manager->key_event;

				event.pressed = false;

				if (wParam == 'Z')		event.append( Key::Z);
				if (wParam == 'Q')		event.append( Key::Q);
				if (wParam == 'S')		event.append( Key::S);
				if (wParam == 'D')		event.append( Key::D);
				if (wParam == VK_LEFT)	event.append( Key::LEFT);
				if (wParam == VK_RIGHT)	event.append( Key::RIGHT);
				if (wParam == VK_UP)	event.append( Key::UP);
				if (wParam == VK_DOWN)	event.append( Key::DOWN);

				self->OnKeyEvent(event);
			}
			break;
		}
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////
void Window::Impl::OnClose()
{
	m_WinState.b_is_closed = true;
	DestroyWindow(m_Data.hWnd);
	PostQuitMessage(0);
}

/////////////////////////////////////////////////////////////////////////
void Window::Impl::OnResize(unsigned int width, unsigned int height)
{
	if (hApplication->b_init_success)
	{
		hApplication->on_window_resize();
	}

	printf("%dx%d aspect=%f\n", m_WinInfo.width, m_WinInfo.height, m_WinInfo.aspect_ratio);
}

inline void Window::Impl::UpdateGUI() const
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplWin32_NewFrame();
}

inline void Window::Impl::ShutdownGUI() const
{
	ImGui_ImplWin32_Shutdown();
}

/////////////////////////////////////////////////////////////////////////

void Window::Impl::OnLeftMouseButtonUp(MouseEvent event)
{ 
	hApplication->on_lmb_up(event);
};
void Window::Impl::OnLeftMouseButtonDown(MouseEvent event)
{ 
	hApplication->on_lmb_down(event);
};
void Window::Impl::OnMouseMove(MouseEvent event) 
{ 
	hApplication->on_mouse_move(event);
}
void Window::Impl::OnKeyEvent(KeyEvent event)
{
	hApplication->on_key_event(event);
}

bool Window::Impl::AsyncKeyState(Key key) const
{
	return GetAsyncKeyState(keyToWindows(key));
}

bool Window::Impl::is_in_focus() const
{
	return b_is_in_focus;
}

/////////////////////////////////////////////////////////////////////////
bool Window::Impl::IsClosed()  const { return m_WinState.b_is_closed; }
int  Window::Impl::GetHeight() const { return m_WinInfo.height; }
int  Window::Impl::GetWidth()  const { return m_WinInfo.width; }
float Window::Impl::GetAspectRatio() const { return m_WinInfo.aspect_ratio; };
inline const WindowData* Window::Impl::GetData() const { return &m_Data; }

void Window::Impl::HandleEvents()
{
	MSG message;

	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) > 0)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

/* Time in seconds */
double Window::Impl::GetTime() const
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return (double)now.QuadPart / m_PerfCounterFreq;
}

#else

#endif

/////////////////////////////////////////////////////////////////////////
Window::Window(const WindowInfo& info, Application* hApp)
	: pImpl(new Window::Impl(info, hApp))
{
	Initialize();
}

/////////////////////////////////////////////////////////////////////////
Window::~Window()
{
}

/////////////////////////////////////////////////////////////////////////
void Window::Initialize()
{
	pImpl->Create();
	pImpl->Show();
}

/////////////////////////////////////////////////////////////////////////
bool Window::is_closed() const { return pImpl->IsClosed(); }
int Window::GetHeight() const { return pImpl->GetHeight(); }
int Window::GetWidth()  const { return pImpl->GetWidth(); }
float Window::GetAspectRatio() const { return pImpl->GetAspectRatio(); }
void Window::update() const { return pImpl->UpdateGUI(); }
void Window::ShutdownGUI() const { pImpl->ShutdownGUI(); }
const WindowData* Window::GetData() const { return pImpl->GetData(); }
void Window::handle_events() { return pImpl->HandleEvents(); }
bool Window::is_in_focus() const { return pImpl->is_in_focus(); }
double Window::GetTime() const { return pImpl->GetTime(); }
void Window::OnClose()  { return pImpl->OnClose(); }
void Window::OnResize(unsigned int width, unsigned int height) { return pImpl->OnResize(width, height); }

bool Window::AsyncKeyState(Key key) const
{
	return pImpl->AsyncKeyState(key);
}

void Window::OnKeyEvent(KeyEvent keypress) { pImpl->OnKeyEvent(keypress); }

void Window::OnLeftMouseButtonUp(MouseEvent event)   { pImpl->OnLeftMouseButtonUp(event); };
void Window::OnLeftMouseButtonDown(MouseEvent event) { pImpl->OnLeftMouseButtonDown(event); };
void Window::OnMouseMove(MouseEvent event) { pImpl->OnMouseMove(event); };

IWindow::IWindow()
{
}

IWindow::~IWindow()
{
}
