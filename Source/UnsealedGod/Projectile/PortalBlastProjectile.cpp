// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalBlastProjectile.h"

APortalBlastProjectile::APortalBlastProjectile()
{
	// 부모 생성자가 먼저 실행되어 컴포넌트는 이미 만들어져 있다. 여기서는 값만 바꾼다.

	// 속도 3000cm/s = 30m/s. 15m 앞의 보스까지 약 0.5초.
	mMovement->InitialSpeed = 3000.f;
	mMovement->MaxSpeed = 3000.f;

	// 직선으로 날아가는 마법탄이라 중력을 받지 않는다.
	mMovement->ProjectileGravityScale = 0.f;

	mBody->SetSphereRadius(15.f);

	// 기본 크기 1.2배. (실제 적용은 발사할 때 SetProjectileScale에서)
	mBaseScale = 1.2f;

	mLifeTime = 3.f;

	// 사거리 10m (10/15). 무제한(속도 × 3초 ≈ 90m)이면 뒤로 빠지며 쏘기만 해도 안전해서,
	// 보스 근처에 머물러야 하도록 줄였다. → 도약(6m 이상)·돌진(4~12m)이 실제 위협이 된다.
	mMaxRange = 1000.f;

	// ---------------------------------------------------------------- 이펙트 (기디언 팩의 Primary 폴더)
	// 날아가는 동안의 보라색 구체.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ProjectileFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Primary/FX/P_Gideon_Primary_Projectile.P_Gideon_Primary_Projectile'"));

	if (ProjectileFX.Succeeded())
		mParticle->SetTemplate(ProjectileFX.Object);

	// 캐릭터에 맞았을 때.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		HitCharacterFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Primary/FX/P_Gideon_Primary_HitCharacter.P_Gideon_Primary_HitCharacter'"));

	if (HitCharacterFX.Succeeded())
		mHitCharacterParticle = HitCharacterFX.Object;

	// 벽·바닥에 맞았을 때.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		HitWorldFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Primary/FX/P_Gideon_Primary_HitWorld.P_Gideon_Primary_HitWorld'"));

	if (HitWorldFX.Succeeded())
		mHitWorldParticle = HitWorldFX.Object;

	// 못 맞히고 사라질 때.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		NoHitFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Primary/FX/P_Gideon_Primary_NoHit.P_Gideon_Primary_NoHit'"));

	if (NoHitFX.Succeeded())
		mNoHitParticle = NoHitFX.Object;

	// 명중 소리는 넣지 않는다 (10/26 내 판단 : 평타는 "쏘는 소리"만). 쏘는 소리는 AM_Gideon_Attack 노티파이.
	// 부모의 mHitCharacterSound·mHitWorldSound가 비어 있으면 명중 때 아무 소리도 나지 않는다.
}
