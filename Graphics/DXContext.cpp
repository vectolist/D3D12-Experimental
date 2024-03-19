#include "DXContext.h"

DX12Context::DX12Context(HWND hwnd, int cx, int cy)
    : mHwnd(hwnd)
{
    CreateDevice();
    CreateCommands();
    ResizeSwapChain(cx, cy);
}

DX12Context::~DX12Context()
{
}

void DX12Context::Release()
{
    waitSyncFrame();

    CloseHandle(mFenceEvent);

    for(UINT i =0; i < FrameCount; ++i){
        Safe_Release(mRTV[i]);
    }

    Safe_Release(mRtvDescHeap);
    Safe_Release(mSwapChain);

    Safe_Release(mCommandAlloc);
    Safe_Release(mCommandList);
    Safe_Release(mCommandQueue);

    Safe_Release(mFence);

    Safe_Release(mFactroy);
    Safe_Release(mDevice);

#if defined(_DEBUG) 
    if (mDXGIDebug) {
        OutputDebugStringA("-------------------------------- Debug Layout ----------------------------------\n");
        mDXGIDebug->ReportLiveObjects(
            DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
        );
        OutputDebugStringA("--------------------------------------------------------------------------------\n");
    }

    Safe_Release(mDebug);
    Safe_Release(mDXGIDebug);
#endif
}

void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    comptr<IDXGIAdapter1> adapter;

    comptr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if(adapter == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }
    
    *ppAdapter = adapter.Detach();
}

void DX12Context::CreateDevice()
{
    UINT factoryFlag = 0;
#if defined(_DEBUG)
    {
        HR(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebug)));
        mDebug->EnableDebugLayer();

        HR(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDXGIDebug)));
        mDXGIDebug->EnableLeakTrackingForThread();
        factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
    }

#endif

    HR(CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&mFactroy)));
    

    if(mUseWarpDeivce){
        IDXGIAdapter* warpadapter{};
        HR(mFactroy->EnumWarpAdapter(IID_PPV_ARGS(&warpadapter)));

        HR(D3D12CreateDevice(warpadapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
        printf("- created device with wrap adapter\n");
        Safe_Release(warpadapter);
    }
    else{
        IDXGIAdapter1* hardwareAdapter{};

        GetHardwareAdapter(mFactroy, &hardwareAdapter, false);

        HR(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
        printf("- created device with hardware adapter\n");
        Safe_Release(hardwareAdapter);
    }
    //create Fence
    HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

    mFenceValue = 1;

    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if(!mFenceEvent){
        HR(GetLastError());
    }

    ID3D12InfoQueue* info{};
    mDevice->QueryInterface(IID_PPV_ARGS(&info));

    info->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
    info->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

    Safe_Release(info);

}

void DX12Context::CreateCommands()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = 0;

    HR(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

    HR(mDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAlloc)));
    
    HR(mDevice->CreateCommandList(
        queueDesc.NodeMask, queueDesc.Type, 
        mCommandAlloc, nullptr, IID_PPV_ARGS(&mCommandList)
    ));

    HR(mCommandList->Close());
    
}

void DX12Context::ResizeSwapChain(int cx, int cy)
{
    //reset all of about swapchain
    Safe_Release(mSwapChain);
    Safe_Release(mRtvDescHeap);
    for(UINT i = 0; i < FrameCount; ++i){
        Safe_Release(mRTV[i]);
    }

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = FrameCount;
    desc.Width = cx;
    desc.Height = cy;
    desc.Format = RTVFormat;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc.Count = 1;

    IDXGISwapChain1* swapchain{};
    HR(mFactroy->CreateSwapChainForHwnd(
        mCommandQueue, 
        mHwnd, &desc,nullptr, nullptr,
        &swapchain
    ));

    HR(mFactroy->MakeWindowAssociation(mHwnd, DXGI_MWA_NO_ALT_ENTER));

    HR(swapchain->QueryInterface(IID_PPV_ARGS(&mSwapChain)));
    Safe_Release(swapchain);

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    //create heaps
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = FrameCount;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mRtvDescHeap));

    mRTVHeapOffet = mDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{mRtvDescHeap->GetCPUDescriptorHandleForHeapStart()};

    for(UINT i = 0; i < FrameCount; ++i){
        HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRTV[i])));
        mDevice->CreateRenderTargetView(mRTV[i], nullptr, rtvHandle);
        rtvHandle.Offset(1, mRTVHeapOffet);

    }

    mViewport.TopLeftX = 0.f;
    mViewport.TopLeftY = 0.f;
    mViewport.Width = (float)cx;
    mViewport.Height = (float)cy;
    mViewport.MinDepth = 0.f;
    mViewport.MaxDepth = 1.f;

    mScissor = {0, 0, cx, cy};
}

void DX12Context::waitSyncFrame()
{
    const UINT64 fence = mFenceValue;
    HR(mCommandQueue->Signal(mFence, fence));
    mFenceValue++;

    if(mFence->GetCompletedValue() < fence){
        //printf("======current fence queued======\n");
        HR(mFence->SetEventOnCompletion(fence, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }
}

void DX12Context::beginRecord(const D3D12_VIEWPORT &vp, const D3D12_RECT &rc)
{
    //all reset & rdy to record.
    HR(mCommandAlloc->Reset());
    HR(mCommandList->Reset(mCommandAlloc, nullptr));

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mRTV[mFrameIndex],
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    mCommandList->ResourceBarrier(1, &barrier);

    mCommandList->RSSetViewports(1, &vp);
    mCommandList->RSSetScissorRects(1, &rc);

    D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        mRtvDescHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRTVHeapOffet
    );

    const float clearColor[4] = {0.3f, 0.4f, 0.3f, 1.f};
    mCommandList->ClearRenderTargetView(backBufferHandle, clearColor, 0, nullptr);
    mCommandList->OMSetRenderTargets(1, &backBufferHandle, FALSE, nullptr);

}

void DX12Context::endRecord()
{
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mRTV[mFrameIndex],
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    mCommandList->ResourceBarrier(1, &barrier);
    mCommandList->Close();
    
    ID3D12CommandList* cmdLists[] =  {mCommandList};
    mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    mSwapChain->Present(0, 0);

    waitSyncFrame();

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();    
}
