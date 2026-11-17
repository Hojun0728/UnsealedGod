// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SendGameplayEvent.generated.h"

// ============================================================================
// UAnimNotify_SendGameplayEvent
// 애니메이션의 특정 프레임에서 "게임플레이 이벤트"를 보내는 범용 노티파이다.
// (수업 코드에서 공격 판정 때 쓴 SendGameplayEventToActor를 노티파이로 만든 것)
//
// 사용 예 : 회피 몽타주의 "포털로 뛰어드는 프레임"에 놓고 태그를 Event.Dodge.Teleport로 지정
//   → 실행 중인 회피 어빌리티가 이 이벤트를 받아서 그 순간 순간이동한다.
//
// 노티파이가 캐릭터 함수를 직접 부르지 않고 이벤트만 보내는 이유 :
// 노티파이는 "언제"만 알려주고, "무엇을 할지"는 이벤트를 기다리는 어빌리티가 정한다.
// 그래서 보스 패턴·Q·R 스킬 등 어디서든 태그만 바꿔서 재사용할 수 있다.
// ============================================================================
UCLASS(meta = (DisplayName = "Send Gameplay Event"))
class UNSEALEDGOD_API UAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

protected:
	// 보낼 이벤트 태그. 몽타주의 노티파이 디테일에서 고른다.
	UPROPERTY(EditAnywhere, Category = "Event")
	FGameplayTag	mEventTag;

	// 이벤트에 함께 담아 보낼 숫자. (GAS 이벤트 데이터의 EventMagnitude 칸)
	// 뜻은 이벤트를 받는 쪽이 정한다. 기본 0이면 받는 쪽은 "추가 정보 없음"으로 본다.
	// 사용 예 : 보스 밀기(Event.Boss.Lunge) → 타겟을 바라볼 때 더할 좌우 각도(도). 3연타 3타가 비스듬히 내려찍어서 보정.
	UPROPERTY(EditAnywhere, Category = "Event")
	float	mEventMagnitude = 0.f;

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 타임라인에 "Event Event.Dodge.Teleport"처럼 태그 이름이 보이게 한다.
	virtual FString GetNotifyName_Implementation() const override;
};
