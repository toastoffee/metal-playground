/**
  ******************************************************************************
  * @file           : main.cpp
  * @author         : toastoffee
  * @brief          : None
  * @attention      : None
  * @date           : 2024/10/9
  ******************************************************************************
  */

#include <iostream>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "renderer.hpp"
#include "delegates.hpp"

int main( int argc, char* argv[] )
{

    std::cout << "primitive";

    NS::AutoreleasePool* autoreleasePool = NS::AutoreleasePool::alloc()->init();

    MyAppDelegate appDelegate;

    NS::Application* sharedApplication = NS::Application::sharedApplication();
    sharedApplication->setDelegate(&appDelegate);
    sharedApplication->run();

    autoreleasePool->release();

    return 0;
}