// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_Invincible.h"

void UAnimNotifyState_Invincible::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp))
		return;

	// 메시를 가진 액터의 ASC를 찾는다. (ASC가 없는 미리보기 액터면 nullptr)
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner());

	if (!ASC)
		return;

	// "느슨한(Loose) 태그" : GE 없이 코드에서 직접 붙였다 떼는 태그.
	// 같은 태그를 여러 번 붙이면 개수(Count)가 쌓이고, 뗄 때 하나씩 줄어든다.
	ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));
}

void UAnimNotifyState_Invincible::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
		return;

	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp->GetOwner());

	if (!ASC)
		return;

	ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));
}
