
#pragma once

// ============================================================================
// GameInfo.h
// 프로젝트 전체에서 공통으로 쓰는 헤더 파일이다. (수업 코드의 GameInfo.h와 같은 역할)
// 다른 클래스들은 이 파일 하나만 include하면 자주 쓰는 엔진 기능을 바로 쓸 수 있다.
// 공통 로그 카테고리, 공통 매크로, 공통 구조체·열거형도 여기에 모은다.
// ============================================================================

// 엔진의 핵심 기능(액터, 컴포넌트, 수학, 문자열 등)을 한 번에 가져온다.
#include "EngineMinimal.h"
// GEngine(화면 디버그 메시지 출력 등)을 쓰기 위해 필요하다.
#include "Engine/Engine.h"
// 생성자에서 에셋을 경로로 찾아오는 ConstructorHelpers가 들어있다.
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"

// 캐릭터 관련 컴포넌트들.
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// 투사체 관련 컴포넌트들.
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// 파티클(캐스케이드) 관련. 파라곤 팩의 이펙트는 전부 캐스케이드 형식이라
// 나이아가라(UNiagaraComponent)가 아니라 UParticleSystem / UParticleSystemComponent를 쓴다.
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

// UGameplayStatics : 이펙트 생성(SpawnEmitterAtLocation) 등 자주 쓰는 정적 함수 모음.
#include "Kismet/GameplayStatics.h"

// AI 관련 (수업 코드의 GameInfo.h와 같은 헤더들)
#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

// GAS(게임플레이 어빌리티 시스템) 관련 헤더.
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"

// 참고: 이 파일에 UENUM, USTRUCT 같은 리플렉션 타입이 생기면
// 그때 #include "GameInfo.generated.h"를 맨 아래 include로 추가해야 한다.
// 지금은 리플렉션 타입이 없어서 generated.h 파일이 만들어지지 않으므로 넣지 않는다.

// 로그 카테고리 선언.
// UE_LOG(UGDebug, Warning, TEXT("...")) 처럼 써서 출력 로그 창에서 우리 로그만 골라 볼 수 있다.
// 실제 정의는 GameInfo.cpp의 DEFINE_LOG_CATEGORY에 있다.
DECLARE_LOG_CATEGORY_EXTERN(UGDebug, Warning, All);

// ----------------------------------------------------------------------------
// AttributeSet용 매크로 (수업 코드와 동일)
// AttributeSet 변수 하나를 만들 때마다 필요한 함수 4개를 한 줄로 만들어준다.
//   GetHPAttribute() : "HP라는 속성 자체"를 가리키는 정보 (GAS에 "어떤 값을 바꿀지" 알려줄 때 사용)
//   GetHP()          : 현재 HP 값
//   SetHP(값)        : HP 값 변경
//   InitHP(값)       : HP 초기값 설정
// 매크로 : 컴파일 전에 "글자 그대로 바꿔치기"되는 코드 조각이다.
//          ATTRIBUTE_FUNCTION(UBaseAttributeSet, HP) 한 줄이 위 4개 함수 코드로 바뀐다.
// ----------------------------------------------------------------------------
#define	ATTRIBUTE_FUNCTION(ClassName, AttributeName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, AttributeName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(AttributeName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(AttributeName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(AttributeName)
