// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalMeteorProjectile.h"
#include "../Character/GameCharacter.h"
#include "../GameMode/ArenaGameMode.h"
#include "Sound/SoundBase.h"

APortalMeteorProjectile::APortalMeteorProjectile()
{
	// 평타(30m/s)보다 느리고 큰 운석. 느린 만큼 "보스 후딜에 맞춰 쏘는" 타이밍이 중요해진다.
	mMovement->InitialSpeed = 2000.f;
	mMovement->MaxSpeed = 2000.f;
	mMovement->ProjectileGravityScale = 0.f;

	mBody->SetSphereRadius(25.f);
	mBaseScale = 1.f;

	// 사거리 15m. 평타(10m)보다 길다.
	mMaxRange = 1500.f;

	// ---------------------------------------------------------------- 이펙트 (기디언 팩의 ProjectileMeteor 폴더)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ProjectileFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/ProjectileMeteor/FX/P_Gideon_RMB_Proj.P_Gideon_RMB_Proj'"));

	if (ProjectileFX.Succeeded())
		mParticle->SetTemplate(ProjectileFX.Object);

	// 폭발 이펙트. 캐릭터에 맞든 땅에 맞든 같은 폭발을 쓴다.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		HitWorldFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/ProjectileMeteor/FX/P_Gideon_RMB_HitWorld.P_Gideon_RMB_HitWorld'"));

	if (HitWorldFX.Succeeded())
	{
		mHitWorldParticle = HitWorldFX.Object;
		mHitCharacterParticle = HitWorldFX.Object;
	}

	// 사거리 끝에서 터지는 폭발.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		NoHitFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/ProjectileMeteor/FX/P_Gideon_RMB_NoHitExplosion.P_Gideon_RMB_NoHitExplosion'"));

	if (NoHitFX.Succeeded())
		mNoHitParticle = NoHitFX.Object;

	// ---------------------------------------------------------------- 폭발 소리 (10/29 : AI 효과음으로 교체)
	// 묵직한 돌 착탄 + 마법 폭발을 겹친 소리 (터지는 순간을 파일 맨 앞에 맞춤). 캐릭터든 땅이든 같은 폭발.
	static ConstructorHelpers::FObjectFinder<USoundBase>
		ExplodeSound(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Meteor_Impact.S_SFX_Meteor_Impact'"));

	if (ExplodeSound.Succeeded())
	{
		mHitWorldSound = ExplodeSound.Object;
		mHitCharacterSound = ExplodeSound.Object;
	}
}

void APortalMeteorProjectile::Explode(const FVector& Location)
{
	// 쏜 캐릭터(기디언)의 범위 공격 함수를 빌려 쓴다. → "기디언과 다른 편"(보스·미니언)만 맞는다.
	// GetInstigator() : 스폰할 때 넣은 Pawn(기디언). Cast로 AGameCharacter인지 확인한다.
	AGameCharacter* Shooter = Cast<AGameCharacter>(GetInstigator());

	// 반환값 : 범위 안의 적을 하나라도 맞혔는지 (10/21 : 맞혔을 때만 히트스톱)
	bool	bHit = false;

	if (IsValid(Shooter))
		bHit = Shooter->ApplyAreaDamage(Location, mExplosionRadius, mDamage);

	// 타격감 (10/21) : 폭발 위치 기준으로 흔들림(멀수록 약하게), 맞혔으면 짧은 히트스톱.
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();

	if (IsValid(GameMode))
		GameMode->PlayHitFeedback(EHitFeedback::MeteorExplode, Location, bHit);
}

void APortalMeteorProjectile::StopCallback(const FHitResult& ImpactResult)
{
	// 부모의 StopCallback(대상 하나에게 피해)은 부르지 않는다. 운석은 폭발 범위 안의 모두에게 준다.
	// 보스 몸에 직접 맞아도 보스는 폭발 원 안에 있으니 똑같이 맞는다.
	Explode(ImpactResult.ImpactPoint);

	if (IsValid(mHitWorldParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mHitWorldParticle,
			ImpactResult.ImpactPoint, ImpactResult.ImpactNormal.Rotation(), GetActorScale3D());
	}

	// 폭발 소리 (10/26) : 부모의 StopCallback을 안 부르므로 여기서 직접 낸다.
	PlayHitSound(mHitWorldSound, ImpactResult.ImpactPoint);

	Destroy();
}

void APortalMeteorProjectile::LifeSpanExpired()
{
	// 사거리 끝 : 그 자리에서 폭발 피해를 주고, 부모가 사라지는 이펙트(mNoHitParticle = 폭발)를 터뜨리고 삭제한다.
	Explode(GetActorLocation());

	Super::LifeSpanExpired();
}
