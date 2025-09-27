#pragma once

/**
 * @brief Occlusion Culling 을 담당하는 클래스
 */
class UStaticMeshComponent;

class COcclusionCuller
{
public:
    COcclusionCuller();

    /**
     * @brief 컬링 프로세스를 위한 환경을 초기화하고 View/Projection 행렬을 설정.
     * 매 프레임 컬링을 시작하기 전에 호출되어야 함
     */
    void InitializeCuller(const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix);

     /**
     * @brief 오클루전 컬링의 전체 프로세스를 실행하고 최종 가시 오브젝트 목록을 반환
     * @param AllStaticMeshes 프러스텀 컬링을 통과한 모든 스태틱 메시 목록
     * @param CameraPos 현재 카메라 위치
     * @return 렌더링되어야 할 UStaticMeshComponent 목록
     */
    TArray<TObjectPtr<UStaticMeshComponent>> PerformCulling(const TArray<TObjectPtr<UStaticMeshComponent>>& AllStaticMeshes, const FVector& CameraPos);

    // Constants
    static constexpr int Z_BUFFER_WIDTH = 256;
    static constexpr int Z_BUFFER_HEIGHT = 256;
    static constexpr int Z_BUFFER_SIZE = Z_BUFFER_WIDTH * Z_BUFFER_HEIGHT;

private:
    /**
     * @brief 해당 메시 컴포넌트가 Z-Buffer에 의해 가려지는지 테스트합니다.
     * @return 바운딩 박스의 코너 중 하나라도 보이면 true를 반환합니다.
     */
    bool IsMeshVisible(const UStaticMeshComponent* MeshComp) const;

    struct BatchProjectionResult BatchProject4(const struct BatchProjectionInput& Input) const;

    /**
     * @brief World Pos > Clip Space > Screen Coordinate
     * @return 스크린 좌표 (X, Y)와 NDC Depth (Z)를 담은 FVector
     */
    FVector Project(const FVector& WorldPos) const;

    /**
     * @brief CPU Z-Buffer에 삼각형을 래스터라이징, 깊이 테스트 수행
     */
    void RasterizeTriangle(const FVector& P1, const FVector& P2, const FVector& P3, TArray<float>& ZBuffer);

    /**
     * @brief PrimitiveComponent의 AABB를 12개의 삼각형 정점으로 변환
     */
    TArray<FVector> ConvertAABBToTriangles(const class UPrimitiveComponent* PrimitiveComp) const;
     /**
     * @brief 화면 공간 면적 및 거리를 기준으로 오클루더 역할을 할 객체 동적 선택
     * @param AllCandidates 프러스텀 컬링을 통과한 모든 스태틱 메시 후보 목록
     * @param MaxOccluders Z-Buffer 구성에 사용할 최대 오클루더 수
     * @return 오클루더로 선정된 UStaticMeshComponent 목록
     */
    TArray<UStaticMeshComponent*> SelectOccluders(const TArray<TObjectPtr<UStaticMeshComponent>>& AllCandidates, const FVector& CameraPos, uint32 MaxOccluders = 100);

    void RasterizeOccluders(const TArray<UStaticMeshComponent*>& SelectedOccluders);

    TArray<float> CPU_ZBuffer;
    FMatrix CurrentViewProj;

};

struct FOccluderScore
{
    UStaticMeshComponent* MeshComp;
    float Score;

    bool operator>(const FOccluderScore& Other) const { return Score > Other.Score; }
};

struct alignas(16) BatchProjectionInput
{
    __m128 WorldX, WorldY, WorldZ; // 4개 점의 X, Y, Z 좌표
};

struct alignas(16) BatchProjectionResult
{
    __m128 ScreenX, ScreenY, ScreenZ; // 4개 점의 스크린 좌표
    __m128i PixelX, PixelY;          // 정수형 픽셀 좌표
    __m128 InBoundsMask;             // 화면 경계 내부 마스크
};
