// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_Invincible.generated.h"

// ============================================================================
// UAnimNotifyState_Invincible
// 몽타주 위에 "막대"로 놓는 노티파이 스테이트다. 막대가 시작될 때 무적, 끝날 때 무적 해제.
// (일반 노티파이는 한 순간, 노티파이 스테이트는 시작~끝 구간이 있다)
//
// 몬스터헌터의 구르기처럼 "회피 동작 중 짧은 구간만" 무적을 주기 위해 만들었다.
// 막대의 위치·길이만 바꾸면 회피 판정을 빡빡하게/너그럽게 조정할 수 있다. (빌드 필요 없음)
//
// 실제로 데미지를 막는 곳은 UBaseAttributeSet::PreGameplayEffectExecute다.
// 여기서는 캐릭터에게 State.Invincible 태그를 붙였다 떼기만 한다.
// ============================================================================
UCLASS(meta = (DisplayName = "Invincible Window"))
class UNSEALEDGOD_API UAnimNotifyState_Invincible : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 막대 시작 지점을 지날 때. TotalDuration : 막대 길이(초)
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// 막대 끝 지점을 지날 때. (몽타주가 중간에 끊겨도 호출된다)
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
