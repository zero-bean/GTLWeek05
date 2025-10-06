#pragma once
#include "Physics/Public/BoundingVolume.h"

struct FAABB;

struct FOBB : public IBoundingVolume
{
public:
	FVector Center;    // 박스의 중심점
	FVector Extents;   // 박스 각 축 방향으로의 절반 크기
	FVector Axes[3];   // 박스의 회전을 나타내는 로컬 축 (단위 벡터)

	FOBB();
	FOBB(const FVector& InCenter, const FVector& InExtents);

	/**
	 * @brief SAT 알고리즘을 이용한 OBB 충돌 검사 함수 
	 */
	bool Intersects(const FOBB& Other) const;

	/**
	 * @brief SAT 알고리즘을 이용한 AABB 충돌 검사 함수
	 */
	bool Intersects(const FAABB& Other) const;

	/* *
	* @brief IBoundingVolume의 순수가상함수 오버라이딩
	*/
	bool RaycastHit() const override { return false; }
	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::OBB; }
	bool Intersects(const IBoundingVolume& Other) const override;

private:
	/**
	 * @brief 주어진 축에 OBB 혹은 AABB를 투영하여 최소, 최대값을 얻습니다.
	 * @param Axis 투영할 축 (단위 벡터)
	 * @param OutMin 투영된 간격의 최소값
	 * @param OutMax 투영된 간격의 최대값
	 */
	void Project(const FVector& Axis, float& OutMin, float& OutMax) const;
	void Project(const FAABB& InAABB, const FVector& Axis, float& OutMin, float& OutMax) const;
};

