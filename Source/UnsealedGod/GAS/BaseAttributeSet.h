// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "AttributeSet.h"
#include "BaseAttributeSet.generated.h"

// ============================================================================
// UBaseAttributeSet
// 캐릭터의 "수치"(체력, 마나, 공격력)를 담는 GAS의 그릇이다.
// 플레이어·보스·미니언이 모두 이 클래스를 같이 쓴다. (마나는 플레이어만 실제로 사용)
//
// GAS 용어 정리
//   ASC(AbilitySystemComponent) : GAS의 본체. 캐릭터에 붙는 컴포넌트로,
//                                 AttributeSet·GameplayEffect·GameplayAbility를 모두 관리한다.
//   AttributeSet                : 수치 묶음. ASC에 등록되어야 동작한다.
//   GameplayEffect(GE)          : 수치를 바꾸는 "규칙". (예 : "HP를 -10 한다", "1초마다 MP +5")
//   GameplayAbility(GA)         : 스킬 하나. (10/14부터 사용)
//
// FGameplayAttributeData : 값을 2개 가지고 있다.
//   BaseValue    : 영구적인 기본값. 데미지, 마나 소모, 회복처럼 "즉시/주기적" 효과가 바꾼다.
//   CurrentValue : 실제로 쓰이는 값. BaseValue + 지속 버프·디버프 결과다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UBaseAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

protected:
	// AllowPrivateAccess : protected/private 변수를 블루프린트에서 읽을 수 있게 허용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BaseAttributeSet", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData	HP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BaseAttributeSet", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData	HPMax;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BaseAttributeSet", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData	MP;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BaseAttributeSet", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData	MPMax;

	// 공격력. 2페이즈에서 보스가 미니언을 흡수할 때마다 오르는 값으로 쓸 예정이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BaseAttributeSet", meta = (AllowPrivateAccess = "true"))
	FGameplayAttributeData	Attack;

public:
	// virtual / override
	//   virtual  : "자식 클래스가 이 함수를 덮어쓸 수 있다"는 표시다.
	//   override : "부모의 virtual 함수를 덮어쓴다"는 표시다.
	//              부모 함수와 이름·인자가 다르면 컴파일 에러를 내줘서 오타를 잡아준다.
	// GAS가 값을 바꾸기 직전에 이 함수들을 자동으로 호출해 준다. 여기서 값을 검사·보정한다.

	// CurrentValue가 바뀌기 직전에 호출된다.
	// float& NewValue : 참조(&)로 받기 때문에, 여기서 NewValue를 바꾸면 실제로 적용될 값이 바뀐다.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	// BaseValue가 바뀌기 직전에 호출된다. (데미지·마나 소모·마나 회복은 BaseValue를 바꾼다)
	// 함수 끝의 const : 부모(엔진)에서 const로 선언된 함수라서 똑같이 맞춰야 한다.
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// 즉시·주기 효과(데미지 등)가 "실행되기 직전"에 호출된다.
	// false를 반환하면 그 효과의 실행이 취소된다. → 무적 처리에 사용한다.
	// FGameplayEffectModCallbackData& : 어떤 Attribute를 얼마나 바꾸려는지, 대상이 누구인지가 들어 있다.
	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;

	// 즉시·주기 효과가 "실행된 직후"에 호출된다. (Pre에서 무적으로 막힌 효과는 여기까지 오지 않는다)
	// HP가 깎였으면 맞은 캐릭터의 OnDamaged를 불러 피격 반응을 하게 한다.
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

public:
	ATTRIBUTE_FUNCTION(UBaseAttributeSet, HP)
	ATTRIBUTE_FUNCTION(UBaseAttributeSet, HPMax)
	ATTRIBUTE_FUNCTION(UBaseAttributeSet, MP)
	ATTRIBUTE_FUNCTION(UBaseAttributeSet, MPMax)
	ATTRIBUTE_FUNCTION(UBaseAttributeSet, Attack)

private:
	// HP·MP를 0 ~ 최대치 사이로 자르는 공통 함수.
	// Pre 함수 두 개가 같은 일을 하므로 한 곳에 모아둔다.
	// const 함수인 PreAttributeBaseChange에서도 부르기 위해 이 함수도 const로 만든다.
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
