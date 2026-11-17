// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifyState_HideMesh.h"

void UAnimNotifyState_HideMesh::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	// SetVisibility(보일지, 자식 컴포넌트에도 적용할지)
	// 두 번째 인자 true : 메시에 붙어 있는 것들(손에 붙은 발사 이펙트 등)도 같이 숨긴다.
	// 미리보기 창에서도 동작해서, 몽타주 에디터에서 막대 구간 동안 기디언이 사라져 보이는 게 정상이다.
	if (IsValid(MeshComp))
		MeshComp->SetVisibility(false, true);
}

void UAnimNotifyState_HideMesh::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (IsValid(MeshComp))
		MeshComp->SetVisibility(true, true);
}
