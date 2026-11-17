// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_IceGrab.h"
#include "../../Boss/Aurora.h"
#include "../../Boss/BossWarningDecal.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_IceGrab::UGameplayAbility_Pattern_IceGrab()
{
	// 부모(얼음 조각) 생성자 값을 포박용으로 덮어쓴다.
	// 일반 패턴 (얼음 조각의 우선순위 8은 "이미 얼었을 때"용)
	mPriority = 0;

	mMinRange = 0.f;
	// 0~10m (10/28 밸런스 : 손을 뻗는 잡기라 조금 멀어도 잡히는 게 자연스럽다. 6m는 원거리 기디언에게 거의 안 나왔음)
	mMaxRange = 1000.f;

	mCoolDownTime = 15.f;	// 10/28 밸런스 : 12 → 15초 (두 번 연속 잡기 방지)
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossIceGrab"));

	// 페이즈 배속 없음 (빠른 값을 안 넣으면 2페이즈에도 1.0배속)
	mMontagePlayRate = 1.f;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_IceGrab.AM_Aurora_IceGrab'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		TeleportFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Leap/FX/P_Aurora_Decoy_Explode_Fragments.P_Aurora_Decoy_Explode_Fragments'"));

	if (TeleportFX.Succeeded())
		mTeleportParticle = TeleportFX.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		ChargeSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_IceGrab_Charge.S_SFX_IceGrab_Charge'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		TeleportSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_IceGrab_Teleport.S_SFX_IceGrab_Teleport'"));

	static ConstructorHelpers::FObjectFinder<USoundBase>
		TauntVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonAurora/Audio/Cues/Aurora_Emote_Taunt.Aurora_Emote_Taunt'"));

	if (ChargeSnd.Succeeded())		mChargeSound = ChargeSnd.Object;
	if (TeleportSnd.Succeeded())	mTeleportSound = TeleportSnd.Object;
	if (TauntVoice.Succeeded())		mTauntVoice = TauntVoice.Object;
}

bool UGameplayAbility_Pattern_IceGrab::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	// UGameplayAbility_BossPattern:: : 부모(얼음 조각)를 건너뛰고 할아버지의 거리 조건만 쓴다.
	return UGameplayAbility_BossPattern::CanUsePattern(Boss, Target);
}

void UGameplayAbility_Pattern_IceGrab::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 같은 어빌리티 객체를 재사용하므로 지난번 상태를 비운다.
	mEscaped = false;
	mResolved = false;
	mChargeAudio = nullptr;
	mRecoveryTime = mFailRecoveryTime;
	mRecoveryRandom = 0.f;

	// 할아버지(BossPattern) : 멈춤 → 기디언 쪽 바라보기 → 몽타주 재생 (Default = 손으로 얼음 보내기)
	// 부모(얼음 조각)의 ActivateAbility는 "바로 다가가 찌르기"라 건너뛴다.
	UGameplayAbility_BossPattern::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive())
		return;

	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());
	ACharacter* Target = IsValid(Boss) ? Cast<ACharacter>(Boss->GetTarget()) : nullptr;

	if (!IsValid(Target))
		return;

	// ---------------------------------------------------------------- 판정 준비 : 기디언 무적 태그 감시 (세바로그 속박과 같다)
	// RegisterGameplayTagEvent(태그, 언제) : 태그가 붙거나 떨어질 때 알려 준다. 돌려받은 핸들로 끝날 때 해제한다.
	mLastInvincibleTime = -100.f;
	mTargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	if (UAbilitySystemComponent* TargetASC = mTargetASC.Get())
	{
		mInvincibleTagHandle = TargetASC->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")), EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UGameplayAbility_Pattern_IceGrab::OnTargetInvincibleChanged);
	}

	// ---------------------------------------------------------------- 손에서 얼음이 나가는 순간 (한 번)
	UAbilityTask_WaitGameplayEvent* WaitCast = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.IceGrab")), nullptr, true);

	WaitCast->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_IceGrab::OnGrabCastEvent);
	WaitCast->ReadyForActivation();

	// ---------------------------------------------------------------- 기디언 발밑 예고 원 (따라다니며 차오름)
	float	HalfHeight = Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Feet = Target->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	// 부모 함수 (11/4) : 미리 불러 둔 머티리얼로 원 예고. 패턴이 끝나면 부모 EndAbility가 지운다
	SpawnWarningCircle(Feet, mWarningRadius, ToRealTime(mCastTime));

	// AttachToActor : 기디언에 붙여서 같이 움직이게 한다. KeepWorldTransform = 지금 위치 그대로 붙임 (발밑 유지)
	if (IsValid(mWarningDecal))
		mWarningDecal->AttachToActor(Target, FAttachmentTransformRules::KeepWorldTransform);

	// 예고 소리 : 원과 같은 순간 시작 → 소리 안의 "팍"(0.87초)이 원이 다 차는 순간(판정)과 맞는다. 배속이 없는 패턴이라 항상 맞다.
	// 기디언 몸에 붙인다 = 잡히는 쪽에서 들린다 (원과 같이 따라다닌다)
	if (IsValid(mChargeSound))
		mChargeAudio = UGameplayStatics::SpawnSoundAttached(mChargeSound, Target->GetRootComponent());
}

void UGameplayAbility_Pattern_IceGrab::OnGrabCastEvent(FGameplayEventData Payload)
{
	DestroyWarningDecal();

	// 판정 : 이 순간을 가운데로 앞뒤 절반씩. 조금 일찍 눌렀으면 지금 바로, 아니면 절반만큼 더 기다렸다가 본다.
	if (IsDodgingNow())
	{
		ResolveGrab(true);
		return;
	}

	UAbilityTask_WaitDelay* WaitLate = UAbilityTask_WaitDelay::WaitDelay(this, mJudgeWindow * 0.5f);
	WaitLate->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_IceGrab::OnJudgeLate);
	WaitLate->ReadyForActivation();
}

void UGameplayAbility_Pattern_IceGrab::OnJudgeLate()
{
	ResolveGrab(IsDodgingNow());
}

bool UGameplayAbility_Pattern_IceGrab::IsDodgingNow() const
{
	UAbilitySystemComponent* TargetASC = mTargetASC.Get();

	if (!TargetASC)
		return false;

	if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
		return true;

	// 무적이 이미 끝났어도 끝난 지 절반 안이면 "판정 구간 안에 무적이었다"
	return GetWorld()->GetTimeSeconds() - mLastInvincibleTime <= mJudgeWindow * 0.5f;
}

void UGameplayAbility_Pattern_IceGrab::OnTargetInvincibleChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (UWorld* World = GetWorld())
		mLastInvincibleTime = World->GetTimeSeconds();
}

void UGameplayAbility_Pattern_IceGrab::StopWatchingInvincible()
{
	if (UAbilitySystemComponent* TargetASC = mTargetASC.Get())
	{
		TargetASC->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")),
			EGameplayTagEventType::NewOrRemoved).Remove(mInvincibleTagHandle);
	}

	mInvincibleTagHandle.Reset();
	mTargetASC = nullptr;
}

void UGameplayAbility_Pattern_IceGrab::ResolveGrab(bool bDodged)
{
	if (mResolved)
		return;

	mResolved = true;

	StopWatchingInvincible();

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AGameCharacter* Target = IsValid(Boss) ? Cast<AGameCharacter>(Boss->GetTarget()) : nullptr;

	if (!IsValid(Boss) || !IsValid(Target))
		return;

	// 얼음 내성 중이면(방금 얼음이 풀림) 잡히지 않는다 = 피한 것과 같다 (10/28)
	if (UAbilitySystemComponent* ImmuneASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
	{
		if (ImmuneASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.FreezeImmune"))))
			bDodged = true;
	}

	// 실패 : 손을 내보내는 동작을 끝까지 보여 주고(Default 섹션 끝 = 몽타주 끝) 1초 쉰다 = 반격 틈
	if (bDodged || Target->IsDeath())
	{
		UE_LOG(UGDebug, Warning, TEXT("IceGrab : dodged"));
		mRecoveryTime = mFailRecoveryTime;
		return;
	}

	UE_LOG(UGDebug, Warning, TEXT("IceGrab : grabbed"));

	// ---------------------------------------------------------------- 성공 : 얼린다 (서리 고리와 같은 얼음 시스템)
	FGameplayEventData	FreezeEvent;
	FreezeEvent.Instigator = Boss;
	FreezeEvent.EventMagnitude = mGrabFreezeTime;

	// 포박의 얼음은 맞아도 깨지지 않는다 (찌르기는 연타로만 줄인다). 얼음 어빌리티가 이 표시를 보고 State.FrozenNoShatter를 붙인다
	FreezeEvent.InstigatorTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.FrozenNoShatter")));

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target,
		FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Frozen")), FreezeEvent);

	// 다 찔렀을 때의 후딜 (연타로 깨면 부모가 1.5초로 바꾼다)
	mRecoveryTime = 0.8f;
	mRecoveryRandom = 0.4f;

	// ---------------------------------------------------------------- 순간이동 : 기디언 정면 또는 등 뒤 (랜덤)
	if (IsValid(mTeleportParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mTeleportParticle, Boss->GetActorLocation(),
			Boss->GetActorRotation(), Boss->GetActorScale3D());

	// 기디언 옆에 나타나므로 기디언 자리에서 낸다
	if (IsValid(mTeleportSound))
		UGameplayStatics::PlaySoundAtLocation(this, mTeleportSound, Target->GetActorLocation());

	// 기디언이 보고 있는 방향(수평). FMath::RandBool : 반반 확률
	FVector	Facing = Target->GetActorForwardVector();
	Facing.Z = 0.0;
	Facing = Facing.GetSafeNormal();

	float	Side = FMath::RandBool() ? 1.f : -1.f;

	Boss->TeleportToGround(Target->GetActorLocation() + Facing * (Side * mAppearDistance));

	// 찌르는 방향 맞추기 (얼음 조각과 같은 각도 보정)
	if (AAurora* Aurora = Cast<AAurora>(Boss))
		FaceTarget(Aurora->GetSculptureYawOffset());

	if (IsValid(mTeleportParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mTeleportParticle, Boss->GetActorLocation(),
			Boss->GetActorRotation(), Boss->GetActorScale3D());

	// 나타나서 도발 (몸에 붙임) → 얼어 있는 기디언에게 "잡혔다"를 대사로
	if (IsValid(mTauntVoice))
		UGameplayStatics::SpawnSoundAttached(mTauntVoice, Boss->GetMesh());

	// ---------------------------------------------------------------- 찌르기 : Stab 섹션 + 부모의 찌르기 반복
	if (UAnimInstance* AnimInstance = Boss->GetMesh()->GetAnimInstance())
		AnimInstance->Montage_JumpToSection(mStabSectionName, mMontage);

	OnApproachFinished();
}

void UGameplayAbility_Pattern_IceGrab::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 판정 전에 끊겨도(보스 사망 등) 감시를 정리한다. (바닥 예고는 부모 EndAbility가 지운다)
	// 예고 소리는 판정 전에 끊겼을 때만 끈다 (판정 뒤에는 "팍"의 울림이 남아도 된다)
	if (!mResolved && IsValid(mChargeAudio))
		mChargeAudio->FadeOut(0.1f, 0.f);

	mChargeAudio = nullptr;

	StopWatchingInvincible();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float UGameplayAbility_Pattern_IceGrab::GetStabDuration() const
{
	if (!IsValid(mMontage))
		return Super::GetStabDuration();

	// GetSectionIndex(이름) → GetSectionLength(번호) : 그 섹션의 길이(몽타주 시간)
	int32	SectionIndex = mMontage->GetSectionIndex(mStabSectionName);

	if (SectionIndex == INDEX_NONE)
		return Super::GetStabDuration();

	return ToRealTime(mMontage->GetSectionLength(SectionIndex));
}
