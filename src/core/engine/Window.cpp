#include "Window.h"
#include "core/engine/logger.h"

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

	/* Returns time in seconds since window creation */
	double get_time() const;

	void OnClose();
	void OnResize(unsigned int width, unsigned int height);


	void OnMouseDown(MouseEvent event);
	void OnMouseUp(MouseEvent event);
	void OnMouseMove(MouseEvent event);
	void OnKeyEvent(KeyEvent event);
	bool AsyncKeyState(Key key) const;

	bool is_in_focus() const;

	void init_perf_counter();

	WindowInfo m_WinInfo;
	WindowState m_WinState;
	WindowData m_Data;

	Application* hApplication;

	HWND hWnd;
	HINSTANCE hInstance;

	double m_perf_counter_freq;
	double m_perf_counter_start;

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
	init_perf_counter();



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
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;


				event.b_first_click = false;
				event.curr_pos_x = GET_X_LPARAM(lParam);
				event.curr_pos_y = GET_Y_LPARAM(lParam);

				self->OnMouseMove(event);
			}
			break;

			case WM_LBUTTONDOWN:
			{
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;


				event.b_lmb_click = true;
				event.b_first_click = true;

				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				ScreenToClient(self->hWnd, &pt);

				event.curr_click_px = pt.x;
				event.curr_click_py = pt.y;
				self->OnMouseDown(event);
			}
			break;

			case WM_MBUTTONDOWN:
			{
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;


				event.b_mmb_click = true;
				event.b_first_click = true;
				event.b_wheel_scroll = false;
				event.wheel_zdelta = 0;
				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				ScreenToClient(self->hWnd, &pt);

				event.curr_click_px = pt.x;
				event.curr_click_py = pt.y;
				self->OnMouseDown(event);
			}
			break;

			case WM_RBUTTONDOWN:
			{
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;


				event.b_rmb_click = true;
				event.b_first_click = true;

				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				ScreenToClient(self->hWnd, &pt);

				event.curr_click_px = pt.x;
				event.curr_click_py = pt.y;
				self->OnMouseDown(event);
			}
			break;

			case WM_MOUSEWHEEL:
			{
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;

				event.wheel_zdelta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
				event.b_wheel_scroll = true;
				self->OnMouseDown(event);
			}
			break;

			case WM_LBUTTONUP:
			{
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;


				event.b_lmb_click = false;
				event.curr_click_px = GET_X_LPARAM(lParam);
				event.curr_click_py = GET_Y_LPARAM(lParam);

				self->OnMouseUp(event);
			}
			break;

			case WM_MBUTTONUP:
			{
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;


				event.b_mmb_click = false;

				POINT pt;
				pt.x = GET_X_LPARAM(lParam);
				pt.y = GET_Y_LPARAM(lParam);
				ScreenToClient(self->hWnd, &pt);

				event.curr_click_px = pt.x;
				event.curr_click_py = pt.y;
				self->OnMouseUp(event);
			}
			break;

			case WM_RBUTTONUP:
			{
				MouseEvent event = self->hApplication->m_event_manager->mouse_event;

				event.b_rmb_click = false;
				event.curr_click_px = GET_X_LPARAM(lParam);
				event.curr_click_py = GET_Y_LPARAM(lParam);

				self->OnMouseUp(event);
			}
			break;

			/* Keyboard events */
			case WM_KEYDOWN:
			{
				KeyEvent& event = self->hApplication->m_event_manager->key_event;

				if(uMsg == WM_KEYDOWN) event.pressed = true;

				if (wParam == 'Z')		event.key = Key::Z;
				if (wParam == 'Q')		event.key = Key::Q;
				if (wParam == 'S')		event.key = Key::S;
				if (wParam == 'D')		event.key = Key::D;
				if (wParam == VK_LEFT)	event.key = Key::LEFT;
				if (wParam == VK_RIGHT)	event.key = Key::RIGHT;
				if (wParam == VK_UP)	event.key = Key::UP;
				if (wParam == VK_DOWN)	event.key = Key::DOWN;

				self->OnKeyEvent(event);
			}
			break;
			case WM_KEYUP:
			{
				KeyEvent& event = self->hApplication->m_event_manager->key_event;

				event.pressed = false;

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

void Window::Impl::OnMouseDown(MouseEvent event)
{ 
	hApplication->on_mouse_down(event);
};
void Window::Impl::OnMouseUp(MouseEvent event)
{ 
	hApplication->on_mouse_up(event);
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

void Window::Impl::init_perf_counter()
{
	LARGE_INTEGER perf_counter_freq;
	QueryPerformanceFrequency(&perf_counter_freq);
	m_perf_counter_freq = double(perf_counter_freq.QuadPart);
	QueryPerformanceCounter(&perf_counter_freq);
	m_perf_counter_start = double(perf_counter_freq.QuadPart);
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

double Window::Impl::get_time() const
{
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	return double(now.QuadPart - m_perf_counter_start) / m_perf_counter_freq;
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
double Window::GetTime() const { return pImpl->get_time(); }
void Window::OnClose()  { return pImpl->OnClose(); }
void Window::OnResize(unsigned int width, unsigned int height) { return pImpl->OnResize(width, height); }

bool Window::AsyncKeyState(Key key) const
{
	return pImpl->AsyncKeyState(key);
}

void Window::OnKeyEvent(KeyEvent keypress) { pImpl->OnKeyEvent(keypress); }

void Window::OnMouseUp(MouseEvent event)   { pImpl->OnMouseUp(event); };
void Window::OnMouseDown(MouseEvent event) { pImpl->OnMouseDown(event); };
void Window::OnMouseMove(MouseEvent event) { pImpl->OnMouseMove(event); };

IWindow::IWindow()
{
}

IWindow::~IWindow()
{
}
