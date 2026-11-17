// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Character/GameCharacter.h"
#include "Minion.generated.h"

// ============================================================================
// AMinion
// 보스가 2페이즈부터 소환하는 근접 미니언 (파라곤 Dusk 레인 근접 미니언 외형).
// 플레이어를 쫓아가서 칼로 공격한다. 기디언의 Q·R 범위 스킬이 빛나는 상황을 만드는 역할.
//
// AGameCharacter를 상속하므로 GAS 체력·피해·사망, 근접 판정(Melee Hit Window)을 그대로 쓴다.
//   AI    : AEnemyController가 BT_Minion을 실행 (선택[공격 범위면 공격 / 아니면 Move To 플레이어])
//   공격  : AM_Minion_Attack (섹션 Default = Attack_A, B = Attack_B 중 랜덤)
//   사망  : Death_A~E 중 랜덤 → 끝나면 조각 이펙트 + 사라짐 (원작처럼 쓰러져 눕는 동작이 없음)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AMinion : public AGameCharacter
{
	GENERATED_BODY()

public:
	AMinion();

protected:
	// ---------------------------------------------------------------- 에셋
	// 비헤이비어 트리·공격 몽타주. (11/4) 생성자에서 미리 불러온다.
	// (10/15~11/3에는 실행 중에 경로로 불러왔다 → 2페이즈 첫 소환 순간 디스크를 읽느라 멈칫할 수 있었다)
	UPROPERTY()
	TObjectPtr<UBehaviorTree>	mBehaviorTree;

	UPROPERTY()
	TObjectPtr<UAnimMontage>	mAttackMontage;

	// 공격 몽타주의 섹션들. 공격할 때마다 이 중 하나를 랜덤으로 재생한다.
	TArray<FName>	mAttackSections = { TEXT("Default"), TEXT("B") };

	// 사망 동작 (Death_A~E). 하나를 랜덤으로 "시퀀스를 바로 몽타주로" 재생한다.
	UPROPERTY()
	TArray<TObjectPtr<UAnimSequence>>	mDeathAnims;

	// 등장·사망 이펙트 (파라곤 미니언 팩)
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mSpawnParticle;

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mDeathParticle;

	// (10/29) 먼지가 되어 흩어지는 소리 (사망 이펙트와 같은 순간)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mDeathSound;

	// ---------------------------------------------------------------- 수치
	// 공격을 시작하는 거리(cm, 미니언 중심 → 플레이어 중심)
	UPROPERTY(EditAnywhere, Category = "Minion")
	float	mAttackRange = 180.f;

	// ---------------------------------------------------------------- 상태
	bool	mAttacking = false;

	FTimerHandle	mDeathTimer;

public:
	virtual UBehaviorTree* GetBehaviorTree() const override
	{
		return mBehaviorTree;
	}

	bool IsAttacking()	const
	{
		return mAttacking;
	}

	float GetAttackRange()	const
	{
		return mAttackRange;
	}

	// 공격 시작. 타겟을 바라보고 공격 몽타주를 재생한다. 시작했으면 true. (BTTask_MinionAttack이 부른다)
	bool Attack(AActor* Target);

	// 전투 끝 (AGameCharacter의 virtual 덮어쓰기). 승리면 부서지며 사라지고, 패배면 AI를 멈추고 그 자리에 선다.
	virtual void OnBattleEnd(bool bPlayerWon) override;

protected:
	virtual void BeginPlay() override;

	virtual void Death() override;

	// 공격 몽타주가 끝났을 때 (UAnimInstance의 몽타주 종료 알림)
	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 사망 동작이 끝났을 때 → 조각 이펙트 + 사라짐
	void OnDeathAnimFinished();
};
