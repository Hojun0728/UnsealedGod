// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_IceSculpture.h"
#include "../../Boss/Aurora.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_Repeat.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_IceSculpture::UGameplayAbility_Pattern_IceSculpture()
{
	// 얼어 있으면 반드시 이어진다 (필살기 10 다음)
	mPriority = 8;

	// 얼음 지진(반지름 10m)으로 얼었을 때도 닿게 10m
	mMinRange = 0.f;
	mMaxRange = 1000.f;

	mWeight = 1.f;

	// 얼어 있을 때만 쓸 수 있으니 쿨다운은 필요 없다
	mCoolDownTime = 0.f;

	mMontagePlayRate = 1.f;
	mLungeSpeed = 0.f;

	// 기디언 앞 1.5m에서 찌른다
	mStrikeDistance = 150.f;

	// 끝까지 다 찔렀을 때의 후딜은 기본값(0.8 ± 0.4). 탈출했을 때는 mEscapeRecoveryTime

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_IceSculpture.AM_Aurora_IceSculpture'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		StabFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Aurora/P_Aurora_Sculpture_Chip.P_Aurora_Sculpture_Chip'"));

	if (StabFX.Succeeded())
		mStabParticle = StabFX.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		StabSnd1(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Aurora_Stab_1.S_SFX_Aurora_Stab_1'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		StabSnd2(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Aurora_Stab_2.S_SFX_Aurora_Stab_2'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		StabSnd3(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Aurora_Stab_3.S_SFX_Aurora_Stab_3'"));

	for (auto* Finder : { &StabSnd1, &StabSnd2, &StabSnd3 })
	{
		if (Finder->Succeeded())
			mStabSounds.Add(Finder->Object);
	}
}

bool UGameplayAbility_Pattern_IceSculpture::IsTargetFrozen(const AActor* Target)
{
	// GetAbilitySystemComponent : 액터가 가진 ASC를 찾는다. (const를 벗겨 넘기는 건 찾기만 하고 바꾸지 않기 때문)
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Target));

	return ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Frozen")));
}

bool UGameplayAbility_Pattern_IceSculpture::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	// 부모 : 거리(0~10m). 그리고 기디언이 얼어 있을 때만.
	return Super::CanUsePattern(Boss, Target) && IsTargetFrozen(Target);
}

void UGameplayAbility_Pattern_IceSculpture::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 같은 어빌리티 객체를 재사용하므로 지난번 탈출 후딜을 기본값으로 되돌린다.
	mEscaped = false;
	mRecoveryTime = 0.8f;
	mRecoveryRandom = 0.4f;

	// 부모 : 멈춤 → 기디언 쪽 바라보기 → 몽타주 재생 (찌르기 구간)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive())
		return;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Target))
		return;

	// ---------------------------------------------------------------- 찌르는 방향 맞추기 (10/28)
	// 부모는 기디언을 정면으로 보게 돌린다. 이 동작은 몸 옆을 찌르므로, 오로라가 정한 각도만큼 더 틀어 선다.
	if (AAurora* Aurora = Cast<AAurora>(Boss))
		FaceTarget(Aurora->GetSculptureYawOffset());

	// ---------------------------------------------------------------- 다가가기 (기디언 앞 mStrikeDistance까지)
	FVector	ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
	ToTarget.Z = 0.0;

	float	Distance = FMath::Max(ToTarget.Size() - mStrikeDistance, 0.f);

	if (Distance > 1.f)
	{
		// 거리 ÷ 시간 = 속도. 끝날 때 속도 0 → 도착하면 미끄러지지 않고 선다. (세바로그 돌진과 같은 태스크)
		UAbilityTask_ApplyRootMotionConstantForce* Approach =
			UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
				this, NAME_None, ToTarget.GetSafeNormal(), Distance / mApproachTime, mApproachTime,
				false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector,
				0.f, true);

		Approach->ReadyForActivation();
	}

	UAbilityTask_WaitDelay* WaitApproach = UAbilityTask_WaitDelay::WaitDelay(this, mApproachTime);
	WaitApproach->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_IceSculpture::OnApproachFinished);
	WaitApproach->ReadyForActivation();
}

float UGameplayAbility_Pattern_IceSculpture::GetStabDuration() const
{
	// 몽타주 전체가 찌르기 동작. 다가가는 시간 동안에는 찌르지 않으므로 뺀다. (실제 시간 = 몽타주 시간 ÷ 재생 속도)
	if (!IsValid(mMontage))
		return mStabInterval * mStabCount;

	return FMath::Max(ToRealTime(mMontage->GetPlayLength()) - mApproachTime, 0.f);
}

void UGameplayAbility_Pattern_IceSculpture::OnApproachFinished()
{
	// RepeatAction(어빌리티, 간격, 횟수) : 간격마다 OnPerformAction을 횟수만큼 부른다. (첫 번째는 바로)
	// 찌르기 횟수 = 찌르는 동작의 실제 길이 ÷ 간격 (10/28) : 몽타주에서 위쪽 찌르기를 잘라 내도 동작과 피해 횟수가 맞는다
	int32	Count = FMath::Max(1, FMath::FloorToInt(GetStabDuration() / FMath::Max(mStabInterval, 0.05f)));

	UAbilityTask_Repeat* Stabs = UAbilityTask_Repeat::RepeatAction(this, mStabInterval, Count);
	Stabs->OnPerformAction.AddDynamic(this, &UGameplayAbility_Pattern_IceSculpture::OnStab);
	Stabs->ReadyForActivation();
}

void UGameplayAbility_Pattern_IceSculpture::OnStab(int32 ActionNumber)
{
	if (mEscaped)
		return;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Boss) || !IsValid(Target))
		return;

	// ---------------------------------------------------------------- 연타로 깨고 나왔다 → 찌르기를 멈추고 반격 틈
	if (!IsTargetFrozen(Target))
	{
		mEscaped = true;
		mRecoveryTime = mEscapeRecoveryTime;
		mRecoveryRandom = 0.f;

		// 몽타주를 멈추면 부모의 OnMontageFinished가 어빌리티를 끝내고, 위의 후딜을 보스에게 알린다.
		if (UAnimInstance* AnimInstance = Boss->GetMesh()->GetAnimInstance())
			AnimInstance->Montage_Stop(0.25f, mMontage);

		return;
	}

	// 찌르는 소리 : 동작은 매번 찌르므로 맞든 안 맞든 낸다
	if (!mStabSounds.IsEmpty())
	{
		USoundBase* Sound = mStabSounds[FMath::RandRange(0, mStabSounds.Num() - 1)];

		if (IsValid(Sound))
			UGameplayStatics::PlaySoundAtLocation(this, Sound, Boss->GetActorLocation());
	}

	// ---------------------------------------------------------------- 찌르기 (가까이 있을 때만)
	FVector	ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
	ToTarget.Z = 0.0;

	if (ToTarget.Size() > mStabReach)
		return;

	FHitResult	Hit;
	Hit.ImpactPoint = Target->GetActorLocation();
	Hit.Location = Hit.ImpactPoint;

	Boss->ApplyDamageToTarget(Target, Boss->GetAttack() * mStabDamageScale, Hit, false, false);

	if (IsValid(mStabParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mStabParticle, Target->GetActorLocation());
}
