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

#include <simd/simd.h>

struct InstanceData
{
    simd::float4x4 instanceTransform;
    simd::float4 instanceColor;
};

struct CameraData
{
    simd::float4x4 perspectiveTransform;
    simd::float4x4 worldTransform;
};

class Renderer {
private:
    MTL::Device* _device;
    MTL::CommandQueue* _commandQueue;
    MTL::RenderPipelineState* _PSO;
    MTL::Library* _shaderLibrary;

    MTL::Buffer* _vertexDataBuffer;
    MTL::Buffer* _instanceDataBuffer[3];
    MTL::Buffer* _cameraDataBuffer[3];
    MTL::Buffer* _indexBuffer;

    float _angle;
    int _frame;
    dispatch_semaphore_t _semaphore;
    MTL::DepthStencilState* _depthStencilState;

public:
    explicit Renderer(MTL::Device* device);
    ~Renderer();

    void buildShaders();
    void buildBuffers();
    void buildDepthStencilStates();
    void draw(MTK::View* view);
};


#endif //METAL_PLAYGROUND_RENDERER_HPP
