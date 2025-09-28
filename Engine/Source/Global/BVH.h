#pragma once
#include "pch.h"
#include "Physics/Public/AABB.h"

class UPrimitiveComponent;
struct FStaticMesh;

struct FNode
{
	int32 ObjectIndex;
	int32 ParentIndex;
	int32 Child1;
	int32 Child2;
	bool bIsLeaf;
	FAABB Box;
	int32 TriangleBaseIndex; // �ε��� ���ۿ��� �ﰢ���� ���� �ε���
};

/**
* @brief: Narrow Phase Picking�� ���Ǵ� FBVH (Bounding Volume Hierarchy) Ŭ����
*/
class FBVH
{
public:
	FBVH() = default;
	explicit FBVH(FStaticMesh* InMesh);

	void Build(FStaticMesh* InMesh);
	int32 GetRootIndex() const { return RootIndex; }
	int32 GetNodeCount() const { return Nodes.size(); }
	const FNode& GetNode(uint32 Index) const;
	FNode& GetNode(uint32 Index);
	void Clear();

	/**
	* @brief ����Ʈ���� cost(����Ʈ���� ��� AABB�� ǥ������ ��)�� ���.
	* @param SubTreeRootIndex: ����Ʈ���� ��Ʈ ��� �ε���
	* @param bInternalOnly: true�� ���� ����� cost�� �����ϰ� ���
	* @return cost ��	
	*/
	float GetCost(int32 SubTreeRootIndex, bool bInternalOnly = false) const;

	/**
	* @brief: 트리의 유효성 검사.
	*/
	bool CheckValidity() const;

	/**
	* @brief: Ray와 BVH를 순회하여 교차하는 삼각형들의 인덱스 리스트를 반환
	* @param Ray: 교차 검사를 수행할 Ray (Local 좌표계)
	* @param OutTriangleIndices: 교차하는 삼각형들의 base index 리스트 (output)
	* @return: 교차하는 삼각형이 있으면 true, 없으면 false
	*/
	bool TraverseRay(const FRay& Ray, TArray<int32>& OutTriangleIndices) const;

	/**
	* @brief: 새 리프 노드를 특정 노드의 형제로 추가했을 때 전체 뉱업 트리의 비용 증가량 계산
	* @param CandidateIndex: 후보 형제 노드 인덱스
	* @param NewLeafAABB: 새로운 리프 노드의 AABB
	* @return: 비용 증가량
	*/
	float CalculateCostIncrease(int32 CandidateIndex, const FAABB& NewLeafAABB) const;

private:
	/**
	* @brief ���ο� leaf node�� ����.
	* @note: cost�� ���� �������� ������ sibling node�� ã�Ƽ� ���Եǵ��� ��.
	* @return ���Ե� leaf node�� �ε���
	*/
	int32 InsertLeaf(int32 InTriangleBaseIndex);

	// --- �� leaf node ���� ���� ���� �޼ҵ�� ---

	//@brief ���ο� leaf node�� �߰��Ǿ��� �� cost�� ���� ���� �����ϴ� sibling node Ž��.
	int32 FindBestSibling(const FAABB& NewLeafAABB);
	//@brief ���ο� leaf node�� ���� sibling node�� ���� internal node�� �����ϰ� Ʈ���� ����.
	void InsertInternalNode(int32 LeafIndex, int32 SiblingIndex);
	//@brief �־��� ����� '�θ�'���� ��Ʈ���� �ö󰡸� AABB Refit ����.
	void RefitAncestors(int32 RefitStartIndex);

	FStaticMesh* Mesh = nullptr; // BVH ���� ����ƽ �޽�
	TArray<FNode> Nodes;
	int32 RootIndex = -1;
	float Cost = 0.0f; // ��ü Ʈ���� cost
};

FAABB GetTriangleAABB(const FNormalVertex& V0, const FNormalVertex& V1, const FNormalVertex& V2);