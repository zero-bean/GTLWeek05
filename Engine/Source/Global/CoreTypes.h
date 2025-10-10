#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Types.h"
#include "Core/Public/Name.h"
#include <Texture/Public/Material.h>

//struct BatchLineContants
//{
//	float CellSize;
//	//FMatrix BoundingBoxModel;
//	uint32 ZGridStartIndex; // 인덱스 버퍼에서, z방향쪽 그리드가 시작되는 인덱스
//	uint32 BoundingBoxStartIndex; // 인덱스 버퍼에서, 바운딩박스가 시작되는 인덱스
//};

struct FViewProjConstants
{
	FViewProjConstants()
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
};

struct FDecalConstants
{
	FDecalConstants(const FMatrix& InDecalWorld, const FMatrix& InDecalInverseWorld)
	{
		DecalWorld = InDecalWorld;
		DecalInverseWorld = InDecalInverseWorld;
	}

	FMatrix DecalInverseWorld;
	FMatrix DecalWorld;

private:
	// 사용하기 불편하다면 지워도 상관없습니다. (25.10.07 PYB)
	FDecalConstants() = delete;
};

struct FModelConstants
{
	FModelConstants(const FMatrix& InWorld, const FMatrix& InWorldInverseTranspose)
	{
		World = InWorld;
		WorldInverseTranspose = InWorldInverseTranspose;
	}

	FMatrix World;
	FMatrix WorldInverseTranspose;

private:
	// 사용하기 불편하다면 지워도 상관없습니다. (25.10.07 PYB)
	FModelConstants() = delete;
};

struct FMaterialConstants
{
	FVector4 Ka;
	FVector4 Kd;
	FVector4 Ks;
	float Ns;
	float Ni;
	float D;
	uint32 MaterialFlags;
	float Time; // Time in seconds
};

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 TexCoord;
};

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};

/**
 * @brief 변환 정보를 담는 구조체
 */
struct FTransform
{
	FVector Location = FVector(0.0f, 0.0f, 0.0f);
	FVector Rotation = FVector(0.0f, 0.0f, 0.0f);
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	FTransform() = default;

	FTransform(const FVector& InLocation, const FVector& InRotation = FVector::ZeroVector(),
		const FVector& InScale = FVector::OneVector())
		: Location(InLocation), Rotation(InRotation), Scale(InScale)
	{
	}
};

/**
 * @brief 2차원 좌표의 정보를 담는 구조체
 */
struct FPoint
{
	float X = 0.0f;
	float Y = 0.0f;
};

/**
 * @brief 윈도우를 비롯한 2D 화면의 정보를 담는 구조체
 */
struct FRect
{
	float Left = 0.0f;
	float Top = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;

	float GetRight() const { return Left + Width; }
	float GetBottom() const { return Top + Height; }
};
