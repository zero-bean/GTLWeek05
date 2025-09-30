#pragma once
#include "Name.h"
#include "ObjectPtr.h"

class UObject;

/**
 * @brief UClass Metadata System
 * Runtime에 컴파일 시에 다양한 클래스 정보를 제공하기 위해 만들어진 클래스
 * @param ClassName 클래스 이름
 * @param SuperClass 부모 클래스
 * @param ClassSize 클래스 크기
 * @param Constructor 생성자 함수 포인터
 */
class UClass
{
public:
	// 생성자 함수 포인터 타입 정의
    typedef UObject* (*ClassConstructorType)();

public:
	UClass(const FName& InName, TObjectPtr<UClass> InSuperClass, size_t InClassSize, ClassConstructorType InConstructor);

    // Getter
    const FName& GetName() const { return ClassName; }
    TObjectPtr<UClass> GetSuperClass() const { return SuperClass; }
    size_t GetClassSize() const { return ClassSize; }
    
	bool IsChildOf(TObjectPtr<UClass> InClass) const;
	TObjectPtr<UObject> CreateDefaultObject() const;


private:
	FName ClassName;
	TObjectPtr<UClass> SuperClass;
	size_t ClassSize;
	ClassConstructorType Constructor;
};

/**
 * @brief RTTI 매크로 시스템
 *
 * 언리얼엔진의 UCLASS(), DECLARE_CLASS, IMPLEMENT_CLASS와 유사한 매크로들 정의
 */
#define UCLASS()

// 클래스 본문 내부에서 사용하는 매크로 (friend 등 내부 선언 처리)
#define GENERATED_BODY() \
public: \
    friend class UClass; \
    friend class UObject;

// 클래스 선언부에 사용하는 매크로
#define DECLARE_CLASS(ClassName, SuperClassName) \
public: \
    typedef ClassName ThisClass; \
    typedef SuperClassName Super; \
    static UClass* StaticClass(); \
    virtual UClass* GetClass() const; \
    static UObject* CreateDefaultObject##ClassName();

// 클래스 구현부에 사용하는 매크로
#define IMPLEMENT_CLASS(ClassName, SuperClassName) \
    UClass* ClassName::StaticClass() \
    { \
        /* 정적 지역 변수를 사용하여 UClass 객체를 자동 관리 */ \
        static UClass Instance( \
            FString(#ClassName), \
            SuperClassName::StaticClass(), \
            sizeof(ClassName), \
            &ClassName::CreateDefaultObject##ClassName \
        ); \
        return &Instance; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    UObject* ClassName::CreateDefaultObject##ClassName() \
    { \
        return new ClassName(); \
    }


// 추상 클래스에 사용하는 매크로 (기본 객체를 생성하지 않음)
#define IMPLEMENT_ABSTRACT_CLASS(ClassName, SuperClassName) \
    UClass* ClassName::StaticClass() \
    { \
        static UClass Instance( \
            FString(#ClassName), \
            SuperClassName::StaticClass(), \
            sizeof(ClassName), \
            nullptr \
        ); \
        return &Instance; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    }

// UObject의 기본 매크로 (다른 클래스들의 베이스)
#define IMPLEMENT_CLASS_BASE(ClassName) \
    UClass* ClassName::StaticClass() \
    { \
        static UClass Instance( \
            FString(#ClassName), \
            nullptr, \
            sizeof(ClassName), \
            &ClassName::CreateDefaultObject##ClassName \
        ); \
        return &Instance; \
    } \
    UClass* ClassName::GetClass() const \
    { \
        return ClassName::StaticClass(); \
    } \
    UObject* ClassName::CreateDefaultObject##ClassName() \
    { \
        return new ClassName(); \
    }
