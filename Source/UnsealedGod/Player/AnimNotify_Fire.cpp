// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotify_Fire.h"
#include "PlayerCharacter.h"

void UAnimNotify_Fire::Notify(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp))
		return;

	// SkeletalMeshComponent를 가지고 있는 Actor를 얻어와 플레이어 캐릭터로 형변환한다.
	// 몽타주 에디터의 미리보기 창에서는 주인이 미리보기용 액터라서 nullptr이 나온다.
	// → 미리보기에서는 투사체가 안 나가는 게 정상이다.
	TObjectPtr<APlayerCharacter>	PlayerChar =
		MeshComp->GetOwner<APlayerCharacter>();

	if (IsValid(PlayerChar))
		PlayerChar->FireProjectile(mSocketName, mDamageScale, mProjectileScale, mStrongHit);
}

FString UAnimNotify_Fire::GetNotifyName_Implementation() const
{
	// FString::Printf : printf처럼 형식에 맞춰 문자열을 만든다.
	// *mSocketName.ToString() : FName → FString 변환 후, %s에 넣기 위해 * 로 문자 배열을 꺼낸다.
	FString	Name = FString::Printf(TEXT("Fire %s x%.1f"), *mSocketName.ToString(), mDamageScale);

	// 크기 배율이 1이 아닐 때만 표시한다. (막타 노티파이가 타임라인에서 바로 구분되게)
	// FMath::IsNearlyEqual : float는 오차가 있어서 == 대신 "거의 같은지"로 비교한다.
	if (!FMath::IsNearlyEqual(mProjectileScale, 1.f))
		Name += FString::Printf(TEXT(" Size%.1f"), mProjectileScale);

	// 강타면 표시한다. (몽타주 타임라인에서 어느 노티파이가 강타인지 바로 보이게)
	if (mStrongHit)
		Name += TEXT(" Strong");

	return Name;
}
