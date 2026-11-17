// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Pattern_Exhausted.h"
#include "GameplayAbility_Pattern_AuroraExhausted.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_AuroraExhausted (11/4)
// 오로라의 지침. 필살기(얼음 지진)를 피하면 지친다 → 플레이어의 회복 구간.
//   (10/28~11/3에는 후딜 4초 동안 가만히 서 있기만 해서 어색했다)
//
// 흐름은 세바로그 지침(부모)과 완전히 같다. 생성자에서 동작·보이스·시간만 바꾼다.
//   동작 : 오로라 팩의 Stun_Start → Stun_Loop. Stun_End가 없어서 끝 동작은 비워 둔다 (부모가 반복 동작을 섞으며 끝낸다)
//   보이스 : 지칠 때 Aurora_Effort_Frustration, 지친 동안 Aurora_Effort_BreathingLowHealth (세바로그와 같은 조합)
//   들어오는 길 : Cryoseism이 폭발을 피했을 때 ABossBase::RequestExhaust → 지침 대기 → 우선순위 8로 바로 나온다
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_AuroraExhausted : public UGameplayAbility_Pattern_Exhausted
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_AuroraExhausted();
};
