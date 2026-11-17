// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "GameplayEffect_Burn.generated.h"

// ============================================================================
// UGameplayEffect_Burn
// 일정 시간 동안 "주기마다" HP를 깎는 지속 피해(도트) GameplayEffect다. (기디언 F 버든)
//
// GE_Damage(즉시 한 번)와 다른 점 : Period(주기)를 정하면 GAS가 알아서
// "0.5초마다 Modifier를 한 번씩 실행"해 준다. 타이머 코드를 직접 짤 필요가 없다.
// 한 번 실행할 때 깎는 양은 GE_Damage와 같은 "GameplayEffect.Damage" 태그로 넘겨준다(SetByCaller).
// 각 실행은 즉시 피해처럼 AttributeSet의 PreGameplayEffectExecute를 거치므로 무적 처리도 똑같이 적용된다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayEffect_Burn : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGameplayEffect_Burn();
};
