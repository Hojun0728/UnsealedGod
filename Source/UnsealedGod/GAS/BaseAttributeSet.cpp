// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAttributeSet.h"
// FGameplayEffectModCallbackData 구조체가 정의된 헤더.
#include "GameplayEffectExtension.h"
// 피해를 받은 캐릭터에게 알려주기 위해 (OnDamaged)
#include "../Character/GameCharacter.h"

bool UBaseAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
		return false;

	// 무적 처리 : HP를 "깎는" 효과인데, 대상에게 State.Invincible 태그가 있으면 실행하지 않는다.
	// 투사체·보스 망치·충격파 등 모든 공격이 GE_Damage를 쓰기 때문에 여기 한 곳만 막으면 전부 막힌다.
	// EvaluatedData.Magnitude : 이번에 더할 값 (데미지는 음수)
	// Data.Target : 효과를 받는 쪽의 ASC
	// 예외 : 효과에 "막을 수 없음"(GameplayEffect.Damage.Unblockable) 태그가 붙어 있으면 무적이어도 들어간다.
	//   → 필살기(기둥 엄폐 궁극기) 즉사. 회피 무적으로 넘기면 기둥에 숨는 의미가 없어지기 때문. (10/15)
	// Data.EffectSpec.GetDynamicAssetTags() : 피해를 줄 때 코드가 이 효과에 붙인 태그들
	bool	bUnblockable = Data.EffectSpec.GetDynamicAssetTags().HasTag(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage.Unblockable")));

	if (!bUnblockable &&
		Data.EvaluatedData.Attribute == GetHPAttribute() &&
		Data.EvaluatedData.Magnitude < 0.f &&
		Data.Target.HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
	{
		UE_LOG(UGDebug, Warning, TEXT("Damage blocked : Invincible"));
		return false;
	}

	return true;
}

void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// HP를 깎은 효과만 본다. (마나 소모·회복 등은 무시)
	if (Data.EvaluatedData.Attribute != GetHPAttribute() || Data.EvaluatedData.Magnitude >= 0.f)
		return;

	// 맞은 캐릭터 = 이 AttributeSet의 주인. GetOwningActor() : AttributeSet을 가진 액터 (ASC의 Owner)
	AGameCharacter* Victim = Cast<AGameCharacter>(GetOwningActor());

	if (!IsValid(Victim))
		return;

	// 공격한 캐릭터 = 효과를 만든 쪽. Context의 "Instigator"(MakeEffectContext를 부른 ASC의 주인)
	// 투사체·장판처럼 중간 물체가 있어도 Instigator는 발사한 캐릭터다.
	AActor* Attacker = Data.EffectSpec.GetContext().GetInstigator();

	// 큰 공격인지 : 피해를 줄 때 붙인 "GameplayEffect.Damage.Heavy" 태그
	bool	bHeavy = Data.EffectSpec.GetDynamicAssetTags().HasTag(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage.Heavy")));

	// Magnitude는 음수(-15)라서 양수로 바꿔 넘긴다.
	// virtual 함수 : 실제 객체가 기디언이면 기디언의 OnDamaged(피격 반응)가 실행된다.
	Victim->OnDamaged(-Data.EvaluatedData.Magnitude, Attacker, bHeavy);
}

void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute,
	float& NewValue)
{
	// Super : 부모 클래스(UAttributeSet)의 같은 함수를 먼저 실행한다.
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UBaseAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
	float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// 수업 코드는 PreAttributeChange(CurrentValue)에서만 최대치 제한을 했다.
	// 그런데 데미지·마나 소모·마나 회복 같은 즉시/주기 효과는 BaseValue를 바꾸기 때문에,
	// BaseValue도 제한하지 않으면 "화면엔 MP 100인데 실제 BaseValue는 150"처럼
	// 값이 최대치를 넘어 쌓이는 문제가 생긴다. (마나 회복이 계속 돌기 때문에 꼭 필요)
	ClampAttribute(Attribute, NewValue);
}

void UBaseAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute,
	float& NewValue) const
{
	// HP나 MP는 0보다 작거나 HPMax·MPMax를 초과할 수 없다.
	// FMath::Clamp(값, 최소, 최대) : 값을 최소~최대 사이로 자른다.
	if (Attribute == GetHPAttribute())
		NewValue = FMath::Clamp(NewValue, 0.f, GetHPMax());

	else if (Attribute == GetMPAttribute())
		NewValue = FMath::Clamp(NewValue, 0.f, GetMPMax());
}
