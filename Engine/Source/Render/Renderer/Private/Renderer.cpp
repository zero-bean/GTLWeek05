#include "pch.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Render/FontRenderer/Public/FontRenderer.h"
#include "Component/Public/UUIDTextComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Editor/Public/Editor.h"
#include "Editor/Public/Viewport.h"
#include "Editor/Public/ViewportClient.h"
#include "Editor/Public/Camera.h"
#include "Level/Public/Level.h"

#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"
#include "Component/Mesh/Public/StaticMesh.h"
#include "Optimization/Public/OcclusionCuller.h"

IMPLEMENT_SINGLETON_CLASS_BASE(URenderer)

URenderer::URenderer() = default;

URenderer::~URenderer() = default;

void URenderer::Init(HWND InWindowHandle)
{
	DeviceResources = new UDeviceResources(InWindowHandle);
	Pipeline = new UPipeline(GetDeviceContext());
	ViewportClient = new FViewport();

	// 래스터라이저 상태 생성
	CreateRasterizerState();
	CreateDepthStencilState();
	CreateDefaultShader();
	CreateTextureShader();
	CreateConstantBuffer();

	// FontRenderer 초기화
	FontRenderer = new UFontRenderer();
	if (!FontRenderer->Initialize())
	{
		UE_LOG("FontRenderer 초기화 실패");
		SafeDelete(FontRenderer);
	}

	ViewportClient->InitializeLayout(DeviceResources->GetViewportInfo());
}

void URenderer::Release()
{
	ReleaseConstantBuffer();
	ReleaseDefaultShader();
	ReleaseDepthStencilState();
	ReleaseRasterizerState();

	SafeDelete(ViewportClient);

	// FontRenderer 해제
	SafeDelete(FontRenderer);

	SafeDelete(Pipeline);
	SafeDelete(DeviceResources);
}

/**
 * @brief 래스터라이저 상태를 생성하는 함수
 */
void URenderer::CreateRasterizerState()
{
	// 현재 따로 생성하지 않음
}

/**
 * @brief Renderer에서 주로 사용할 Depth-Stencil State를 생성하는 함수
 */
void URenderer::CreateDepthStencilState()
{
	// 3D Default Depth Stencil 설정 (Depth 판정 O, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DefaultDescription = {};

	DefaultDescription.DepthEnable = TRUE;
	DefaultDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DefaultDescription.DepthFunc = D3D11_COMPARISON_LESS;

	DefaultDescription.StencilEnable = FALSE;
	DefaultDescription.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DefaultDescription.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	GetDevice()->CreateDepthStencilState(&DefaultDescription, &DefaultDepthStencilState);

	// Disabled Depth Stencil 설정 (Depth 판정 X, Stencil 판정 X)
	D3D11_DEPTH_STENCIL_DESC DisabledDescription = {};

	DisabledDescription.DepthEnable = FALSE;
	DisabledDescription.StencilEnable = FALSE;

	GetDevice()->CreateDepthStencilState(&DisabledDescription, &DisabledDepthStencilState);
}

/**
 * @brief Shader 기반의 CSO 생성 함수
 */
void URenderer::CreateDefaultShader()
{
	ID3DBlob* VertexShaderCSO;
	ID3DBlob* PixelShaderCSO;

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&VertexShaderCSO, nullptr);

	GetDevice()->CreateVertexShader(VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), nullptr, &DefaultVertexShader);

	D3DCompileFromFile(L"Asset/Shader/SampleShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&PixelShaderCSO, nullptr);

	GetDevice()->CreatePixelShader(PixelShaderCSO->GetBufferPointer(),
		PixelShaderCSO->GetBufferSize(), nullptr, &DefaultPixelShader);

	D3D11_INPUT_ELEMENT_DESC DefaultLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};

	GetDevice()->CreateInputLayout(DefaultLayout, ARRAYSIZE(DefaultLayout), VertexShaderCSO->GetBufferPointer(),
		VertexShaderCSO->GetBufferSize(), &DefaultInputLayout);

	Stride = sizeof(FNormalVertex);

	VertexShaderCSO->Release();
	PixelShaderCSO->Release();
}

void URenderer::CreateTextureShader()
{
	ID3DBlob* TextureVSBlob;
	ID3DBlob* TexturePSBlob;

	D3DCompileFromFile(L"Asset/Shader/TextureShader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0,
		&TextureVSBlob, nullptr);

	GetDevice()->CreateVertexShader(TextureVSBlob->GetBufferPointer(),
		TextureVSBlob->GetBufferSize(), nullptr, &TextureVertexShader);

	D3DCompileFromFile(L"Asset/Shader/TextureShader.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0,
		&TexturePSBlob, nullptr);

	GetDevice()->CreatePixelShader(TexturePSBlob->GetBufferPointer(),
		TexturePSBlob->GetBufferSize(), nullptr, &TexturePixelShader);

	D3D11_INPUT_ELEMENT_DESC TextureLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FNormalVertex, Normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FNormalVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0	},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FNormalVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0	}
	};
	GetDevice()->CreateInputLayout(TextureLayout, ARRAYSIZE(TextureLayout), TextureVSBlob->GetBufferPointer(),
		TextureVSBlob->GetBufferSize(), &TextureInputLayout);

	// TODO(KHJ): ShaderBlob 파일로 저장하고, 이후 이미 존재하는 경우 컴파일 없이 Blob을 로드할 수 있도록 할 것
	// TODO(KHJ): 실제 텍스처용 셰이더를 별도로 생성해야 함 (UV 좌표 포함)

	TextureVSBlob->Release();
	TexturePSBlob->Release();
}

/**
 * @brief 래스터라이저 상태를 해제하는 함수
 */
void URenderer::ReleaseRasterizerState()
{
	for (auto& Cache : RasterCache)
	{
		if (Cache.second != nullptr)
		{
			Cache.second->Release();
		}
	}
	RasterCache.clear();
}

/**
 * @brief Shader Release
 */
void URenderer::ReleaseDefaultShader()
{
	if (DefaultInputLayout)
	{
		DefaultInputLayout->Release();
		DefaultInputLayout = nullptr;
	}

	if (DefaultPixelShader)
	{
		DefaultPixelShader->Release();
		DefaultPixelShader = nullptr;
	}

	if (DefaultVertexShader)
	{
		DefaultVertexShader->Release();
		DefaultVertexShader = nullptr;
	}
}

/**
 * @brief 렌더러에 사용된 모든 리소스를 해제하는 함수
 */
void URenderer::ReleaseDepthStencilState()
{
	if (DefaultDepthStencilState)
	{
		DefaultDepthStencilState->Release();
		DefaultDepthStencilState = nullptr;
	}

	if (DisabledDepthStencilState)
	{
		DisabledDepthStencilState->Release();
		DisabledDepthStencilState = nullptr;
	}

	// 렌더 타겟을 초기화
	if (GetDeviceContext())
	{
		GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);
	}
}

// Renderer.cpp
void URenderer::Update()
{
	RenderBegin();

	// FViewportClient로부터 모든 뷰포트를 가져옵니다.
	for (FViewportClient& ViewportClient : ViewportClient->GetViewports())
	{
		// 0. 현재 뷰포트가 닫혀있다면 렌더링을 하지 않습니다.
		if (ViewportClient.GetViewportInfo().Width < 1.0f || ViewportClient.GetViewportInfo().Height < 1.0f) { continue; }

		// 1. 현재 뷰포트의 영역을 설정합니다.
		ViewportClient.Apply(GetDeviceContext());

		// 2. 카메라의 View/Projection 행렬로 상수 버퍼를 업데이트합니다.
		UCamera* CurrentCamera = &ViewportClient.Camera;
		CurrentCamera->Update(ViewportClient.GetViewportInfo());
		UpdateConstantBuffer(ConstantBufferViewProj, CurrentCamera->GetFViewProjConstants(), 1, true);

		// 3. 씬(레벨, 에디터 요소 등)을 이 뷰포트와 카메라 기준으로 렌더링합니다.
		{
			TIME_PROFILE(RenderLevel)
				RenderLevel(CurrentCamera);
		}

		// 4. 에디터를 렌더링합니다.
		{
			TIME_PROFILE(RenderEditor)
			GEditor->GetEditorModule()->RenderEditor(CurrentCamera);
		}

	}

	// 최상위 에디터/GUI는 프레임에 1회만
	{
		TIME_PROFILE(UUIManager)
			UUIManager::GetInstance().Render();
	}
	{
		TIME_PROFILE(UStatOverlay)
			UStatOverlay::GetInstance().Render();
	}

	RenderEnd(); // Present 1회
}


/**
 * @brief Render Prepare Step
 */
void URenderer::RenderBegin() const
{
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	GetDeviceContext()->ClearRenderTargetView(RenderTargetView, ClearColor);
	auto* DepthStencilView = DeviceResources->GetDepthStencilView();
	GetDeviceContext()->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* rtvs[] = { RenderTargetView }; // 배열 생성

	GetDeviceContext()->OMSetRenderTargets(1, rtvs, DeviceResources->GetDepthStencilView());
	DeviceResources->UpdateViewport();
}

/**
 * @brief Buffer에 데이터 입력 및 Draw
 */

void URenderer::RenderLevel(UCamera* InCurrentCamera)
{
	const TObjectPtr<ULevel>& CurrentLevel = GWorld->GetLevel();

	// Level 없으면 Early Return
	if (!CurrentLevel) { return; }

	uint64 ShowFlags =  GWorld->GetLevel()->GetShowFlags();

	TArray<TObjectPtr<UPrimitiveComponent>> DefaultPrimitives;
	TArray<TObjectPtr<UBillBoardComponent>> BillBoards;
	TArray<TObjectPtr<UTextComponent>> Texts;

	if (ShowFlags & EEngineShowFlags::SF_Primitives)
	{
		// 오클루전 컬링
		TIME_PROFILE(Occlusion)
		static COcclusionCuller Culler;
		const FViewProjConstants& ViewProj = InCurrentCamera->GetFViewProjConstants();
		Culler.InitializeCuller(ViewProj.View, ViewProj.Projection);
		TArray<TObjectPtr<UPrimitiveComponent>> FinalVisiblePrims = Culler.PerformCulling(
			InCurrentCamera->GetViewVolumeCuller().GetRenderableObjects(),
			InCurrentCamera->GetLocation()
		);
		TIME_PROFILE_END(Occlusion)

			TIME_PROFILE(FinalVisiblePrims)
		TArray<TObjectPtr<UStaticMeshComponent>> FinalVisibleMeshes;
		FinalVisibleMeshes.reserve(FinalVisiblePrims.size());
		for (auto& Prim : FinalVisiblePrims)
		{
			TObjectPtr<UStaticMeshComponent> StaticMesh = Cast<UStaticMeshComponent>(Prim);
			if (StaticMesh)
			{
				FinalVisibleMeshes.push_back(StaticMesh);
				continue;
			}

			TObjectPtr<UBillBoardComponent> BillBoard = Cast<UBillBoardComponent>(Prim);
			if (BillBoard)
			{
				BillBoards.push_back(BillBoard);
				continue;
			}

			TObjectPtr<UTextComponent> Text = Cast<UTextComponent>(Prim);
			if (Text && !Text->IsExactly(UUUIDTextComponent::StaticClass()))
			{
				Texts.push_back(Text);
				if (GEditor->GetEditorModule()->GetPickedBillboard() == \
					Cast<UUUIDTextComponent>(Text))
				continue;
			}

			DefaultPrimitives.push_back(Prim);
		}
		TIME_PROFILE_END(FinalVisiblePrims)
		RenderStaticMeshes(FinalVisibleMeshes);


		TIME_PROFILE(PrimitiveComponent)
		for (auto& PrimitiveComponent : DefaultPrimitives)
		{
			if (Cast<UTextComponent>(PrimitiveComponent) ||
				PrimitiveComponent->IsExactly(UUUIDTextComponent::StaticClass()) ||
				PrimitiveComponent->IsExactly(UTextComponent::StaticClass()))
				continue;

			FRenderState RenderState = PrimitiveComponent->GetRenderState();
			const EViewModeIndex ViewMode = GEditor->GetEditorModule()->GetViewMode();
			if (ViewMode == EViewModeIndex::VMI_Wireframe)
			{
				RenderState.CullMode = ECullMode::None;
				RenderState.FillMode = EFillMode::WireFrame;
			}
			ID3D11RasterizerState* LoadedRasterizerState = GetRasterizerState(RenderState);

			RenderPrimitiveDefault(PrimitiveComponent, LoadedRasterizerState);
		}
		TIME_PROFILE_END(PrimitiveComponent)
		
		RenderText(InCurrentCamera, Texts);
		RenderBillBoard(InCurrentCamera, BillBoards);
	}

	TIME_PROFILE(RenderUUID)
	if (ShowFlags & EEngineShowFlags::SF_BillboardText)
	{
		if (UUUIDTextComponent* PickedBillboard = GEditor->GetEditorModule()->GetPickedBillboard())
		{
			RenderUUID(PickedBillboard, InCurrentCamera);
		}
	}
}

/**
 * @brief Editor용 Primitive를 렌더링하는 함수 (Gizmo, Axis 등)
 * @param InPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 */
void URenderer::RenderPrimitive(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState)
{
	// Always visible 옵션에 따라 Depth 테스트 여부 결정
	ID3D11DepthStencilState* DepthStencilState =
		InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

	ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

	// Pipeline 정보 구성
	FPipelineInfo PipelineInfo = {
		DefaultInputLayout,
		DefaultVertexShader,
		RasterizerState,
		DepthStencilState,
		DefaultPixelShader,
		nullptr,
		InPrimitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	// Update constant buffers
	UpdateConstantBuffer(ConstantBufferModels,
		FMatrix::GetModelMatrix(InPrimitive.Location, FVector::GetDegreeToRadian(InPrimitive.Rotation), InPrimitive.Scale), 0, true);

	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);
	UpdateConstantBuffer(ConstantBufferColor, InPrimitive.Color, 2, true);

	// Set vertex buffer and draw
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, Stride);
	Pipeline->Draw(InPrimitive.NumVertices, 0);
}

/**
 * @brief Index Buffer를 사용하는 Editor Primitive 렌더링 함수
 * @param InPrimitive 렌더링할 에디터 프리미티브
 * @param InRenderState 렌더링 상태
 * @param bInUseBaseConstantBuffer 기본 상수 버퍼 사용 여부
 * @param InStride 정점 스트라이드
 * @param InIndexBufferStride 인덱스 버퍼 스트라이드
 */
void URenderer::RenderPrimitiveIndexed(const FEditorPrimitive& InPrimitive, const FRenderState& InRenderState,
	bool bInUseBaseConstantBuffer, uint32 InStride, uint32 InIndexBufferStride)
{
	// Always visible 옵션에 따라 Depth 테스트 여부 결정
	ID3D11DepthStencilState* DepthStencilState =
		InPrimitive.bShouldAlwaysVisible ? DisabledDepthStencilState : DefaultDepthStencilState;

	ID3D11RasterizerState* RasterizerState = GetRasterizerState(InRenderState);

	// 커스텀 셰이더가 있으면 사용, 없으면 기본 셰이더 사용
	ID3D11InputLayout* InputLayout = InPrimitive.InputLayout ? InPrimitive.InputLayout : DefaultInputLayout;
	ID3D11VertexShader* VertexShader = InPrimitive.VertexShader ? InPrimitive.VertexShader : DefaultVertexShader;
	ID3D11PixelShader* PixelShader = InPrimitive.PixelShader ? InPrimitive.PixelShader : DefaultPixelShader;

	// Pipeline 정보 구성
	FPipelineInfo PipelineInfo = {
		InputLayout,
		VertexShader,
		RasterizerState,
		DepthStencilState,
		PixelShader,
		nullptr,
		InPrimitive.Topology
	};

	Pipeline->UpdatePipeline(PipelineInfo);

	// 기본 상수 버퍼 사용하는 경우에만 업데이트
	if (bInUseBaseConstantBuffer)
	{
		UpdateConstantBuffer(ConstantBufferModels,
			FMatrix::GetModelMatrix(InPrimitive.Location, FVector::GetDegreeToRadian(InPrimitive.Rotation), InPrimitive.Scale), 0, true);
		Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);
		UpdateConstantBuffer(ConstantBufferColor, InPrimitive.Color, 2, true);
	}

	// Set buffers and draw indexed
	Pipeline->SetIndexBuffer(InPrimitive.IndexBuffer, InIndexBufferStride);
	Pipeline->SetVertexBuffer(InPrimitive.Vertexbuffer, InStride);
	Pipeline->DrawIndexed(InPrimitive.NumIndices, 0, 0);
}

/**
 * @brief 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
 */
void URenderer::RenderEnd() const
{
	TIME_PROFILE(DrawCall)
	GetSwapChain()->Present(0, 0); // 1: VSync 활성화
	TIME_PROFILE_END(DrawCall)
}

void URenderer::RenderStaticMeshes(TArray<TObjectPtr<UStaticMeshComponent>>& MeshComponents)
{
	TIME_PROFILE(RenderStaticMeshes)
	sort(MeshComponents.begin(), MeshComponents.end(),
		[](TObjectPtr<UStaticMeshComponent> A, TObjectPtr<UStaticMeshComponent> B) {
			uint64_t MeshA = A->GetStaticMesh() ? A->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			uint64_t MeshB = B->GetStaticMesh() ? B->GetStaticMesh()->GetAssetPathFileName().GetComparisonIndex() : 0;
			return MeshA < MeshB;
		});

	FStaticMesh* CurrentMeshAsset = nullptr;
	UMaterial* CurrentMaterial = nullptr;
	ID3D11RasterizerState* CurrentRasterizer = nullptr;
	const EViewModeIndex ViewMode =  GEditor->GetEditorModule()->GetViewMode();

	for (UStaticMeshComponent* MeshComp : MeshComponents) 
	{
		if (!MeshComp->GetStaticMesh()) { continue; }
		FStaticMesh* MeshAsset = MeshComp->GetStaticMesh()->GetStaticMeshAsset();
		if (!MeshAsset) { continue; }

		// RasterizerState 설정
		FRenderState RenderState = MeshComp->GetRenderState();
		if (ViewMode == EViewModeIndex::VMI_Wireframe) 
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		ID3D11RasterizerState* RasterizerState = GetRasterizerState(RenderState);

		// Pipeline 변경시에만 업데이트
		if (CurrentRasterizer != RasterizerState) 
		{
			static FPipelineInfo PipelineInfo = {
				TextureInputLayout,
				TextureVertexShader,
				RasterizerState,
				DefaultDepthStencilState,
				TexturePixelShader,
				nullptr,
			};
			PipelineInfo.RasterizerState = RasterizerState;
			Pipeline->UpdatePipeline(PipelineInfo);
			CurrentRasterizer = RasterizerState;
		}

		// Mesh 변경시에만 버퍼 바인딩
		if (CurrentMeshAsset != MeshAsset)
		{
			Pipeline->SetVertexBuffer(MeshComp->GetVertexBuffer(), sizeof(FNormalVertex));
			Pipeline->SetIndexBuffer(MeshComp->GetIndexBuffer(), 0);
			CurrentMeshAsset = MeshAsset;
		}

		// Transform 업데이트 (메시별로)
		UpdateConstantBuffer(ConstantBufferModels, MeshComp->GetWorldTransformMatrix(), 0, true);

		// 머티리얼이 없으면 전체 메시 렌더링
		if (MeshAsset->MaterialInfo.empty() || MeshComp->GetStaticMesh()->GetNumMaterials() == 0) 
		{
			Pipeline->DrawIndexed(MeshAsset->Indices.size(), 0, 0);
			continue;
		}

		if (MeshComp->IsScrollEnabled()) 
		{
			UTimeManager& TimeManager = UTimeManager::GetInstance();
			MeshComp->SetElapsedTime(MeshComp->GetElapsedTime() + TimeManager.GetDeltaTime());
		}

		// 섹션별 렌더링
		for (const FMeshSection& section : MeshAsset->Sections)
		{
			UMaterial* Material = MeshComp->GetMaterial(section.MaterialSlot);

			// Material 변경시에만 상수버퍼 + 텍스처 바인딩
			if (CurrentMaterial != Material) {

				FMaterialConstants MaterialConstants = {};
				FVector AmbientColor = Material->GetAmbientColor(); MaterialConstants.Ka = FVector4(AmbientColor.X, AmbientColor.Y, AmbientColor.Z, 1.0f);
				FVector DiffuseColor = Material->GetDiffuseColor(); MaterialConstants.Kd = FVector4(DiffuseColor.X, DiffuseColor.Y, DiffuseColor.Z, 1.0f);
				FVector SpecularColor = Material->GetSpecularColor(); MaterialConstants.Ks = FVector4(SpecularColor.X, SpecularColor.Y, SpecularColor.Z, 1.0f);
				MaterialConstants.Ns = Material->GetSpecularExponent();
				MaterialConstants.Ni = Material->GetRefractionIndex();
				MaterialConstants.D = Material->GetDissolveFactor();
				MaterialConstants.MaterialFlags = 0;
				MaterialConstants.Time = MeshComp->GetElapsedTime();

				// Update Constant Buffer
				UpdateConstantBuffer(ConstantBufferMaterial, MaterialConstants, 2, false);

				if (Material->GetDiffuseTexture())
				{
					auto* Proxy = Material->GetDiffuseTexture()->GetRenderProxy();
					Pipeline->SetTexture(0, false, Proxy->GetSRV());
					Pipeline->SetSamplerState(0, false, Proxy->GetSampler());
				}
				if (Material->GetAmbientTexture())
				{
					auto* Proxy = Material->GetAmbientTexture()->GetRenderProxy();
					Pipeline->SetTexture(1, false, Proxy->GetSRV());
				}
				if (Material->GetSpecularTexture())
				{
					auto* Proxy = Material->GetSpecularTexture()->GetRenderProxy();
					Pipeline->SetTexture(2, false, Proxy->GetSRV());
				}
				if (Material->GetAlphaTexture())
				{
					auto* Proxy = Material->GetAlphaTexture()->GetRenderProxy();
					Pipeline->SetTexture(4, false, Proxy->GetSRV());
				}

				CurrentMaterial = Material;
			}

			Pipeline->DrawIndexed(section.IndexCount, section.StartIndex, 0);
		}
	}
}

void URenderer::RenderBillBoard(UCamera* InCurrentCamera, TArray<TObjectPtr<UBillBoardComponent>>& InBillBoardComp)
{
	ID3D11RasterizerState* CurrentRasterizer = nullptr;
	const EViewModeIndex ViewMode = GEditor->GetEditorModule()->GetViewMode();

	// Diffuse 말고 다른 모든 텍스처를 초기화한다.
	Pipeline->SetTexture(1, false, nullptr);
	Pipeline->SetTexture(2, false, nullptr);
	Pipeline->SetTexture(4, false, nullptr);

	for (UBillBoardComponent* BillBoardComp : InBillBoardComp)
	{
		BillBoardComp->FaceCamera(
			InCurrentCamera->GetLocation(),
			InCurrentCamera->GetUp(),
			InCurrentCamera->GetRight()
		);

		// RasterizerState 설정
		FRenderState RenderState = BillBoardComp->GetRenderState();
		if (ViewMode == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		ID3D11RasterizerState* RasterizerState = GetRasterizerState(RenderState);

		// Pipeline 변경시에만 업데이트
		if (CurrentRasterizer != RasterizerState)
		{
			static FPipelineInfo PipelineInfo = {
				TextureInputLayout,
				TextureVertexShader,
				RasterizerState,
				DefaultDepthStencilState,
				TexturePixelShader,
				nullptr,
			};
			PipelineInfo.RasterizerState = RasterizerState;
			Pipeline->UpdatePipeline(PipelineInfo);
			CurrentRasterizer = RasterizerState;
		}

		Pipeline->SetVertexBuffer(BillBoardComp->GetVertexBuffer(), sizeof(FNormalVertex));
		Pipeline->SetIndexBuffer(BillBoardComp->GetIndexBuffer(), 0);

		// Transform 업데이트 (메시별로)
		UpdateConstantBuffer(ConstantBufferModels, BillBoardComp->GetWorldTransformMatrix(), 0, true);

		Pipeline->SetTexture(0, false, BillBoardComp->GetSprite().second);
		Pipeline->SetSamplerState(0, false, const_cast<ID3D11SamplerState*>(BillBoardComp->GetSampler()));

		Pipeline->DrawIndexed(BillBoardComp->GetNumIndices(), 0, 0);
	}
}

void  URenderer::RenderText(UCamera* InCurrentCamera, TArray<TObjectPtr<UTextComponent>>& InTextComp)
{
	const FViewProjConstants& ViewProj = InCurrentCamera->GetFViewProjConstants();

	ID3D11RasterizerState* LoadedRasterizerState = nullptr;

	for (const TObjectPtr<UTextComponent>& Text : InTextComp)
	{
		FRenderState RenderState = Text->GetRenderState();
		const EViewModeIndex ViewMode = GEditor->GetEditorModule()->GetViewMode();
		if (ViewMode == EViewModeIndex::VMI_Wireframe)
		{
			RenderState.CullMode = ECullMode::None;
			RenderState.FillMode = EFillMode::WireFrame;
		}
		LoadedRasterizerState = GetRasterizerState(RenderState);

		FontRenderer->RenderText(
			Text->GetText().c_str(),
			Text->GetWorldTransformMatrix(),
			ViewProj,
			0.0f,
			-1.0f,
			1.0f,
			2.0f,
			true
		);
	}

	//// Pipeline 정보 구성
	//FPipelineInfo PipelineInfo = {
	//	InputLayout,
	//	VertexShader,
	//	RasterizerState,
	//	DepthStencilState,
	//	PixelShader,
	//	nullptr,
	//	InPrimitive.Topology
	//};

	// RenderText 이후 파이프라인 복원
	FPipelineInfo PipelineInfo = {};

	PipelineInfo = {
		DefaultInputLayout,
		DefaultVertexShader,
		nullptr,
		DefaultDepthStencilState,
		DefaultPixelShader,
		nullptr,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	Pipeline->UpdatePipeline(PipelineInfo);
}

void URenderer::RenderUUID(UUUIDTextComponent* InBillBoardComp, UCamera* InCurrentCamera)
{
	if (!InCurrentCamera)	return;

	// 이제 올바른 카메라 위치를 전달하여 빌보드 회전 업데이트
	InBillBoardComp->UpdateRotationMatrix(InCurrentCamera->GetLocation());

	FString UUIDString = "UID: " + std::to_string(InBillBoardComp->GetUUID());
	FMatrix RT = InBillBoardComp->GetRTMatrix();

	// UEditor에서 가져오는 대신, 인자로 받은 카메라의 ViewProj 행렬을 사용
	const FViewProjConstants& viewProjConstData = InCurrentCamera->GetFViewProjConstants();
	FontRenderer->RenderText(UUIDString.c_str(), RT, viewProjConstData);
}

void URenderer::RenderPrimitiveDefault(UPrimitiveComponent* InPrimitiveComp, ID3D11RasterizerState* InRasterizerState)
{
	// Update pipeline info
	FPipelineInfo PipelineInfo = {
		DefaultInputLayout,
		DefaultVertexShader,
		InRasterizerState,
		DefaultDepthStencilState,
		DefaultPixelShader,
		nullptr,
	};
	Pipeline->UpdatePipeline(PipelineInfo);

	// Update pipeline buffers
	//UpdateConstantBuffer(ConstantBufferModels,
	//	FMatrix::GetModelMatrix(InPrimitiveComp->GetRelativeLocation(), FVector::GetDegreeToRadian(InPrimitiveComp->GetRelativeRotation()), InPrimitiveComp->GetRelativeScale3D()),
	//	0, true);
	
	Pipeline->SetConstantBuffer(2, false, ConstantBufferColor);

	// Transform 업데이트 (메시별로)
	UpdateConstantBuffer(ConstantBufferModels, InPrimitiveComp->GetWorldTransformMatrix(), 0, true);
	UpdateConstantBuffer(ConstantBufferColor, InPrimitiveComp->GetColor(), 2, true);

	// Bind vertex buffer
	Pipeline->SetVertexBuffer(InPrimitiveComp->GetVertexBuffer(), Stride);

	// Draw vertex + index
	if (InPrimitiveComp->GetIndexBuffer() && InPrimitiveComp->GetIndicesData())
	{
		Pipeline->SetIndexBuffer(InPrimitiveComp->GetIndexBuffer(), 0);
		Pipeline->DrawIndexed(InPrimitiveComp->GetNumIndices(), 0, 0);
	}
	// Draw vertex
	else
	{
		Pipeline->Draw(static_cast<uint32>(InPrimitiveComp->GetNumVertices()), 0);
	}
}

/**
 * @brief FVertex 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FNormalVertex* InVertices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 변경되지 않는 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief FVector 타입용 정점 Buffer 생성 함수
 * @param InVertices 정점 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @param bCpuAccess CPU에서 접근 가능한 동적 버퍼 여부
 * @return 생성된 D3D11 정점 버퍼
 */
ID3D11Buffer* URenderer::CreateVertexBuffer(FVector* InVertices, uint32 InByteWidth, bool bCpuAccess) const
{
	D3D11_BUFFER_DESC VertexBufferDescription = {};
	VertexBufferDescription.ByteWidth = InByteWidth;
	VertexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE; // 기본값: 정적 데이터
	VertexBufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	// CPU 접근이 필요한 경우 동적 버퍼로 변경
	if (bCpuAccess)
	{
		VertexBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // CPU에서 자주 수정할 경우
		VertexBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU 쓰기 가능
		VertexBufferDescription.MiscFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA VertexBufferInitData = { InVertices };

	ID3D11Buffer* VertexBuffer = nullptr;
	GetDevice()->CreateBuffer(&VertexBufferDescription, &VertexBufferInitData, &VertexBuffer);

	return VertexBuffer;
}

/**
 * @brief Index Buffer 생성 함수
 * @param InIndices 인덱스 데이터 포인터
 * @param InByteWidth 버퍼 크기 (바이트 단위)
 * @return 생성된 D3D11 인덱스 버퍼
 */
ID3D11Buffer* URenderer::CreateIndexBuffer(const void* InIndices, uint32 InByteWidth) const
{
	D3D11_BUFFER_DESC IndexBufferDescription = {};
	IndexBufferDescription.ByteWidth = InByteWidth;
	IndexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA IndexBufferInitData = {};
	IndexBufferInitData.pSysMem = InIndices;

	ID3D11Buffer* IndexBuffer = nullptr;
	GetDevice()->CreateBuffer(&IndexBufferDescription, &IndexBufferInitData, &IndexBuffer);
	return IndexBuffer;
}

/**
 * @brief 창 크기 변경 시 렌더 타곟 및 버퍼를 재설정하는 함수
 * @param InWidth 새로운 창 너비
 * @param InHeight 새로운 창 높이
 */
void URenderer::OnResize(uint32 InWidth, uint32 InHeight) const
{
	// 필수 리소스가 유효하지 않으면 Early Return
	if (!DeviceResources || !GetDevice() || !GetDeviceContext() || !GetSwapChain())
	{
		return;
	}

	// 기존 버퍼들 해제
	DeviceResources->ReleaseFrameBuffer();
	DeviceResources->ReleaseDepthBuffer();
	GetDeviceContext()->OMSetRenderTargets(0, nullptr, nullptr);

	// SwapChain 버퍼 크기 재설정
	HRESULT Result = GetSwapChain()->ResizeBuffers(2, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0);
	if (FAILED(Result))
	{
		UE_LOG("OnResize Failed");
		return;
	}

	// 버퍼 재생성 및 렌더 타겟 설정
	DeviceResources->UpdateViewport();
	DeviceResources->CreateFrameBuffer();
	DeviceResources->CreateDepthBuffer();

	// 새로운 렌더 타겟 바인딩
	auto* RenderTargetView = DeviceResources->GetRenderTargetView();
	ID3D11RenderTargetView* RenderTargetViews[] = { RenderTargetView };
	GetDeviceContext()->OMSetRenderTargets(1, RenderTargetViews, DeviceResources->GetDepthStencilView());
}

/**
 * @brief Vertex Buffer 해제 함수
 * @param InVertexBuffer 해제할 정점 버퍼
 */
void URenderer::ReleaseVertexBuffer(ID3D11Buffer* InVertexBuffer)
{
	if (InVertexBuffer)
	{
		InVertexBuffer->Release();
	}
}

/**
 * @brief Index Buffer 해제 함수
 * @param InIndexBuffer 해제할 인덱스 버퍼
 */
void URenderer::ReleaseIndexBuffer(ID3D11Buffer* InIndexBuffer)
{
	if (InIndexBuffer)
	{
		InIndexBuffer->Release();
	}
}

/**
 * @brief 커스텀 Vertex Shader와 Input Layout을 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param InInputLayoutDescs Input Layout 스팩 배열
 * @param OutVertexShader 출력될 Vertex Shader 포인터
 * @param OutInputLayout 출력될 Input Layout 포인터
 */
void URenderer::CreateVertexShaderAndInputLayout(const wstring& InFilePath,
	const TArray<D3D11_INPUT_ELEMENT_DESC>& InInputLayoutDescs,
	ID3D11VertexShader** OutVertexShader,
	ID3D11InputLayout** OutInputLayout)
{
	ID3DBlob* VertexShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Vertex Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainVS", "vs_5_0",
		0, 0,
		&VertexShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Vertex Shader 객체 생성
	GetDevice()->CreateVertexShader(VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), nullptr, OutVertexShader);

	// Input Layout 생성
	GetDevice()->CreateInputLayout(InInputLayoutDescs.data(), static_cast<uint32>(InInputLayoutDescs.size()),
		VertexShaderBlob->GetBufferPointer(),
		VertexShaderBlob->GetBufferSize(), OutInputLayout);

	// TODO(KHJ): 이 값이 여기에 있는 게 맞나? 검토 필요
	Stride = sizeof(FNormalVertex);

	VertexShaderBlob->Release();
}

/**
 * @brief 커스텀 Pixel Shader를 생성하는 함수
 * @param InFilePath 셰이더 파일 경로
 * @param OutPixelShader 출력될 Pixel Shader 포인터
 */
void URenderer::CreatePixelShader(const wstring& InFilePath, ID3D11PixelShader** OutPixelShader) const
{
	ID3DBlob* PixelShaderBlob = nullptr;
	ID3DBlob* ErrorBlob = nullptr;

	// Pixel Shader 컴파일
	HRESULT Result = D3DCompileFromFile(InFilePath.data(), nullptr, nullptr, "mainPS", "ps_5_0",
		0, 0,
		&PixelShaderBlob, &ErrorBlob);

	// 컴파일 실패 시 에러 처리
	if (FAILED(Result))
	{
		if (ErrorBlob)
		{
			OutputDebugStringA(static_cast<char*>(ErrorBlob->GetBufferPointer()));
			ErrorBlob->Release();
		}
		return;
	}

	// Pixel Shader 객체 생성
	GetDevice()->CreatePixelShader(PixelShaderBlob->GetBufferPointer(),
		PixelShaderBlob->GetBufferSize(), nullptr, OutPixelShader);

	PixelShaderBlob->Release();
}

/**
 * @brief 렌더링에 사용될 상수 버퍼들을 생성하는 함수
 */
void URenderer::CreateConstantBuffer()
{
	// 모델 변환 행렬용 상수 버퍼 생성 (Slot 0)
	{
		D3D11_BUFFER_DESC ModelConstantBufferDescription = {};
		ModelConstantBufferDescription.ByteWidth = sizeof(FMatrix) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ModelConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ModelConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ModelConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ModelConstantBufferDescription, nullptr, &ConstantBufferModels);
	}

	// 색상 정보용 상수 버퍼 생성 (Slot 2)
	{
		D3D11_BUFFER_DESC ColorConstantBufferDescription = {};
		ColorConstantBufferDescription.ByteWidth = sizeof(FVector4) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ColorConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ColorConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ColorConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ColorConstantBufferDescription, nullptr, &ConstantBufferColor);
	}

	// 카메라 View/Projection 행렬용 상수 버퍼 생성 (Slot 1)
	{
		D3D11_BUFFER_DESC ViewProjConstantBufferDescription = {};
		ViewProjConstantBufferDescription.ByteWidth = sizeof(FViewProjConstants) + 0xf & 0xfffffff0; // 16바이트 단위 정렬
		ViewProjConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		ViewProjConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		ViewProjConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&ViewProjConstantBufferDescription, nullptr, &ConstantBufferViewProj);
	}

	{
		D3D11_BUFFER_DESC MaterialConstantBufferDescription = {};
		MaterialConstantBufferDescription.ByteWidth = sizeof(FMaterial) + 0xf & 0xfffffff0;
		MaterialConstantBufferDescription.Usage = D3D11_USAGE_DYNAMIC; // 매 프레임 CPU에서 업데이트
		MaterialConstantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		MaterialConstantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		GetDevice()->CreateBuffer(&MaterialConstantBufferDescription, nullptr, &ConstantBufferMaterial);
	}
}

/**
 * @brief 상수 버퍼 소멸 함수
 */
void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBufferModels)
	{
		ConstantBufferModels->Release();
		ConstantBufferModels = nullptr;
	}

	if (ConstantBufferColor)
	{
		ConstantBufferColor->Release();
		ConstantBufferColor = nullptr;
	}

	if (ConstantBufferViewProj)
	{
		ConstantBufferViewProj->Release();
		ConstantBufferViewProj = nullptr;
	}

	if (ConstantBufferMaterial)
	{
		ConstantBufferMaterial->Release();
		ConstantBufferMaterial = nullptr;
	}
}

bool URenderer::UpdateVertexBuffer(ID3D11Buffer* InVertexBuffer, const TArray<FVector>& InVertices) const
{
	if (!GetDeviceContext() || !InVertexBuffer || InVertices.empty())
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource = {};
	HRESULT ResultHandle = GetDeviceContext()->Map(
		InVertexBuffer,
		0, // 서브리소스 인덱스 (버퍼는 0)
		D3D11_MAP_WRITE_DISCARD, // 전체 갱신
		0, // 플래그 없음
		&MappedResource
	);

	if (FAILED(ResultHandle))
	{
		return false;
	}

	// GPU 메모리에 새 데이터 복사
	// TODO: 어쩔 때 한번 read access violation 걸림
	memcpy(MappedResource.pData, InVertices.data(), sizeof(FVector) * InVertices.size());

	// GPU 접근 재허용
	GetDeviceContext()->Unmap(InVertexBuffer, 0);

	return true;
}

ID3D11RasterizerState* URenderer::GetRasterizerState(const FRenderState& InRenderState)
{
	D3D11_FILL_MODE FillMode = ToD3D11(InRenderState.FillMode);
	D3D11_CULL_MODE CullMode = ToD3D11(InRenderState.CullMode);

	const FRasterKey Key{ FillMode, CullMode };
	if (auto Iter = RasterCache.find(Key); Iter != RasterCache.end())
	{
		return Iter->second;
	}

	ID3D11RasterizerState* RasterizerState = nullptr;
	D3D11_RASTERIZER_DESC RasterizerDesc = {};
	RasterizerDesc.FillMode = FillMode;
	RasterizerDesc.CullMode = CullMode;
	RasterizerDesc.FrontCounterClockwise = TRUE;
	RasterizerDesc.DepthClipEnable = TRUE; // ✅ 근/원거리 평면 클리핑 활성화 (핵심)

	HRESULT ResultHandle = GetDevice()->CreateRasterizerState(&RasterizerDesc, &RasterizerState);

	if (FAILED(ResultHandle))
	{
		return nullptr;
	}

	RasterCache.emplace(Key, RasterizerState);
	return RasterizerState;
}

D3D11_CULL_MODE URenderer::ToD3D11(ECullMode InCull)
{
	switch (InCull)
	{
	case ECullMode::Back:
		return D3D11_CULL_BACK;
	case ECullMode::Front:
		return D3D11_CULL_FRONT;
	case ECullMode::None:
		return D3D11_CULL_NONE;
	default:
		return D3D11_CULL_BACK;
	}
}

D3D11_FILL_MODE URenderer::ToD3D11(EFillMode InFill)
{
	switch (InFill)
	{
	case EFillMode::Solid:
		return D3D11_FILL_SOLID;
	case EFillMode::WireFrame:
		return D3D11_FILL_WIREFRAME;
	default:
		return D3D11_FILL_SOLID;
	}
}
