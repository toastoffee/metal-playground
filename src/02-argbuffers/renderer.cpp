/**
  ******************************************************************************
  * @file           : renderer.cpp
  * @author         : toastoffee
  * @brief          : None
  * @attention      : None
  * @date           : 2024/10/9
  ******************************************************************************
  */



#include "renderer.hpp"

Renderer::Renderer(MTL::Device *device)
: _device(device->retain()) {
    _commandQueue = _device->newCommandQueue();

    buildShaders();
    buildBuffers();
}

Renderer::~Renderer() {
    _commandQueue->release();
    _device->release();

    _vertexPositionsBuffer->release();
    _vertexColorsBuffer->release();
    _PSO->release();

    _shaderLibrary->release();
    _argBuffer->release();
}

void Renderer::draw(MTK::View *view) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* cmd = _commandQueue->commandBuffer();
    MTL::RenderPassDescriptor* rpd = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);

    // add draw calls here
    enc->setRenderPipelineState(_PSO);
    enc->setVertexBuffer(_argBuffer, 0, 0);
    enc->useResource( _vertexPositionsBuffer, MTL::ResourceUsageRead );
    enc->useResource( _vertexColorsBuffer, MTL::ResourceUsageRead );

    enc->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3));

    enc->endEncoding();
    cmd->presentDrawable(view->currentDrawable());
    cmd->commit();

    pool->release();
}

void Renderer::buildShaders() {
    using NS::StringEncoding::UTF8StringEncoding;

    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
            half3 color;
        };

        struct VertexData
        {
            device float3* positions [[id(0)]];
            device float3* colors [[id(1)]];
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]], uint vertexId [[vertex_id]] )
        {
            v2f o;
            o.position = float4( vertexData->positions[ vertexId ], 1.0 );
            o.color = half3(vertexData->colors[ vertexId ]);
            return o;
        }

        half4 fragment fragmentMain( v2f in [[stage_in]] )
        {
            return half4( in.color, 1.0 );
        }
    )";

    NS::Error* error = nullptr;
    MTL::Library* library = _device->newLibrary(NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &error);
    if(!library) {
        __builtin_printf("%s", error->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* vertexFn = library->newFunction(NS::String::string("vertexMain", UTF8StringEncoding));
    MTL::Function* fragFn = library->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

    MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
    desc->setVertexFunction(vertexFn);
    desc->setFragmentFunction(fragFn);
    desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm_sRGB);

    _PSO = _device->newRenderPipelineState(desc, &error);
    if(!_PSO) {
        __builtin_printf( "%s", error->localizedDescription()->utf8String() );
        assert( false );
    }

    vertexFn->release();
    fragFn->release();
    desc->release();

    _shaderLibrary = library;
}

void Renderer::buildBuffers() {
    const size_t NumVertices = 3;

    simd::float3 positions[NumVertices] =
    {
        { -0.8f,  0.8f, 0.0f },
        {  0.0f, -0.8f, 0.0f },
        { +0.8f,  0.8f, 0.0f }
    };

    simd::float3 colors[NumVertices] =
    {
        {  1.0, 0.0f, 0.0f },
        {  0.0f, 1.0, 0.0f },
        {  0.0f, 0.0f, 1.0 }
    };

    const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
    const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

    MTL::Buffer* pVertexPositionsBuffer = _device->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pVertexColorsBuffer = _device->newBuffer( colorDataSize, MTL::ResourceStorageModeManaged );

    _vertexPositionsBuffer = pVertexPositionsBuffer;
    _vertexColorsBuffer = pVertexColorsBuffer;

    memcpy( _vertexPositionsBuffer->contents(), positions, positionsDataSize );
    memcpy( _vertexColorsBuffer->contents(), colors, colorDataSize );

    _vertexPositionsBuffer->didModifyRange( NS::Range::Make( 0, _vertexPositionsBuffer->length() ) );
    _vertexColorsBuffer->didModifyRange( NS::Range::Make( 0, _vertexColorsBuffer->length() ) );

    using NS::StringEncoding::UTF8StringEncoding;
    assert( _shaderLibrary );

    MTL::Function* vertexFn = _shaderLibrary->newFunction( NS::String::string( "vertexMain", UTF8StringEncoding ) );
    MTL::ArgumentEncoder* argumentEncoder = vertexFn->newArgumentEncoder(0);

    MTL::Buffer* argBuffer = _device->newBuffer(argumentEncoder->encodedLength(), MTL::ResourceStorageModeManaged);
    _argBuffer = argBuffer;

    argumentEncoder->setArgumentBuffer(_argBuffer, 0);

    argumentEncoder->setBuffer( _vertexPositionsBuffer, 0, 0 );
    argumentEncoder->setBuffer( _vertexColorsBuffer, 0, 1 );

    _argBuffer->didModifyRange( NS::Range::Make( 0, _argBuffer->length() ) );

    vertexFn->release();
    argumentEncoder->release();
}
