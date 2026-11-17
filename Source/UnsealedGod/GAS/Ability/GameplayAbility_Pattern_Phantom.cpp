// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Phantom.h"
#include "../../Boss/BossBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_Phantom::UGameplayAbility_Pattern_Phantom()
{
	// 3페이즈부터 (커지고 빨라진 "과부하" 신이 순간이동까지 쓴다)
	ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Boss.Phase.3")));

	// 5~25m : 거리를 벌린 플레이어를 기습. (가까우면 3연타가 맡는다)
	mMinRange = 500.f;
	mMaxRange = 2500.f;

	mWeight = 1.f;

	mCoolDownTime = 15.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossPhantom"));

	// 휘두르기 속도 : 3연타와 같게 (평소 1.5배, 빠른 페이즈 2배). 3페이즈부터 나오므로 실제로는 항상 2배.
	mMontagePlayRate = 1.5f;
	mFastPlayRate = 2.f;

	// 후딜 0.8 ± 0.3 : 기습이 끝나면 반격 틈
	mRecoveryTime = 0.8f;
	mRecoveryRandom = 0.3f;

	// 휘두르기 몽타주 = 부모의 mMontage. (11/4 : 실행 중 경로로 → 생성자에서 미리)
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		SwingAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_Phantom.AM_Sevarog_Phantom'"));

	if (SwingAsset.Succeeded())
		mMontage = SwingAsset.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		BlinkAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Speedburst.Speedburst'"));

	if (BlinkAnim.Succeeded())
		mBlinkAnim = BlinkAnim.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		AppearSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Wraith_Teleport.S_SFX_Wraith_Teleport'"));

	if (AppearSnd.Succeeded())
		mAppearSound = AppearSnd.Object;
}

void UGameplayAbility_Pattern_Phantom::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	mSwinging = false;

	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());

	// 휘두르기 속도 (빠른 페이즈 반영). 부모 ActivateAbility를 건너뛰므로 직접 정한다.
	mPlayRate = (IsValid(Boss) && Boss->IsFastPhase() && mFastPlayRate > 0.f) ? mFastPlayRate : mMontagePlayRate;

	// 부모(BossPattern)의 "몽타주 하나 재생"은 건너뛰고 Base(쿨다운 확정)만. (속박·지침과 같은 방식)
	UGameplayAbility_Base::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	UAnimInstance* AnimInstance = IsValid(Boss) ? Boss->GetMesh()->GetAnimInstance() : nullptr;

	if (!IsValid(Boss) || !IsValid(Boss->GetTarget()) || !AnimInstance || !IsValid(mMontage))
	{
		UE_LOG(UGDebug, Warning, TEXT("Phantom : Invalid Boss, Target or AM_Sevarog_Phantom"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(Boss->GetController()))
		AIController->StopMovement();

	FaceTarget();

	// ---------------------------------------------------------------- ① 제자리 흔들림 (사라지기 예고)
	// Speedburst 길이가 mBlinkTime보다 짧으면 반복한다. (Ceil : 올림 → 0.5초를 덮을 만큼)
	if (IsValid(mBlinkAnim))
	{
		float	Length = mBlinkAnim->GetPlayLength() / FMath::Max(mBlinkAnim->RateScale, 0.01f);
		int32	Loops = FMath::Max(FMath::CeilToInt(mBlinkTime / FMath::Max(Length, 0.01f)), 1);

		AnimInstance->PlaySlotAnimationAsDynamicMontage(mBlinkAnim, TEXT("DefaultSlot"), 0.1f, 0.1f, 1.f, Loops);
	}

	UAbilityTask_WaitDelay* WaitBlink = UAbilityTask_WaitDelay::WaitDelay(this, mBlinkTime);
	WaitBlink->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnBlinkFinished);
	WaitBlink->ReadyForActivation();
}

void UGameplayAbility_Pattern_Phantom::OnBlinkFinished()
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss) || Boss->IsDeath())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// ---------------------------------------------------------------- ② 사라짐
	Boss->StartVanish(mVanishTime);

	UAbilityTask_WaitDelay* WaitVanish = UAbilityTask_WaitDelay::WaitDelay(this, mVanishTime);
	WaitVanish->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnVanished);
	WaitVanish->ReadyForActivation();
}

void UGameplayAbility_Pattern_Phantom::OnVanished()
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Boss) || Boss->IsDeath() || !IsValid(Target))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// ---------------------------------------------------------------- ③ 나타날 자리 + 예고 원
	// 자리는 "지금" 정한다 → 예고 원을 보고 피하면 그 자리를 비켜난 것이 된다. (원이 기디언을 따라가지 않는다)
	mAppearSpot = FindAppearSpot(Boss, Target);

	// 부모 함수 (11/4) : 미리 불러 둔 머티리얼로 원 예고. 패턴이 끝나면 부모 EndAbility가 지운다
	SpawnWarningCircle(mAppearSpot, mWarningRadius, mAppearWarnTime);

	UAbilityTask_WaitDelay* WaitWarn = UAbilityTask_WaitDelay::WaitDelay(this, mAppearWarnTime);
	WaitWarn->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnAppear);
	WaitWarn->ReadyForActivation();
}

void UGameplayAbility_Pattern_Phantom::OnAppear()
{
	DestroyWarningDecal();

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss) || Boss->IsDeath())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// ---------------------------------------------------------------- ④ 나타나서 휘두르기
	Boss->TeleportToGround(mAppearSpot);
	FaceTarget();
	Boss->StartAppear(mAppearTime);

	// 예고 원(0.45초)이 끝나는 순간 = 나타남. 바로 휘두르기가 이어진다
	if (IsValid(mAppearSound))
		UGameplayStatics::PlaySoundAtLocation(this, mAppearSound, mAppearSpot);

	mSwinging = true;

	// 몽타주에 밀기 노티파이(Event.Boss.Lunge)를 넣으면 3연타처럼 추적하며 휘두른다. (부모의 밀기 처리를 그대로 쓴다)
	UAbilityTask_WaitGameplayEvent* WaitLunge = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.Lunge")), nullptr, false);

	WaitLunge->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnLungeEvent);
	WaitLunge->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* PlaySwing =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, mMontage, mPlayRate);

	PlaySwing->OnCompleted.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnSwingMontageEnded);
	PlaySwing->OnBlendOut.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnSwingMontageEnded);
	PlaySwing->OnInterrupted.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnSwingMontageEnded);
	PlaySwing->OnCancelled.AddDynamic(this, &UGameplayAbility_Pattern_Phantom::OnSwingMontageEnded);
	PlaySwing->ReadyForActivation();
}

void UGameplayAbility_Pattern_Phantom::OnSwingMontageEnded()
{
	if (!mSwinging)
		return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FVector UGameplayAbility_Pattern_Phantom::FindAppearSpot(ABossBase* Boss, AActor* Target) const
{
	// 기디언의 오른쪽 방향. 50% 확률로 왼쪽(-)으로 뒤집는다.
	// FMath::RandBool : true/false 반반
	FVector	Side = Target->GetActorRightVector();
	Side.Z = 0.0;
	Side = Side.GetSafeNormal();

	if (FMath::RandBool())
		Side = -Side;

	FVector	Origin = Target->GetActorLocation();

	FCollisionQueryParams	Params(NAME_None, false, Target);
	Params.AddIgnoredActor(Boss);

	// 한쪽 방향으로 선을 쏴서 벽·기둥까지의 거리를 본다. 막히면 막힌 곳보다 1.2m 앞 (보스 몸이 들어갈 자리), 최소 1.5m
	auto	Probe = [&](const FVector& Direction, float& OutDistance)
	{
		FHitResult	Hit;
		OutDistance = mAppearDistance;

		if (GetWorld()->LineTraceSingleByObjectType(Hit, Origin, Origin + Direction * mAppearDistance,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic), Params))
		{
			OutDistance = FMath::Max(Hit.Distance - 120.f, 150.f);
		}
	};

	float	Distance = 0.f;
	Probe(Side, Distance);

	if (Distance < mAppearDistance)
	{
		float	OtherDistance = 0.f;
		Probe(-Side, OtherDistance);

		if (OtherDistance > Distance)
		{
			Side = -Side;
			Distance = OtherDistance;
		}
	}

	// 바닥 높이 = 기디언 발밑 (아레나 바닥은 평평하다). TeleportToGround가 정확한 바닥을 다시 찾는다.
	float	HalfHeight = 0.f;

	if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		HalfHeight = TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	return Origin + Side * Distance - FVector(0.0, 0.0, HalfHeight);
}

void UGameplayAbility_Pattern_Phantom::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 사라진 채로 끊겼으면 다시 보이게
	if (ABossBase* Boss = Cast<ABossBase>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr))
	{
		if (Boss->IsVanished())
			Boss->StartAppear(0.f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
