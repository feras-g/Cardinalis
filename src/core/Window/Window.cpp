#include "Window.h"
#include "Core/EngineLogger.h"

///// Window Implementation
#if defined(_WIN32)	

#include <Windows.h>
#include "Core/Application.h"
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

	inline void UpdateGUI()  const;
	inline void ShutdownGUI()  const;
	inline bool IsClosed()  const;
	inline int  GetHeight() const;
	inline int  GetWidth()	const;
	inline const WindowData*  GetData()	const;
	void HandleEvents();

	double GetTimestampSeconds();

	void OnClose();
	void OnResize(unsigned int width, unsigned int height);

	WindowInfo m_WinInfo;
	WindowState m_WinState;
	WindowData m_Data;

	Application* hApplication;

	HWND hWnd;
	HINSTANCE hInstance;
};

/////////////////////////////////////////////////////////////////////////
Window::Impl::Impl(const WindowInfo& info, Application* hApp) : m_WinInfo(info), m_WinState({}), hWnd(NULL), hInstance(NULL) , hApplication(hApp)
{
}

Window::Impl::~Impl()
{
}

/////////////////////////////////////////////////////////////////////////
void Window::Impl::Create()
{
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
	DWORD style = WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;// | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	// Coordinates of the top-left corner in a centered window
	int top  = (GetSystemMetrics(SM_CYSCREEN)  - h) / 2;
	int left = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;

	m_WinInfo.aspect = (float)w / h;
	
	CreateWindowA(title, title, style, left, top, w, h, nullptr, nullptr, m_Data.hInstance, this);

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
	ShowWindow(m_Data.hWnd, SW_SHOWDEFAULT);
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
	if (self)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			self->OnClose();
			return 0;
		case WM_SIZE:
		{
			// Don't change window size on minimization
			if (wParam != SIZE_MINIMIZED)
			{
				UINT width  = LOWORD(lParam);
				UINT height = HIWORD(lParam);

				// Size doesn't change when restoring a minimized window in my implementation
				// but it is still recognized as a WM_SIZE event, so don't handle that case
				if (self->m_WinInfo.width != width || self->m_WinInfo.height != height)
				{
					self->m_WinInfo.width  = width;
					self->m_WinInfo.height = height;

					self->OnResize(width, height);
				}
			}
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 6));
			EndPaint(hWnd, &ps);
		}
		break;
		}
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////
void Window::Impl::OnClose()
{
	m_WinState.bIsClosed = true;
	DestroyWindow(m_Data.hWnd);
	PostQuitMessage(0);
}

/////////////////////////////////////////////////////////////////////////
void Window::Impl::OnResize(unsigned int width, unsigned int height)
{
	if (hApplication->bInitSuccess)
	{
		hApplication->OnWindowResize();
	}

	printf("%dx%d aspect=%f\n", m_WinInfo.width, m_WinInfo.height, m_WinInfo.aspect);
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
bool Window::Impl::IsClosed()  const { return m_WinState.bIsClosed; }
int  Window::Impl::GetHeight() const { return m_WinInfo.height; }
int  Window::Impl::GetWidth()  const { return m_WinInfo.width; }
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

double Window::Impl::GetTimestampSeconds()
{
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) 
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return now.QuadPart / s_frequency.QuadPart;
	}
	else 
	{
		return GetTickCount();
	}
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
inline bool Window::IsClosed() const { return pImpl->IsClosed(); }
inline int Window::GetHeight() const { return pImpl->GetHeight(); }
inline int Window::GetWidth()  const { return pImpl->GetWidth(); }
inline void Window::UpdateGUI() const { return pImpl->UpdateGUI(); }
inline void Window::ShutdownGUI() const { pImpl->ShutdownGUI(); }
inline const WindowData* Window::GetData() const { return pImpl->GetData(); }
void Window::HandleEvents() { return pImpl->HandleEvents(); }
inline double Window::GetTimestampSeconds() { return pImpl->GetTimestampSeconds(); }
void Window::OnClose()  { return pImpl->OnClose(); }
void Window::OnResize(unsigned int width, unsigned int height) { return pImpl->OnResize(width, height); }

IWindow::IWindow()
{
}

IWindow::~IWindow()
{
}
