/**
  ******************************************************************************
  * @file           : delegates.hpp
  * @author         : toastoffee
  * @brief          : None
  * @attention      : None
  * @date           : 2024/10/9
  ******************************************************************************
  */



#ifndef METAL_PLAYGROUND_DELEGATES_HPP
#define METAL_PLAYGROUND_DELEGATES_HPP

#include "renderer.hpp"

class MyMTKViewDelegate : public MTK::ViewDelegate {
private:
    Renderer *_renderer{};

public:
    explicit MyMTKViewDelegate( MTL::Device* pDevice );
    ~MyMTKViewDelegate() override;
    void drawInMTKView( MTK::View* pView ) override;
};

class MyAppDelegate : public NS::ApplicationDelegate {
private:
    NS::Window* _window;
    MTK::View* _mtkView;
    MTL::Device* _device;
    MyMTKViewDelegate* _viewDelegate = nullptr;

public:
    ~MyAppDelegate() override;

    NS::Menu* createMenuBar();

    void applicationWillFinishLaunching(NS::Notification *pNotification) override;

    void applicationDidFinishLaunching(NS::Notification *pNotification) override;

    bool applicationShouldTerminateAfterLastWindowClosed(NS::Application *pSender) override;


};

#endif //METAL_PLAYGROUND_DELEGATES_HPP
