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

static constexpr size_t kMaxFramesInFlight = 3;
static constexpr size_t kNumInstances = 32;

Renderer::Renderer(MTL::Device *device)
: _device(device->retain())
, _angle(0.f)
, _frame(0) {

    _commandQueue = _device->newCommandQueue();
    buildShaders();
    buildBuffers();
    _semaphore = dispatch_semaphore_create(kMaxFramesInFlight);
}

Renderer::~Renderer() {
    _commandQueue->release();
    _device->release();
    _PSO->release();

    _shaderLibrary->release();
    _vertexDataBuffer->release();
    _indexBuffer->release();

    for(int i = 0; i < kMaxFramesInFlight; ++i) {
        _instanceDataBuffer[i]->release();
    }
}

void Renderer::draw(MTK::View *view) {

    using simd::float4;
    using simd::float4x4;

    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    _frame = (_frame + 1) % kMaxFramesInFlight;
    MTL::Buffer* pInstanceDataBuffer = _instanceDataBuffer[ _frame ];

    MTL::CommandBuffer* cmd = _commandQueue->commandBuffer();
    dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
    cmd->addCompletedHandler(^void(MTL::CommandBuffer* pCmd) {
        dispatch_semaphore_signal(this->_semaphore);
    });

    _angle += 0.01f;

    const float scl = 0.1f;

    InstanceData* pInstanceData = reinterpret_cast<InstanceData *>(pInstanceDataBuffer->contents());
    for ( size_t i = 0; i < kNumInstances; ++i )
    {
        float iDivNumInstances = i / (float)kNumInstances;
        float xoff = (iDivNumInstances * 2.0f - 1.0f) + (1.f/kNumInstances);
        float yoff = sin( ( iDivNumInstances + _angle ) * 2.0f * M_PI);
        pInstanceData[ i ].instanceTransform = (float4x4){ (float4){ scl * sinf(_angle), scl * cosf(_angle), 0.f, 0.f },
                                                           (float4){ scl * cosf(_angle), scl * -sinf(_angle), 0.f, 0.f },
                                                           (float4){ 0.f, 0.f, scl, 0.f },
                                                           (float4){ xoff, yoff, 0.f, 1.f } };

        float r = iDivNumInstances;
        float g = 1.0f - r;
        float b = sinf( M_PI * 2.0f * iDivNumInstances );
        pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };
    }
    pInstanceDataBuffer->didModifyRange( NS::Range::Make( 0, pInstanceDataBuffer->length() ) );

    MTL::RenderPassDescriptor* rpd = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);

    // add draw calls here
    enc->setRenderPipelineState(_PSO);
    enc->setVertexBuffer(_vertexDataBuffer, 0, 0);
    enc->setVertexBuffer(pInstanceDataBuffer, 0, 1);

    enc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                 6, MTL::IndexType::IndexTypeUInt16,
                                 _indexBuffer,
                                 0,
                                 kNumInstances );

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
            float3 position;
        };

        struct InstanceData
        {
            float4x4 instanceTransform;
            float4 instanceColor;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               device const InstanceData* instanceData [[buffer(1)]],
                               uint vertexId [[vertex_id]],
                               uint instanceId [[instance_id]] )
        {
            v2f o;
            float4 pos = float4( vertexData[ vertexId ].position, 1.0 );
            o.position = instanceData[ instanceId ].instanceTransform * pos;
            o.color = half3( instanceData[ instanceId ].instanceColor.rgb );
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

    using simd::float3;

    const float s = 0.5f;

    float3 verts[] = {
            { -s, -s, +s },
            { +s, -s, +s },
            { +s, +s, +s },
            { -s, +s, +s }
    };

    uint16_t indices[] = {
            0, 1, 2,
            2, 3, 0,
    };

    const size_t vertexDataSize = sizeof( verts );
    const size_t indexDataSize = sizeof( indices );

    MTL::Buffer* pVertexBuffer = _device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pIndexBuffer = _device->newBuffer( indexDataSize, MTL::ResourceStorageModeManaged );

    _vertexDataBuffer = pVertexBuffer;
    _indexBuffer = pIndexBuffer;

    memcpy( _vertexDataBuffer->contents(), verts, vertexDataSize );
    memcpy( _indexBuffer->contents(), indices, indexDataSize );

    _vertexDataBuffer->didModifyRange( NS::Range::Make( 0, _vertexDataBuffer->length() ) );
    _indexBuffer->didModifyRange( NS::Range::Make( 0, _indexBuffer->length() ) );

    using NS::StringEncoding::UTF8StringEncoding;
    assert( _shaderLibrary );

    const size_t instanceDataSize = kMaxFramesInFlight * kNumInstances * sizeof(InstanceData);

    for (size_t i = 0; i < kMaxFramesInFlight; ++i) {
        _instanceDataBuffer[i] = _device->newBuffer(instanceDataSize, MTL::ResourceStorageModeManaged);
    }
}

