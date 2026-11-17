// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Cryoseism.h"
#include "../../Boss/BossBase.h"
#include "../../GameMode/ArenaGameMode.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_Cryoseism::UGameplayAbility_Pattern_Cryoseism()
{
	// 필살기 : 조건이 되면 다른 패턴보다 먼저 (세바로그 필살기와 같은 우선순위)
	mPriority = 10;

	// (10/28 수정) 폭발(22프레임 = 0.73초)까지를 mWindupTime초로 늘려 천천히 떠오르며 힘을 모은다 → 공중 정점에서 팡.
	// 0.73 ÷ 2.0 ≈ 0.37배속. 폭발 순간 코드가 1.0배속으로 되돌린다. (서리 고리와 같은 방식)
	mMontagePlayRate = mBlastTime / mWindupTime;

	mLungeSpeed = 0.f;

	// 후딜 0 : 맞혔으면 얼어 있는 동안 바로 다음 패턴이 들어온다.
	// (11/4) 피했으면 끝날 때 지침 대기를 세운다 → 지침 패턴(4초, 지친 동작 + 헐떡임)이 바로 나와 회복 구간이 된다.
	// (10/28~11/3에는 후딜 4초 동안 가만히 서 있기만 해서 어색했다)
	mRecoveryTime = 0.f;
	mRecoveryRandom = 0.f;

	// 2페이즈부터는 필살기 대기가 없어도 25초마다 쓴다 (10/28 밸런스 : 원 밖으로 뛰는 압박 + 주기적인 회복 구간)
	mCoolDownTime = 25.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossCryoseism"));

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_Cryoseism.AM_Aurora_Cryoseism'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	// ---------------------------------------------------------------- 이펙트
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		WarmupFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Ultimate/FX/P_Aurora_Ultimate_Warmup.P_Aurora_Ultimate_Warmup'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ExplodeFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Ultimate/FX/P_Aurora_Ultimate_Explode.P_Aurora_Ultimate_Explode'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		GroundFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Ultimate/FX/P_Aurora_Frozen_Ground_Water_Ultimate.P_Aurora_Frozen_Ground_Water_Ultimate'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ShatterFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Leap/FX/P_Aurora_Decoy_Explode_Fragments.P_Aurora_Decoy_Explode_Fragments'"));

	// 기 모으기 소리 (10/29). 폭발 소리는 게임 모드 표의 CryoseismBlast
	static ConstructorHelpers::FObjectFinder<USoundBase>
		ChargeSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Cryoseism_Charge.S_SFX_Cryoseism_Charge'"));

	if (ChargeSnd.Succeeded())
		mChargeSound = ChargeSnd.Object;

	if (WarmupFX.Succeeded())	mWarmupParticle = WarmupFX.Object;
	if (ExplodeFX.Succeeded())	mExplodeParticle = ExplodeFX.Object;
	if (GroundFX.Succeeded())	mGroundParticle = GroundFX.Object;
	if (ShatterFX.Succeeded())	mShatterParticle = ShatterFX.Object;
}

bool UGameplayAbility_Pattern_Cryoseism::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	// const ABossBase* : "읽기만 하는" 보스 포인터. CanUsePattern이 const 함수라 보스 값을 바꾸지 않는다.
	const ABossBase* BossBase = Cast<ABossBase>(Boss);

	// 50% 필살기 대기가 있거나, 2페이즈 이상이면 (쿨다운은 GAS가 따로 확인한다)
	return IsValid(BossBase) && IsValid(Target) &&
		(BossBase->HasPendingUltimate() || BossBase->GetPhase() >= 2);
}

void UGameplayAbility_Pattern_Cryoseism::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	mActiveParticles.Empty();
	mChargeAudio = nullptr;
	mTargetPhase = 0;
	mBlasted = false;

	// 기본은 "피했다". 맞히면 폭발 순간 true로 바꾼다.
	mHitTarget = false;

	// "필살기 대기"를 꺼내 쓴다. 폭발 순간 들어갈 페이즈를 함께 받는다.
	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());

	if (IsValid(Boss))
		mTargetPhase = Boss->ConsumePendingUltimate();

	// 부모 : 멈춤 → 기디언 쪽 바라보기 → 몽타주 재생 (0.37배속으로 천천히 떠오름)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive() || !IsValid(Boss))
		return;

	// ---------------------------------------------------------------- 원 예고 (그 자리, 폭발 순간까지)
	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Ground = Boss->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	// 부모 함수 (11/4) : 미리 불러 둔 머티리얼로 원 예고. 패턴이 끝나면 부모 EndAbility가 지운다
	SpawnWarningCircle(Ground, mRadius, ToRealTime(mBlastTime));

	// ---------------------------------------------------------------- 기 모으기 (몸에 붙임)
	// (10/28 : 얼음 고치 P_Aurora_Ultimate_Frozen은 "얼어붙은 사람"처럼 보여서 뺐다. 은은한 빛만)
	if (IsValid(mWarmupParticle))
	{
		UParticleSystemComponent* FX = UGameplayStatics::SpawnEmitterAttached(mWarmupParticle, Boss->GetMesh());

		if (IsValid(FX))
			mActiveParticles.Add(FX);
	}

	// 기 모으기 소리 (2.2초, 차오르기만 하고 터지지 않는 소리). 몸에 붙여 틀고 폭발 순간 끊는다
	if (IsValid(mChargeSound))
		mChargeAudio = UGameplayStatics::SpawnSoundAttached(mChargeSound, Boss->GetMesh());

	// ---------------------------------------------------------------- 폭발 노티파이 대기
	UAbilityTask_WaitGameplayEvent* WaitBlast = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.Cryoseism")), nullptr, true);

	WaitBlast->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Cryoseism::OnBlastEvent);
	WaitBlast->ReadyForActivation();
}

void UGameplayAbility_Pattern_Cryoseism::OnBlastEvent(FGameplayEventData Payload)
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss) || mBlasted)
		return;

	mBlasted = true;

	DestroyWarningDecal();

	// 터진 뒤에는 원래 속도로 내려온다. Montage_SetPlayRate : 재생 중인 몽타주의 속도를 바꾼다.
	if (UAnimInstance* AnimInstance = Boss->GetMesh()->GetAnimInstance())
		AnimInstance->Montage_SetPlayRate(mMontage, 1.f);

	// 모은 힘이 터진다 : 붙어 있던 기 모으기 이펙트를 끄고, 큰 얼음 파편이 그 자리를 덮는다.
	ClearParticles();

	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Ground = Boss->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	if (IsValid(mShatterParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mShatterParticle, Boss->GetActorLocation(),
			Boss->GetActorRotation(), FVector(mShatterScale));

	for (UParticleSystem* Particle : { mExplodeParticle.Get(), mGroundParticle.Get() })
	{
		if (IsValid(Particle))
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, Ground, Boss->GetActorRotation(),
				Boss->GetActorScale3D());
	}

	// 기 모으기 소리를 짧게 줄이며 끊는다 → "고조 끝 = 폭발" (세바로그 필살기와 같은 방식)
	if (IsValid(mChargeAudio))
		mChargeAudio->FadeOut(mChargeFadeTime, 0.f);

	mChargeAudio = nullptr;

	// 화면 흔들림·폭발 소리 : 게임 모드의 타격감 표 (10/29 : 흔들림은 세바로그 필살기와 같고 소리만 얼음 폭발인 사건으로 분리)
	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
		GameMode->PlayHitFeedback(EHitFeedback::CryoseismBlast, Ground);

	// ---------------------------------------------------------------- 페이즈 진입 (= 이 프레임에 얼음 여제로 변신)
	// 오로라의 OnPhaseEntered가 메시를 바꾼다. 위의 파편·흔들림이 바뀌는 한 프레임을 가린다.
	if (mTargetPhase > 0)
		Boss->EnterPhase(mTargetPhase);

	// ---------------------------------------------------------------- 판정 : 원 안 + 회피 무적 아님 (점프는 못 피함)
	ACharacter* Target = Cast<ACharacter>(Boss->GetTarget());

	if (!IsValid(Target))
		return;

	FVector	ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
	ToTarget.Z = 0.0;

	if (ToTarget.Size() > mRadius + Target->GetCapsuleComponent()->GetScaledCapsuleRadius())
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (!TargetASC ||
		TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
		return;

	FHitResult	Hit;
	Hit.ImpactPoint = Target->GetActorLocation();
	Hit.Location = Hit.ImpactPoint;

	Boss->ApplyDamageToTarget(Target, Boss->GetAttack() * mDamageScale, Hit, false, false);

	// 얼어붙음 (서리 고리와 같은 이벤트·같은 탈출 시스템)
	FGameplayEventData	FreezeEvent;
	FreezeEvent.Instigator = Boss;
	FreezeEvent.EventMagnitude = mFreezeTime;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target,
		FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Frozen")), FreezeEvent);

	// 맞혔으면 지치지 않고 바로 다음 패턴 (얼음은 무적이 아니라 그대로 맞는다. 피했을 때만 지쳐서 회복 구간)
	mHitTarget = true;
}

void UGameplayAbility_Pattern_Cryoseism::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearParticles();

	// 폭발 전에 끊겼으면 기 모으기 소리도 끈다
	if (IsValid(mChargeAudio))
		mChargeAudio->FadeOut(mChargeFadeTime, 0.f);

	mChargeAudio = nullptr;

	// 안전장치 : 폭발 전에 끊겼어도(몽타주가 끊긴 경우 등) 페이즈는 꼭 올린다. 필살기 대기를 이미 꺼냈기 때문.
	ABossBase* Boss = Cast<ABossBase>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

	if (!mBlasted && mTargetPhase > 0 && IsValid(Boss) && !Boss->IsDeath())
		Boss->EnterPhase(mTargetPhase);

	// (11/4) 폭발을 피했으면 지친다. 끊긴 경우(보스 사망 등)는 제외
	// IsActive() : 몽타주 블렌드아웃·완료 알림이 둘 다 와서 EndAbility가 두 번 불려도 한 번만
	if (IsActive() && mBlasted && !mHitTarget && !bWasCancelled && IsValid(Boss) && !Boss->IsDeath())
		Boss->RequestExhaust();

	mBlasted = true;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGameplayAbility_Pattern_Cryoseism::ClearParticles()
{
	for (UParticleSystemComponent* FX : mActiveParticles)
	{
		if (IsValid(FX))
			FX->Deactivate();
	}

	mActiveParticles.Empty();
}
