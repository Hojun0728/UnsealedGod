// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBase.h"
#include "../GAS/Effect/GameplayEffect_Damage.h"
#include "../GameMode/ArenaGameMode.h"
#include "../Boss/BossBase.h"
#include "Sound/SoundBase.h"

// Sets default values
AProjectileBase::AProjectileBase()
{
	// 매 프레임 처리할 일은 ProjectileMovement가 알아서 하므로 액터 Tick은 끈다.
	PrimaryActorTick.bCanEverTick = false;

	mBody = CreateDefaultSubobject<USphereComponent>(TEXT("Body"));
	mMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	mParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Particle"));

	SetRootComponent(mBody);
	mParticle->SetupAttachment(mBody);

	// 이동 컴포넌트가 움직일 대상 = 루트(구체). (수업 코드와 동일)
	mMovement->SetUpdatedComponent(mBody);

	// 충돌 설정. "PlayerProjectile" 프로필은 Config/DefaultEngine.ini에 정의되어 있다.
	//   - 벽·바닥(WorldStatic), 캐릭터(Pawn)에는 막힌다(Block) → 멈춤 → 명중
	//   - 조준용 트레이스(Visibility), 카메라, 다른 플레이어 투사체는 무시한다
	mBody->SetCollisionProfileName(TEXT("PlayerProjectile"));
	mBody->InitSphereRadius(15.f);

	// 기본 이동값. 자식 클래스가 생성자에서 바꾼다.
	mMovement->InitialSpeed = 2000.f;
	mMovement->MaxSpeed = 2000.f;
	mMovement->ProjectileGravityScale = 0.f;

	// 이동 방향으로 투사체(와 이펙트)가 회전하게 한다.
	mMovement->bRotationFollowsVelocity = true;

	// 무언가에 막혀 멈추면 ProjectileStop을 호출하도록 등록한다. (수업 코드와 동일)
	// AddDynamic(객체, &클래스::함수) : 다이나믹 델리게이트에 함수를 등록한다.
	mMovement->OnProjectileStop.AddDynamic(this, &AProjectileBase::ProjectileStop);
}

// Called when the game starts or when spawned
void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// 발사한 캐릭터(Instigator)와는 부딪히지 않게 한다.
	// 손(소켓)에서 생성되기 때문에 이걸 안 하면 생성되자마자 자기 몸에 맞아 터진다.
	// 플레이어와 보스는 둘 다 Pawn이라 충돌 채널로는 구분할 수 없어서, "이 액터만 무시"로 처리한다.
	// GetInstigator() : 스폰할 때 FActorSpawnParameters.Instigator로 넣어준 Pawn을 돌려준다.
	if (IsValid(GetInstigator()))
		mBody->IgnoreActorWhenMoving(GetInstigator(), true);

	// 수명을 정한다. 시간이 다 되면 LifeSpanExpired()가 호출된다.
	// 최대 사거리가 있으면 수명 = 사거리 ÷ 속도 (예 : 1000cm ÷ 3000cm/초 ≈ 0.33초)
	float	LifeTime = mLifeTime;

	if (mMaxRange > 0.f && mMovement->InitialSpeed > 0.f)
		LifeTime = mMaxRange / mMovement->InitialSpeed;

	SetLifeSpan(LifeTime);
}

void AProjectileBase::SetProjectileScale(float Multiplier)
{
	// 액터 크기를 바꾸면 루트(충돌 구체)와 그 아래 붙은 이펙트가 모두 같은 비율로 커진다.
	// 반지름 15cm 구체 × 1.2 = 18cm, 막타 × 1.8 = 27cm
	// FVector(값) : X·Y·Z를 모두 같은 값으로 채운 벡터 (균등 스케일)
	SetActorScale3D(FVector(mBaseScale * Multiplier));
}

void AProjectileBase::LifeSpanExpired()
{
	// 못 맞히고 사라지는 이펙트. (에셋이 지정된 경우에만)
	// 마지막 인자 : 이펙트 크기. 투사체가 크면 사라지는 이펙트도 같이 크게 한다.
	if (IsValid(mNoHitParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mNoHitParticle,
			GetActorLocation(), GetActorRotation(), GetActorScale3D());
	}

	// 부모(AActor)의 원래 동작 = Destroy()
	Super::LifeSpanExpired();
}

void AProjectileBase::ProjectileStop(const FHitResult& ImpactResult)
{
	// 델리게이트로 받은 것을 virtual 함수로 넘긴다. (수업 코드와 동일한 구조)
	// 이렇게 한 단계 거치는 이유 : UFUNCTION 델리게이트 함수는 그대로 두고,
	// 자식 클래스는 StopCallback만 override하면 되도록 하기 위함이다.
	StopCallback(ImpactResult);
}

void AProjectileBase::StopCallback(const FHitResult& ImpactResult)
{
	// 정면 방패에 막혔으면 여기서 끝 (막는 이펙트·소리는 보스가 냈다)
	if (TryBlockByFrontGuard(ImpactResult))
		return;

	// 맞은 액터. (벽, 바닥, 보스 등)
	AActor* HitActor = ImpactResult.GetActor();

	bool	Damaged = ApplyDamageToTarget(HitActor, ImpactResult);

	// 강타가 캐릭터에 맞았으면 타격감 요청 (10/21). 벽·바닥에 맞은 건 제외.
	if (Damaged && mStrongHit)
	{
		AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();

		if (IsValid(GameMode))
			GameMode->PlayHitFeedback(EHitFeedback::StrongShotHit);
	}

	// 캐릭터에 맞았으면 캐릭터용, 아니면 월드용 이펙트를 고른다.
	// 삼항 연산자 (조건 ? A : B) : 조건이 참이면 A, 거짓이면 B.
	UParticleSystem* HitParticle = Damaged ? mHitCharacterParticle : mHitWorldParticle;

	if (IsValid(HitParticle))
	{
		// ImpactPoint : 실제로 맞은 지점, ImpactNormal : 맞은 면이 바라보는 방향
		// 면 방향으로 이펙트를 회전시키면 벽에서 튀어나오듯 터진다.
		// 마지막 인자 : 투사체 크기에 맞춰 명중 이펙트도 키운다. (막타는 더 크게 터진다)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticle,
			ImpactResult.ImpactPoint, ImpactResult.ImpactNormal.Rotation(),
			GetActorScale3D());
	}

	// 명중 소리 (10/26) : 이펙트와 같은 규칙으로 캐릭터용 / 월드용을 고른다.
	PlayHitSound(Damaged ? mHitCharacterSound : mHitWorldSound, ImpactResult.ImpactPoint);

	Destroy();
}

bool AProjectileBase::TryBlockByFrontGuard(const FHitResult& ImpactResult)
{
	// Cast : 맞은 액터가 보스일 때만 주소가 나오고, 벽·미니언·기디언이면 nullptr
	ABossBase* Boss = Cast<ABossBase>(ImpactResult.GetActor());

	if (!IsValid(Boss) || !Boss->TryBlockFrontHit(ImpactResult.ImpactPoint))
		return false;

	Destroy();
	return true;
}

void AProjectileBase::PlayHitSound(USoundBase* Sound, const FVector& Location)
{
	// PlaySoundAtLocation : 월드의 한 지점에서 소리를 한 번 낸다. (끝나면 알아서 사라진다)
	// 투사체는 바로 Destroy되므로 투사체에 붙이지 않고 "자리"에서 낸다.
	if (IsValid(Sound))
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
}

bool AProjectileBase::ApplyDamageToTarget(AActor* Target, const FHitResult& ImpactResult)
{
	if (!IsValid(Target) || mDamage <= 0.f)
		return false;

	// 대상의 ASC를 얻어온다. IAbilitySystemInterface를 구현한 액터(플레이어·보스·미니언)만 있다.
	// 벽이나 바닥은 ASC가 없어서 nullptr이 나온다.
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	// 데미지를 "누가 줬는지" 기록하기 위해 발사한 캐릭터의 ASC도 얻어온다.
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetInstigator());

	if (!TargetASC || !SourceASC)
		return false;

	// GE 적용 3단계 (PlayerCharacter::ApplyEffectToSelf와 같은 흐름, 대상만 다르다)
	// 1. Context : 누가(SourceASC), 무엇으로(이 투사체), 어디에(HitResult) 맞혔는지
	FGameplayEffectContextHandle	Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddHitResult(ImpactResult);

	// 2. Spec : GE_Damage + 피해량(SetByCaller, 음수로 넣어야 HP가 깎인다)
	FGameplayEffectSpecHandle	Spec = SourceASC->MakeOutgoingSpec(
		UGameplayEffect_Damage::StaticClass(), 1.f, Context);

	if (!Spec.IsValid())
		return false;

	Spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage")), -mDamage);

	// 3. 대상에게 적용한다. (자기 자신이 아니라 Target에게)
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	UE_LOG(UGDebug, Warning, TEXT("Projectile Hit : %s, Damage : %.1f"),
		*Target->GetName(), mDamage);

	return true;
}
