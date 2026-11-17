// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MeleeHit.generated.h"

// ============================================================================
// UAnimNotifyState_MeleeHit
// 보스 공격 몽타주 위에 "막대"로 놓는 근접 판정 구간.
// 막대가 있는 동안 매 프레임 망치 손잡이 → 머리 사이의 여러 점 주변을 검사해서, 플레이어에게 피해를 준다.
// 한 막대(한 번의 휘두르기) 동안 같은 대상은 한 번만 맞는다.
//
// 몽타주 디테일에서 정하는 값
//   Socket Name       : 판정 끝점 (세바로그 망치 머리 = HammerCenter)
//   Start Socket Name : 판정 시작점 (세바로그 손 = weapon_r). 비우면 끝점 한 점만
//   Point Count       : 손 → 머리 사이 점 개수 (기본 4)
//   Radius       : 판정 구 반지름(cm)
//   Damage Scale : 피해 배율 (보스 공격력 × 배율). 3연타 1·2타 1.5, 3타 2.5
//   Heavy        : 큰 공격 (맞으면 넘어짐)
//   Freeze Time  : 0보다 크면 맞은 플레이어가 얼어붙는다 (10/28)
//
// 주의 : 노티파이 객체는 몽타주 에셋에 하나만 있어서 여러 캐릭터가 공유한다.
// 그래서 "이번에 맞은 대상" 같은 상태는 여기 저장하지 않고 공격하는 캐릭터(AGameCharacter)가 들고 있다. (보스·미니언 공용)
// ============================================================================
UCLASS(meta = (DisplayName = "Melee Hit Window"))
class UNSEALEDGOD_API UAnimNotifyState_MeleeHit : public UAnimNotifyState
{
	GENERATED_BODY()

protected:
	// 판정 끝점 (망치 머리)
	UPROPERTY(EditAnywhere, Category = "Melee")
	FName	mSocketName = TEXT("HammerCenter");

	// 판정 시작점 (손 쥐는 곳). 손 → 머리 사이를 여러 점으로 나눠 망치 "전체"를 검사한다.
	// None으로 비우면 머리 한 점만 검사한다. (10/14 : 머리만 검사하면 붙어 있는 플레이어 머리 위로 지나가 안 맞았음)
	UPROPERTY(EditAnywhere, Category = "Melee")
	FName	mStartSocketName = TEXT("weapon_r");

	// 손 → 머리 사이 판정 점 개수 (양 끝 포함). 4면 손·1/3·2/3·머리.
	// ClampMin : 디테일 패널에서 이 값보다 작게 넣을 수 없게 한다.
	UPROPERTY(EditAnywhere, Category = "Melee", meta = (ClampMin = "1"))
	int32	mPointCount = 4;

	UPROPERTY(EditAnywhere, Category = "Melee")
	float	mRadius = 80.f;

	UPROPERTY(EditAnywhere, Category = "Melee")
	float	mDamageScale = 1.f;

	// 큰 공격 : 맞은 플레이어가 넘어진다(Knock_Fwd → Stun_End). 3연타 3타·돌진 마무리 망치에 체크한다.
	UPROPERTY(EditAnywhere, Category = "Melee")
	bool	mHeavy = false;

	// (10/28) 얼리는 공격 : 0보다 크면 맞은 플레이어가 이 시간(초, 최대)만큼 얼어붙는다. WASD 연타로 일찍 깰 수 있다.
	// 넘어짐(Heavy)과 같이 쓰면 넘어진 동안은 얼음이 막히므로, 얼리는 공격은 Heavy를 끈다. (오로라 얼음 돌진 = 5)
	UPROPERTY(EditAnywhere, Category = "Melee", meta = (ClampMin = "0"))
	float	mFreezeTime = 0.f;

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	// 막대 구간 동안 매 프레임 호출된다. FrameDeltaTime : 지난 프레임부터 흐른 시간
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation, float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 타임라인에 "Melee HammerCenter x1.5"처럼 보이게 한다.
	virtual FString GetNotifyName_Implementation() const override;
};
