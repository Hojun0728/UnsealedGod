// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Animation/AnimInstance.h"
#include "BossAnimInstance.generated.h"

// ============================================================================
// UBossAnimInstance
// 보스 애님 블루프린트(ABP_Sevarog)의 C++ 부모 클래스다. (UPlayerAnimInstance와 같은 구조)
// C++에서 "애니메이션을 고르는 데 필요한 값"을 계산하고, 블루프린트에서 애니메이션을 고른다.
// 공격 동작은 패턴 어빌리티가 몽타주로 재생하므로(DefaultSlot), 여기에는 이동·사망 값만 있다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	// 이동 속도 (cm/초). 블렌드스페이스에 연결한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float	mMoveSpeed = 0.f;

	// 이동 방향 (-180 ~ 180도). 보스는 이동 방향으로 몸을 돌리므로 대부분 0 근처다.
	// 블렌드스페이스가 방향 축을 쓰면 연결한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float	mMoveDirection = 0.f;

	// 공중에 떠 있는지. (도약 패턴은 몽타주로 처리하지만, 떨어지는 상태 확인용으로 둔다)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool	mAir = false;

	// 사망했는지. true면 ABP가 Death_front로 바꾼다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool	mDeath = false;

	// 어깨 장식 단계 (0 ~ 3) = 보스 페이즈 - 1. ABP의 "int로 포즈 블렌딩"이 Stage_1~4 중 하나를 고른다.
	// 미니언처럼 보스가 아닌 캐릭터는 항상 0.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32	mStageIndex = 0;

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
