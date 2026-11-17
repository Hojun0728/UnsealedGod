// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_HideMesh.generated.h"

// ============================================================================
// UAnimNotifyState_HideMesh
// 몽타주 위에 "막대"로 놓으면, 그 구간 동안 캐릭터 메시를 숨긴다.
// 회피(토른 스페이스)에서 포털로 뛰어든 순간 잠깐 사라졌다가 도착 지점에 나타나는 연출용.
// (구조는 UAnimNotifyState_Invincible과 같다 : 시작에 켜고, 끝에 끈다)
//
// 메시의 "보이기"만 끈다. 충돌·위치·애니메이션은 그대로 동작한다.
// 회피 어빌리티가 끝날 때 안전장치로 다시 보이게 하므로, 몽타주가 중간에 끊겨도 투명하게 남지 않는다.
// ============================================================================
UCLASS(meta = (DisplayName = "Hide Mesh Window"))
class UNSEALEDGOD_API UAnimNotifyState_HideMesh : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
