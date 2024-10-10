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

MyMTKViewDelegate::MyMTKViewDelegate(MTL::Device *pDevice)
: MTK::ViewDelegate(), _renderer(new Renderer(pDevice)) {
}

MyMTKViewDelegate::~MyMTKViewDelegate() {
    delete _renderer;
}

void MyMTKViewDelegate::drawInMTKView(MTK::View *pView) {
    _renderer->draw(pView);
}




MyAppDelegate::~MyAppDelegate() {
    _mtkView->release();
    _window->release();
    _device->release();

    delete _viewDelegate;
}

NS::Menu *MyAppDelegate::createMenuBar() {
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Menu* mainMenu = NS::Menu::alloc()->init();
    NS::MenuItem* appMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* appMenu = NS::Menu::alloc()->init(NS::String::string("app name", UTF8StringEncoding));

    NS::String* appName = NS::RunningApplication::currentApplication()->localizedName();
    NS::String* quitItemName = NS::String::string("Quit", UTF8StringEncoding) ->stringByAppendingString(appName);
    SEL quitCb = NS::MenuItem::registerActionCallback("appQuit", [](void*, SEL, const NS::Object* pSender){
        auto app = NS::Application::sharedApplication();
        app->terminate(pSender);
    });

    NS::MenuItem* appQuitItem = appMenu->addItem(quitItemName, quitCb, NS::String::string("q", UTF8StringEncoding));
    appQuitItem->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    appMenuItem->setSubmenu(appMenu);

    NS::MenuItem* windowMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* windowMenu = NS::Menu::alloc()->init(NS::String::string("Window", UTF8StringEncoding));

    SEL closeWindowCb = NS::MenuItem::registerActionCallback("windowClose", [](void*, SEL, const NS::Object*){
        auto app = NS::Application::sharedApplication();
        app->windows()->object<NS::Window>(0)->close();
    });
    NS::MenuItem* closeWindowItem = windowMenu->addItem( NS::String::string( "Close Window", UTF8StringEncoding ), closeWindowCb, NS::String::string( "w", UTF8StringEncoding ) );
    closeWindowItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );

    windowMenuItem->setSubmenu(windowMenu);

    mainMenu->addItem(appMenuItem);
    mainMenu->addItem(windowMenuItem);

    appMenuItem->release();
    windowMenuItem->release();
    appMenu->release();
    windowMenu->release();

    return mainMenu->autorelease();
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


