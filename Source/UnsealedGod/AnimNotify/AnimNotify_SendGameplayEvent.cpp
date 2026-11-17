// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_SendGameplayEvent.h"

void UAnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp) || !mEventTag.IsValid())
		return;

	// 이 애니메이션을 재생 중인 액터. (미리보기 창에서는 ASC가 없는 미리보기 액터라 아무 일도 안 일어난다)
	AActor* Owner = MeshComp->GetOwner();

	if (!IsValid(Owner))
		return;

	// 이벤트에 담을 정보 : 누가 보냈는지 + 숫자 하나(받는 쪽이 뜻을 정함)
	FGameplayEventData	EventData;
	EventData.EventTag = mEventTag;
	EventData.Instigator = Owner;
	EventData.EventMagnitude = mEventMagnitude;

	// 액터의 ASC로 이벤트를 보낸다. 이 태그를 기다리는 어빌리티(WaitGameplayEvent)가 받는다. (수업 코드와 같은 함수)
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, mEventTag, EventData);
}

FString UAnimNotify_SendGameplayEvent::GetNotifyName_Implementation() const
{
	// 숫자가 있으면 이름 뒤에 붙여서 타임라인에서 바로 보이게 한다. 예 : "Event Event.Boss.Lunge (-30)"
	if (!FMath::IsNearlyZero(mEventMagnitude))
		return FString::Printf(TEXT("Event %s (%g)"), *mEventTag.ToString(), mEventMagnitude);

	return FString::Printf(TEXT("Event %s"), *mEventTag.ToString());
}
