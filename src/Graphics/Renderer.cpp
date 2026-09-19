#include "Renderer.h"

#include <d3dcompiler.h>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    struct Vertex
    {
        float x, y, z;
        float r, g, b;
    };

    void ReleaseIfValid(HANDLE& handle)
    {
        if (handle)
        {
            CloseHandle(handle);
            handle = nullptr;
        }
    }

    void CheckHR(HRESULT hr)
    {
        if (FAILED(hr))
            throw std::runtime_error("DirectX 12 operation failed.");
    }
}

namespace Graphics
{
    bool Renderer::Initialize(HWND windowHandle, uint32_t renderWidth, uint32_t renderHeight)
    {
        width = renderWidth;
        height = renderHeight;

        try
        {
            if (!CreateDevice()) return false;
            if (!CreateCommandObjects()) return false;
            if (!CreateSwapChain(windowHandle)) return false;
            if (!CreateRenderTargets()) return false;
            if (!CreatePipeline()) return false;
            if (!CreateFence()) return false;
        }
        catch (...)
        {
            Shutdown();
            return false;
        }

        return true;
    }

    bool Renderer::CreateDevice()
    {
        UINT flags = 0;

#if defined(_DEBUG)
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
                debugController->EnableDebugLayer();
        }
#endif

        if (FAILED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory))))
            return false;

        ComPtr<IDXGIAdapter1> adapter;

        for (UINT index = 0;
             factory->EnumAdapterByGpuPreference(
                 index,
                 DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                 IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
             ++index)
        {
            DXGI_ADAPTER_DESC1 desc{};
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter.Reset();
                continue;
            }

            if (SUCCEEDED(D3D12CreateDevice(
                adapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&device))))
            {
                return true;
            }

            adapter.Reset();
        }

        return false;
    }

    bool Renderer::CreateCommandObjects()
    {
        if (FAILED(device->CreateCommandQueue(
            &D3D12_COMMAND_QUEUE_DESC{
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                D3D12_COMMAND_QUEUE_FLAG_NONE,
                0
            },
            IID_PPV_ARGS(&commandQueue))))
        {
            return false;
        }

        if (FAILED(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator))))
            return false;

        if (FAILED(device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList))))
            return false;

        return SUCCEEDED(commandList->Close());
    }

    bool Renderer::CreateSwapChain(HWND windowHandle)
    {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = width;
        desc.Height = height;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferCount = FrameCount;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> tempSwapChain;

        if (FAILED(factory->CreateSwapChainForHwnd(
            commandQueue.Get(),
            windowHandle,
            &desc,
            nullptr,
            nullptr,
            &tempSwapChain)))
            return false;

        if (FAILED(factory->MakeWindowAssociation(
            windowHandle,
            DXGI_MWA_NO_ALT_ENTER)))
            return false;

        return SUCCEEDED(tempSwapChain.As(&swapChain));
    }

    bool Renderer::CreateRenderTargets()
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.NumDescriptors = FrameCount;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (FAILED(device->CreateDescriptorHeap(
            &heapDesc,
            IID_PPV_ARGS(&rtvHeap))))
            return false;

        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE handle =
            rtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (uint32_t i = 0; i < FrameCount; ++i)
        {
            if (FAILED(swapChain->GetBuffer(
                i,
                IID_PPV_ARGS(&renderTargets[i]))))
                return false;

            D3D12_CPU_DESCRIPTOR_HANDLE current = handle;
            current.ptr += static_cast<SIZE_T>(i) * rtvDescriptorSize;

            device->CreateRenderTargetView(
                renderTargets[i].Get(),
                nullptr,
                current);
        }

        frameIndex = swapChain->GetCurrentBackBufferIndex();
        return true;
    }

    bool Renderer::CompileShader(
        const wchar_t* path,
        const char* entryPoint,
        const char* target,
        ComPtr<ID3DBlob>& bytecode)
    {
        ComPtr<ID3DBlob> errors;

        const UINT flags =
#if defined(_DEBUG)
            D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        HRESULT hr = D3DCompileFromFile(
            path,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            target,
            flags,
            0,
            &bytecode,
            &errors);

        return SUCCEEDED(hr);
    }

    bool Renderer::CreatePipeline()
    {
        D3D12_ROOT_SIGNATURE_DESC rootDesc{};
        rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signatureBlob;
        ComPtr<ID3DBlob> errorBlob;

        if (FAILED(D3D12SerializeRootSignature(
            &rootDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signatureBlob,
            &errorBlob)))
            return false;

        if (FAILED(device->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature))))
            return false;

        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

        if (!CompileShader(
            L"shaders/Triangle.hlsl",
            "VSMain",
            "vs_5_0",
            vertexShader))
            return false;

        if (!CompileShader(
            L"shaders/Triangle.hlsl",
            "PSMain",
            "ps_5_0",
            pixelShader))
            return false;

        D3D12_INPUT_ELEMENT_DESC inputLayout[] =
        {
            {
                "POSITION",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                0,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            },
            {
                "COLOR",
                0,
                DXGI_FORMAT_R32G32B32_FLOAT,
                0,
                12,
                D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                0
            }
        };

        D3D12_RASTERIZER_DESC rasterizer{};
        rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizer.CullMode = D3D12_CULL_MODE_NONE;
        rasterizer.FrontCounterClockwise = FALSE;
        rasterizer.DepthClipEnable = TRUE;

        D3D12_BLEND_DESC blend{};
        blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
        pso.InputLayout = { inputLayout, _countof(inputLayout) };
        pso.pRootSignature = rootSignature.Get();
        pso.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        pso.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
        pso.RasterizerState = rasterizer;
        pso.BlendState = blend;
        pso.SampleMask = UINT_MAX;
        pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso.NumRenderTargets = 1;
        pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso.SampleDesc.Count = 1;

        return SUCCEEDED(device->CreateGraphicsPipelineState(
            &pso,
            IID_PPV_ARGS(&pipelineState)));
    }

    bool Renderer::CreateFence()
    {
        if (FAILED(device->CreateFence(
            0,
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&fence))))
            return false;

        fenceValues[0] = 1;
        fenceValues[1] = 1;

        fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        return fenceEvent != nullptr;
    }

    void Renderer::Render(const Adv::AdvControl& control)
    {
        (void)control;

        if (FAILED(commandAllocator->Reset()))
            return;

        if (FAILED(commandList->Reset(commandAllocator.Get(), pipelineState.Get())))
            return;

        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv =
            rtvHeap->GetCPUDescriptorHandleForHeapStart();

        rtv.ptr += static_cast<SIZE_T>(frameIndex) * rtvDescriptorSize;

        const float clearColor[] = { 0.02f, 0.025f, 0.04f, 1.0f };
        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        commandList->SetGraphicsRootSignature(rootSignature.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);

        const auto presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);

        commandList->ResourceBarrier(1, &presentBarrier);

        if (FAILED(commandList->Close()))
            return;

        ID3D12CommandList* lists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(1, lists);

        swapChain->Present(1, 0);

        MoveToNextFrame();
    }

    void Renderer::MoveToNextFrame()
    {
        const uint64_t currentFence = fenceValues[frameIndex];

        if (FAILED(commandQueue->Signal(fence.Get(), currentFence)))
            return;

        frameIndex = swapChain->GetCurrentBackBufferIndex();

        if (fence->GetCompletedValue() < fenceValues[frameIndex])
        {
            fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        fenceValues[frameIndex] = currentFence + 1;
    }

    void Renderer::WaitForGPU()
    {
        if (!commandQueue || !fence || !fenceEvent)
            return;

        const uint64_t value = fenceValues[frameIndex];

        if (SUCCEEDED(commandQueue->Signal(fence.Get(), value)))
        {
            fenceValues[frameIndex]++;

            if (fence->GetCompletedValue() < value)
            {
                fence->SetEventOnCompletion(value, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }
        }
    }

    void Renderer::Shutdown()
    {
        WaitForGPU();

        ReleaseIfValid(fenceEvent);

        for (auto& target : renderTargets)
            target.Reset();

        pipelineState.Reset();
        rootSignature.Reset();
        rtvHeap.Reset();
        swapChain.Reset();
        commandList.Reset();
        commandAllocator.Reset();
        commandQueue.Reset();
        fence.Reset();
        device.Reset();
        factory.Reset();
    }
}
