#pragma once

#include "d3dUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
};

struct PassConstants
{
    // ���� ��ġ, �þ� ��İ� ���� ���, �׸��� ȭ��(���� ���) ũ�⿡ 
    // ���� ������ �̷� ���ۿ� ����.
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;

    //���� �ð� ����ġ ���� ������ �� ���ۿ� ����.
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

//CPU�� �� �������� ��� ��ϵ��� �����ϴ� �� �ʿ��� �ڿ����� ��ǥ�ϴ�
//Ŭ����. ���� ���α׷����� �ʿ��� �ڿ��� �ٸ� ���̹Ƿ�,
//�̷� Ŭ������ ��� ���� ���� ���� ���α׷����� �޶�� �� ���̴�.
struct FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    // ��� �Ҵ��ڴ� GPU�� ��ɵ��� �� ó���� �� �缳���ؾ� �Ѵ�.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // ��� ���۴� �װ��� �����ϴ� ��ɵ��� GPU�� �� ó���� �Ŀ�
    // �����ؾ� �Ѵ�.���� �����Ӹ��� ��� ���۸� ���� ������ �Ѵ�.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

    // Fence�� ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϴ� ���̴�.
    // �� ���� GPU�� ���� �� ������ �ڿ����� ����ϰ� �ִ���
    // �����ϴ� �뵵�� ���δ�.
    UINT64 Fence = 0;
};