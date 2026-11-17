// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_HammerCombo.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_HammerCombo
// 세바로그의 망치 3연타 (모든 페이즈, Slow 애니메이션 1.5배속 / 3페이즈부터 2배속).
// 부모(UGameplayAbility_BossPattern)의 공통 동작을 그대로 쓰고 생성자에서 수치만 정한다.
//   - 가까울 때(5m 이내)만 사용
//   - 휘두를 때마다 타겟 쪽으로 돌며 약 1.5m 밀고 나온다 (몽타주의 Lunge 노티파이)
//   - 피해·판정 구간은 몽타주의 Melee Hit Window 노티파이에서 정한다 (1·2타 1.5배, 3타 2.5배)
//   - 마지막 Return2Idle = 후딜 = 플레이어의 반격 틈
// 3페이즈부터(50% 필살기 이후)는 같은 몽타주를 2배속으로 재생한다. (FAST_v2는 너무 빨라서 사용 안 함)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_HammerCombo : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_HammerCombo();
};
