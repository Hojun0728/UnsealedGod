// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "GameplayEffect_Heal.generated.h"

// ============================================================================
// UGameplayEffect_Heal
// HP를 "더하는" 즉시 효과. (10/19 회복 물약)
// GE_Damage에 양수를 넣어도 HP가 오르지만, 따로 만든 이유 :
//   피해 효과는 AttributeSet에서 무적 차단·피격 반응(OnDamaged)을 거친다 → 회복이 섞이면 헷갈린다.
// 회복량은 "GameplayEffect.Heal" 태그로 넘겨준다(SetByCaller, 양수). 최대 HP는 AttributeSet이 잘라 준다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayEffect_Heal : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGameplayEffect_Heal();
};
