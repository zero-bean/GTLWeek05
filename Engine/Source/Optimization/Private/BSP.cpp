#include "pch.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Optimization/Public/BSP.h"\
#include "Manager/Level/Public/LevelManager.h"
#include "Physics/Public/AABB.h"

#include <algorithm>
#include <cfloat>

/* public */

/*
	��Ʈ ��� ����

	1. ���忡 �������� ����� ������ root ���� null�� �д�.
	2. �׷��� �ʴٸ� ������ ������ ��ȸ�ϸ鼭 �� ���� min max ���� ���Ѵ�.
	3. max - min�� root ����� ����ü�� ���� max + min / 2�� root ����� �������� ��´�.
*/
void FBSP::Initialize(const TArray<TObjectPtr<UPrimitiveComponent>>& LevelPrimitives)
{
	// �������� ����� ������ Tree�� ������ �ʴ´�.
	if (LevelPrimitives.empty())
		return;

	// ��Ʈ�� �Ҵ��Ѵ�.
	Root = new BSPNode;

	FVector WorldMin(FLT_MAX, FLT_MAX, FLT_MAX);
	FVector WorldMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (const TObjectPtr<UPrimitiveComponent>& Primitive : LevelPrimitives)
	{
		FVector BoxMin, BoxMax;
		Primitive->GetWorldAABB(BoxMin, BoxMax);

		// BillBoard �� BoundingBox�� ���� ��ü�� ���� ����
		if (BoxMin.Empty() && BoxMax.Empty())
			continue;

        WorldMin.X = std::min(WorldMin.X, BoxMin.X);
        WorldMax.X = std::max(WorldMax.X, BoxMax.X);
        WorldMin.Y = std::min(WorldMin.Y, BoxMin.Y);
        WorldMax.Y = std::max(WorldMax.Y, BoxMax.Y);
        WorldMin.Z = std::min(WorldMin.Z, BoxMin.Z);
        WorldMax.Z = std::max(WorldMax.Z, BoxMax.Z);
	}

	float XGap = WorldMax.X - WorldMin.X;
	float YGap = WorldMax.Y - WorldMin.Y;
	float ZGap = WorldMax.Z - WorldMin.Z;

	float MaxGap = std::max(XGap, std::max(YGap, ZGap));

	Root->Extent = FVector(MaxGap, MaxGap, MaxGap);
	Root->Position = FVector(
		(WorldMax.X + WorldMin.X) / 2,
		(WorldMax.Y + WorldMin.Y) / 2,
		(WorldMax.Z + WorldMin.Z) / 2
	);

	MinimumExtent = MaxGap * pow(0.5f, 4.0f);

	Root->Primitives = LevelPrimitives;
	
	// ��������� �ٵ��� ���´�.
	Divide(Root);
}

void FBSP::Shutdown()
{
	PostOrder(Root,
		[](BSPNode* Node)
		{
			delete Node;
		}
	);

	Root = nullptr;
	MinimumExtent = 0;
}

void FBSP::PreOrder(BSPNode* Node, const std::function<void(BSPNode*)>& Apply)
{
	if (!Node)
		return;

	Apply(Node);

	PreOrder(Node->Left, Apply);
	PreOrder(Node->Right, Apply);
}

void FBSP::PreOrderUntil(BSPNode* Node, const std::function<bool(BSPNode*)>& Apply)
{
	if (!Node)
		return;

	if (!Apply(Node))
		return;
	
	PreOrder(Node->Left, Apply);
	PreOrder(Node->Right, Apply);
}

void FBSP::InOrder(BSPNode* Node, const std::function<void(BSPNode*)>& Apply)
{
	if (!Node)
		return;

	InOrder(Node->Left, Apply);

	Apply(Node);

	InOrder(Node->Right, Apply);
}


void FBSP::PostOrder(BSPNode* Node, const std::function<void(BSPNode*)>& Apply)
{
	if (!Node)
		return;

	PostOrder(Node->Left, Apply);
	PostOrder(Node->Right, Apply);

	Apply(Node);
}

/* private */

/*
	���� ����

	�������� ���� ����

	1. �������� Ÿ���� Cube�̸� ũ�Ⱑ �ּ� ���� ���Ѱ� ���ų� ������ �������� �ʴ´�.
	2. �ּ� ���� ������ (root ����� ũ��) * (1 / 2)^4�̴�.

	�ڽ� ���� 7���� �� �ϳ��� ���¸� �� �� �ִ�.

	1. ������ü = �ʱ� ��Ʈ��� ��� �� ��ü
	2. 3. 4) ������ü�� xy, yz, xz ������� 2����� ��
	5. 6. 7) 2, 3, 4�� �ڱⰡ �߸��� ���� ������� �̵���� ��
	8. 5. 6. 7.�� 1�� ���·� �ۿ� ���ҵ� �� �ۿ� ����.

	�θ� ���� ������ ���� ��쿡 �ڽ� ��带 ������.

	1. �����ΰ� 1�� �̻��� ������ ��� ���� ��
	2. �� �� �ڽ��� �߸��� ���� ��鿡 �������� �ʴ� ���� ��� �ϳ� ���� ��
*/
void FBSP::DivideCube(BSPNode* Cube)
{
	if (!Cube)
		return;

	if (Cube->Primitives.empty())
		return;

	if (Cube->Extent.X <= MinimumExtent)
		return;

	TArray<TObjectPtr<UPrimitiveComponent>> MinusSide, PlusSide, Intersected;

	// ���� ��迡 ��ġ�� ������ ���� ���� ��츦 ���Ѵ�.
	// ���� ������ ���ٸ� X->Y->Z ������ �켱������ ������.
	uint32 XYIntersected = CountXYIntersected(Cube->Primitives);
	uint32 YZIntersected = CountYZIntersected(Cube->Primitives);
	uint32 XZIntersected = CountXZIntersected(Cube->Primitives);

	uint32 MaxIntersected = std::max(XYIntersected, std::max(YZIntersected, XZIntersected));

	if (XYIntersected == MaxIntersected)
	{
		DivideWithXYPlane(Cube, Cube->Primitives, MinusSide, PlusSide, Intersected);

		Cube->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			Cube->Left = new BSPNode;

			Cube->Left->Type = BSPType::XYExtended;
			Cube->Left->Position = FVector(
				Cube->Position.X,
				Cube->Position.Y,
				Cube->Position.Z - (Cube->Extent.Z / 4.0f)
			);
			Cube->Left->Extent = FVector(
				Cube->Extent.X,
				Cube->Extent.Y,
				Cube->Extent.Z / 2.0f
			);
			Cube->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			Cube->Right = new BSPNode;

			Cube->Right->Type = BSPType::XYExtended;
			Cube->Right->Position = FVector(
				Cube->Position.X,
				Cube->Position.Y,
				Cube->Position.Z + (Cube->Extent.Z / 4.0f)
			);
			Cube->Right->Extent = FVector(
				Cube->Extent.X,
				Cube->Extent.Y,
				Cube->Extent.Z / 2.0f
			);
			Cube->Right->Primitives = PlusSide;
		}
	}
	else if (YZIntersected == MaxIntersected)
	{
		DivideWithYZPlane(Cube, Cube->Primitives, MinusSide, PlusSide, Intersected);

		Cube->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			Cube->Left = new BSPNode;

			Cube->Left->Type = BSPType::YZExtended;
			Cube->Left->Position = FVector(
				Cube->Position.X - (Cube->Extent.X / 4.0f),
				Cube->Position.Y,
				Cube->Position.Z
			);
			Cube->Left->Extent = FVector(
				Cube->Extent.X / 2.0f,
				Cube->Extent.Y,
				Cube->Extent.Z
			);
			Cube->Left->Primitives = MinusSide;
		}
		
		if (!PlusSide.empty())
		{
			Cube->Right = new BSPNode;

			Cube->Right->Type = BSPType::YZExtended;
			Cube->Right->Position = FVector(
				Cube->Position.X + (Cube->Extent.X / 4.0f),
				Cube->Position.Y,
				Cube->Position.Z
			);
			Cube->Right->Extent = FVector(
				Cube->Extent.X / 2.0f,
				Cube->Extent.Y,
				Cube->Extent.Z
			);
			Cube->Right->Primitives = PlusSide;
		}
	}
	else
	{
		DivideWithXZPlane(Cube, Cube->Primitives, MinusSide, PlusSide, Intersected);

		Cube->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			Cube->Left = new BSPNode;

			Cube->Left->Type = BSPType::XZExtended;
			Cube->Left->Position = FVector(
				Cube->Position.X,
				Cube->Position.Y - (Cube->Extent.Y / 4.0f),
				Cube->Position.Z
			);
			Cube->Left->Extent = FVector(
				Cube->Extent.X,
				Cube->Extent.Y / 2.0f,
				Cube->Extent.Z
			);
			Cube->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			Cube->Right = new BSPNode;

			Cube->Right->Type = BSPType::XZExtended;
			Cube->Right->Position = FVector(
				Cube->Position.X,
				Cube->Position.Y + (Cube->Extent.Y / 4.0f),
				Cube->Position.Z
			);
			Cube->Right->Extent = FVector(
				Cube->Extent.X,
				Cube->Extent.Y / 2.0f,
				Cube->Extent.Z
			);
			Cube->Right->Primitives = PlusSide;
		}
	}

	Divide(Cube->Left);
	Divide(Cube->Right);
}

void FBSP::DevideXYExtended(BSPNode* XYExtended)
{
	if (!XYExtended)
		return;

	if (XYExtended->Primitives.empty())
		return;

	TArray<TObjectPtr<UPrimitiveComponent>> MinusSide, PlusSide, Intersected;
	uint32 YZIntersected = CountYZIntersected(XYExtended->Primitives);
	uint32 XZIntersected = CountXZIntersected(XYExtended->Primitives);

	uint32 MaxIntersected = std::max(YZIntersected, XZIntersected);

	if (YZIntersected == MaxIntersected)
	{
		DivideWithYZPlane(XYExtended, XYExtended->Primitives, MinusSide, PlusSide, Intersected);

		XYExtended->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			XYExtended->Left = new BSPNode;

			XYExtended->Left->Type = BSPType::YExtended;
			XYExtended->Left->Position = FVector(
				XYExtended->Position.X - (XYExtended->Extent.X / 4.0f),
				XYExtended->Position.Y,
				XYExtended->Position.Z
			);
			XYExtended->Left->Extent = FVector(
				XYExtended->Extent.X / 2.0f,
				XYExtended->Extent.Y,
				XYExtended->Extent.Z
			);
			XYExtended->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			XYExtended->Right = new BSPNode;

			XYExtended->Right->Type = BSPType::YExtended;
			XYExtended->Right->Position = FVector(
				XYExtended->Position.X + (XYExtended->Extent.X / 4.0f),
				XYExtended->Position.Y,
				XYExtended->Position.Z
			);
			XYExtended->Right->Extent = FVector(
				XYExtended->Extent.X / 2.0f,
				XYExtended->Extent.Y,
				XYExtended->Extent.Z
			);
			XYExtended->Right->Primitives = PlusSide;
		}
	}
	else
	{
		DivideWithXZPlane(XYExtended, XYExtended->Primitives, MinusSide, PlusSide, Intersected);

		XYExtended->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			XYExtended->Left = new BSPNode;

			XYExtended->Left->Type = BSPType::XExtended;
			XYExtended->Left->Position = FVector(
				XYExtended->Position.X,
				XYExtended->Position.Y - (XYExtended->Extent.Y / 4.0f),
				XYExtended->Position.Z
			);
			XYExtended->Left->Extent = FVector(
				XYExtended->Extent.X,
				XYExtended->Extent.Y / 2.0f,
				XYExtended->Extent.Z
			);
			XYExtended->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			XYExtended->Right = new BSPNode;

			XYExtended->Right->Type = BSPType::XExtended;
			XYExtended->Right->Position = FVector(
				XYExtended->Position.X,
				XYExtended->Position.Y + (XYExtended->Extent.Y / 4.0f),
				XYExtended->Position.Z
			);
			XYExtended->Right->Extent = FVector(
				XYExtended->Extent.X,
				XYExtended->Extent.Y / 2.0f,
				XYExtended->Extent.Z
			);
			XYExtended->Right->Primitives = PlusSide;
		}

		Divide(XYExtended->Left);
		Divide(XYExtended->Right);
	}
}

void FBSP::DevideYZExtended(BSPNode* YZExtended)
{
	if (!YZExtended)
		return;

	if (YZExtended->Primitives.empty())
		return;

	TArray<TObjectPtr<UPrimitiveComponent>> MinusSide, PlusSide, Intersected;
	uint32 XYIntersected = CountXYIntersected(YZExtended->Primitives);
	uint32 XZIntersected = CountXZIntersected(YZExtended->Primitives);

	uint32 MaxIntersected = std::max(XYIntersected, XZIntersected);

	if (XYIntersected == MaxIntersected)
	{
		DivideWithXYPlane(YZExtended, YZExtended->Primitives, MinusSide, PlusSide, Intersected);

		YZExtended->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			YZExtended->Left = new BSPNode;

			YZExtended->Left->Type = BSPType::YExtended;
			YZExtended->Left->Position = FVector(
				YZExtended->Position.X,
				YZExtended->Position.Y,
				YZExtended->Position.Z - (YZExtended->Extent.Z / 4.0f)
			);
			YZExtended->Left->Extent = FVector(
				YZExtended->Extent.X,
				YZExtended->Extent.Y,
				YZExtended->Extent.Z / 2.0f
			);
			YZExtended->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			YZExtended->Right = new BSPNode;

			YZExtended->Right->Type = BSPType::YExtended;
			YZExtended->Right->Position = FVector(
				YZExtended->Position.X,
				YZExtended->Position.Y,
				YZExtended->Position.Z + (YZExtended->Extent.Z / 4.0f)
			);
			YZExtended->Right->Extent = FVector(
				YZExtended->Extent.X,
				YZExtended->Extent.Y,
				YZExtended->Extent.Z / 2.0f
			);
			YZExtended->Right->Primitives = PlusSide;
		}
	}
	else
	{
		DivideWithXZPlane(YZExtended, YZExtended->Primitives, MinusSide, PlusSide, Intersected);

		YZExtended->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			YZExtended->Left = new BSPNode;

			YZExtended->Left->Type = BSPType::ZExtended;
			YZExtended->Left->Position = FVector(
				YZExtended->Position.X,
				YZExtended->Position.Y - (YZExtended->Extent.X / 4.0f),
				YZExtended->Position.Z
			);
			YZExtended->Left->Extent = FVector(
				YZExtended->Extent.X,
				YZExtended->Extent.Y / 2.0f,
				YZExtended->Extent.Z
			);
			YZExtended->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			YZExtended->Right = new BSPNode;

			YZExtended->Right->Type = BSPType::ZExtended;
			YZExtended->Right->Position = FVector(
				YZExtended->Position.X,
				YZExtended->Position.Y + (YZExtended->Extent.X / 4.0f),
				YZExtended->Position.Z
			);
			YZExtended->Right->Extent = FVector(
				YZExtended->Extent.X,
				YZExtended->Extent.Y / 2.0f,
				YZExtended->Extent.Z
			);
			YZExtended->Right->Primitives = PlusSide;
		}
	}
}

void FBSP::DevideXZExtended(BSPNode* XZExtended)
{
	if (!XZExtended)
		return;

	if (XZExtended->Primitives.empty())
		return;

	TArray<TObjectPtr<UPrimitiveComponent>> MinusSide, PlusSide, Intersected;
	uint32 XYIntersected = CountXYIntersected(XZExtended->Primitives);
	uint32 YZIntersected = CountYZIntersected(XZExtended->Primitives);

	uint32 MaxIntersected = std::max(XYIntersected, YZIntersected);

	if (XYIntersected == MaxIntersected)
	{
		DivideWithXYPlane(XZExtended, XZExtended->Primitives, MinusSide, PlusSide, Intersected);

		XZExtended->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			XZExtended->Left = new BSPNode;

			XZExtended->Left->Type = BSPType::XExtended;
			XZExtended->Left->Position = FVector(
				XZExtended->Position.X,
				XZExtended->Position.Y,
				XZExtended->Position.Z - (XZExtended->Extent.Z / 4.0f)
			);
			XZExtended->Left->Extent = FVector(
				XZExtended->Extent.X,
				XZExtended->Extent.Y,
				XZExtended->Extent.Z / 2.0f
			);
			XZExtended->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			XZExtended->Right = new BSPNode;

			XZExtended->Right->Type = BSPType::XExtended;
			XZExtended->Right->Position = FVector(
				XZExtended->Position.X,
				XZExtended->Position.Y,
				XZExtended->Position.Z + (XZExtended->Extent.Z / 4.0f)
			);
			XZExtended->Right->Extent = FVector(
				XZExtended->Extent.X,
				XZExtended->Extent.Y,
				XZExtended->Extent.Z / 2.0f
			);
			XZExtended->Right->Primitives = PlusSide;
		}
	}
	else
	{
		DivideWithYZPlane(XZExtended, XZExtended->Primitives, MinusSide, PlusSide, Intersected);

		XZExtended->Primitives = Intersected;

		if (!MinusSide.empty())
		{
			XZExtended->Left = new BSPNode;

			XZExtended->Left->Type = BSPType::ZExtended;
			XZExtended->Left->Position = FVector(
				XZExtended->Position.X - (XZExtended->Extent.X / 4.0f),
				XZExtended->Position.Y,
				XZExtended->Position.Z
			);
			XZExtended->Left->Extent = FVector(
				XZExtended->Extent.X / 2.0f,
				XZExtended->Extent.Y,
				XZExtended->Extent.Z
			);
			XZExtended->Left->Primitives = MinusSide;
		}

		if (!PlusSide.empty())
		{
			XZExtended->Right = new BSPNode;

			XZExtended->Right->Type = BSPType::ZExtended;
			XZExtended->Right->Position = FVector(
				XZExtended->Position.X + (XZExtended->Extent.X / 4.0f),
				XZExtended->Position.Y,
				XZExtended->Position.Z
			);
			XZExtended->Right->Extent = FVector(
				XZExtended->Extent.X / 2.0f,
				XZExtended->Extent.Y,
				XZExtended->Extent.Z
			);
			XZExtended->Right->Primitives = PlusSide;
		}
	}

	Divide(XZExtended->Left);
	Divide(XZExtended->Right);
}

void FBSP::DevideXExtended(BSPNode* XExtended)
{
	if (!XExtended)
		return;

	if (XExtended->Primitives.empty())
		return;

	TArray<TObjectPtr<UPrimitiveComponent>> MinusSide, PlusSide, Intersected;
	uint32 YZIntersected = CountYZIntersected(XExtended->Primitives);
	
	DivideWithYZPlane(XExtended, XExtended->Primitives, MinusSide, PlusSide, Intersected);

	XExtended->Primitives = Intersected;

	if (!MinusSide.empty())
	{
		XExtended->Left = new BSPNode;

		XExtended->Left->Type = BSPType::Cube;
		XExtended->Left->Position = FVector(
			XExtended->Position.X - (XExtended->Extent.X / 4.0f),
			XExtended->Position.Y,
			XExtended->Position.Z
		);
		XExtended->Left->Extent = FVector(
			XExtended->Extent.X / 2.0f,
			XExtended->Extent.Y,
			XExtended->Extent.Z
		);
		XExtended->Left->Primitives = MinusSide;
	}

	if (!PlusSide.empty())
	{
		XExtended->Right = new BSPNode;

		XExtended->Right->Type = BSPType::Cube;
		XExtended->Right->Position = FVector(
			XExtended->Position.X + (XExtended->Extent.X / 4.0f),
			XExtended->Position.Y,
			XExtended->Position.Z
		);
		XExtended->Right->Extent = FVector(
			XExtended->Extent.X / 2.0f,
			XExtended->Extent.Y,
			XExtended->Extent.Z
		);
		XExtended->Right->Primitives = PlusSide;
	}

	Divide(XExtended->Left);
	Divide(XExtended->Right);
}

void FBSP::DevideYExtended(BSPNode* YExtended)
{
	if (!YExtended)
		return;

	if (YExtended->Primitives.empty())
		return;

	TArray<TObjectPtr<UPrimitiveComponent>> MinusSide, PlusSide, Intersected;
	uint32 YZIntersected = CountXZIntersected(YExtended->Primitives);

	DivideWithXZPlane(YExtended, YExtended->Primitives, MinusSide, PlusSide, Intersected);

	YExtended->Primitives = Intersected;

	if (!MinusSide.empty())
	{
		YExtended->Left = new BSPNode;

		YExtended->Left->Type = BSPType::Cube;
		YExtended->Left->Position = FVector(
			YExtended->Position.X,
			YExtended->Position.Y - (YExtended->Extent.Y / 4.0f),
			YExtended->Position.Z
		);
		YExtended->Left->Extent = FVector(
			YExtended->Extent.X,
			YExtended->Extent.Y / 2.0f,
			YExtended->Extent.Z
		);
		YExtended->Left->Primitives = MinusSide;
	}

	if (!PlusSide.empty())
	{
		YExtended->Right = new BSPNode;

		YExtended->Right->Type = BSPType::Cube;
		YExtended->Right->Position = FVector(
			YExtended->Position.X,
			YExtended->Position.Y + (YExtended->Extent.Y / 4.0f),
			YExtended->Position.Z
		);
		YExtended->Right->Extent = FVector(
			YExtended->Extent.X,
			YExtended->Extent.Y / 2.0f,
			YExtended->Extent.Z
		);
		YExtended->Right->Primitives = PlusSide;
	}

	Divide(YExtended->Left);
	Divide(YExtended->Right);
}

void FBSP::DevideZExtended(BSPNode* ZExtended)
{
	if (!ZExtended)
		return;

	if (ZExtended->Primitives.empty())
		return;

	TArray<TObjectPtr<UPrimitiveComponent>> MinusSide, PlusSide, Intersected;
	uint32 YZIntersected = CountXYIntersected(ZExtended->Primitives);

	DivideWithXYPlane(ZExtended, ZExtended->Primitives, MinusSide, PlusSide, Intersected);

	ZExtended->Primitives = Intersected;

	if (!MinusSide.empty())
	{
		ZExtended->Left = new BSPNode;

		ZExtended->Left->Type = BSPType::Cube;
		ZExtended->Left->Position = FVector(
			ZExtended->Position.X,
			ZExtended->Position.Y,
			ZExtended->Position.Z - (ZExtended->Extent.Y / 4.0f)
		);
		ZExtended->Left->Extent = FVector(
			ZExtended->Extent.X,
			ZExtended->Extent.Y,
			ZExtended->Extent.Z / 2.0f
		);
		ZExtended->Left->Primitives = MinusSide;
	}

	if (!PlusSide.empty())
	{
		ZExtended->Right = new BSPNode;

		ZExtended->Right->Type = BSPType::Cube;
		ZExtended->Right->Position = FVector(
			ZExtended->Position.X,
			ZExtended->Position.Y,
			ZExtended->Position.Z + (ZExtended->Extent.Y / 4.0f)
		);
		ZExtended->Right->Extent = FVector(
			ZExtended->Extent.X,
			ZExtended->Extent.Y,
			ZExtended->Extent.Z / 2.0f
		);
		ZExtended->Right->Primitives = PlusSide;
	}

	Divide(ZExtended->Left);
	Divide(ZExtended->Right);
}

void FBSP::Divide(BSPNode* Node)
{
	if (!Node)
		return;

	switch (Node->Type)
	{
		case BSPType::Cube:
			DivideCube(Node);
			break;
		case BSPType::XYExtended:
			DevideXYExtended(Node);
			break;
		case BSPType::YZExtended:
			DevideYZExtended(Node);
			break;
		case BSPType::XZExtended:
			DevideXZExtended(Node);
			break;
		case BSPType::XExtended:
			DevideXExtended(Node);
			break;
		case BSPType::YExtended:
			DevideYExtended(Node);
			break;
		case BSPType::ZExtended:
			DevideZExtended(Node);
			break;
	}
}

bool FBSP::IntersectWithXYPlane(const float ZMin, const float ZMax)
{
	if (!Root)
		return false;

	return Root->Position.Z >= ZMin && Root->Position.Z <= ZMax;
}

bool FBSP::IntersectWithYZPlane(const float XMin, const float XMax)
{
	if (!Root)
		return false;

	return Root->Position.X >= XMin && Root->Position.X <= XMax;
}

bool FBSP::IntersectWithXZPlane(const float YMin, const float YMax)
{
	if (!Root)
		return false;

	return Root->Position.Y >= YMin && Root->Position.Y <= YMax;
}

uint32 FBSP::CountXYIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives)
{
	uint32 Intersected = 0;

	for (const TObjectPtr<UPrimitiveComponent>& Primitive : Primitives)
	{
		FVector BoxMin, BoxMax;
		Primitive->GetWorldAABB(BoxMin, BoxMax);

		if (BoxMin.Empty() && BoxMax.Empty() && IntersectWithXYPlane(BoxMin.Z, BoxMax.Z))
			Intersected++;
	}

	return Intersected;
}

uint32 FBSP::CountYZIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives)
{
	uint32 Intersected = 0;

	for (const TObjectPtr<UPrimitiveComponent>& Primitive : Primitives)
	{
		FVector BoxMin, BoxMax;
		Primitive->GetWorldAABB(BoxMin, BoxMax);

		if (BoxMin.Empty() && BoxMax.Empty() && IntersectWithYZPlane(BoxMin.X, BoxMax.X))
			Intersected++;
	}

	return Intersected;
}

uint32 FBSP::CountXZIntersected(const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives)
{
	uint32 Intersected = 0;

	for (const TObjectPtr<UPrimitiveComponent>& Primitive : Primitives)
	{
		FVector BoxMin, BoxMax;
		Primitive->GetWorldAABB(BoxMin, BoxMax);

		if (BoxMin.Empty() && BoxMax.Empty() && IntersectWithXYPlane(BoxMin.Y, BoxMax.Y))
			Intersected++;
	}

	return Intersected;
}

void FBSP::DivideWithXYPlane(
	BSPNode* N,
	const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
	TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
	TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
	TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
)
{
	for (const TObjectPtr<UPrimitiveComponent>& Primitive : Primitives)
	{
		FVector BoxMin, BoxMax;
		Primitive->GetWorldAABB(BoxMin, BoxMax);

		if (BoxMin.Empty() && BoxMax.Empty() && IntersectWithXYPlane(BoxMin.Z, BoxMax.Z))
			Intersected.push_back(Primitive);
		else if (Primitive->GetRelativeLocation().Z < N->Position.Z)
			MinusSide.push_back(Primitive);
		else
			PlusSide.push_back(Primitive);
	}
}

void FBSP::DivideWithYZPlane(
	BSPNode* N,
	const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
	TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
	TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
	TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
)
{
	for (const TObjectPtr<UPrimitiveComponent>& Primitive : Primitives)
	{
		FVector BoxMin, BoxMax;
		Primitive->GetWorldAABB(BoxMin, BoxMax);

		if (BoxMin.Empty() && BoxMax.Empty() && IntersectWithXYPlane(BoxMin.Z, BoxMax.Z))
			Intersected.push_back(Primitive);
		else if (Primitive->GetRelativeLocation().X < N->Position.X)
			MinusSide.push_back(Primitive);
		else
			PlusSide.push_back(Primitive);
	}
}

void FBSP::DivideWithXZPlane(
	BSPNode* N,
	const TArray<TObjectPtr<UPrimitiveComponent>>& Primitives,
	TArray<TObjectPtr<UPrimitiveComponent>>& MinusSide,
	TArray<TObjectPtr<UPrimitiveComponent>>& PlusSide,
	TArray<TObjectPtr<UPrimitiveComponent>>& Intersected
)
{
	for (const TObjectPtr<UPrimitiveComponent>& Primitive : Primitives)
	{
		FVector BoxMin, BoxMax;
		Primitive->GetWorldAABB(BoxMin, BoxMax);

		if (BoxMin.Empty() && BoxMax.Empty() && IntersectWithXYPlane(BoxMin.Z, BoxMax.Z))
			Intersected.push_back(Primitive);
		else if (Primitive->GetRelativeLocation().Y < N->Position.Y)
			MinusSide.push_back(Primitive);
		else
			PlusSide.push_back(Primitive);
	}
}
