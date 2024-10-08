/**
  ******************************************************************************
  * @file           : renderer.hpp
  * @author         : toastoffee
  * @brief          : None
  * @attention      : None
  * @date           : 2024/10/9
  ******************************************************************************
  */



#ifndef METAL_PLAYGROUND_RENDERER_HPP
#define METAL_PLAYGROUND_RENDERER_HPP

#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

class Renderer {
private:
    MTL::Device* _device;
    MTL::CommandQueue* _commandQueue;

public:
    explicit Renderer(MTL::Device* device);
    ~Renderer();

    void draw(MTK::View* view);
};


#endif //METAL_PLAYGROUND_RENDERER_HPP
