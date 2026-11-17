// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Burden.h"
#include "../../Player/PlayerCharacter.h"
#include "../../Projectile/BurdenProjectile.h"

UGameplayAbility_Burden::UGameplayAbility_Burden()
{
	// ---------------------------------------------------------------- 비용·쿨다운
	mMPCost = 20.f;
	mCoolDownTime = 10.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Burden"));

	// ---------------------------------------------------------------- 몽타주
	// RMB_Intro(0.43초) + RMB_Cast(1.63초) = 2.06초 → 1.3배속 약 1.6초
	// (11/4) 생성자에서 미리 불러온다. (처음 쓸 때 경로로 불러오면 싸움 중에 디스크를 읽느라 멈칫한다)
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_Burden.AM_Gideon_Burden'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	mPlayRate = 1.3f;

	// 시전 중 이동 허용 (10/16 테스트 후 결정). 평타·점프는 여전히 막힌다(State.Casting).
	SetAllowMoveWhileCasting(true);

	// (11/4) E(회피)로 끊을 수 있는 스킬. (부모 함수 설명 참고)
	SetDodgeCancelable();

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		MuzzleFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Burden/FX/P_Gideon_Burden_MuzzleFlash.P_Gideon_Burden_MuzzleFlash'"));

	if (MuzzleFX.Succeeded())
		mMuzzleParticle = MuzzleFX.Object;
}

void UGameplayAbility_Burden::OnFire(APlayerCharacter* Player)
{
	// 투사체의 mDamage = "한 번" 깎는 양. 투사체가 맞으면 이 값으로 GE_Burn을 건다.
	Player->SpawnProjectile(ABurdenProjectile::StaticClass(), mSocketName,
		Player->GetAttack() * mTickDamageScale);

	if (IsValid(mMuzzleParticle))
		UGameplayStatics::SpawnEmitterAttached(mMuzzleParticle, Player->GetMesh(), mSocketName);
}
