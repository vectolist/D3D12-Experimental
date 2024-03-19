
#include <DXConfig.h>
#include <DXContext.h>
#include <string>


#define WIDTH 1024
#define HEIGHT 860

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

int main(int args, char* argv[])
{

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpfnWndProc = WndProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "DirectX12";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if(FAILED(RegisterClassEx(&wc))){
        Log_Error("failed register %s class", wc.lpszClassName);
    }

    DWORD style = WS_OVERLAPPEDWINDOW;

    int x = (GetSystemMetrics(SM_CXSCREEN) - WIDTH) /2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - HEIGHT) /2;


    HWND hwnd = CreateWindowEx(NULL, wc.lpszClassName, wc.lpszClassName,
        style, x, y, WIDTH, HEIGHT, nullptr, 0,wc.hInstance, 0
        );
    
    if(!hwnd) Log_Error("failed to create window");

    RECT rc{};
    GetClientRect(hwnd, &rc);
    AdjustWindowRect(&rc, style, 0);

    int cx = rc.right - rc.left;
    int cy = rc.bottom - rc.top;

    DX12Context context(hwnd, cx, cy);
    auto device = context.getDevice();
    
    //-------------------- root signature ------------------------
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feattureData{};
    feattureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if(FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feattureData, sizeof(feattureData)))){
        feattureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_DESCRIPTOR_RANGE range[1];
    range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;   //const buffer
    range[0].NumDescriptors = 1;
    range[0].BaseShaderRegister = 0;                        //b0
    range[0].RegisterSpace = 0;
    //range[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
    range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParam[1];
    rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; //default
    rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];

    D3D12_ROOT_SIGNATURE_DESC rootDesc{
        1, rootParam , 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    };
    

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* signature{};
    ID3DBlob* err{};

    HR(D3D12SerializeRootSignature(&rootDesc, 
        D3D_ROOT_SIGNATURE_VERSION_1, &signature, &err));

    ID3D12RootSignature* rootSigniture{};

    HR(context.getDevice()->CreateRootSignature(0,
        signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSigniture)));

    Safe_Release(signature);
    Safe_Release(err);
    
    //-------------------- shader ------------------------
    ID3DBlob* vertShader{};
    ID3DBlob* pixelShader{};

    UINT flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    HRESULT res = D3DCompileFromFile(L"constbuffer.hlsl",
        nullptr, nullptr, "VSMain", "vs_5_0", flag, 0, &vertShader, &err);
    if(FAILED(res)){
        MessageBoxA(NULL, (LPCSTR)err->GetBufferPointer(), "error", MB_OK);
    }

    res = D3DCompileFromFile(L"constbuffer.hlsl",
        nullptr, nullptr, "PSMain", "ps_5_0", flag, 0, &pixelShader, &err);
    if(FAILED(res)){
        MessageBoxA(NULL, (LPCSTR)err->GetBufferPointer(), "error", MB_OK);
    }

    D3D12_INPUT_ELEMENT_DESC inputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_SHADER_BYTECODE code{};
    code.BytecodeLength = vertShader->GetBufferSize();
    code.pShaderBytecode = vertShader->GetBufferPointer();

    //-------------------- pipeline state ------------------------
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = {inputDesc, _countof(inputDesc)};
    psoDesc.pRootSignature = rootSigniture;
    psoDesc.VS = D3D12_SHADER_BYTECODE{ vertShader->GetBufferPointer(),vertShader->GetBufferSize() };
    psoDesc.PS = D3D12_SHADER_BYTECODE{ pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    
    ID3D12PipelineState* pipelineState{};

    HR(context.getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

    //-------------------- vertex resource ------------------------
    struct Vertex{
        float pos[3];
        float color[4];
    };

    Vertex vertices[] = {
        {{  0.0f,  0.5f, 0.f}, {0.f, 1.f, 0.f, 1.f} },
        {{  0.5f, -0.5f, 0.f}, {1.f, 0.f, 0.f, 1.f} },
        {{ -0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f, 1.f} }
    };

    const UINT verticesSize = sizeof(vertices);


    ID3D12Resource* vertexBuffer{};

    HR(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(verticesSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer)
    ));

    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange = {0,0};
    vertexBuffer->Map(0, &readRange, (void**)&pVertexDataBegin);
    memcpy(pVertexDataBegin, vertices, sizeof(vertices));
    vertexBuffer->Unmap(0, nullptr);

    D3D12_VERTEX_BUFFER_VIEW vertexView{};
    vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexView.StrideInBytes = sizeof(Vertex);
    vertexView.SizeInBytes = verticesSize;

    
    //-------------------- constant buffer ------------------------
    ID3D12DescriptorHeap* cbHeap = nullptr;

    D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc{};
    cbHeapDesc.NumDescriptors = 1;
    cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    HR(device->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&cbHeap)));

    struct  __declspec(align(256)) ConstBuffer
    {
        float data[16];       
    };
    
    ConstBuffer view = {
        1   ,    0,     0,  0,
        0   ,    1,     0,  0,
        0   ,    0,     1,  0,
        0   ,    0,     0,  1
    };
    

    const UINT constantBufferSize = sizeof(ConstBuffer);
    ID3D12Resource* constBuffer{};
    UINT8* cbvBegin =nullptr;

    D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);


    HR(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constBuffer)
    ));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc{};
    cbDesc.BufferLocation = constBuffer->GetGPUVirtualAddress();
    cbDesc.SizeInBytes = constantBufferSize;

    device->CreateConstantBufferView(&cbDesc, cbHeap->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RANGE _range(0, 0);
    constBuffer->Map(0, &_range, (void**)&cbvBegin);


    memcpy(&cbvBegin[0 * constantBufferSize], &view, sizeof(view));

    context.waitSyncFrame();

    ShowWindow(hwnd, TRUE);

    MSG msg{};

    while (msg.message != WM_QUIT)
    {
        if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
            if(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
                PostQuitMessage(0);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else{
        const auto vp = context.getViewport();
        const auto rc = context.getRect();

        context.beginRecord(vp, rc);

        auto cmdList = context.getCommandList();

        cmdList->SetPipelineState(pipelineState);
        cmdList->SetGraphicsRootSignature(rootSigniture);
        
        ID3D12DescriptorHeap* descHeapList[] = {cbHeap};
        cmdList->SetDescriptorHeaps(_countof(descHeapList), descHeapList);

        cmdList->SetGraphicsRootDescriptorTable(0, 
            cbHeap->GetGPUDescriptorHandleForHeapStart());

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &vertexView);

        

        static float delta = 0.0f;
        delta += 0.003f;
        
        //moving the x value left and right
        view.data[12] = std::sinf(delta) / 2.f;
        memcpy(&cbvBegin[0 * constantBufferSize], &view, sizeof(view));
        
        cmdList->DrawInstanced(3, 1,0,0);

        context.endRecord();
        }
        
    }
    //relase all of resouces
    {
        constBuffer->Unmap(0, nullptr);

        //Safe_Release(vertexView);
        Safe_Release(constBuffer);
        Safe_Release(cbHeap);
        Safe_Release(vertexBuffer);

        Safe_Release(vertShader);
        Safe_Release(pixelShader);

        Safe_Release(pipelineState);
        Safe_Release(rootSigniture);
    }
    

    context.Release();
    
    return 0;
};

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp){
    return DefWindowProc(hwnd, msg, wp, lp);
}