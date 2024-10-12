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
#include "math.hpp"

static constexpr size_t kMaxFramesInFlight = 3;
static constexpr size_t kNumInstances = 32;

Renderer::Renderer(MTL::Device *device)
: _device(device->retain())
, _angle(0.f)
, _frame(0) {

    _commandQueue = _device->newCommandQueue();
    buildShaders();
    buildDepthStencilStates();
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
    _depthStencilState->release();

    for ( int i = 0; i < kMaxFramesInFlight; ++i )
    {
        _instanceDataBuffer[i]->release();
    }
    for ( int i = 0; i < kMaxFramesInFlight; ++i )
    {
        _cameraDataBuffer[i]->release();
    }
}

void Renderer::draw(MTK::View *view) {

    using simd::float3;
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

    float3 objectPosition = { 0.f, 0.f, -5.f };

    float4x4 rt = Math::makeTranslate( objectPosition );
    float4x4 rr = Math::makeYRotate( -_angle );
    float4x4 rtInv = Math::makeTranslate( { -objectPosition.x, -objectPosition.y, -objectPosition.z } );
    float4x4 fullObjectRot = rt * rr * rtInv;

    for ( size_t i = 0; i < kNumInstances; ++i )
    {
        float iDivNumInstances = i / (float)kNumInstances;
        float xoff = (iDivNumInstances * 2.0f - 1.0f) + (1.f/kNumInstances);
        float yoff = sin( ( iDivNumInstances + _angle ) * 2.0f * M_PI);

        // Use the tiny math library to apply a 3D transformation to the instance.
        float4x4 scale = Math::makeScale( (float3){ scl, scl, scl } );
        float4x4 zrot = Math::makeZRotate( _angle );
        float4x4 yrot = Math::makeYRotate( _angle );
        float4x4 translate = Math::makeTranslate( Math::add( objectPosition, { xoff, yoff, 0.f } ) );

        pInstanceData[ i ].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;
        float r = iDivNumInstances;
        float g = 1.0f - r;
        float b = sinf( M_PI * 2.0f * iDivNumInstances );
        pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };
    }
    pInstanceDataBuffer->didModifyRange( NS::Range::Make( 0, pInstanceDataBuffer->length() ) );

    // Update camera state:

    MTL::Buffer* pCameraDataBuffer = _cameraDataBuffer[ _frame ];
    CameraData* pCameraData = reinterpret_cast< CameraData *>( pCameraDataBuffer->contents() );
    pCameraData->perspectiveTransform = Math::makePerspective( 45.f * M_PI / 180.f, 1.f, 0.03f, 500.0f ) ;
    pCameraData->worldTransform = Math::makeIdentity();
    pCameraDataBuffer->didModifyRange( NS::Range::Make( 0, sizeof( CameraData ) ) );

    // begin render pass

    MTL::RenderPassDescriptor* rpd = view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* enc = cmd->renderCommandEncoder(rpd);
    // add draw calls here
    enc->setRenderPipelineState(_PSO);
    enc->setDepthStencilState( _depthStencilState );

    enc->setVertexBuffer(_vertexDataBuffer, 0, 0);
    enc->setVertexBuffer(pInstanceDataBuffer, 0, 1);
    enc->setVertexBuffer( pCameraDataBuffer, /* offset */ 0, /* index */ 2 );

    enc->setCullMode( MTL::CullModeBack );
    enc->setFrontFacingWinding( MTL::Winding::WindingCounterClockwise );

    enc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                 6 * 6, MTL::IndexType::IndexTypeUInt16,
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

        struct CameraData
        {
            float4x4 perspectiveTransform;
            float4x4 worldTransform;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               device const InstanceData* instanceData [[buffer(1)]],
                               device const CameraData& cameraData [[buffer(2)]],
                               uint vertexId [[vertex_id]],
                               uint instanceId [[instance_id]] )
        {
            v2f o;
            float4 pos = float4( vertexData[ vertexId ].position, 1.0 );
            pos = instanceData[ instanceId ].instanceTransform * pos;
            pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
            o.position = pos;
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
    desc->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );

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
            { -s, +s, +s },

            { -s, -s, -s },
            { -s, +s, -s },
            { +s, +s, -s },
            { +s, -s, -s }
    };

    uint16_t indices[] = {
            0, 1, 2, /* front */
            2, 3, 0,

            1, 7, 6, /* right */
            6, 2, 1,

            7, 4, 5, /* back */
            5, 6, 7,

            4, 0, 3, /* left */
            3, 5, 4,

            3, 2, 6, /* top */
            6, 5, 3,

            4, 7, 1, /* bottom */
            1, 0, 4
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

    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i )
    {
        _cameraDataBuffer[ i ] = _device->newBuffer( cameraDataSize, MTL::ResourceStorageModeManaged );
    }
}

void Renderer::buildDepthStencilStates() {
    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDsDesc->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    pDsDesc->setDepthWriteEnabled( true );

    _depthStencilState = _device->newDepthStencilState( pDsDesc );

    pDsDesc->release();
}

