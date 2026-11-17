// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_MeleeHit.h"
#include "../Character/GameCharacter.h"

void UAnimNotifyState_MeleeHit::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// 메시의 주인이 우리 캐릭터(보스·미니언)일 때만 동작한다. (몽타주 미리보기 창에서는 아무 일도 안 일어난다)
	AGameCharacter* Attacker = IsValid(MeshComp) ? MeshComp->GetOwner<AGameCharacter>() : nullptr;

	if (IsValid(Attacker))
		Attacker->BeginMeleeHit();
}

void UAnimNotifyState_MeleeHit::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AGameCharacter* Attacker = IsValid(MeshComp) ? MeshComp->GetOwner<AGameCharacter>() : nullptr;

	if (IsValid(Attacker))
		Attacker->TickMeleeHit(mStartSocketName, mSocketName, mPointCount, mRadius, mDamageScale, mHeavy, mFreezeTime);
}

void UAnimNotifyState_MeleeHit::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AGameCharacter* Attacker = IsValid(MeshComp) ? MeshComp->GetOwner<AGameCharacter>() : nullptr;

	if (IsValid(Attacker))
		Attacker->EndMeleeHit();
}

FString UAnimNotifyState_MeleeHit::GetNotifyName_Implementation() const
{
	// 큰 공격이면 끝에 " Heavy"를 붙여 타임라인에서 바로 구분되게 한다. (삼항 연산자 : 조건 ? 참 : 거짓)
	return FString::Printf(TEXT("Melee %s x%.1f%s%s"), *mSocketName.ToString(), mDamageScale,
		mHeavy ? TEXT(" Heavy") : TEXT(""), mFreezeTime > 0.f ? TEXT(" Freeze") : TEXT(""));
}
