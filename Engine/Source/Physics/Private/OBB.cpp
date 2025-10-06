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
	// 기본 생성 시에는 회전이 없으므로 월드 축과 동일하게 설정
	Axes[0] = FVector(1.f, 0.f, 0.f); // Local X
	Axes[1] = FVector(0.f, 1.f, 0.f); // Local Y
	Axes[2] = FVector(0.f, 0.f, 1.f); // Local Z
}

void FOBB::Project(const FVector& Axis, float& OutMin, float& OutMax) const
{
	// 1. 꼭짓점의 정사영 길이를 구합니다.
	//    이는 중심점에서 가장 멀리 떨어진 꼭짓점을 기준 축(Axis)에 정사영시킨 거리와 같습니다.
	//    꼭짓점의 위치는 중심점 + X뼈대 + Y뼈대 + Z뼈대
	const float R = Extents.X * std::abs(Axis.Dot(Axes[0])) +
					Extents.Y * std::abs(Axis.Dot(Axes[1])) +
					Extents.Z * std::abs(Axis.Dot(Axes[2]));

	// 2. OBB의 중심점의 정사영 좌표를 구합니다.
	const float C = Center.Dot(Axis);

	// 3. 투영된 간격을 계산하여 저장합니다.
	OutMin = C - R;
	OutMax = C + R;
}

void FOBB::Project(const FAABB& InAABB, const FVector& Axis, float& OutMin, float& OutMax) const
{
	// 0. AABB의 중심과 절반 크기(Extents)를 구합니다.
	const FVector AABBCenter = InAABB.GetCenter();
	const FVector AABBExtents = (InAABB.Max - InAABB.Min) * 0.5f;

	// 1. 꼭짓점의 정사영 길이를 구합니다.
	//    이는 중심점에서 가장 멀리 떨어진 꼭짓점을 기준 축(Axis)에 정사영시킨 거리와 같습니다.
	//    꼭짓점의 위치는 중심점 + X뼈대 + Y뼈대 + Z뼈대
	const float R = AABBExtents.X * std::abs(Axis.Dot(Axes[0])) +
					AABBExtents.Y * std::abs(Axis.Dot(Axes[1])) +
					AABBExtents.Z * std::abs(Axis.Dot(Axes[2]));

	// 2. AABB의 중심점의 정사영 좌표를 구합니다.
	const float C = AABBCenter.Dot(Axis);

	// 3. 투영된 간격을 계산하여 저장합니다.
	OutMin = C - R;
	OutMax = C + R;
}

bool FOBB::Intersects(const FOBB& Other) const
{
	// 두 OBB 간의 상대 위치 벡터
	FVector TestAxes[15];
	int AxisIndex = 0;

	// 1. 첫 번째 박스의 로컬 축 3개
	TestAxes[AxisIndex++] = Axes[0];
	TestAxes[AxisIndex++] = Axes[1];
	TestAxes[AxisIndex++] = Axes[2];

	// 2. 두 번째 박스의 로컬 축 3개
	TestAxes[AxisIndex++] = Other.Axes[0];
	TestAxes[AxisIndex++] = Other.Axes[1];
	TestAxes[AxisIndex++] = Other.Axes[2];

	// 3. 두 박스 축 간의 외적(Cross Product) 축 9개
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			TestAxes[AxisIndex++] = this->Axes[i].Cross(Other.Axes[j]);
		}
	}

	// 15개의 모든 축에 대해 겹침 검사
	for (int i = 0; i < 15; ++i)
	{
		const FVector& Axis = TestAxes[i];

		// 축의 길이가 0에 가까우면 (두 축이 거의 평행하면) 테스트를 건너뛴다.
		if (Axis.LengthSquared() < MATH_EPSILON) { continue; }

		// 각 OBB를 현재 축에 투영한다.
		float MinA, MaxA, MinB, MaxB;
		this->Project(Axis, MinA, MaxA);
		Other.Project(Axis, MinB, MaxB);

		// 투영된 간격이 서로 떨어져 있다면, 분리 축을 찾은 것이므로 충돌하지 않는다.
		if (MaxA < MinB || MaxB < MinA) { return false; }
	}

	// 15개 축 모두에서 간격이 겹쳤으므로 충돌.
	return true;
}

bool FOBB::Intersects(const FAABB& Other) const
{
	FVector testAxes[15];
	int axisIndex = 0;

	// 축 1-3: OBB의 로컬 축 3개
	testAxes[axisIndex++] = Axes[0];
	testAxes[axisIndex++] = Axes[1];
	testAxes[axisIndex++] = Axes[2];

	// 축 4-6: AABB의 축 (월드 축 3개)
	testAxes[axisIndex++] = FVector(1.f, 0.f, 0.f);
	testAxes[axisIndex++] = FVector(0.f, 1.f, 0.f);
	testAxes[axisIndex++] = FVector(0.f, 0.f, 1.f);

	// 축 7-15: OBB와 AABB 축 간의 외적 축 9개
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			const FVector AABBAxis = FVector((j == 0), (j == 1), (j == 2));
			testAxes[axisIndex++] = Axes[i].Cross(AABBAxis);
		}
	}

	// 2. 모든 축에 대해 겹침 검사를 수행합니다.
	for (int i = 0; i < 15; ++i)
	{
		const FVector& Axis = testAxes[i];

		// 축의 길이가 0에 가까우면 (두 축이 거의 평행하면) 테스트를 건너뛴다.
		if (Axis.LengthSquared() < MATH_EPSILON) { continue; }

		// 각 도형을 현재 축에 투영합니다.
		float MinOBB, MaxOBB, MinAABB, MaxAABB;
		Project(Axis, MinOBB, MaxOBB);
		Project(Other, Axis, MinAABB, MaxAABB);

		// 투영된 간격이 서로 떨어져 있다면, 분리 축을 찾은 것이므로 충돌하지 않습니다.
		if (MaxOBB < MinAABB || MaxAABB < MinOBB)
		{
			return false; // 분리됨!
		}
	}

	// 15개 축 모두에서 간격이 겹쳤다면, 두 도형은 충돌한 것입니다.
	return true;
}

bool FOBB::Intersects(const IBoundingVolume& Other) const
{
	switch (Other.GetType())
	{
	case EBoundingVolumeType::OBB:
	{
		// SAT 알고리즘을 통해 충돌 여부를 판단합니다.
		const FOBB& OtherOBB = static_cast<const FOBB&>(Other);
		return Intersects(OtherOBB);
	}
	case EBoundingVolumeType::AABB:
	{
		// SAT 알고리즘을 통해 충돌 여부를 판단합니다.
		const FAABB& OtherAABB = static_cast<const FAABB&>(Other);
		return Intersects(OtherAABB);
	}
	default:
		return false;
	}
}