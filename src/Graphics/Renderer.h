#pragma once

#include <windows.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "AdvControl.h"

using Microsoft::WRL::ComPtr;

namespace Graphics
{
    class Renderer
    {
    public:
        bool Initialize(HWND window, uint32_t width, uint32_t height);
        void Render(const Adv::AdvControl& control);
        void Shutdown();

    private:
        bool CreateDevice();
        bool CreateCommandObjects();
        bool CreateSwapChain(HWND window);
        bool CreateRenderTargets();
        bool CreatePipeline();
        bool CreateFence();

        bool CompileShader(
            const wchar_t* path,
            const char* entryPoint,
            const char* target,
            ComPtr<ID3DBlob>& bytecode
        );

        void WaitForGPU();
        void MoveToNextFrame();

        static constexpr uint32_t FrameCount = 2;

        uint32_t width = 0;
        uint32_t height = 0;

        ComPtr<IDXGIFactory7> factory;
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> commandQueue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ComPtr<IDXGISwapChain4> swapChain;

        ComPtr<ID3D12DescriptorHeap> rtvHeap;
        ComPtr<ID3D12RootSignature> rootSignature;
        ComPtr<ID3D12PipelineState> pipelineState;

        ComPtr<ID3D12Resource> renderTargets[FrameCount];

        ComPtr<ID3D12Fence> fence;
        HANDLE fenceEvent = nullptr;
        uint64_t fenceValues[FrameCount]{};
        uint32_t frameIndex = 0;
        uint32_t rtvDescriptorSize = 0;
    };
}
