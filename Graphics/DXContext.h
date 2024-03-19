#pragma once 

#include <DXConfig.h>
#include <assert.h>

class DX12Context
{
public:
    DX12Context(HWND hwnd, int cx, int cy);
    ~DX12Context();
    void Release();

    void CreateDevice();
    void CreateCommands();
    void ResizeSwapChain(int cx, int cy);

    void waitSyncFrame();

protected:
    HWND mHwnd;

    ID3D12Debug* mDebug{};
    IDXGIDebug1* mDXGIDebug{};

    

    IDXGIFactory4* mFactroy {};

    ID3D12Device* mDevice{};

    //----- command -----
    ID3D12CommandQueue* mCommandQueue{};
    ID3D12GraphicsCommandList* mCommandList{};
    ID3D12CommandAllocator* mCommandAlloc{};

    //SwapChain
    IDXGISwapChain3*        mSwapChain{};
    ID3D12DescriptorHeap*   mRtvDescHeap{};
    UINT                    mRTVHeapOffet{};
    ID3D12Resource*         mRTV[2]{};

    //Synchronization
    ID3D12Fence* mFence{};
    HANDLE mFenceEvent{};
    UINT mFrameIndex {};
    UINT64 mFenceValue{};



    bool mUseWarpDeivce {true};
    static const UINT FrameCount = 2;
    static const DXGI_FORMAT RTVFormat{DXGI_FORMAT_R8G8B8A8_UNORM};
    static const DXGI_FORMAT DSVForamt{DXGI_FORMAT_D24_UNORM_S8_UINT};

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissor;

public:
    inline ID3D12Device* getDevice() { return mDevice;}
    inline ID3D12GraphicsCommandList* getCommandList() { return mCommandList;}
    inline ID3D12CommandQueue* getCommandQueue() { return mCommandQueue;}
    inline IDXGISwapChain* getSwapchain() { return mSwapChain;}
    inline ID3D12CommandAllocator* getCommandAlloc() { return mCommandAlloc;}
    D3D12_VIEWPORT& getViewport() { return mViewport;}
    D3D12_RECT& getRect() { return mScissor;}

    void beginRecord(const D3D12_VIEWPORT& vp, const D3D12_RECT& rc);

    void endRecord();

};