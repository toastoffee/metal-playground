/**
  ******************************************************************************
  * @file           : delegates.cpp
  * @author         : toastoffee
  * @brief          : None
  * @attention      : None
  * @date           : 2024/10/9
  ******************************************************************************
  */



#include "delegates.hpp"

MyMTKViewDelegate::MyMTKViewDelegate(MTL::Device *pDevice) {

}

MyMTKViewDelegate::~MyMTKViewDelegate() {

}

void MyMTKViewDelegate::drawInMTKView(MTK::View *pView) {
    ViewDelegate::drawInMTKView(pView);
}




MyAppDelegate::~MyAppDelegate() {
    _mtkView->release();
    _window->release();
    _device->release();

    delete _viewDelegate;
}

NS::Menu *MyAppDelegate::createMenuBar() {
    return nullptr;
}

void MyAppDelegate::applicationWillFinishLaunching(NS::Notification *pNotification) {
    NS::Menu* menu = createMenuBar();
    auto* app = reinterpret_cast<NS::Application *>(pNotification->object());
    app->setMainMenu(menu);
    app->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
}

void MyAppDelegate::applicationDidFinishLaunching(NS::Notification *pNotification) {
    CGRect frame = (CGRect){{100.0, 100.0}, {512.0, 512.0}};

    _window = NS::Window::alloc()->init(
            frame,
            NS::WindowStyleMaskClosable | NS::WindowStyleMaskTitled,
            NS::BackingStoreBuffered,
            false);

    _device = MTL::CreateSystemDefaultDevice();

    _mtkView = MTK::View::alloc()->init(frame, _device);
    _mtkView->setColorPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    _mtkView->setClearColor(MTL::ClearColor::Make(1.0, 0.0, 0.0, 1.0));

    _viewDelegate = new MyMTKViewDelegate(_device);
    _mtkView->setDelegate(_viewDelegate);

    _window->setContentView(_mtkView);
    _window->setTitle(NS::String::string("00-hello-metal", NS::StringEncoding::UTF8StringEncoding));

    _window->makeKeyAndOrderFront(nullptr);

    auto app = reinterpret_cast<NS::Application *>(pNotification->object());
    app->activateIgnoringOtherApps(true);
}

bool MyAppDelegate::applicationShouldTerminateAfterLastWindowClosed(NS::Application *pSender) {
    return true;
}


