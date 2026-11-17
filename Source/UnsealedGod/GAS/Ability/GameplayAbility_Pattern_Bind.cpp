// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Bind.h"
#include "../../Boss/BossBase.h"
#include "../../Boss/BossWarningDecal.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_Bind::UGameplayAbility_Pattern_Bind()
{
	// 2페이즈부터. ActivationRequiredTags : 이 태그가 보스에게 붙어 있어야 발동할 수 있다. (페이즈 태그는 누적)
	ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Boss.Phase.2")));

	// 거리 무관 (파훼법은 오직 E). 아레나 어디서든.
	mMinRange = 0.f;
	mMaxRange = 3000.f;

	mWeight = 1.f;

	// 쿨다운 20초 : 한 페이즈에 2~3번
	mCoolDownTime = 20.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossBind"));

	// 0.8배속 고정. 빠른 페이즈 값(mFastPlayRate)을 두지 않아서 3·4페이즈에도 그대로다.
	// 손 내리치기 0.88초 ÷ 0.8 = 1.1초가 E 타이밍을 잡는 예고 시간.
	mMontagePlayRate = 0.8f;

	// 후딜은 결과에 따라 실행 중에 정한다 (회피당함 1초 / 성공 0.3초)
	mRecoveryTime = mFailRecoveryTime;
	mRecoveryRandom = 0.f;

	// 몽타주 3개 (11/4 : 실행 중 경로로 → 생성자에서 미리. 처음 속박할 때 디스크를 읽느라 멈칫하지 않게)
	// 시전(Subjugation)은 부모의 mMontage에 넣는다.
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		CastAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_Bind.AM_Sevarog_Bind'"));

	if (CastAsset.Succeeded())
		mMontage = CastAsset.Object;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		SwingAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_BindSwing.AM_Sevarog_BindSwing'"));

	if (SwingAsset.Succeeded())
		mSwingMontage = SwingAsset.Object;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		TauntAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_Taunt.AM_Sevarog_Taunt'"));

	if (TauntAsset.Succeeded())
		mTauntMontage = TauntAsset.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		BindFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonSevarog/FX/Particles/Abilities/Subjugate/FX/P_Sevarog_Subjugate_Blast.P_Sevarog_Subjugate_Blast'"));

	if (BindFX.Succeeded())
		mBindParticle = BindFX.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		BindSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Bind.S_SFX_Bind'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		AppearSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Wraith_Teleport.S_SFX_Wraith_Teleport'"));

	if (BindSnd.Succeeded())	mBindSound = BindSnd.Object;
	if (AppearSnd.Succeeded())	mAppearSound = AppearSnd.Object;
}

void UGameplayAbility_Pattern_Bind::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 같은 어빌리티 객체를 재사용하므로(InstancedPerActor) 지난 실행의 상태를 비운다.
	mBindSucceeded = false;
	mStage = EBindStage::Cast;
	mRecoveryTime = mFailRecoveryTime;

	// 페이즈 배속 없음 (부모 ActivateAbility를 건너뛰므로 직접 정한다)
	mPlayRate = mMontagePlayRate;

	// 부모(BossPattern)의 "몽타주 하나 재생" 흐름은 건너뛰고, 그 위의 Base(쿨다운 확정)만 부른다. (지침 패턴과 같은 방식)
	UGameplayAbility_Base::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());
	ACharacter* Target = IsValid(Boss) ? Cast<ACharacter>(Boss->GetTarget()) : nullptr;

	if (!IsValid(Boss) || !IsValid(Target) || !IsValid(mMontage))
	{
		UE_LOG(UGDebug, Warning, TEXT("Bind : Invalid Boss, Target or AM_Sevarog_Bind"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(Boss->GetController()))
		AIController->StopMovement();

	FaceTarget();

	// ---------------------------------------------------------------- 판정 준비 : 기디언 무적 태그 감시 (10/22)
	// RegisterGameplayTagEvent(태그, 언제) : 그 태그가 붙거나(New) 떨어질 때(Removed) 알려 주는 델리게이트.
	// AddUObject로 등록하고 돌려받은 핸들로 나중에 해제한다.
	mLastInvincibleTime = -100.f;
	mTargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	if (UAbilitySystemComponent* TargetASC = mTargetASC.Get())
	{
		mInvincibleTagHandle = TargetASC->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")), EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UGameplayAbility_Pattern_Bind::OnTargetInvincibleChanged);
	}

	// ---------------------------------------------------------------- ② 손 내리치기 이벤트 (한 번)
	UAbilityTask_WaitGameplayEvent* WaitCast = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.BindCast")), nullptr, true);

	WaitCast->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnBindCastEvent);
	WaitCast->ReadyForActivation();

	// ---------------------------------------------------------------- ① 시전 몽타주
	UAbilityTask_PlayMontageAndWait* PlayCast = PlayStageMontage(mMontage, mPlayRate);

	PlayCast->OnCompleted.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnCastMontageEnded);
	PlayCast->OnBlendOut.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnCastMontageEnded);
	PlayCast->OnInterrupted.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnCastMontageEnded);
	PlayCast->OnCancelled.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnCastMontageEnded);
	PlayCast->ReadyForActivation();

	// ---------------------------------------------------------------- ① 기디언 발밑 예고 원 (기디언을 따라다님)
	// 원이 다 차는 순간 = 손 내리치기 = 판정. 도약·돌진과 같은 "다 차면 위험" 규칙.
	float	HalfHeight = Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Feet = Target->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	// 부모 함수 (11/4) : 미리 불러 둔 머티리얼로 원 예고. 패턴이 끝나면 부모 EndAbility가 지운다
	SpawnWarningCircle(Feet, mWarningRadius, ToRealTime(mCastTime));

	// AttachToActor : 기디언에 붙여서 같이 움직이게 한다. KeepWorldTransform = 지금 위치 그대로 붙임 (발밑 유지)
	if (IsValid(mWarningDecal))
		mWarningDecal->AttachToActor(Target, FAttachmentTransformRules::KeepWorldTransform);
}

UAbilityTask_PlayMontageAndWait* UGameplayAbility_Pattern_Bind::PlayStageMontage(UAnimMontage* Montage, float Rate)
{
	return UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage, Rate);
}

void UGameplayAbility_Pattern_Bind::OnBindCastEvent(FGameplayEventData Payload)
{
	DestroyWarningDecal();

	// ---------------------------------------------------------------- 판정 (10/22 : 한 순간 → 짧은 구간)
	// 손을 내리치는 순간을 가운데로 앞뒤 mJudgeWindow / 2 안에 한 번이라도 무적(E 회피·넘어짐)이면 회피 성공.
	// 거리·기둥은 보지 않는다 → 파훼법은 오직 E (직접 결정)
	//   앞쪽 절반 : 지금 무적이거나 조금 전(절반 안)에 무적이었으면 → 지금 바로 성공 (조금 일찍 누른 경우)
	//   뒤쪽 절반 : 아니면 절반만큼 더 기다렸다가 그 사이 무적이 시작됐는지 본다 (조금 늦게 누른 경우)
	if (IsDodgingNow())
	{
		ResolveBind(true);
		return;
	}

	UAbilityTask_WaitDelay* WaitLate = UAbilityTask_WaitDelay::WaitDelay(this, mJudgeWindow * 0.5f);
	WaitLate->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnJudgeLate);
	WaitLate->ReadyForActivation();
}

void UGameplayAbility_Pattern_Bind::OnJudgeLate()
{
	ResolveBind(IsDodgingNow());
}

bool UGameplayAbility_Pattern_Bind::IsDodgingNow() const
{
	UAbilitySystemComponent* TargetASC = mTargetASC.Get();

	if (!TargetASC)
		return false;

	if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
		return true;

	// 무적이 이미 끝났어도, 끝난 지 절반 안이면 "판정 구간 안에 무적이었다"
	return GetWorld()->GetTimeSeconds() - mLastInvincibleTime <= mJudgeWindow * 0.5f;
}

void UGameplayAbility_Pattern_Bind::OnTargetInvincibleChanged(const FGameplayTag Tag, int32 NewCount)
{
	// 붙을 때(무적 시작)와 떨어질 때(무적 끝) 모두 "이 시각에는 무적이었다"로 기록한다.
	if (UWorld* World = GetWorld())
		mLastInvincibleTime = World->GetTimeSeconds();
}

void UGameplayAbility_Pattern_Bind::StopWatchingInvincible()
{
	if (UAbilitySystemComponent* TargetASC = mTargetASC.Get())
	{
		TargetASC->RegisterGameplayTagEvent(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")),
			EGameplayTagEventType::NewOrRemoved).Remove(mInvincibleTagHandle);
	}

	mInvincibleTagHandle.Reset();
	mTargetASC = nullptr;
}

void UGameplayAbility_Pattern_Bind::ResolveBind(bool bDodged)
{
	StopWatchingInvincible();

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;
	AGameCharacter* TargetCharacter = Cast<AGameCharacter>(Target);

	if (!IsValid(Boss) || !IsValid(TargetCharacter))
		return;

	if (bDodged || TargetCharacter->IsDeath())
	{
		UE_LOG(UGDebug, Warning, TEXT("Bind : dodged"));

		// 실패 : 시전 몽타주(손 내리치기)를 끝까지 보여 주고 1초 쉰다 = 반격 틈. (종료는 OnCastMontageEnded)
		mRecoveryTime = mFailRecoveryTime;
		return;
	}

	UE_LOG(UGDebug, Warning, TEXT("Bind : bound"));

	// ---------------------------------------------------------------- ③ 성공 : 기디언을 묶는다
	mBindSucceeded = true;
	mStage = EBindStage::Teleport;

	// 기디언의 속박 어빌리티를 이벤트로 발동한다. (넘어짐과 같은 방식, Instigator = 보스)
	FGameplayEventData	BoundPayload;
	BoundPayload.Instigator = Boss;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target,
		FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Bound")), BoundPayload);

	if (IsValid(mBindParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mBindParticle,
			Target->GetActorLocation(), FRotator::ZeroRotator);
	}

	// "걸렸다" : 떠오르는 기디언 몸에 붙여 튼다
	if (IsValid(mBindSound))
		UGameplayStatics::SpawnSoundAttached(mBindSound, Target->GetRootComponent());

	// ---------------------------------------------------------------- ④ 사라진다
	// 시전 몽타주(손 내리치기)는 계속 재생되면서 검게 깎여 사라진다.
	Boss->StartVanish(mVanishTime);

	UAbilityTask_WaitDelay* WaitVanish = UAbilityTask_WaitDelay::WaitDelay(this, mVanishTime + mHiddenTime);
	WaitVanish->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnVanished);
	WaitVanish->ReadyForActivation();
}

void UGameplayAbility_Pattern_Bind::OnCastMontageEnded()
{
	// 성공해서 다음 단계로 넘어갔으면, 시전 몽타주가 밀려 끝나는 알림은 무시한다.
	if (mStage != EBindStage::Cast)
		return;

	// 회피당했거나(판정 후 몽타주 끝까지) 판정 전에 끊김(사망 등) → 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGameplayAbility_Pattern_Bind::OnVanished()
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Boss) || Boss->IsDeath() || !IsValid(Target))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// ---------------------------------------------------------------- ④ 나타날 위치 : 기디언 등 뒤 왼편 45°
	// 기디언은 묶인 동안 몸이 고정돼 있어서, 이 방향이 넘어짐 어빌리티가 보는 "등 뒤"와 같다.
	// RotateAngleAxis(각도, 축) : 벡터를 축 기준으로 돌린다. 뒤(-앞)를 +45° 돌리면 뒤쪽 왼편.
	FVector	Back = -Target->GetActorForwardVector();
	Back.Z = 0.0;
	Back = Back.GetSafeNormal();

	FVector	Origin = Target->GetActorLocation();

	// 벽·기둥에 막히면 반대쪽(뒤쪽 오른편), 그것도 막히면 막힌 곳보다 조금 앞에. (기둥 안에 나타나지 않게)
	// 람다 [&] : 이 함수 안의 변수를 그대로 쓰는 작은 함수. 같은 계산을 두 방향에 쓰려고.
	auto	FindSpot = [&](float Angle, float& OutDistance) -> FVector
	{
		FVector	Direction = Back.RotateAngleAxis(Angle, FVector::UpVector);
		FVector	End = Origin + Direction * mAppearDistance;

		FHitResult				Hit;
		FCollisionQueryParams	Params(NAME_None, false, Target);
		Params.AddIgnoredActor(Boss);

		OutDistance = mAppearDistance;

		if (GetWorld()->LineTraceSingleByObjectType(Hit, Origin, End,
			FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic), Params))
		{
			OutDistance = FMath::Max(Hit.Distance - 120.f, 150.f);
		}

		return Origin + Direction * OutDistance;
	};

	float	LeftDistance = 0.f;
	float	RightDistance = 0.f;
	FVector	Spot = FindSpot(mAppearAngle, LeftDistance);

	if (LeftDistance < mAppearDistance)
	{
		FVector	RightSpot = FindSpot(-mAppearAngle, RightDistance);

		if (RightDistance > LeftDistance)
			Spot = RightSpot;
	}

	Boss->TeleportToGround(Spot);
	FaceTarget();
	Boss->StartAppear(mAppearTime);

	if (IsValid(mAppearSound))
		UGameplayStatics::PlaySoundAtLocation(this, mAppearSound, Boss->GetActorLocation());

	// ---------------------------------------------------------------- ⑤ 일격
	mStage = EBindStage::Swing;

	if (!IsValid(mSwingMontage))
	{
		UE_LOG(UGDebug, Warning, TEXT("Bind : AM_Sevarog_BindSwing not found"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* WaitImpact = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.BindImpact")), nullptr, true);

	WaitImpact->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnBindImpactEvent);
	WaitImpact->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* PlaySwing = PlayStageMontage(mSwingMontage, 1.f);

	PlaySwing->OnCompleted.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnSwingMontageEnded);
	PlaySwing->OnBlendOut.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnSwingMontageEnded);
	PlaySwing->OnInterrupted.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnSwingMontageEnded);
	PlaySwing->OnCancelled.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnSwingMontageEnded);
	PlaySwing->ReadyForActivation();
}

void UGameplayAbility_Pattern_Bind::OnBindImpactEvent(FGameplayEventData Payload)
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Boss) || !IsValid(Target))
		return;

	// 묶인 기디언은 공중에 떠 있어서 판정 구(Melee Hit Window)로는 빗나갈 수 있다 → 확정 피해.
	// 막을 수 없음(true) : 묶인 동안의 무적을 뚫는다. Heavy(true) : 넘어짐 → 등 뒤라 Knock_Fwd로 날아간다.
	FHitResult	Hit;
	Hit.ImpactPoint = Target->GetActorLocation();

	Boss->ApplyDamageToTarget(Target, Boss->GetAttack() * mImpactDamageScale, Hit, true, true);
}

void UGameplayAbility_Pattern_Bind::OnSwingMontageEnded()
{
	if (mStage != EBindStage::Swing)
		return;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	// 죽었거나(몽타주 정지) 도발 몽타주가 없으면 여기서 끝낸다.
	if (!IsValid(Boss) || Boss->IsDeath() || !IsValid(mTauntMontage))
	{
		mRecoveryTime = mSuccessRecoveryTime;
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// ---------------------------------------------------------------- ⑥ 도발 (날아간 기디언 쪽을 보며)
	mStage = EBindStage::Taunt;
	FaceTarget();

	UAbilityTask_PlayMontageAndWait* PlayTaunt = PlayStageMontage(mTauntMontage, 1.f);

	PlayTaunt->OnCompleted.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnTauntMontageEnded);
	PlayTaunt->OnBlendOut.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnTauntMontageEnded);
	PlayTaunt->OnInterrupted.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnTauntMontageEnded);
	PlayTaunt->OnCancelled.AddDynamic(this, &UGameplayAbility_Pattern_Bind::OnTauntMontageEnded);
	PlayTaunt->ReadyForActivation();
}

void UGameplayAbility_Pattern_Bind::OnTauntMontageEnded()
{
	if (mStage != EBindStage::Taunt)
		return;

	mRecoveryTime = mSuccessRecoveryTime;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGameplayAbility_Pattern_Bind::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	StopWatchingInvincible();

	// 사라진 채로 끊겼으면(전투 끝 등) 다시 보이게 한다.
	if (ABossBase* Boss = Cast<ABossBase>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr))
	{
		if (Boss->IsVanished())
			Boss->StartAppear(0.f);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
