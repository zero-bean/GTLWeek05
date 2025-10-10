#include "pch.h"
#include "Physics/Public/OBB.h"
#include "Physics/Public/AABB.h"

FOBB::FOBB()
{
	Center = FVector::ZeroVector();
	Extents = FVector::ZeroVector();
	Axes[0] = FVector(1.f, 0.f, 0.f); // Local X
	Axes[1] = FVector(0.f, 1.f, 0.f); // Local Y
	Axes[2] = FVector(0.f, 0.f, 1.f); // Local Z
}

FOBB::FOBB(const FVector& InCenter, const FVector& InExtents) 
	: Center(InCenter), Extents(InExtents)
{
	// �⺻ ���� �ÿ��� ȸ���� �����Ƿ� ���� ��� �����ϰ� ����
	Axes[0] = FVector(1.f, 0.f, 0.f); // Local X
	Axes[1] = FVector(0.f, 1.f, 0.f); // Local Y
	Axes[2] = FVector(0.f, 0.f, 1.f); // Local Z
}

void FOBB::Project(const FVector& Axis, float& OutMin, float& OutMax) const
{
	// 1. �������� ���翵 ���̸� ���մϴ�.
	//    �̴� �߽������� ���� �ָ� ������ �������� ���� ��(Axis)�� ���翵��Ų �Ÿ��� �����ϴ�.
	//    �������� ��ġ�� �߽��� + X���� + Y���� + Z����
	const float R = Extents.X * std::abs(Axis.Dot(Axes[0])) +
					Extents.Y * std::abs(Axis.Dot(Axes[1])) +
					Extents.Z * std::abs(Axis.Dot(Axes[2]));

	// 2. OBB�� �߽����� ���翵 ��ǥ�� ���մϴ�.
	const float C = Center.Dot(Axis);

	// 3. ������ ������ ����Ͽ� �����մϴ�.
	OutMin = C - R;
	OutMax = C + R;
}

void FOBB::Project(const FAABB& InAABB, const FVector& Axis, float& OutMin, float& OutMax) const
{
	// 0. AABB�� �߽ɰ� ���� ũ��(Extents)�� ���մϴ�.
	const FVector AABBCenter = InAABB.GetCenter();
	const FVector AABBExtents = (InAABB.Max - InAABB.Min) * 0.5f;

	// 1. �������� ���翵 ���̸� ���մϴ�.
	//    �̴� �߽������� ���� �ָ� ������ �������� ���� ��(Axis)�� ���翵��Ų �Ÿ��� �����ϴ�.
	//    �������� ��ġ�� �߽��� + X���� + Y���� + Z����
	const float R = AABBExtents.X * std::abs(Axis.Dot(Axes[0])) +
					AABBExtents.Y * std::abs(Axis.Dot(Axes[1])) +
					AABBExtents.Z * std::abs(Axis.Dot(Axes[2]));

	// 2. AABB�� �߽����� ���翵 ��ǥ�� ���մϴ�.
	const float C = AABBCenter.Dot(Axis);

	// 3. ������ ������ ����Ͽ� �����մϴ�.
	OutMin = C - R;
	OutMax = C + R;
}

bool FOBB::Intersects(const FOBB& Other) const
{
	// �� OBB ���� ��� ��ġ ����
	FVector TestAxes[15];
	int AxisIndex = 0;

	// 1. ù ��° �ڽ��� ���� �� 3��
	TestAxes[AxisIndex++] = Axes[0];
	TestAxes[AxisIndex++] = Axes[1];
	TestAxes[AxisIndex++] = Axes[2];

	// 2. �� ��° �ڽ��� ���� �� 3��
	TestAxes[AxisIndex++] = Other.Axes[0];
	TestAxes[AxisIndex++] = Other.Axes[1];
	TestAxes[AxisIndex++] = Other.Axes[2];

	// 3. �� �ڽ� �� ���� ����(Cross Product) �� 9��
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			TestAxes[AxisIndex++] = this->Axes[i].Cross(Other.Axes[j]);
		}
	}

	// 15���� ��� �࿡ ���� ��ħ �˻�
	for (int i = 0; i < 15; ++i)
	{
		const FVector& Axis = TestAxes[i];

		// ���� ���̰� 0�� ������ (�� ���� ���� �����ϸ�) �׽�Ʈ�� �ǳʶڴ�.
		if (Axis.LengthSquared() < MATH_EPSILON) { continue; }

		// �� OBB�� ���� �࿡ �����Ѵ�.
		float MinA, MaxA, MinB, MaxB;
		this->Project(Axis, MinA, MaxA);
		Other.Project(Axis, MinB, MaxB);

		// ������ ������ ���� ������ �ִٸ�, �и� ���� ã�� ���̹Ƿ� �浹���� �ʴ´�.
		if (MaxA < MinB || MaxB < MinA) { return false; }
	}

	// 15�� �� ��ο��� ������ �������Ƿ� �浹.
	return true;
}

bool FOBB::Intersects(const FAABB& Other) const
{
	FVector testAxes[15];
	int axisIndex = 0;

	// �� 1-3: OBB�� ���� �� 3��
	testAxes[axisIndex++] = Axes[0];
	testAxes[axisIndex++] = Axes[1];
	testAxes[axisIndex++] = Axes[2];

	// �� 4-6: AABB�� �� (���� �� 3��)
	testAxes[axisIndex++] = FVector(1.f, 0.f, 0.f);
	testAxes[axisIndex++] = FVector(0.f, 1.f, 0.f);
	testAxes[axisIndex++] = FVector(0.f, 0.f, 1.f);

	// �� 7-15: OBB�� AABB �� ���� ���� �� 9��
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			const FVector AABBAxis = FVector((j == 0), (j == 1), (j == 2));
			testAxes[axisIndex++] = Axes[i].Cross(AABBAxis);
		}
	}

	// 2. ��� �࿡ ���� ��ħ �˻縦 �����մϴ�.
	for (int i = 0; i < 15; ++i)
	{
		const FVector& Axis = testAxes[i];

		// ���� ���̰� 0�� ������ (�� ���� ���� �����ϸ�) �׽�Ʈ�� �ǳʶڴ�.
		if (Axis.LengthSquared() < MATH_EPSILON) { continue; }

		// �� ������ ���� �࿡ �����մϴ�.
		float MinOBB, MaxOBB, MinAABB, MaxAABB;
		Project(Axis, MinOBB, MaxOBB);
		Project(Other, Axis, MinAABB, MaxAABB);

		// ������ ������ ���� ������ �ִٸ�, �и� ���� ã�� ���̹Ƿ� �浹���� �ʽ��ϴ�.
		if (MaxOBB < MinAABB || MaxAABB < MinOBB)
		{
			return false; // �и���!
		}
	}

	// 15�� �� ��ο��� ������ ���ƴٸ�, �� ������ �浹�� ���Դϴ�.
	return true;
}

bool FOBB::Intersects(const IBoundingVolume& Other) const
{
	switch (Other.GetType())
	{
	case EBoundingVolumeType::OBB:
	{
		// SAT �˰����� ���� �浹 ���θ� �Ǵ��մϴ�.
		const FOBB& OtherOBB = static_cast<const FOBB&>(Other);
		return Intersects(OtherOBB);
	}
	case EBoundingVolumeType::AABB:
	{
		// SAT �˰����� ���� �浹 ���θ� �Ǵ��մϴ�.
		const FAABB& OtherAABB = static_cast<const FAABB&>(Other);
		return Intersects(OtherAABB);
	}
	default:
		return false;
	}
}