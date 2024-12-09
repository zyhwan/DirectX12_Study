//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Direct3D 12���� ���ڸ� �׸��� ����� �����ش�.
//
// Controls:
//   Hold the left mouse button down and move the mouse to rotate.
//   Hold the right mouse button down and move the mouse to zoom in and out.
//***************************************************************************************

#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;


//������ ���� ����ü ������
struct VPosData
{
    XMFLOAT3 Pos;
};

struct VColorData
{
    XMFLOAT4 Color;
};

//��� ���� �߰�
struct ObjectConstants
{
    XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
    float Time;
    XMFLOAT4 PulseColor;
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
    BoxApp(const BoxApp& rhs) = delete;
    BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

    void BuildDescriptorHeaps();
	void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

private:


    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

    std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	std::unique_ptr<MeshGeometry> mBoxGeo = nullptr;
	std::unique_ptr<MeshGeometry> mBoxGe = nullptr;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 5.0f;

    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// ����� ���忡���� ���� ���� �޸� ���� ����� �Ҵ�.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    try
    {
        BoxApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

BoxApp::BoxApp(HINSTANCE hInstance)
: D3DApp(hInstance) 
{
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Initialize()
{
    if(!D3DApp::Initialize())
		return false;
		
    // �ʱ�ȭ ��ɵ��� �غ��ϱ� ���� ��� ����� �缳���Ѵ�.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
 
    BuildDescriptorHeaps();
	BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    // �ʱ�ȭ ��ɵ��� �����Ѵ�.
    ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // �ʱ�ȭ�� �Ϸ�� ������ ��ٸ���.
    FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

    // â�� ũ�Ⱑ �ٲ�����Ƿ� ��Ⱦ�� �����ϰ�
    // ���� ����� �ٽ� ����Ѵ�.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
    // ���� ��ǥ�� ��ī��Ʈ ��ǥ(���� ��ǥ)�� ��ȯ�Ѵ�.
    float x = mRadius*sinf(mPhi)*cosf(mTheta);
    float z = mRadius*sinf(mPhi)*sinf(mTheta);
    float y = mRadius*cosf(mPhi);

    // �þ� ����� �����Ѵ�.
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    mWorld._41 = -1.2f;

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world*view*proj;



	// �ֽ��� worldViewProj ��ķ� ��� ���۸� �����Ѵ�.
	ObjectConstants objConstants;

    objConstants.PulseColor = XMFLOAT4(Colors::Aquamarine);

    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    objConstants.Time = gt.TotalTime();
    mObjectCB->CopyData(0, objConstants);


    mWorld._41 = 1.2f;
    world = XMLoadFloat4x4(&mWorld);
    worldViewProj = world * view * proj;
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
    mObjectCB->CopyData(1, objConstants);
}

void BoxApp::Draw(const GameTimer& gt)
{
    // ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ�
    // �缳���Ѵ�. �缳���� GPU�� ���� ��� ��ϵ���
    // ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// ��� ����� ExecuteCommandList�� ���ؼ� ��� ��⿭��
    // �߰��ߴٸ� ��� ����� �缳���� �� �ִ�. ��� �����
    // �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // �ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // �ĸ� ���ۿ� ���� ���۸� �����.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
    // ������ ����� ��ϵ� ��� ���۵��� �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView()); //(���� ����, �Է� ���Ե鿡 ���� ���� ���� ����, ���� ���� �� �迭�� ù ���Ҹ� ����Ű�� ������)
	mCommandList->IASetVertexBuffers(1, 1, &mBoxGeo->ColorBufferView()); //(���� ����, �Է� ���Ե鿡 ���� ���� ���� ����, ���� ���� �� �迭�� ù ���Ҹ� ����Ű�� ������)
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
    mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
    cbv.Offset(0, mCbvSrvUavDescriptorSize);

    mCommandList->SetGraphicsRootDescriptorTable(0, cbv);
    mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs["box"].IndexCount, 
		1, 0, 0, 0);

    cbv.Offset(1, mCbvSrvUavDescriptorSize);
    mCommandList->SetGraphicsRootDescriptorTable(0, cbv);
    mCommandList->DrawIndexedInstanced(
        mBoxGeo->DrawArgs["pyramid"].IndexCount,
        1, mBoxGeo->DrawArgs["pyramid"].StartIndexLocation, mBoxGeo->DrawArgs["pyramid"].BaseVertexLocation, 0);
	
    // �ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // ��ɵ��� ����� ��ģ��.
	ThrowIfFailed(mCommandList->Close());
 
    // ��� ������ ���� ��� ����� ��⿭�� �߰��Ѵ�.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	
	// �ĸ� ���ۿ� ���� ���۸� ��ȯ�Ѵ�.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// �� �������� ��ɵ��� ��� ó���Ǳ� ��ٸ���. �̷��� ����
    // ��ȿ�����̴�. �̹����� ������ �������� ���� �� ����� ���������,
    // ������ �����鿡���� ������ �ڵ带 ������ ����ȭ�ؼ� �����Ӹ��� �����
    // �ʿ䰡 ���� �����.
	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if((btnState & MK_LBUTTON) != 0)
    {
        // ���콺 �� �ȼ� �̵��� 4���� 1���� ������Ų��.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // �Է¿� ������ ������ �����ؼ� ī�޶� ���ڸ� �߽����� �����ϰ� �Ѵ�.
        mTheta += dx;
        mPhi += dy;

        // mPhi ������ �����Ѵ�.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // ���콺 �� �ȼ� �̵��� ����� 0.005������ ������Ų��.
        float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

        // �Է¿� �����ؼ� ī�޶� �������� �����Ѵ�.
        mRadius += dx - dy;

        // �������� �����Ѵ�.
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
    //��� ���� �� ����
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 2; //2�� ����
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
    //��ü 1���� ��� �ڷḦ ���� ��� ����
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 2, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    //���� ��ü�� ���� �ּ�(0��° ��� ������ �ּ�)
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    // ���ۿ� ��� 0��° ��� ������ ������
    int boxCBufIndex = 0;
	cbAddress += boxCBufIndex*objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    //��� ���� �� ����
	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());


    //�Ƕ�̵� ����
    cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    // ���ۿ� ��� 1��° ��� ������ ������
    boxCBufIndex = 1;
    cbAddress += boxCBufIndex * objCBByteSize;

    cbvDesc.BufferLocation = cbAddress;

    // CD3DX12_CPU_DESCRIPTOR_HANDLE���� ������ ���� �����Ͽ� Offset�� �����Ѵ�.
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(boxCBufIndex, mCbvSrvUavDescriptorSize);

    //��� ���� �� ����
    md3dDevice->CreateConstantBufferView(
        &cbvDesc,
        handle);
}

void BoxApp::BuildRootSignature()
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ���(��� ����, �ؽ�ó, ǥ������� ��)��
    // �Էµȴٰ� ����Ѵ�. ��Ʈ �ñ״�ó�� ���̴� ���α׷��� ����ϴ� �ڿ�����
    // �����Ѵ�. ���̴� ���α׷��� ���������� �ϳ��� �Լ��̰� ���̴��� �ԷµǴ�
    // �ڿ����� �Լ��� �Ű������鿡 �ش��ϹǷ�, ��Ʈ �ñ״�ó�� �� �Լ� ������ �����ϴ�
    // �����̶� �� �� �ִ�.

	// ��Ʈ �Ű������� ���̺��̰ų� ��Ʈ ������ �Ǵ� ��Ʈ ����̴�.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// CVB �ϳ��� ��� ������ ���̺� �����Ѵ�.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
        1, //���̺��� ������ ����
        0); //�� ��Ʈ �Ű������� ���� ���̴� �μ����� ���� �������� ��ȣ.

	slotRootParameter[0].InitAsDescriptorTable(
        1, // ���̺��� ������ ����
        &cbvTable); //�������� �迭�� ����Ű�� ������

	// ��Ʈ ������ ��Ʈ �Ű��������� �迭�̴�.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// ��� ���� �ϳ��� ������ ������ ������ ����Ű��
    // ���� �ϳ��� �̷���� ��Ʈ ������ �����Ѵ�.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if(errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShadersAndInputLayout()
{
    HRESULT hr = S_OK;
    
	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void BoxApp::BuildBoxGeometry()
{
    std::array<VPosData, 13> vertices =
    {
        //������ü ���� ����
        VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f) }),

        //�簢�� ���� ����
        VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+0.0f, +1.0f, +0.0f) })
    };

    std::array<VColorData, 13> colors =
    {
        //������ü �� ����
        VColorData({XMFLOAT4(Colors::White) }),
        VColorData({XMFLOAT4(Colors::Black) }),
        VColorData({XMFLOAT4(Colors::Red) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Blue) }),
        VColorData({XMFLOAT4(Colors::Yellow) }),
        VColorData({XMFLOAT4(Colors::Cyan) }),
        VColorData({XMFLOAT4(Colors::Magenta) }),

        //�簢�� �� ����
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Red) })
    };

	std::array<std::uint16_t, 54> indices =   //std::array<std::uint16_t, 36> indices =
	{
        //������ü �ε���
        0, 1, 2,
        0, 2, 3,

        4, 6, 5,
        4, 7, 6,

        4, 5, 1,
        4, 1, 0,

        3, 2, 6,
        3, 6, 7,

        1, 5, 6,
        1, 6, 2,

        4, 0, 3,
        4, 3, 7,

        //�Ƕ�̵� �ε���
        0, 1, 2,
        1, 3, 2,

        0, 4, 1,

        1, 4, 3,

        3, 4, 2,
        2, 4, 0
	};

    const UINT vbByteSize = (UINT)vertices.size() * sizeof(VPosData);
    const UINT cvByteSize = (UINT)colors.size() * sizeof(VColorData);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	mBoxGeo = std::make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(cvByteSize, &mBoxGeo->ColorBufferCPU))
    CopyMemory(mBoxGeo->ColorBufferCPU->GetBufferPointer(), colors.data(), cvByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);



    //���� ���۸� �⺻ ���� �����Ѵ�.
	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

    //���� ���� �⺻ ���� �����Ѵ�.
    mBoxGeo->ColorBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), colors.data(), cvByteSize, mBoxGeo->ColorBufferUploader);

    //�ε��� ���� �⺻ ���� �����Ѵ�.
	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(VPosData);
	mBoxGeo->VertexBufferByteSize = vbByteSize;

    mBoxGeo->ColorByteStride = sizeof(VColorData);
    mBoxGeo->ColorBufferByteSize = cvByteSize;

	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;


    //�ε������� SubmeshGeometry�� �����Ѵ�.
    //������ü
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;

    SubmeshGeometry submesh_pyramid;
    submesh_pyramid.IndexCount = (UINT)(indices.size() - 36);
    submesh_pyramid.StartIndexLocation = 36;
    submesh_pyramid.BaseVertexLocation = 8;

    mBoxGeo->DrawArgs["pyramid"] = submesh_pyramid;
}

void BoxApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS = 
	{ 
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()), 
		mvsByteCode->GetBufferSize() 
	};
    psoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()), 
		mpsByteCode->GetBufferSize() 
	};
    CD3DX12_RASTERIZER_DESC rast(D3D12_DEFAULT);
    rast.FillMode = D3D12_FILL_MODE_SOLID;
    rast.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(rast);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}
