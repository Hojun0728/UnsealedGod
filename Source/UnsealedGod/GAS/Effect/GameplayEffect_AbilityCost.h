// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "GameplayEffect_AbilityCost.generated.h"

// ============================================================================
// UGameplayEffect_AbilityCost
// 스킬 사용 비용(MP, HP)을 깎는 GameplayEffect다. (수업 코드와 동일)
// 소모량은 "GameplayEffect.MPCost", "GameplayEffect.HPCost" 태그로 넘겨준다(SetByCaller).
// 10/14 이후 스킬(GameplayAbility)이 ApplyCost에서 이 GE를 사용한다.
// 오늘은 2번 테스트 키로 마나 소모를 확인하는 데 사용한다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayEffect_AbilityCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGameplayEffect_AbilityCost();
};
