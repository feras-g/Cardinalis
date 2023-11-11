#pragma once

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
	virtual void update()   const = 0;
	virtual void ShutdownGUI() const = 0;
	virtual bool is_closed()   const = 0;
	virtual void OnClose() = 0;
	virtual void OnResize(unsigned int width, unsigned int height) = 0;
	virtual int GetHeight() const = 0;
	virtual int GetWidth()  const = 0;
	virtual float GetAspectRatio() const = 0;
	virtual double GetTime() const = 0;

	virtual const WindowData* GetData() const = 0;

};

struct WindowInfo
{
	uint32_t width;
	uint32_t height;
	const char* title;
	uint32_t dpi;
	float aspect_ratio;
};

struct WindowState
{
	bool b_is_closed = false;
	bool b_is_minimized = false;
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
	bool is_closed()   const override;
	int  GetHeight()  const override;
	int  GetWidth()	 const override;
	float GetAspectRatio() const override;
	void update()  const override;
	void ShutdownGUI()  const override;
	const WindowData* GetData() const override;
	void handle_events();
	bool is_in_focus() const;

	double GetTime() const;

	void OnClose() override;
	void OnResize(unsigned int width, unsigned int height) override;

	void OnLeftMouseButtonUp(MouseEvent event);
	void OnLeftMouseButtonDown(MouseEvent event);
	void OnMouseMove(MouseEvent event);
	void OnKeyEvent(KeyEvent event);
	bool AsyncKeyState(Key key) const;
};

