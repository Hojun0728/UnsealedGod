// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "GameplayEffect_Damage.generated.h"

// ============================================================================
// UGameplayEffect_Damage
// 대상의 HP를 깎는 GameplayEffect다. (수업 코드와 동일)
// 피해량은 고정값이 아니라, 적용하는 쪽에서 "GameplayEffect.Damage" 태그로 넘겨준다(SetByCaller).
// 사용 예 : 기디언 투사체, 보스 망치, 충격파 등 모든 공격이 이 GE 하나로 데미지를 준다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGameplayEffect_Damage();
};
