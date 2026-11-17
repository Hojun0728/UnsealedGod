// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Character/GameCharacter.h"
#include "IceDecoy.generated.h"

// ============================================================================
// AIceDecoy
// 오로라의 얼음 분신 (원작 Frozen Simulacrum) = 기디언을 향해 걸어오는 얼음 폭탄. (10/28)
// 미니언처럼 AGameCharacter를 상속한다 → GAS 체력·피해·사망을 그대로 쓴다. (평타 3발 = 체력 30이면 부서진다)
//
// 흐름
//   생성 : 오로라가 공중제비로 빠지는 순간 오로라 자리에. 오로라 메시를 얼음 머티리얼로 칠한 모습, 오로라 애님 블루프린트로 걷는다.
//   → 기디언을 향해 걸어온다 (AI 컨트롤러의 MoveToActor = 내비 메시 길찾기, 기디언 속도 600보다 느린 400)
//   → 기디언 mTriggerDistance 안 또는 수명(mLifeTime) 끝 : mFuseTime초 동안 멈춰서 빛나며 금이 감(예고)
//   → 폭발 : 반지름 mExplodeRadius 안이면 피해 + 얼어붙음(연타 탈출) → 얼어 있는 동안 오로라의 공격이 그대로 들어온다
//   부서짐(체력 0) : 무해한 파편만 남기고 사라진다
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AIceDecoy : public AGameCharacter
{
	GENERATED_BODY()

public:
	AIceDecoy();

protected:
	// ---------------------------------------------------------------- 수치 (10/28 직접 결정)
	UPROPERTY(EditAnywhere, Category = "IceDecoy")
	float	mWalkSpeed = 400.f;

	UPROPERTY(EditAnywhere, Category = "IceDecoy")
	float	mTriggerDistance = 200.f;		// 이 거리 안에 들어오면 폭발 예고 시작

	UPROPERTY(EditAnywhere, Category = "IceDecoy")
	float	mFuseTime = 0.6f;				// 예고 시간 (멈춰서 빛남)

	UPROPERTY(EditAnywhere, Category = "IceDecoy")
	float	mExplodeRadius = 300.f;

	UPROPERTY(EditAnywhere, Category = "IceDecoy")
	float	mExplodeDamageScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "IceDecoy")
	float	mFreezeTime = 5.f;

	UPROPERTY(EditAnywhere, Category = "IceDecoy")
	float	mLifeTime = 8.f;				// 이만큼 지나도 못 닿으면 그 자리에서 폭발

	// ---------------------------------------------------------------- 이펙트·소리 (오로라 팩 Leap 폴더 = 원작 분신 이펙트)
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mSpawnParticle;		// 생길 때

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mDamagedParticle;	// 맞아서 금이 갈 때

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mFuseParticle;		// 폭발 예고 (몸에 붙음)

	UPROPERTY()
	TObjectPtr<UParticleSystem>	mExplodeParticle;	// 폭발·부서짐 파편

	UPROPERTY()
	TObjectPtr<class USoundBase>	mFuseSound;		// 폭발 예고 (10/29)

	UPROPERTY()
	TObjectPtr<class USoundBase>	mExplodeSound;

	UPROPERTY()
	TObjectPtr<class USoundBase>	mBreakSound;	// 기디언이 부쉈을 때 (10/29)

	// 얼음 머티리얼 (모든 슬롯에 같은 것을 칠한다 = 통째로 얼음 조각). 보스가 넘겨준다
	UPROPERTY()
	TObjectPtr<UMaterialInterface>	mIceMaterial;

	// ---------------------------------------------------------------- 상태
	bool	mFusing = false;
	float	mLifeElapsed = 0.f;
	float	mRepathElapsed = 0.f;

	FTimerHandle	mFuseTimer;

public:
	// 보스와 같은 편 (기디언의 범위 공격·투사체가 맞는다). 기본값이 "플레이어 편 아님"이라 덮어쓸 필요는 없다.

	virtual void Death() override;
	virtual void OnBattleEnd(bool bPlayerWon) override;

	// 맞을 때마다 금이 가는 이펙트 (AttributeSet이 실제로 피해가 들어갔을 때 부른다)
	virtual void OnDamaged(float Damage, AActor* Attacker, bool bHeavy) override;

	// 보스가 생성 직후 공격력을 넘겨준다. (폭발 피해 = 보스 공격력 × 배율, 페이즈 공격력 배율도 따라간다)
	void SetAttackFrom(float BossAttack);

	// 보스가 생성 직후 얼음 머티리얼을 넘겨준다. 모든 슬롯에 칠한다.
	void SetIceMaterial(UMaterialInterface* Material);

	// 걷는 속도 바꾸기 (2페이즈 분신 2개의 속도를 다르게)
	void SetWalkSpeed(float Speed)
	{
		mWalkSpeed = Speed;
		GetCharacterMovement()->MaxWalkSpeed = Speed;
	}

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 기디언을 향해 걷기 (길찾기 다시 요청)
	void ChasePlayer();

	// 폭발 예고 시작 → mFuseTime초 뒤 Explode
	void StartFuse();
	void Explode();

	// 파편을 남기고 사라진다
	void Shatter();
};
