//***************************************************************************************
// BoxApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Direct3D 12에서 상자를 그리는 방법을 보여준다.
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


//정점과 색상 구조체 나누기
struct VPosData
{
    XMFLOAT3 Pos;
};

struct VColorData
{
    XMFLOAT4 Color;
};

//상수 버퍼 추가
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
	// 디버그 빌드에서는 실행 시점 메모리 점검 기능을 켠다.
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
		
    // 초기화 명령들을 준비하기 위해 명령 목록을 재설정한다.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
 
    BuildDescriptorHeaps();
	BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPSO();

    // 초기화 명령들을 실행한다.
    ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // 초기화가 완료될 때까지 기다린다.
    FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

    // 창의 크기가 바뀌었으므로 종횡비를 갱신하고
    // 투영 행렬을 다시 계산한다.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
    // 구면 좌표를 데카르트 좌표(직교 좌표)로 변환한다.
    float x = mRadius*sinf(mPhi)*cosf(mTheta);
    float z = mRadius*sinf(mPhi)*sinf(mTheta);
    float y = mRadius*cosf(mPhi);

    // 시야 행렬을 구축한다.
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);


    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    mWorld._41 = -1.2f;

    XMMATRIX world = XMLoadFloat4x4(&mWorld);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    XMMATRIX worldViewProj = world*view*proj;



	// 최신의 worldViewProj 행렬로 상수 버퍼를 갱신한다.
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
    // 명령 기록에 관련된 메모리의 재활용을 위해 명령 할당자를
    // 재설정한다. 재설정은 GPU가 관련 명령 목록들을
    // 모두 처리한 후에 일어난다.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// 명령 목록을 ExecuteCommandList를 통해서 명령 대기열에
    // 추가했다면 명령 목록을 재설정할 수 있다. 명령 목록을
    // 재설정하면 메모리가 재활용된다.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // 후면 버퍼와 깊이 버퍼를 지운다.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	
    // 렌더링 결과가 기록될 대상 버퍼들을 지정한다.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView()); //(시작 슬롯, 입력 슬롯들에 묶을 정점 버퍼 개수, 정점 버퍼 뷰 배열의 첫 원소를 가리키는 포인터)
	mCommandList->IASetVertexBuffers(1, 1, &mBoxGeo->ColorBufferView()); //(시작 슬롯, 입력 슬롯들에 묶을 정점 버퍼 개수, 정점 버퍼 뷰 배열의 첫 원소를 가리키는 포인터)
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
	
    // 자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 명령들의 기록을 마친다.
	ThrowIfFailed(mCommandList->Close());
 
    // 명령 실행을 위해 명령 목록을 대기열에 추가한다.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	
	// 후면 버퍼와 전면 버퍼를 교환한다.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// 이 프레임의 명령들이 모두 처리되길 기다린다. 이러한 대기는
    // 비효율적이다. 이번에는 예제의 간단함을 위해 이 방법을 사용하지만,
    // 이후의 예제들에서는 렌더링 코드를 적절히 조직화해서 프레임마다 대기할
    // 필요가 없게 만든다.
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
        // 마우스 한 픽셀 이동을 4분의 1도에 대응시킨다.
        float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
        float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

        // 입력에 기초해 각도를 갱신해서 카메라가 상자를 중심으로 공전하게 한다.
        mTheta += dx;
        mPhi += dy;

        // mPhi 각도를 제한한다.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
    }
    else if((btnState & MK_RBUTTON) != 0)
    {
        // 마우스 한 픽셀 이동을 장면의 0.005단위에 대응시킨다.
        float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

        // 입력에 기초해서 카메라 반지름을 갱신한다.
        mRadius += dx - dy;

        // 반지름을 제한한다.
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void BoxApp::BuildDescriptorHeaps()
{
    //상수 버퍼 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = 2; //2개 생성
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers()
{
    //물체 1개의 상수 자료를 담을 상수 버퍼
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 2, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    //버퍼 자체의 시작 주소(0번째 상수 버퍼의 주소)
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    // 버퍼에 담긴 0번째 상수 버퍼의 오프셋
    int boxCBufIndex = 0;
	cbAddress += boxCBufIndex*objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

    //상수 버퍼 뷰 생성
	md3dDevice->CreateConstantBufferView(
		&cbvDesc,
		mCbvHeap->GetCPUDescriptorHandleForHeapStart());


    //피라미드 생성
    cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

    // 버퍼에 담긴 1번째 상수 버퍼의 오프셋
    boxCBufIndex = 1;
    cbAddress += boxCBufIndex * objCBByteSize;

    cbvDesc.BufferLocation = cbAddress;

    // CD3DX12_CPU_DESCRIPTOR_HANDLE으로 서술자 힙에 접근하여 Offset을 수정한다.
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(boxCBufIndex, mCbvSrvUavDescriptorSize);

    //상수 버퍼 뷰 생성
    md3dDevice->CreateConstantBufferView(
        &cbvDesc,
        handle);
}

void BoxApp::BuildRootSignature()
{
	// 일반적으로 셰이더 프로그램은 특정 자원들(상수 버퍼, 텍스처, 표본추출기 등)이
    // 입력된다고 기대한다. 루트 시그니처은 셰이더 프로그램이 기대하는 자원들을
    // 정의한다. 셰이더 프로그램은 본질적으로 하나의 함수이고 셰이더에 입력되는
    // 자원들은 함수의 매개변수들에 해당하므로, 루트 시그니처는 곧 함수 서명을 정의하는
    // 수단이라 할 수 있다.

	// 루트 매개변수는 테이블이거나 루트 서술자 또는 루트 상수이다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// CVB 하나를 담는 서술자 테이블 생성한다.
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
        1, //테이블의 서술자 개수
        0); //이 루트 매개변수에 묶일 셰이더 인수들의 기준 레지스터 번호.

	slotRootParameter[0].InitAsDescriptorTable(
        1, // 테이블의 서술자 개수
        &cbvTable); //구간들의 배열을 가리키는 포인터

	// 루트 서명은 루트 매개변수들의 배열이다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 상수 버퍼 하나로 구성된 서술자 구간을 가리키는
    // 슬롯 하나로 이루어진 루트 서명을 생성한다.
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
        //정육면체 정점 정보
        VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f) }),

        //사각뿔 정점 정보
        VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
        VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f) }),
        VPosData({ XMFLOAT3(+0.0f, +1.0f, +0.0f) })
    };

    std::array<VColorData, 13> colors =
    {
        //정육면체 색 정보
        VColorData({XMFLOAT4(Colors::White) }),
        VColorData({XMFLOAT4(Colors::Black) }),
        VColorData({XMFLOAT4(Colors::Red) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Blue) }),
        VColorData({XMFLOAT4(Colors::Yellow) }),
        VColorData({XMFLOAT4(Colors::Cyan) }),
        VColorData({XMFLOAT4(Colors::Magenta) }),

        //사각뿔 색 정보
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Green) }),
        VColorData({XMFLOAT4(Colors::Red) })
    };

	std::array<std::uint16_t, 54> indices =   //std::array<std::uint16_t, 36> indices =
	{
        //정육면체 인덱스
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

        //피라미드 인덱스
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



    //정점 버퍼를 기본 힙에 복사한다.
	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);

    //색상 버퍼 기본 힙에 복사한다.
    mBoxGeo->ColorBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
        mCommandList.Get(), colors.data(), cvByteSize, mBoxGeo->ColorBufferUploader);

    //인덱스 버퍼 기본 힙에 복사한다.
	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(VPosData);
	mBoxGeo->VertexBufferByteSize = vbByteSize;

    mBoxGeo->ColorByteStride = sizeof(VColorData);
    mBoxGeo->ColorBufferByteSize = cvByteSize;

	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;


    //인덱스들을 SubmeshGeometry에 세팅한다.
    //정육면체
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
