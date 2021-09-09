//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "D3D12Multithreading.h"
#include "FrameResource.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#define IMGUI_ENABLED

D3D12Multithreading* D3D12Multithreading::s_app = nullptr;

D3D12Multithreading::D3D12Multithreading(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_keyboardInput(),
    m_titleCount(0),
    m_cpuTime(0),
    m_fenceValue(0),
    m_rtvDescriptorSize(0),
    m_currentFrameResourceIndex(0),
    m_pCurrentFrameResource(nullptr)
{
    s_app = this;

    m_keyboardInput.animate = true;

    ThrowIfFailed(DXGIDeclareAdapterRemovalSupport());	

	// Read Processor Info at Startup
	GetProcessorInfo(m_procInfo);
    BOOL disablePriority = false;

    HANDLE process = GetCurrentProcess();

    SetPriorityClass(process, REALTIME_PRIORITY_CLASS);

    // Doesn't Appear to Work...
    SetProcessPriorityBoost(process, disablePriority);

    // TODO: Consider using Thread Priorities
    // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadpriority
}

D3D12Multithreading::~D3D12Multithreading()
{
    s_app = nullptr;
}

void D3D12Multithreading::LoadImGui()
{
#ifdef IMGUI_ENABLED
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpu(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpu(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos + ImGuiBackendFlags_HasSetMousePos;  // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();

    const UINT cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(Win32Application::GetHwnd());
    ImGui_ImplDX12_Init(m_device.Get(), FrameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        cpu.Offset(0, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
        gpu.Offset(0, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
#endif //IMGUI_ENABLED
}

void D3D12Multithreading::OnInit()
{
    LoadPipeline();

    LoadAssets();

    LoadContexts();

    LoadImGui();
}


// Load the rendering pipeline dependencies.
void D3D12Multithreading::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
    NAME_D3D12_OBJECT(m_commandQueue);

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        // Describe and create a depth stencil view (DSV) descriptor heap.
        // Each frame has its own depth stencils (to write shadows onto) 
        // and then there is one for the scene itself.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1 + FrameCount * 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

        // Describe and create a shader resource view (SRV) and constant 
        // buffer view (CBV) descriptor heap.  Heap layout: null views, 
        // object diffuse + normal textures views, frame 1's shadow buffer, 
        // frame 1's 2x constant buffer, frame 2's shadow buffer, frame 2's 
        // 2x constant buffers, etc...
        const UINT nullSrvCount = 2;        // Null descriptors are needed for out of bounds behavior reads.
        const UINT cbvCount = FrameCount * 2;
        const UINT srvCount = _countof(SampleAssets::Textures) + (FrameCount * 1);
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
        cbvSrvHeapDesc.NumDescriptors = nullSrvCount + cbvCount + srvCount;
        cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));
        NAME_D3D12_OBJECT(m_cbvSrvHeap);

        // Describe and create a sampler descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = 2;        // One clamp and one wrap sampler.
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));
        NAME_D3D12_OBJECT(m_samplerHeap);

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// Load the sample assets.
void D3D12Multithreading::LoadAssets()
{
    // Create the root signature.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[4]; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 2 frequently changed diffuse + normal textures - using registers t1 and t2.
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 1 frequently changed constant buffer.
        ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);                                                // 1 infrequently changed shadow texture - starting in register t0.
        ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);                                            // 2 static samplers.

        CD3DX12_ROOT_PARAMETER1 rootParameters[4];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        NAME_D3D12_OBJECT(m_rootSignature);
    }

    // Create the pipeline state, which includes loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
        inputLayoutDesc.pInputElementDescs = SampleAssets::StandardVertexDescription;
        inputLayoutDesc.NumElements = _countof(SampleAssets::StandardVertexDescription);

        CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depthStencilDesc.StencilEnable = FALSE;

        // Describe and create the PSO for rendering the scene.
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = depthStencilDesc;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
        NAME_D3D12_OBJECT(m_pipelineState);

        // Alter the description and create the PSO for rendering
        // the shadow map.  The shadow map does not use a pixel
        // shader or render targets.
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(0, 0);
        psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
        psoDesc.NumRenderTargets = 0;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateShadowMap)));
        NAME_D3D12_OBJECT(m_pipelineStateShadowMap);
    }

    // Create temporary command list for initial GPU setup.
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&commandList)));

    // Create render target views (RTVs).
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < FrameCount; i++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);

        NAME_D3D12_OBJECT_INDEXED(m_renderTargets, i);
    }

    // Create the depth stencil.
    {
        CD3DX12_RESOURCE_DESC shadowTextureDesc(
            D3D12_RESOURCE_DIMENSION_TEXTURE2D,
            0,
            static_cast<UINT>(m_viewport.Width), 
            static_cast<UINT>(m_viewport.Height), 
            1,
            1,
            DXGI_FORMAT_D32_FLOAT,
            1, 
            0,
            D3D12_TEXTURE_LAYOUT_UNKNOWN,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

        D3D12_CLEAR_VALUE clearValue;    // Performance tip: Tell the runtime at resource creation the desired clear value.
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &shadowTextureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&m_depthStencil)));

        NAME_D3D12_OBJECT(m_depthStencil);

        // Create the depth stencil view.
        m_device->CreateDepthStencilView(m_depthStencil.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // Load scene assets.
    UINT fileSize = 0;
    UINT8* pAssetData;
    ThrowIfFailed(ReadDataFromFile(GetAssetFullPath(SampleAssets::DataFileName).c_str(), &pAssetData, &fileSize));

    // Create the vertex buffer.
    {
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        NAME_D3D12_OBJECT(m_vertexBuffer);

        {
            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBufferUpload)));

            // Copy data to the upload heap and then schedule a copy 
            // from the upload heap to the vertex buffer.
            D3D12_SUBRESOURCE_DATA vertexData = {};
            vertexData.pData = pAssetData + SampleAssets::VertexDataOffset;
            vertexData.RowPitch = SampleAssets::VertexDataSize;
            vertexData.SlicePitch = vertexData.RowPitch;

            PIXBeginEvent(commandList.Get(), 0, L"Copy vertex buffer data to default resource...");

            UpdateSubresources<1>(commandList.Get(), m_vertexBuffer.Get(), m_vertexBufferUpload.Get(), 0, 0, 1, &vertexData);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

            PIXEndEvent(commandList.Get());
        }

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.SizeInBytes = SampleAssets::VertexDataSize;
        m_vertexBufferView.StrideInBytes = SampleAssets::StandardVertexStride;
    }

    // Create the index buffer.
    {
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)));

        NAME_D3D12_OBJECT(m_indexBuffer);

        {
            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_indexBufferUpload)));

            // Copy data to the upload heap and then schedule a copy 
            // from the upload heap to the index buffer.
            D3D12_SUBRESOURCE_DATA indexData = {};
            indexData.pData = pAssetData + SampleAssets::IndexDataOffset;
            indexData.RowPitch = SampleAssets::IndexDataSize;
            indexData.SlicePitch = indexData.RowPitch;

            PIXBeginEvent(commandList.Get(), 0, L"Copy index buffer data to default resource...");

            UpdateSubresources<1>(commandList.Get(), m_indexBuffer.Get(), m_indexBufferUpload.Get(), 0, 0, 1, &indexData);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

            PIXEndEvent(commandList.Get());
        }

        // Initialize the index buffer view.
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = SampleAssets::IndexDataSize;
        m_indexBufferView.Format = SampleAssets::StandardIndexFormat;
    }

    // Create shader resources.
    {
        // Get the CBV SRV descriptor size for the current device.
        const UINT cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Get a handle to the start of the descriptor heap.
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());

        {
            // Describe and create 2 null SRVs. Null descriptors are needed in order 
            // to achieve the effect of an "unbound" resource.
            D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
            nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            nullSrvDesc.Texture2D.MipLevels = 1;
            nullSrvDesc.Texture2D.MostDetailedMip = 0;
            nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

            m_device->CreateShaderResourceView(nullptr, &nullSrvDesc, cbvSrvHandle);
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);

            m_device->CreateShaderResourceView(nullptr, &nullSrvDesc, cbvSrvHandle);
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);
        }

        // Create each texture and SRV descriptor.
        const UINT srvCount = _countof(SampleAssets::Textures);
        PIXBeginEvent(commandList.Get(), 0, L"Copy diffuse and normal texture data to default resources...");
        for (UINT i = 0; i < srvCount; i++)
        {
            // Describe and create a Texture2D.
            const SampleAssets::TextureResource &tex = SampleAssets::Textures[i];
            CD3DX12_RESOURCE_DESC texDesc(
                D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                0,
                tex.Width, 
                tex.Height, 
                1,
                static_cast<UINT16>(tex.MipLevels),
                tex.Format,
                1, 
                0,
                D3D12_TEXTURE_LAYOUT_UNKNOWN,
                D3D12_RESOURCE_FLAG_NONE);

            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_textures[i])));

            NAME_D3D12_OBJECT_INDEXED(m_textures, i);

            {
                const UINT subresourceCount = texDesc.DepthOrArraySize * texDesc.MipLevels;
                UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textures[i].Get(), 0, subresourceCount);
                ThrowIfFailed(m_device->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                    D3D12_HEAP_FLAG_NONE,
                    &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_textureUploads[i])));

                // Copy data to the intermediate upload heap and then schedule a copy
                // from the upload heap to the Texture2D.
                D3D12_SUBRESOURCE_DATA textureData = {};
                textureData.pData = pAssetData + tex.Data->Offset;
                textureData.RowPitch = tex.Data->Pitch;
                textureData.SlicePitch = tex.Data->Size;

                UpdateSubresources(commandList.Get(), m_textures[i].Get(), m_textureUploads[i].Get(), 0, 0, subresourceCount, &textureData);
                commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_textures[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
            }

            // Describe and create an SRV.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = tex.Format;
            srvDesc.Texture2D.MipLevels = tex.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            m_device->CreateShaderResourceView(m_textures[i].Get(), &srvDesc, cbvSrvHandle);

            // Move to the next descriptor slot.
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);
        }
        PIXEndEvent(commandList.Get());
    }

    free(pAssetData);

    // Create the samplers.
    {
        // Get the sampler descriptor size for the current device.
        const UINT samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        // Get a handle to the start of the descriptor heap.
        CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(m_samplerHeap->GetCPUDescriptorHandleForHeapStart());

        // Describe and create the wrapping sampler, which is used for 
        // sampling diffuse/normal maps.
        D3D12_SAMPLER_DESC wrapSamplerDesc = {};
        wrapSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        wrapSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        wrapSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        wrapSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        wrapSamplerDesc.MinLOD = 0;
        wrapSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        wrapSamplerDesc.MipLODBias = 0.0f;
        wrapSamplerDesc.MaxAnisotropy = 1;
        wrapSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        wrapSamplerDesc.BorderColor[0] = wrapSamplerDesc.BorderColor[1] = wrapSamplerDesc.BorderColor[2] = wrapSamplerDesc.BorderColor[3] = 0;
        m_device->CreateSampler(&wrapSamplerDesc, samplerHandle);

        // Move the handle to the next slot in the descriptor heap.
        samplerHandle.Offset(samplerDescriptorSize);

        // Describe and create the point clamping sampler, which is 
        // used for the shadow map.
        D3D12_SAMPLER_DESC clampSamplerDesc = {};
        clampSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        clampSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        clampSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        clampSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        clampSamplerDesc.MipLODBias = 0.0f;
        clampSamplerDesc.MaxAnisotropy = 1;
        clampSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        clampSamplerDesc.BorderColor[0] = clampSamplerDesc.BorderColor[1] = clampSamplerDesc.BorderColor[2] = clampSamplerDesc.BorderColor[3] = 0;
        clampSamplerDesc.MinLOD = 0;
        clampSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        m_device->CreateSampler(&clampSamplerDesc, samplerHandle);
    }

	// Let there be light
	RunOn(m_procInfo, CoreTypes::INTEL_ATOM);
    for (int i = 0; i < NumLights; i++)
    {
        // Set up each of the light positions and directions (they all start 
        // in the same place).
        m_lights[i].position = { 0.0f, 15.0f, -30.0f, 1.0f };
        m_lights[i].direction = { 0.0, 0.0f, 1.0f, 0.0f };
        m_lights[i].falloff = { 800.0f, 1.0f, 0.0f, 1.0f };
        m_lights[i].color = { 0.7f, 0.7f, 0.7f, 1.0f };

        XMVECTOR eye = XMLoadFloat4(&m_lights[i].position);
        XMVECTOR at = XMVectorAdd(eye, XMLoadFloat4(&m_lights[i].direction));
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        m_lightCameras[i].Set(eye, at, up);
    }
	RunOn(m_procInfo, CoreTypes::ANY);

    // Close the command list and use it to execute the initial GPU setup.
    ThrowIfFailed(commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create frame resources.
    for (int i = 0; i < FrameCount; i++)
    {
        m_frameResources[i] = new FrameResource(m_device.Get(), m_pipelineState.Get(), m_pipelineStateShadowMap.Get(), m_dsvHeap.Get(), m_cbvSrvHeap.Get(), &m_viewport, i);
        m_frameResources[i]->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);
    }
    m_currentFrameResourceIndex = 0;
    m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.

        // Signal and increment the fence value.
        const UINT64 fenceToWaitFor = m_fenceValue;
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
        m_fenceValue++;

        // Wait until the fence is completed.
        ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

// Initialize threads and events.
void D3D12Multithreading::LoadContexts()
{
#if !SINGLETHREADED
    struct threadwrapper
    {
        static unsigned int WINAPI thunk(LPVOID lpParameter)
        {
            ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
            D3D12Multithreading::Get()->WorkerThread(parameter->threadIndex);
            return 0;
        }
    };

    for (int i = 0; i < NumContexts; i++)
    {
        m_workerBeginRenderFrame[i] = CreateEvent(
            NULL,
            FALSE,
            FALSE,
            NULL);

        m_workerFinishedRenderFrame[i] = CreateEvent(
            NULL,
            FALSE,
            FALSE,
            NULL);

        m_workerFinishShadowPass[i] = CreateEvent(
            NULL,
            FALSE,
            FALSE,
            NULL);

        m_threadParameters[i].threadIndex = i;

        m_threadHandles[i] = reinterpret_cast<HANDLE>(_beginthreadex(
            nullptr,
            0,
            threadwrapper::thunk,
            reinterpret_cast<LPVOID>(&m_threadParameters[i]),
            0,
            nullptr));

        assert(m_workerBeginRenderFrame[i] != NULL);
        assert(m_workerFinishedRenderFrame[i] != NULL);
        assert(m_threadHandles[i] != NULL);

    }
#endif
}

// Update frame-based values.
void D3D12Multithreading::OnUpdate()
{
    m_timer.Tick(NULL);

    PIXSetMarker(m_commandQueue.Get(), 0, L"Getting last completed fence.");

    // Get current GPU progress against submitted workload. Resources still scheduled 
    // for GPU execution cannot be modified or else undefined behavior will result.
    const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

    // Move to the next frame resource.
    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % FrameCount;
    m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];

    // Make sure that this frame resource isn't still in use by the GPU.
    // If it is, wait for it to complete.
    if (m_pCurrentFrameResource->m_fenceValue > lastCompletedFence)
    {
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (eventHandle == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_pCurrentFrameResource->m_fenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    m_cpuTimer.Tick(NULL);
    float frameTime = static_cast<float>(m_timer.GetElapsedSeconds());
    float frameChange = 2.0f * frameTime;

    if (m_keyboardInput.leftArrowPressed)
        m_camera.RotateYaw(-frameChange);
    if (m_keyboardInput.rightArrowPressed)
        m_camera.RotateYaw(frameChange);
    if (m_keyboardInput.upArrowPressed)
        m_camera.RotatePitch(frameChange);
    if (m_keyboardInput.downArrowPressed)
        m_camera.RotatePitch(-frameChange);

    if (m_keyboardInput.animate)
    {
        for (int i = 0; i < NumLights; i++)
        {
            float direction = frameChange * pow(-1.0f, i);
            XMStoreFloat4(&m_lights[i].position, XMVector4Transform(XMLoadFloat4(&m_lights[i].position), XMMatrixRotationY(direction)));

            XMVECTOR eye = XMLoadFloat4(&m_lights[i].position);
            XMVECTOR at = XMVectorSet(0.0f, 8.0f, 0.0f, 0.0f);
            XMStoreFloat4(&m_lights[i].direction, XMVector3Normalize(XMVectorSubtract(at, eye)));
            XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            m_lightCameras[i].Set(eye, at, up);

            m_lightCameras[i].Get3DViewProjMatrices(&m_lights[i].view, &m_lights[i].projection, 90.0f, static_cast<float>(m_width), static_cast<float>(m_height));
        }
    }

    m_pCurrentFrameResource->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);

#ifdef IMGUI_ENABLED
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //ImGuiIO& io = ImGui::GetIO();
    //io.WantCaptureMouse = true;
    ImVec2 v(0, 0);
    ImVec2 s(300.0f, static_cast<float>(m_height));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);

    BOOL priorityBoost;

    GetProcessPriorityBoost(GetCurrentProcess(), &priorityBoost);

#ifdef ENABLE_HYBRID_DETECT
    // Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    ImGui::Begin("Package", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse); // Create a window called "Hello, world!" and append into it.
    ImGui::SetWindowPos(v);
    ImGui::SetWindowSize(s);

    // Create a color picker for scene background color.
    //ImGui::ColorEdit3("Clear Color", new float[3]{ 1.0f, 0.0f, 0.0f });

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), &m_procInfo.brandString[0]);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Vendor                  %s" ,&m_procInfo.vendorID[0]);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Is It Intel             %s", (m_procInfo.IsIntel() ? "Yes" : "No"));
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Is It Hybrid            %s", m_procInfo.hybrid ? "Yes" : "No");
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Is Hybrid OS            %s", IsHybridOSEx() ? "Yes" : "No");
#ifdef ENABLE_SOFTWARE_PROXY
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Software Proxy          %s", "Yes");
#else
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Software Proxy          %s", "No");
#endif
#ifdef ENABLE_RUNON
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Run On Enabled          %s", "Yes");
#else
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Run On Enabled          %s", "No");
#endif
#ifdef ENABLE_RUNON_PRIORITY
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Run On Priority Enabled %s", "Yes");
#else
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Run On Priority Enabled %s", "No");
#endif
#ifdef ENABLE_CPU_SETS
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "CPU Sets Enabled        %s", "Yes");
#else
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "CPU Sets Enabled        %s", "No");
#endif
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Turbo Boost             %s", m_procInfo.turboBoost ? "Yes" : "No");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Turbo Boost 3.0         %s", m_procInfo.turboBoost3_0 ? "Yes" : "No");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Priority Boost          %s", priorityBoost ? "Yes" : "No");
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Groups                  %d", m_procInfo.numGroups);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "NUMA Nodes              %d", m_procInfo.numNUMANodes);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Packages                %d", m_procInfo.numProcessorPackages);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Physical Cores          %d", m_procInfo.numPhysicalCores);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Logical Cores           %d", m_procInfo.numLogicalCores);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "L1 Caches               %d", m_procInfo.numL1Caches);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "L2 Caches               %d", m_procInfo.numL2Caches);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "L3 Caches               %d", m_procInfo.numL3Caches);

    ImGui::Checkbox("Show Cores", &m_showCores);
    ImGui::Checkbox("Show Caches", &m_showCaches);
    ImGui::Checkbox("Show Masks", &m_showMasks);
    ImGui::Checkbox("Spark Lines", &m_showSparklines);
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Groups");
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Group | Core Count | Max Cores |");
    ImGui::Separator();
    int groupID = 0;
    for (auto group : m_procInfo.groups) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "   %2d |         %2d |        %2d |", groupID++, group.activeProcessorCount, group.maximumProcessorCount);
    }
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "NUMA Nodes");
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Group | NUMA Node |");
    ImGui::Separator();
    for (auto node : m_procInfo.nodes) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "   %2d |        %2d |", node.group, node.nodeNumber);
    }
    ImGui::Separator();
            
    //ImGui::Text("Frametime: %.3f ms/frame (%.1f FPS)", m_totalFrameTime / m_currentFrame, 1000.0f / (m_totalFrameTime / m_currentFrame));

    ImGui::End();

    static bool showCore = false;
    static bool showCache = false;

    static unsigned showCoreID = 0;
    static unsigned showCacheID = 0;

    if (m_showCores)
    {
        static int selectedSet = 0;
        static int coreTypes[] = { ANY, NONE, RESERVED0, INTEL_ATOM, RESERVED1, INTEL_CORE };
        char* items[6] = { "Any", "None", "Reserved 0", "Atom", "Reserved 1", "Core" };

        UpdateProcessorInfo(m_procInfo);

        v = ImVec2(300, 0);

        if (m_showCaches)
            s = ImVec2(m_width - 300 - 575, static_cast<float>(m_height / 2));
        else
            s = ImVec2(m_width - 300 - 575, static_cast<float>(m_height));
        ImGui::Begin("Logical Processors", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowPos(v);
        ImGui::SetWindowSize(s);
        ImGui::Combo("CPU Set Filter", &selectedSet, items, 6);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Type  |  ID  | Grp | Idx | Core | Freq.   | Details");
        ImGui::Separator();
        for (int i = 0; i < m_procInfo.cores.size(); i++)
        {
            LOGICAL_PROCESSOR_INFO& core = m_procInfo.cores[i];

#ifdef ENABLE_CPU_SETS
            std::vector<ULONG> set = m_procInfo.cpuSets[coreTypes[selectedSet]];
            if (std::find(set.begin(), set.end(), core.id) == set.end()) continue;
#else
            if (!(core.processorMask.to_ulong() & m_procInfo.coreMasks[coreTypes[selectedSet]])) continue;
#endif

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s  | %4d |  %2d |  %2d |   %2d | %2.2fgHz |",
				CoreTypeString(core.coreType), core.id, core.group, core.logicalProcessorIndex, core.coreIndex, float(core.currentFrequency) / 1000.0f);
            ImGui::SameLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

            char buf[10];
            sprintf(buf, "Show%d", i);
            if (ImGui::Button(buf, ImVec2(60, 15))) {
                showCore = true;
                showCoreID = i;
            }
            ImGui::PopStyleVar();
        }
        ImGui::Separator();

        if (m_showSparklines)
        {
            static std::map<int, std::vector<float>> samples;

            for (int i = 0; i < m_procInfo.cores.size(); i++)
            {
#ifdef ENABLE_CPU_SETS
                std::vector<ULONG> set = m_procInfo.cpuSets[coreTypes[selectedSet]];
                if (std::find(set.begin(), set.end(), m_procInfo.cores[i].id) == set.end()) continue;
#else
                if (!(m_procInfo.cores[i].processorMask.to_ulong() & m_procInfo.coreMasks[coreTypes[selectedSet]])) continue;
#endif
                static std::vector<float> arr = samples[i];
                arr.push_back(float(m_procInfo.cores[i].currentFrequency) / 1000.0f);
                if (arr.size() > 30) arr.erase(arr.begin());
                char buf[10]; sprintf(buf, "%d", i);
                ImGui::PlotLines(buf, &arr[0], arr.size(), 0, "", 1.0f, float(m_procInfo.cores[i].maximumFrequency) / 1000.0f, ImVec2(m_width - 300 - 575 - 30, 20));
            }
        }
      
        ImGui::End();
    }

    if (m_showCaches)
    {
        if (!m_showCores) {
            v = ImVec2(300, 0);
            s = ImVec2(m_width - 300 - 575, static_cast<float>(m_height));
        } else {
            v = ImVec2(300, static_cast<float>(m_height / 2));
            s = ImVec2(m_width - 300 - 575, static_cast<float>(m_height / 2));
        }
        
        ImGui::Begin("Caches", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowPos(v);
        ImGui::SetWindowSize(s);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "ID | Type        | LvL | Size     | Line Size | Details");
        ImGui::Separator();
        for (int i = 0; i < m_procInfo.caches.size(); i++)
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%2d | %-11.11s |  %d  | %5d kb | %3d bytes |", 
                i, CacheTypeString(m_procInfo.caches[i].type),  
                m_procInfo.caches[i].level, 
                m_procInfo.caches[i].size / 1024, 
                m_procInfo.caches[i].lineSize);
            ImGui::SameLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            char buf[10];
            sprintf(buf, "Show(%2d)", i);
            if (ImGui::Button(buf, ImVec2(55, 15)))
            {
                showCache = true;
                showCacheID = i;
            }
            ImGui::PopStyleVar();
        }
        ImGui::Separator();
        ImGui::End();
    }

    if (m_showMasks)
    {
        DWORD_PTR           processAffinityMask;
        DWORD_PTR           systemAffinityMask;
        GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);

        if(!m_showCores && !m_showCaches)
            v = ImVec2(300, 0);
        else
            v = ImVec2(300 + (m_width - 300 - 575), 0);

        s = ImVec2(575, static_cast<float>(m_height));
        ImGui::Begin("Masks", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowPos(v);
        ImGui::SetWindowSize(s);

        ImGui::Separator(); 
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Mask        | Bit Mask");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "System      | %s", std::bitset<64>(systemAffinityMask).to_string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Process     | %s", std::bitset<64>(processAffinityMask).to_string().c_str());
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "None        | %s", std::bitset<64>(m_procInfo.coreMasks[CoreTypes::NONE]).to_string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Any         | %s", std::bitset<64>(m_procInfo.coreMasks[CoreTypes::ANY]).to_string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Core        | %s", std::bitset<64>(m_procInfo.coreMasks[CoreTypes::INTEL_CORE]).to_string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Atom        | %s", std::bitset<64>(m_procInfo.coreMasks[CoreTypes::INTEL_ATOM]).to_string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Reserved 0  | %s", std::bitset<64>(m_procInfo.coreMasks[CoreTypes::RESERVED0]).to_string().c_str());
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Reserved 1  | %s", std::bitset<64>(m_procInfo.coreMasks[CoreTypes::RESERVED1]).to_string().c_str());
        ImGui::Separator();

        for (int i = 0; i < m_procInfo.groups.size(); i++)
        {
            if(i == 0)  ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Group( 0)   | %s", std::bitset<64>(m_procInfo.groups[i].activeProcessorMask).to_string().c_str());
            else        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Group(%2.0d)   | %s", i, std::bitset<64>(m_procInfo.groups[i].activeProcessorMask).to_string().c_str());
        }

        ImGui::Separator();
        for (int i = 0; i < m_procInfo.nodes.size(); i++)
        {
            char buf[3] = "";
            if (m_procInfo.nodes[i].group)  sprintf(buf, "%2d", m_procInfo.nodes[i].group);
            else                            sprintf(buf, " 0");
            if (i == 0) ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Node (%s/ 0)| %s", buf, std::bitset<64>(m_procInfo.nodes[i].mask).to_string().c_str());
            else        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Node (%s/%2.0d)| %s", buf, m_procInfo.nodes[i].nodeNumber, std::bitset<64>(m_procInfo.groups[i].activeProcessorMask).to_string().c_str());
        }

        ImGui::Separator();
        for (int i = 0; i < m_procInfo.cores.size(); i++)
        {
            char buf[3] = "";
            if (m_procInfo.cores[i].group)  sprintf(buf, "%2d", m_procInfo.nodes[i].group);
            else                            sprintf(buf, " 0");
            if (i == 0) ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Core (%s/ 0)| %s", buf, std::bitset<64>(m_procInfo.cores[i].processorMask).to_string().c_str());
            else        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Core (%s/%2.0d)| %s", buf, m_procInfo.cores[i].logicalProcessorIndex, std::bitset<64>(m_procInfo.cores[i].processorMask).to_string().c_str());
        }

        ImGui::Separator();
        for (int i = 0; i < m_procInfo.caches.size(); i++)
        {
            char buf[3] = "";
            if (m_procInfo.caches[i].group) sprintf(buf, "%2d", m_procInfo.caches[i].group);
            else                            sprintf(buf, " 0");
            if(i == 0) ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Cache(%s/ 0)| %s", buf, std::bitset<64>(m_procInfo.caches[i].processorMask).to_string().c_str());
            else       ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Cache(%s/%2.0d)| %s", buf, i, std::bitset<64>(m_procInfo.caches[i].processorMask).to_string().c_str());
        }

        ImGui::End();
    }

    if (showCore && m_showCores)
    {
        LOGICAL_PROCESSOR_INFO core = m_procInfo.cores[showCoreID];

        v = ImVec2(300 + (m_width - 300 - 575), 0);
        s = ImVec2(300, static_cast<float>(m_height));
        ImGui::Begin("Logical Processor");
        ImGui::SetWindowPos(v);
        ImGui::SetWindowSize(s);
        ImGui::Separator();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::Button("Close", ImVec2(55, 15))) showCore = false;
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Logical %s (%d)", CoreTypeString(core.coreType), showCoreID);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Core ID                     %d", core.id);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Group ID                    %d", core.group);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "NUMA Node                   %d", core.node);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Core Index                  %d", core.coreIndex);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Logical Index               %d", core.logicalProcessorIndex);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Details");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Realtime                    %s", core.realTime ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Parked                      %s", core.parked ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Allocated                   %s", core.allocated ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Allocated To Proc           %s", core.allocatedToTargetProcess ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Allocation Tag              %d", core.allocationTag);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Effeciency Class            %d", core.allocationTag);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Scheduling Class            %d", core.allocationTag);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Frequency Information");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Base Frequency              %2.2f gHz", float(core.baseFrequency) / 1000.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Current Frequency           %2.2f gHz", float(core.currentFrequency) / 1000.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Maximum Frequency           %2.2f gHz", float(core.maximumFrequency) / 1000.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Bus Frequency               %d mHz",    core.busFrequency);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Power Information");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "gHz Current                 %2.2f gHz", float(core.powerInformation.currentMhz) / 1000.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "gHz Limit                   %2.2f gHz", float(core.powerInformation.mhzLimit) / 1000.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "gHz Max                     %2.2f gHz", float(core.powerInformation.maxMhz) / 1000.0f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Current Idle State          %d", core.powerInformation.currentIdleState);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Maximum Idle State          %d", core.powerInformation.maxIdleState);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Instruction Set Architecture");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "SSE                         %s", core.SSE                  ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX                         %s", core.AVX                  ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX2                        %s", core.AVX2                 ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512                      %s", core.AVX2                 ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512F                     %s", core.AVX512F              ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512DQ                    %s", core.AVX512DQ             ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512PF                    %s", core.AVX512PF             ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512ER                    %s", core.AVX512ER             ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512CD                    %s", core.AVX512CD             ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512BW                    %s", core.AVX512BW             ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512VL                    %s", core.AVX512VL             ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_IFMA                 %s", core.AVX512_IFMA          ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_VBMI                 %s", core.AVX512_VBMI          ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_VBMI2                %s", core.AVX512_VBMI2         ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_VNNI                 %s", core.AVX512_VNNI          ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_BITALG               %s", core.AVX512_BITALG        ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_VPOPCNTDQ            %s", core.AVX512_VPOPCNTDQ     ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_4VNNIW               %s", core.AVX512_4VNNIW        ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_4FMAPS               %s", core.AVX512_4FMAPS        ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "AVX512_VP2INTERSECT         %s", core.AVX512_VP2INTERSECT  ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "SGX                         %s", core.SGX                  ? "Yes" : "No");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "SHA                         %s", core.SHA                  ? "Yes" : "No");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Associated Caches");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "ID | Type        | Lvl | Size     | Line");
        ImGui::Separator();
        for (int i = 0; i < m_procInfo.caches.size(); i++)
        {
            if (core.group == m_procInfo.caches[i].group)
            {
                if ((core.processorMask.to_ulong() & m_procInfo.caches[i].processorMask.to_ulong()))
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%2d | %-11.11s |  %d  | %5d kb | %2d b",
                        i, CacheTypeString(m_procInfo.caches[i].type),
                        m_procInfo.caches[i].level,
                        m_procInfo.caches[i].size / 1024,
                        m_procInfo.caches[i].lineSize);
                }
            }
        }
        ImGui::Separator();
        ImGui::End();
    }

    if (showCache && m_showCaches)
    {
        CACHE_INFO cache = m_procInfo.caches[showCacheID];

        if(showCore && m_showCores)
            v = ImVec2(300 + (m_width - 300 - 575) + 300, 0);
        else
            v = ImVec2(300 + (m_width - 300 - 575), 0);

        s = ImVec2(275, static_cast<float>(m_height));
        ImGui::Begin("Cache");
        ImGui::SetWindowPos(v);
        ImGui::SetWindowSize(s);
        ImGui::Separator();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::Button("Close", ImVec2(55, 15))) showCache = false;
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%s Cache (%d)", CacheTypeString(cache.type), showCacheID);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Level                    %12d",  cache.level);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Associativity            %12d",  cache.associativity);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Line Size (bytes)        %12d",  cache.lineSize);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Total Size (kilo bytes)  %12d",  cache.size / 1024);
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Associated Cores");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "ID | Type       | Current  | Max");
        ImGui::Separator();
        for (int i = 0; i < m_procInfo.cores.size(); i++)
        {
            if ((cache.group == m_procInfo.cores[i].group))
            {
                if ((cache.processorMask.to_ulong() & m_procInfo.cores[i].processorMask.to_ulong()))
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%2d | %-10.10s | %2.2f gHz | %2.2f gHz",
                        i, CoreTypeString(m_procInfo.cores[i].coreType),
                        float(m_procInfo.cores[i].currentFrequency) / 1000.0f,
                        float(m_procInfo.cores[i].maximumFrequency) / 1000.0f);
                }
            }
        }
        ImGui::Separator();
        ImGui::End();
    }
#endif

    ImGui::PopStyleVar();
#endif //IMGUI_ENABLED
}

// Render the scene.
void D3D12Multithreading::OnRender()
{
	// Suggest that we run on Core!
	RunOn(m_procInfo, INTEL_CORE);

    try
    {
        BeginFrame();

#if SINGLETHREADED
        for (int i = 0; i < NumContexts; i++)
        {
            WorkerThread(i);
        }
        MidFrame();
        EndFrame();
        m_commandQueue->ExecuteCommandLists(_countof(m_pCurrentFrameResource->m_batchSubmit), m_pCurrentFrameResource->m_batchSubmit);
#else
        for (int i = 0; i < NumContexts; i++)
        {
            SetEvent(m_workerBeginRenderFrame[i]); // Tell each worker to start drawing.
        }

        MidFrame();
        EndFrame();

        WaitForMultipleObjects(NumContexts, m_workerFinishShadowPass, TRUE, INFINITE);

        // You can execute command lists on any thread. Depending on the work 
        // load, apps can choose between using ExecuteCommandLists on one thread 
        // vs ExecuteCommandList from multiple threads.
        m_commandQueue->ExecuteCommandLists(NumContexts + 2, m_pCurrentFrameResource->m_batchSubmit); // Submit PRE, MID and shadows.

        WaitForMultipleObjects(NumContexts, m_workerFinishedRenderFrame, TRUE, INFINITE);

        // Submit remaining command lists.
        m_commandQueue->ExecuteCommandLists(_countof(m_pCurrentFrameResource->m_batchSubmit) - NumContexts - 2, m_pCurrentFrameResource->m_batchSubmit + NumContexts + 2);
#endif

        m_cpuTimer.Tick(NULL);
        if (m_titleCount == TitleThrottle)
        {
            WCHAR cpu[64];
            swprintf_s(cpu, L"%.4f CPU", m_cpuTime / m_titleCount);
            SetCustomWindowText(cpu);

            m_titleCount = 0;
            m_cpuTime = 0;
        }
        else
        {
            m_titleCount++;
            m_cpuTime += m_cpuTimer.GetElapsedSeconds() * 1000;
            m_cpuTimer.ResetElapsedTime();
        }

        // Present and update the frame index for the next frame.
        PIXBeginEvent(m_commandQueue.Get(), 0, L"Presenting to screen");
        ThrowIfFailed(m_swapChain->Present(1, 0));
        PIXEndEvent(m_commandQueue.Get());
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // Signal and increment the fence value.
        m_pCurrentFrameResource->m_fenceValue = m_fenceValue;
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
        m_fenceValue++;
    }
    catch (HrException& e)
    {
        if (e.Error() == DXGI_ERROR_DEVICE_REMOVED || e.Error() == DXGI_ERROR_DEVICE_RESET)
        {
            RestoreD3DResources();
        }
        else
        {
            throw;
        }
    }
}

// Release sample's D3D objects.
void D3D12Multithreading::ReleaseD3DResources()
{
    m_fence.Reset();
    ResetComPtrArray(&m_renderTargets);
    m_commandQueue.Reset();
    m_swapChain.Reset();
    m_device.Reset();
}

// Tears down D3D resources and reinitializes them.
void D3D12Multithreading::RestoreD3DResources()
{
    // Give GPU a chance to finish its execution in progress.
    try
    {
        WaitForGpu();
    }
    catch (HrException&)
    {
        // Do nothing, currently attached adapter is unresponsive.
    }
    ReleaseD3DResources();
    OnInit();
}

// Wait for pending GPU work to complete.
void D3D12Multithreading::WaitForGpu()
{
    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

    // Wait until the fence has been processed.
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
}

void D3D12Multithreading::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    {
        const UINT64 fence = m_fenceValue;
        const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

        // Signal and increment the fence value.
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
        m_fenceValue++;

        // Wait until the previous frame is finished.
        if (lastCompletedFence < fence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
        CloseHandle(m_fenceEvent);
    }

    // Close thread events and thread handles.
    for (int i = 0; i < NumContexts; i++)
    {
        CloseHandle(m_workerBeginRenderFrame[i]);
        CloseHandle(m_workerFinishShadowPass[i]);
        CloseHandle(m_workerFinishedRenderFrame[i]);
        CloseHandle(m_threadHandles[i]);
    }

    for (int i = 0; i < _countof(m_frameResources); i++)
    {
        delete m_frameResources[i];
    }
}

void D3D12Multithreading::OnKeyDown(UINT8 key)
{
    switch (key)
    {
    case VK_LEFT:
        m_keyboardInput.leftArrowPressed = true;
        break;
    case VK_RIGHT:
        m_keyboardInput.rightArrowPressed = true;
        break;
    case VK_UP:
        m_keyboardInput.upArrowPressed = true;
        break;
    case VK_DOWN:
        m_keyboardInput.downArrowPressed = true;
        break;
    case VK_SPACE:
        m_keyboardInput.animate = !m_keyboardInput.animate;
        break;
    }
}

void D3D12Multithreading::OnKeyUp(UINT8 key)
{
    switch (key)
    {
    case VK_LEFT:
        m_keyboardInput.leftArrowPressed = false;
        break;
    case VK_RIGHT:
        m_keyboardInput.rightArrowPressed = false;
        break;
    case VK_UP:
        m_keyboardInput.upArrowPressed = false;
        break;
    case VK_DOWN:
        m_keyboardInput.downArrowPressed = false;
        break;
    }
}

// Assemble the CommandListPre command list.
void D3D12Multithreading::BeginFrame()
{
    m_pCurrentFrameResource->Init();

    // Indicate that the back buffer will be used as a render target.
    m_pCurrentFrameResource->m_commandLists[CommandListPre]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the render target and depth stencil.
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_pCurrentFrameResource->m_commandLists[CommandListPre]->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_pCurrentFrameResource->m_commandLists[CommandListPre]->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    ThrowIfFailed(m_pCurrentFrameResource->m_commandLists[CommandListPre]->Close());
}

// Assemble the CommandListMid command list.
void D3D12Multithreading::MidFrame()
{
    // Transition our shadow map from the shadow pass to readable in the scene pass.
    m_pCurrentFrameResource->SwapBarriers();



    ThrowIfFailed(m_pCurrentFrameResource->m_commandLists[CommandListMid]->Close());
}

// Assemble the CommandListPost command list.
void D3D12Multithreading::EndFrame()
{



    m_pCurrentFrameResource->Finish();

    // Indicate that the back buffer will now be used to present.
    m_pCurrentFrameResource->m_commandLists[CommandListPost]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_pCurrentFrameResource->m_commandLists[CommandListPost]->Close());
}

// Worker thread body. workerIndex is an integer from 0 to NumContexts 
// describing the worker's thread index.
void D3D12Multithreading::WorkerThread(int threadIndex)
{
	// Suggest we run on Atom!
	RunOn(m_procInfo, INTEL_ATOM);

    assert(threadIndex >= 0);
    assert(threadIndex < NumContexts);
#if !SINGLETHREADED

    while (threadIndex >= 0 && threadIndex < NumContexts)
    {
        // Wait for main thread to tell us to draw.

        WaitForSingleObject(m_workerBeginRenderFrame[threadIndex], INFINITE);

#endif
        ID3D12GraphicsCommandList* pShadowCommandList = m_pCurrentFrameResource->m_shadowCommandLists[threadIndex].Get();
        ID3D12GraphicsCommandList* pSceneCommandList = m_pCurrentFrameResource->m_sceneCommandLists[threadIndex].Get();

        //
        // Shadow pass
        //

        // Populate the command list.
        SetCommonPipelineState(pShadowCommandList);
        m_pCurrentFrameResource->Bind(pShadowCommandList, FALSE, nullptr, nullptr);    // No need to pass RTV or DSV descriptor heap.

        // Set null SRVs for the diffuse/normal textures.
        pShadowCommandList->SetGraphicsRootDescriptorTable(0, m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

        // Distribute objects over threads by drawing only 1/NumContexts 
        // objects per worker (i.e. every object such that objectnum % 
        // NumContexts == threadIndex).
        PIXBeginEvent(pShadowCommandList, 0, L"Worker drawing shadow pass...");

        for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += NumContexts)
        {
            SampleAssets::DrawParameters drawArgs = SampleAssets::Draws[j];

            pShadowCommandList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);
        }

        PIXEndEvent(pShadowCommandList);

        ThrowIfFailed(pShadowCommandList->Close());

#if !SINGLETHREADED
        // Submit shadow pass.
        SetEvent(m_workerFinishShadowPass[threadIndex]);
#endif

        //
        // Scene pass
        // 

        // Populate the command list.  These can only be sent after the shadow 
        // passes for this frame have been submitted.
        SetCommonPipelineState(pSceneCommandList);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        m_pCurrentFrameResource->Bind(pSceneCommandList, TRUE, &rtvHandle, &dsvHandle);

        PIXBeginEvent(pSceneCommandList, 0, L"Worker drawing scene pass...");

        D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvHeapStart = m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
        const UINT cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        const UINT nullSrvCount = 2;
        for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += NumContexts)
        {
            SampleAssets::DrawParameters drawArgs = SampleAssets::Draws[j];

            // Set the diffuse and normal textures for the current object.
            CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(cbvSrvHeapStart, nullSrvCount + drawArgs.DiffuseTextureIndex, cbvSrvDescriptorSize);
            pSceneCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);

            pSceneCommandList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);
        }

        if (threadIndex == NumContexts - 1)
        {
#ifdef IMGUI_ENABLED
            ImGui::Render();
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pSceneCommandList);
#endif
        }

        PIXEndEvent(pSceneCommandList);
        ThrowIfFailed(pSceneCommandList->Close());

#if !SINGLETHREADED
        // Tell main thread that we are done.
        SetEvent(m_workerFinishedRenderFrame[threadIndex]); 
    }
#endif
}

void D3D12Multithreading::SetCommonPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
    pCommandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.Get(), m_samplerHeap.Get() };
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    pCommandList->RSSetViewports(1, &m_viewport);
    pCommandList->RSSetScissorRects(1, &m_scissorRect);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    pCommandList->IASetIndexBuffer(&m_indexBufferView);
    pCommandList->SetGraphicsRootDescriptorTable(3, m_samplerHeap->GetGPUDescriptorHandleForHeapStart());
    pCommandList->OMSetStencilRef(0);

    // Render targets and depth stencil are set elsewhere because the 
    // depth stencil depends on the frame resource being used.

    // Constant buffers are set elsewhere because they depend on the 
    // frame resource being used.

    // SRVs are set elsewhere because they change based on the object 
    // being drawn.
}
