// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Hoarfrost.h"
#include "../../Boss/BossBase.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_Hoarfrost::UGameplayAbility_Pattern_Hoarfrost()
{
	// 1페이즈부터 (10/28 내 판단 : 처음에는 2페이즈부터였지만 1페이즈부터 쓰기로)

	// 사용 거리 0~5m (원 반지름과 같다 = 원 안에 있을 때만 쓴다)
	mMinRange = 0.f;
	mMaxRange = 500.f;

	mWeight = 1.f;

	mCoolDownTime = 8.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossHoarfrost"));

	// 0.75배속 : 베는 순간(11프레임 = 0.37초)까지의 예고 약 0.49초 (10/28 밸런스 : 0.6배속 0.61초는 너무 여유로웠음)
	// 2페이즈 0.9배속 = 약 0.41초
	mMontagePlayRate = 0.75f;
	mFastPlayRate = 0.9f;

	// 제자리에서 쓴다 (밀기 없음)
	mLungeSpeed = 0.f;

	// 안 걸렸을 때 반격 틈
	mRecoveryTime = 1.f;
	mRecoveryRandom = 0.3f;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_Hoarfrost.AM_Aurora_Hoarfrost'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	// ---------------------------------------------------------------- 이펙트·소리
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		GroundFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Frozen_Ground_Water_Freeze.P_Aurora_Frozen_Ground_Water_Freeze'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		SwirlFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Whrilwind.P_Aurora_Freeze_Whrilwind'"));

	if (GroundFX.Succeeded())	mGroundParticle = GroundFX.Object;
	if (SwirlFX.Succeeded())	mSwirlParticle = SwirlFX.Object;

	// 베기 + 바닥에 얼음 고리가 퍼지는 소리 (10/29 : 마법 효과음 팩의 얼어붙는 바람 → AI 효과음)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		CastSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Hoarfrost_Slash_1.S_SFX_Hoarfrost_Slash_1'"));

	if (CastSnd.Succeeded())
		mCastSound = CastSnd.Object;
}

bool UGameplayAbility_Pattern_Hoarfrost::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	const ABossBase* BossBase = Cast<ABossBase>(Boss);

	if (!IsValid(BossBase) || !IsValid(Target))
		return false;

	float	Radius = BossBase->GetPhase() >= 2 ? mPhase2RingRadius : mRingRadius;

	// 수평 거리가 원 반지름 안일 때만 (원 밖의 기디언에게 쓰면 의미가 없다)
	FVector	ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
	ToTarget.Z = 0.0;

	return ToTarget.Size() <= Radius;
}

void UGameplayAbility_Pattern_Hoarfrost::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{

	// 이번 원 반지름 : 2페이즈(얼음 여제)면 7m
	if (const ABossBase* PhaseBoss = Cast<ABossBase>(ActorInfo->AvatarActor.Get()))
		mCurrentRadius = PhaseBoss->GetPhase() >= 2 ? mPhase2RingRadius : mRingRadius;

	// 안 걸렸을 때의 반격 틈 (지난번에 걸려서 0으로 바꿨을 수 있으니 매번 되돌린다)
	mRecoveryTime = 1.f;
	mRecoveryRandom = 0.3f;

	// 부모 : 쿨다운 확정 → 멈춤 → 기디언 쪽 바라보기 → 몽타주 재생 (0.6배속)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive())
		return;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	// ---------------------------------------------------------------- 원 예고 (보스 발밑, 베는 순간까지 차오름)
	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Ground = Boss->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	// 부모 함수 (11/4) : 미리 불러 둔 머티리얼로 원 예고. 패턴이 끝나면 부모 EndAbility가 지운다
	SpawnWarningCircle(Ground, mCurrentRadius, ToRealTime(mHitTime));

	// ---------------------------------------------------------------- 베는 순간 노티파이 대기
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.Hoarfrost")), nullptr, true);

	WaitHit->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Hoarfrost::OnHoarfrostEvent);
	WaitHit->ReadyForActivation();
}

void UGameplayAbility_Pattern_Hoarfrost::OnHoarfrostEvent(FGameplayEventData Payload)
{
	DestroyWarningDecal();

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	// ---------------------------------------------------------------- 1. 베고 난 뒤는 원래 속도로
	// Montage_SetPlayRate : 재생 중인 몽타주의 속도를 바꾼다. (예고만 느리게, 나머지는 굼뜨지 않게)
	if (UAnimInstance* AnimInstance = Boss->GetMesh()->GetAnimInstance())
		AnimInstance->Montage_SetPlayRate(mMontage, mAfterHitPlayRate);

	// ---------------------------------------------------------------- 2. 이펙트·소리 (보스 발밑)
	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Ground = Boss->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	for (UParticleSystem* Particle : { mGroundParticle.Get(), mSwirlParticle.Get() })
	{
		if (IsValid(Particle))
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, Ground, Boss->GetActorRotation(),
				FVector(mParticleScale));
	}

	if (IsValid(mCastSound))
		UGameplayStatics::PlaySoundAtLocation(this, mCastSound, Ground);

	// ---------------------------------------------------------------- 3. 판정
	ACharacter* Target = Cast<ACharacter>(Boss->GetTarget());

	if (!IsValid(Target))
		return;

	// 원 안인지 (수평 거리만). 캡슐 반지름만큼 너그럽게 = 몸이 원에 걸치기만 해도 맞는다
	FVector	ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
	ToTarget.Z = 0.0;

	if (ToTarget.Size() > mCurrentRadius + Target->GetCapsuleComponent()->GetScaledCapsuleRadius())
		return;

	// 점프 중(공중)이면 안 걸린다. IsFalling : 캐릭터 이동이 "떨어지는 중(공중)" 상태인지
	if (Target->GetCharacterMovement()->IsFalling())
		return;

	// 회피 무적이면 피해도 얼음도 없다. (피해는 AttributeSet이 막지만, 얼음 이벤트는 여기서 막아야 한다)
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (!TargetASC ||
		TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
		return;

	FHitResult	Hit;
	Hit.ImpactPoint = Target->GetActorLocation();
	Hit.Location = Hit.ImpactPoint;

	Boss->ApplyDamageToTarget(Target, Boss->GetAttack() * mDamageScale, Hit, false, false);

	// 얼어붙음 : 기디언의 UGameplayAbility_Frozen이 이 이벤트로 발동한다. EventMagnitude = 최대 얼음 시간
	FGameplayEventData	FreezeEvent;
	FreezeEvent.Instigator = Boss;
	FreezeEvent.EventMagnitude = mFreezeTime;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target,
		FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Frozen")), FreezeEvent);

	// 걸렸으면 후딜 없이 바로 다음 패턴 → 얼어 있는 동안 공격이 들어온다 (10/28)
	mRecoveryTime = 0.f;
	mRecoveryRandom = 0.f;
}
