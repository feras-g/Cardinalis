#ifndef ENGINE_WINDOW_H
#define ENGINE_WINDOW_H

#include <memory>

#include "InputEvents.h"

struct WindowData;

class IWindow
{
public:
	IWindow();
	IWindow(const IWindow&) = delete;
	IWindow& operator=(const IWindow&) = delete;
	virtual ~IWindow();

	virtual void Initialize() = 0;
	virtual void UpdateGUI()   const = 0;
	virtual void ShutdownGUI() const = 0;
	virtual bool IsClosed()   const = 0;
	virtual void OnClose() = 0;
	virtual void OnResize(unsigned int width, unsigned int height) = 0;
	virtual int GetHeight() const = 0;
	virtual int GetWidth()  const = 0;
	virtual double GetTime() const = 0;

	virtual const WindowData* GetData() const = 0;

};

struct WindowInfo
{
	const char* title;
	uint32_t width;
	uint32_t height;
	float aspect;
};

struct WindowState
{
	bool bIsClosed = false;
	bool bIsMinimized = false;
};

class Application;

class Window : public IWindow
{
	class Impl;
	std::unique_ptr<Impl> pImpl;

public:
	Window(const WindowInfo& info, Application* hApp);
	~Window() override;

	void Initialize() override;
	inline bool IsClosed()   const override;
	inline int  GetHeight()  const override;
	inline int  GetWidth()	 const override;
	inline void UpdateGUI()  const override;
	inline void ShutdownGUI()  const override;
	inline const WindowData* GetData() const override;
	void HandleEvents();

	inline double GetTime() const;

	void OnClose() override;
	void OnResize(unsigned int width, unsigned int height) override;

	void OnLeftMouseButtonUp();
	void OnLeftMouseButtonDown();
	void OnMouseMove(int x, int y);
	void OnKeyEvent(KeyEvent event);
	bool AsyncKeyState(Key key);
};

#endif // !WINDOW_H

