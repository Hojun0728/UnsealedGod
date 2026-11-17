// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_SwordCombo.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_SwordCombo
// 오로라의 검 3연타 (모든 페이즈). 세바로그 망치 3연타(HammerCombo)와 같은 구조다. (10/28)
// 부모(UGameplayAbility_BossPattern)의 공통 동작을 그대로 쓰고 생성자에서 몽타주·수치만 정한다.
//   - 가까울 때(4m 이내)만 사용
//   - 휘두를 때마다 타겟 쪽으로 돌며 밀고 나온다 (몽타주의 Lunge 노티파이)
//   - 판정·피해는 몽타주의 Melee Hit Window (칼 손잡이 Sword_Base → 칼끝 Sword_Tip, 1·2타 1.2배, 3타 2.0배 + 넘어짐)
//   - 마지막 Slow_C_Recovery(칼을 거두는 동작) = 후딜 = 플레이어의 반격 틈
// → 보스가 바뀌어도 "근접 연타"는 부모 코드 그대로, 몽타주와 숫자만 다르다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_SwordCombo : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_SwordCombo();
};
