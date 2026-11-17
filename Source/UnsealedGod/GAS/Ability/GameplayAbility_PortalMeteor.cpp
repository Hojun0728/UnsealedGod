// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_PortalMeteor.h"
#include "../../Player/PlayerCharacter.h"
#include "../../Projectile/PortalMeteorProjectile.h"

UGameplayAbility_PortalMeteor::UGameplayAbility_PortalMeteor()
{
	// ---------------------------------------------------------------- 비용·쿨다운 (부모의 부모가 처리)
	mMPCost = 25.f;
	mCoolDownTime = 6.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.PortalMeteor"));

	// ---------------------------------------------------------------- 몽타주
	// CosmicRift_Start(0.97초) + CosmicRift_Execute(0.3초) = 약 1.3초
	// (11/4) 생성자에서 미리 불러온다. (처음 쓸 때 경로로 불러오면 싸움 중에 디스크를 읽느라 멈칫한다)
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_PortalMeteor.AM_Gideon_PortalMeteor'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	mPlayRate = 1.f;

	// 시전 중 이동 허용 (10/16 테스트 후 결정). 평타·점프는 여전히 막힌다(State.Casting).
	SetAllowMoveWhileCasting(true);

	// (11/4) E(회피)로 끊을 수 있는 스킬. (부모 함수 설명 참고)
	SetDodgeCancelable();

	// RMB 세트의 "운석이 나오는 포털". (R 운석 비는 Meteor 세트를 써서 둘이 구분된다)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		PortalFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/ProjectileMeteor/FX/P_Gideon_RMB_Portal.P_Gideon_RMB_Portal'"));

	if (PortalFX.Succeeded())
		mPortalParticle = PortalFX.Object;
}

void UGameplayAbility_PortalMeteor::OnFire(APlayerCharacter* Player)
{
	// 조준한 바닥 (최대 15m) → 그 위 8m 하늘에서 수직으로 떨어뜨린다.
	FVector		Ground = Player->GetAimGroundLocation(mMaxDistance);
	FVector		SkyLocation = Ground + FVector(0.0, 0.0, mDropHeight);

	// FRotator(Pitch, Yaw, Roll) : Pitch -90 = 아래를 본다. 투사체는 앞(X축)으로 날아가므로 아래로 떨어진다.
	FRotator	DownRotation(-90.0, 0.0, 0.0);

	// 운석은 땅(또는 그 위의 적)에 닿는 순간 폭발한다. (APortalMeteorProjectile::StopCallback)
	Player->SpawnProjectileAt(APortalMeteorProjectile::StaticClass(), SkyLocation, DownRotation,
		Player->GetAttack() * mDamageScale);

	// 하늘에 포털 이펙트. 방향이 이상하면 회전값을 조정한다. (테스트 후)
	if (IsValid(mPortalParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(Player->GetWorld(), mPortalParticle,
			SkyLocation, DownRotation);
	}
}
