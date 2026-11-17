// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "UObject/Object.h"
#include "Input.generated.h"

// ============================================================================
// UInput
// 향상된 입력(Enhanced Input)에서 쓰는 에셋 2종을 보관하는 클래스다. (수업 코드와 동일한 구조)
//   - InputMappingContext(IMC) : "어떤 키가 어떤 액션인지" 연결표
//   - InputAction(IA)          : "이동", "점프" 같은 행동 하나
// 캐릭터는 이 클래스의 CDO에서 에셋을 꺼내 쓴다.
//
// CDO(Class Default Object) : 언리얼이 클래스마다 하나씩 미리 만들어두는 "기본값 객체"다.
// 생성자는 CDO를 만들 때 실행되므로, 생성자에서 에셋을 찾아두면
// GetDefault<UGideonInput>()으로 어디서든 같은 에셋을 꺼내 쓸 수 있다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UInput : public UObject
{
	GENERATED_BODY()

public:
	UInput();

public:
	// UPROPERTY()를 붙이면 언리얼의 가비지 컬렉터(GC)가 이 변수가 가리키는 객체를
	// "사용 중"으로 인식해서 메모리에서 지우지 않는다.
	// (수업 코드에는 UPROPERTY가 없지만, 안전하게 붙여둔다.)
	//
	// TObjectPtr<T> : 언리얼 객체를 가리키는 포인터다. T* 와 거의 같지만
	// 에디터에서 추적·지연 로딩 기능이 더해진 언리얼 전용 포인터라서 멤버 변수에는 이걸 쓴다.
	UPROPERTY()
	TObjectPtr<UInputMappingContext>	mContext;

protected:
	// TMap<키, 값> : 이름(문자열)으로 InputAction을 찾을 수 있는 사전(딕셔너리) 자료구조다.
	// FString : 언리얼엔진에서 제공하는 문자열 클래스이다.
	UPROPERTY()
	TMap<FString, TObjectPtr<UInputAction>>	mActions;

public:
	// 함수 뒤의 const : "이 함수는 멤버 변수를 바꾸지 않는다"는 약속이다.
	// GetDefault로 얻은 CDO는 const 포인터라서, const 함수만 호출할 수 있다.
	TObjectPtr<UInputAction> FindAction(const FString& Name)	const;
};


// ============================================================================
// UGideonInput
// 기디언 전용 입력 에셋 목록. UInput을 상속받아 생성자에서 실제 에셋 경로를 지정한다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGideonInput : public UInput
{
	GENERATED_BODY()

public:
	UGideonInput();
};
