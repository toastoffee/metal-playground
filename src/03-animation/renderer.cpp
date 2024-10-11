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

const int Renderer::kMaxFramesInFlight = 3;

Renderer::Renderer(MTL::Device *device)
: _device(device->retain())
, _angle(0.f)
, _frame(0) {
    _commandQueue = _device->newCommandQueue();

    buildShaders();
    buildBuffers();
    buildFrameData();

    _semaphore = dispatch_semaphore_create(kMaxFramesInFlight);
}

Renderer::~Renderer() {
    _commandQueue->release();
    _device->release();

    _vertexPositionsBuffer->release();
    _vertexColorsBuffer->release();
    _PSO->release();

    _shaderLibrary->release();
    _argBuffer->release();

    for (int i = 0; i < kMaxFramesInFlight; ++i) {
        _frameData[i]->release();
    }
}

void Renderer::draw(MTK::View *view) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    _frame = (_frame + 1) % kMaxFramesInFlight;
    MTL::Buffer* frameDataBuffer = _frameData[_frame];

    MTL::CommandBuffer* cmd = _commandQueue->commandBuffer();
    dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
    cmd->addCompletedHandler(^void(MTL::CommandBuffer* pCmd) {
        dispatch_semaphore_signal(this->_semaphore);
    });

    reinterpret_cast<FrameData *>(frameDataBuffer->contents())->angle = (_angle += 0.01f);
    frameDataBuffer->didModifyRange(NS::Range::Make(0, sizeof(FrameData)));

    MTL::RenderPassDescriptor* rpd = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);

    // add draw calls here
    enc->setRenderPipelineState(_PSO);
    enc->setVertexBuffer(_argBuffer, 0, 0);
    enc->useResource( _vertexPositionsBuffer, MTL::ResourceUsageRead );
    enc->useResource( _vertexColorsBuffer, MTL::ResourceUsageRead );
    enc->setVertexBuffer(frameDataBuffer, 0, 1);

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

        struct FrameData
        {
            float angle;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]], constant FrameData* frameData [[buffer(1)]], uint vertexId [[vertex_id]] )
        {
            float a = frameData->angle;
            float3x3 rotationMatrix = float3x3( sin(a), cos(a), 0.0, cos(a), -sin(a), 0.0, 0.0, 0.0, 1.0 );
            v2f o;
            o.position = float4( rotationMatrix * vertexData->positions[ vertexId ], 1.0 );
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

void Renderer::buildFrameData() {
    for ( int i = 0; i < Renderer::kMaxFramesInFlight; ++i )
    {
        _frameData[ i ]= _device->newBuffer( sizeof( FrameData ), MTL::ResourceStorageModeManaged );
    }
}
