#pragma once
#include "Physics/Public/BoundingVolume.h"

struct FAABB;

struct FOBB : public IBoundingVolume
{
public:
	FVector Center;    // �ڽ��� �߽���
	FVector Extents;   // �ڽ� �� �� ���������� ���� ũ��
	FVector Axes[3];   // �ڽ��� ȸ���� ��Ÿ���� ���� �� (���� ����)

	FOBB();
	FOBB(const FVector& InCenter, const FVector& InExtents);

	/**
	 * @brief SAT �˰����� �̿��� OBB �浹 �˻� �Լ� 
	 */
	bool Intersects(const FOBB& Other) const;

	/**
	 * @brief SAT �˰����� �̿��� AABB �浹 �˻� �Լ�
	 */
	bool Intersects(const FAABB& Other) const;

	/* *
	* @brief IBoundingVolume�� ���������Լ� �������̵�
	*/
	bool RaycastHit() const override { return false; }
	EBoundingVolumeType GetType() const override { return EBoundingVolumeType::OBB; }
	bool Intersects(const IBoundingVolume& Other) const override;

private:
	/**
	 * @brief �־��� �࿡ OBB Ȥ�� AABB�� �����Ͽ� �ּ�, �ִ밪�� ����ϴ�.
	 * @param Axis ������ �� (���� ����)
	 * @param OutMin ������ ������ �ּҰ�
	 * @param OutMax ������ ������ �ִ밪
	 */
	void Project(const FVector& Axis, float& OutMin, float& OutMax) const;
	void Project(const FAABB& InAABB, const FVector& Axis, float& OutMin, float& OutMax) const;
};

