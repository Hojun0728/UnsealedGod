// Fill out your copyright notice in the Description page of Project Settings.


#include "BossAnimInstance.h"
#include "../Character/GameCharacter.h"
#include "BossBase.h"
#include "KismetAnimationLibrary.h"

void UBossAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 보스 클래스(ABossBase)가 아니라 공통 부모(AGameCharacter)로 형변환한다.
	// 이 애님 인스턴스에 필요한 건 속도·사망 여부뿐이라, 더 일반적인 타입으로 받으면
	// 나중에 미니언 애님 블루프린트에서도 그대로 재사용할 수 있다.
	AGameCharacter* Owner = Cast<AGameCharacter>(TryGetPawnOwner());

	if (!IsValid(Owner))
		return;

	UCharacterMovementComponent* Movement = Owner->GetCharacterMovement();

	FVector	Velocity = Movement->Velocity;

	// 바닥 평면(XY)의 속도만 쓴다. (점프·낙하 속도 제외)
	mMoveSpeed = Velocity.Size2D();

	mMoveDirection = UKismetAnimationLibrary::CalculateDirection(Velocity,
		Owner->GetActorRotation());

	mAir = Movement->IsFalling();

	mDeath = Owner->IsDeath();

	// 보스일 때만 페이즈 → 어깨 장식 단계. (Cast 실패 = 보스가 아님 → 0)
	ABossBase* Boss = Cast<ABossBase>(Owner);

	mStageIndex = IsValid(Boss) ? FMath::Clamp(Boss->GetPhase() - 1, 0, 3) : 0;
}
