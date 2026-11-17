// Fill out your copyright notice in the Description page of Project Settings.


#include "BurdenProjectile.h"
#include "../GAS/Effect/GameplayEffect_Burn.h"

ABurdenProjectile::ABurdenProjectile()
{
	// 빠르고 작은 저주탄. 맞히기 쉬운 대신 피해는 천천히 들어간다.
	mMovement->InitialSpeed = 3500.f;
	mMovement->MaxSpeed = 3500.f;
	mMovement->ProjectileGravityScale = 0.f;

	mBody->SetSphereRadius(20.f);
	mBaseScale = 1.f;

	// 사거리 15m.
	mMaxRange = 1500.f;

	// ---------------------------------------------------------------- 이펙트 (기디언 팩의 Burden 폴더)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ProjectileFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Burden/FX/P_Gideon_Burden_Projectile.P_Gideon_Burden_Projectile'"));

	if (ProjectileFX.Succeeded())
		mParticle->SetTemplate(ProjectileFX.Object);

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		HitCharacterFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Burden/FX/P_Gideon_Burden_HitCharacter.P_Gideon_Burden_HitCharacter'"));

	if (HitCharacterFX.Succeeded())
		mHitCharacterParticle = HitCharacterFX.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		HitWorldFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Burden/FX/P_Gideon_Burden_HitWorld.P_Gideon_Burden_HitWorld'"));

	if (HitWorldFX.Succeeded())
		mHitWorldParticle = HitWorldFX.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		NoHitFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Burden/FX/P_Gideon_Burden_NoHit.P_Gideon_Burden_NoHit'"));

	if (NoHitFX.Succeeded())
		mNoHitParticle = NoHitFX.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		DoTFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Burden/FX/P_Gideon_Burden_DoT.P_Gideon_Burden_DoT'"));

	if (DoTFX.Succeeded())
		mDoTParticle = DoTFX.Object;
}

bool ABurdenProjectile::ApplyBurn(AActor* Target, const FHitResult& ImpactResult)
{
	if (!IsValid(Target) || mDamage <= 0.f)
		return false;

	// 부모의 ApplyDamageToTarget과 같은 GE 적용 3단계. 다른 점은 GE 종류(GE_Burn)뿐이다.
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());

	if (!TargetASC || !SourceASC)
		return false;

	FGameplayEffectContextHandle	Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddHitResult(ImpactResult);

	FGameplayEffectSpecHandle	Spec = SourceASC->MakeOutgoingSpec(
		UGameplayEffect_Burn::StaticClass(), 1.f, Context);

	if (!Spec.IsValid())
		return false;

	// "한 번 실행할 때" 깎는 양. GAS가 0.5초마다 이 값으로 Modifier를 실행한다.
	Spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage")), -mDamage);

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	UE_LOG(UGDebug, Warning, TEXT("Burden : %s, %.1f per tick"), *Target->GetName(), mDamage);

	return true;
}

void ABurdenProjectile::StopCallback(const FHitResult& ImpactResult)
{
	// 정면 방패에 막히면 저주도 걸리지 않는다. (10/28)
	if (TryBlockByFrontGuard(ImpactResult))
		return;

	AActor* HitActor = ImpactResult.GetActor();

	// 벽·바닥이면 부모 처리 그대로 (월드 명중 이펙트 + 삭제).
	// mDamage를 잠깐 0으로 두는 이유 : 부모는 맞은 대상에게 GE_Damage(즉시 피해)를 주는데,
	// 버든은 즉시 피해 없이 도트만 주기 때문이다. (벽에 맞았을 때만 부모를 쓰므로 실제로는 안전장치)
	if (!ApplyBurn(HitActor, ImpactResult))
	{
		mDamage = 0.f;
		Super::StopCallback(ImpactResult);
		return;
	}

	// ---------------------------------------------------------------- 적에게 맞음
	if (IsValid(mHitCharacterParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mHitCharacterParticle,
			ImpactResult.ImpactPoint, ImpactResult.ImpactNormal.Rotation());
	}

	// 대상의 몸(루트 컴포넌트)에 도트 이펙트를 붙인다. 대상이 움직이면 같이 따라간다.
	if (IsValid(mDoTParticle))
	{
		mDoTComponent = UGameplayStatics::SpawnEmitterAttached(mDoTParticle,
			HitActor->GetRootComponent());
	}

	// 투사체 자신은 숨기고(날아가는 이펙트 끄기·충돌 끄기) 도트 시간만큼 남는다.
	mAttached = true;
	mParticle->DeactivateSystem();
	mBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 도트 시간 = GE_Burn의 지속 시간. CDO(기본값 객체)에서 읽어서 두 값이 항상 같게 한다.
	// GetDefault<T>() : T 클래스의 CDO. 읽기 전용(const) 포인터라 값을 바꿀 수는 없다.
	float	BurnDuration = 0.f;
	GetDefault<UGameplayEffect_Burn>()->DurationMagnitude.GetStaticMagnitudeIfPossible(1.f, BurnDuration);

	SetLifeSpan(FMath::Max(BurnDuration, 0.1f));
}

void ABurdenProjectile::LifeSpanExpired()
{
	// 못 맞히고 사거리 끝 : 부모 처리 그대로 (빗나감 이펙트 + 삭제)
	if (!mAttached)
	{
		Super::LifeSpanExpired();
		return;
	}

	// 도트 끝 : 대상에 붙여 둔 이펙트를 끄고 삭제한다.
	// DeactivateSystem : 새 입자만 멈추고 이미 나온 입자는 자연스럽게 사라지게 한다.
	if (IsValid(mDoTComponent))
		mDoTComponent->DeactivateSystem();

	Destroy();
}
