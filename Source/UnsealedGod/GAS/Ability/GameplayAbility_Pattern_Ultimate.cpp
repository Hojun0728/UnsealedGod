// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Ultimate.h"
#include "../../Boss/BossBase.h"
#include "../../Boss/UltimateLight.h"
#include "../../GameMode/ArenaGameMode.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "DrawDebugHelpers.h"

UGameplayAbility_Pattern_Ultimate::UGameplayAbility_Pattern_Ultimate()
{
	// 거리 조건은 쓰지 않는다. (CanUsePattern을 덮어써서 "필살기 대기"만 확인)
	mMinRange = 0.f;
	mMaxRange = 100000.f;

	// 일반 패턴(0)보다 높아서 조건이 되면 반드시 이것만 후보에 남는다.
	mPriority = 10;

	// 쿨다운 없음 (체력 기준으로만 나온다)
	mCoolDownTime = 0.f;

	// 극적인 패턴이라 원래 속도
	mMontagePlayRate = 1.f;

	mLungeSpeed = 0.f;

	// 후딜 2초 ± 0.3 : 필살기를 버틴 보상 = 큰 반격 틈
	mRecoveryTime = 2.f;
	mRecoveryRandom = 0.3f;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_Ultimate.AM_Sevarog_Ultimate'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;
}

bool UGameplayAbility_Pattern_Ultimate::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	// const ABossBase* : "읽기만 하는" 보스 포인터. CanUsePattern이 const 함수라 보스 값을 바꾸지 않는다.
	const ABossBase* BossBase = Cast<ABossBase>(Boss);

	return IsValid(BossBase) && IsValid(Target) && BossBase->HasPendingUltimate();
}

void UGameplayAbility_Pattern_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	mLanded = false;
	mTargetPhase = 0;
	mPlayVictory = false;

	// "필살기 대기"를 꺼내 쓴다. 끝나면 들어갈 페이즈를 함께 받는다.
	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());

	if (IsValid(Boss))
	{
		mTargetPhase = Boss->ConsumePendingUltimate();

		// Victory 페이즈를 "이번에 처음 넘는" 필살기만 Victory를 한다. (75%·50%를 한 번에 넘어도 한다)
		mPlayVictory = Boss->GetPhase() < mVictoryPhase && mTargetPhase >= mVictoryPhase;
	}

	// 부모 : 멈춤 → 바라보기 → 몽타주 재생 (Default 섹션 = 도약부터)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive())
		return;

	UE_LOG(UGDebug, Warning, TEXT("Ultimate start (→ Phase %d, Victory : %s)"), mTargetPhase,
		mPlayVictory ? TEXT("Yes") : TEXT("No"));

	// 이륙 노티파이 (도약 패턴과 같은 태그)
	UAbilityTask_WaitGameplayEvent* WaitTakeoff = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.LeapTakeoff")), nullptr, true);

	WaitTakeoff->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Ultimate::OnTakeoffEvent);
	WaitTakeoff->ReadyForActivation();

	// 충격 노티파이
	UAbilityTask_WaitGameplayEvent* WaitImpact = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.UltimateImpact")), nullptr, true);

	WaitImpact->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Ultimate::OnImpactEvent);
	WaitImpact->ReadyForActivation();
}

void UGameplayAbility_Pattern_Ultimate::OnTakeoffEvent(FGameplayEventData Payload)
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	// 아레나 중앙으로 몸을 돌리고 뛴다. (매번 같은 곳에서 시전해야 기둥 위치를 배울 수 있다)
	FVector	ToCenter = Boss->GetArenaCenter() - Boss->GetActorLocation();
	ToCenter.Z = 0.0;

	if (!ToCenter.IsNearlyZero())
		Boss->SetActorRotation(FRotator(0.0, ToCenter.Rotation().Yaw, 0.0));

	float	AirTime = ToRealTime(mAirTime);

	UAbilityTask_ApplyRootMotionJumpForce* Jump =
		CreateJumpToTask(Boss->GetArenaCenter(), mJumpHeight, AirTime);

	if (!Jump)
		return;

	// 날아가는 동안 기둥을 통과한다. (기둥 위에 올라가서 시전하던 버그, 10/15)
	Boss->SetIgnorePillars(true);

	Jump->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Ultimate::OnLanded);
	Jump->ReadyForActivation();
}

void UGameplayAbility_Pattern_Ultimate::OnLanded()
{
	if (mLanded)
		return;

	mLanded = true;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	Boss->SetIgnorePillars(false);

	// 안전장치 : 어떤 이유로든 중앙에서 mCenterTolerance보다 멀리 착지했으면 중앙 바닥으로 순간이동.
	// 필살기는 "매번 같은 곳에서 시전"해야 기둥 위치를 배울 수 있으므로 반드시 중앙.
	// (11/6) 툭 옮기지 않고 망령 순간이동과 같은 모습으로 : 그림자를 남기며 사라짐 → 중앙에서 나타남.
	// 플레이어가 3페이즈 망령 패턴에서 이미 배운 "세바로그가 이렇게 사라지면 다른 곳에 나타난다"를 그대로 쓴다
	if (FVector::Dist2D(Boss->GetActorLocation(), Boss->GetArenaCenter()) > mCenterTolerance)
	{
		Boss->StartVanish(mRelocateVanishTime);

		UAbilityTask_WaitDelay* WaitVanish = UAbilityTask_WaitDelay::WaitDelay(this, mRelocateVanishTime);

		WaitVanish->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Ultimate::OnRelocateVanished);
		WaitVanish->ReadyForActivation();
		return;
	}

	BeginCharge();
}

void UGameplayAbility_Pattern_Ultimate::OnRelocateVanished()
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	Boss->TeleportToGround(Boss->GetArenaCenter());
	Boss->StartAppear(mRelocateAppearTime);

	BeginCharge();
}

void UGameplayAbility_Pattern_Ultimate::BeginCharge()
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	// ---------------------------------------------------------------- 바닥 전체 붉은 원
	// 중앙부터 벽까지 mImpactDelay초 동안 차오른다. 다 차는 순간 = 충격. (도약·돌진과 같은 "다 차면 위험" 규칙)
	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Ground = Boss->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	// 부모 함수 (11/4) : 미리 불러 둔 머티리얼로 원 예고
	SpawnWarningCircle(Ground, mFloorWarningRadius, mImpactDelay);

	// 보스 가슴의 붉은 빛 (10/22) : 바닥 원과 같은 시간 동안 차오른다. 기둥 뒤에 그림자 = 안전지대.
	mLight = Boss->SpawnUltimateLight(mTraceHeightOffset, mImpactDelay);
	Boss->PlayUltimateChargeEffect();

	// ---------------------------------------------------------------- Slam 타이머
	// 충격이 "착지 후 정확히 mImpactDelay초"가 되도록, 망치를 드는 시간만큼 먼저 Slam을 시작한다.
	//   예) 5초 - 2.54초 = 착지 후 2.46초에 Slam 시작 (그 전까지는 손 뻗기 → 기 모으기 반복)
	float	SlamWindup = ToRealTime(mSlamImpactTime);
	float	WaitTime = FMath::Max(mImpactDelay - SlamWindup, 0.f);

	UAbilityTask_WaitDelay* WaitSlam = UAbilityTask_WaitDelay::WaitDelay(this, WaitTime);

	WaitSlam->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Ultimate::OnStartSlam);
	WaitSlam->ReadyForActivation();
}

void UGameplayAbility_Pattern_Ultimate::OnStartSlam()
{
	// 내려찍기 직전에 기디언을 바라본다. (어디서 맞든 결과는 같지만, 보스가 플레이어를 노리는 느낌)
	FaceTarget();

	JumpToSection(mSlamSectionName);
}

void UGameplayAbility_Pattern_Ultimate::OnImpactEvent(FGameplayEventData Payload)
{
	// 충격 순간 : 빛이 번쩍이고(스스로 꺼지며 사라짐), 발밑 충격파 이펙트 (10/22)
	if (IsValid(mLight))
		mLight->Flash();

	mLight = nullptr;

	if (ABossBase* ImpactBoss = Cast<ABossBase>(GetAvatarActorFromActorInfo()))
		ImpactBoss->PlayUltimateImpactEffect();

	// (빛을 먼저 번쩍이고 손을 놓은 뒤에 지운다 — DestroyFloorDecal은 남아 있는 빛도 지우기 때문)
	DestroyFloorDecal();

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	ACharacter* Target = IsValid(Boss) ? Cast<ACharacter>(Boss->GetTarget()) : nullptr;

	// ---------------------------------------------------------------- 엄폐 판정
	if (IsValid(Target))
	{
		if (IsTargetCovered(Boss, Target))
		{
			UE_LOG(UGDebug, Warning, TEXT("Ultimate : covered (survived)"));
		}
		else
		{
			// 즉사 : 아주 큰 피해 + "막을 수 없음" → 회피 무적이어도 들어간다. (HP는 0에서 잘린다)
			UE_LOG(UGDebug, Warning, TEXT("Ultimate : exposed (lethal)"));

			FHitResult	Hit;
			Hit.ImpactPoint = Target->GetActorLocation();

			Boss->ApplyDamageToTarget(Target, 99999.f, Hit, true);
		}

		// 충격파 이펙트(P_UltMeshArc_Expanding)·사운드는 11/3 작업에서 추가
	}

	// 타격감 (10/21) : 아레나 전체에 충격 → 기둥 뒤에서 살아남아도 강하게 흔들린다.
	// (즉사했으면 전투가 이미 끝나서 게임 모드가 무시한다)
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();

	if (IsValid(GameMode))
		GameMode->PlayHitFeedback(EHitFeedback::UltimateImpact);

	// ---------------------------------------------------------------- 3페이즈로 가는 필살기(50%) : Victory로
	if (mPlayVictory)
	{
		// Slam 섹션이 끝나기 0.1초 전에 넘긴다. (끝까지 가면 다음 섹션이 없어서 몽타주가 끝나 버린다)
		float	Remain = ToRealTime(mSlamLength - mSlamImpactTime) - 0.1f;

		UAbilityTask_WaitDelay* WaitVictory = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(Remain, 0.f));

		WaitVictory->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Ultimate::OnStartVictory);
		WaitVictory->ReadyForActivation();
	}
}

void UGameplayAbility_Pattern_Ultimate::OnStartVictory()
{
	// 망치로 기디언을 가리키도록 돌아선다.
	FaceTarget();

	JumpToSection(mVictorySectionName);

	// 실제 페이즈 진입(크기·공격력·Boss.Phase 태그)은 Victory가 끝나는 EndAbility에서 한다.
	UE_LOG(UGDebug, Warning, TEXT("Ultimate : phase shift (Victory)"));
}

bool UGameplayAbility_Pattern_Ultimate::IsTargetCovered(ABossBase* Boss, AActor* Target) const
{
	// 시작 : 보스 가슴 높이 / 끝 : 기디언 캡슐 중심 (몸 중심 1점)
	FVector	Start = Boss->GetActorLocation() + FVector(0.0, 0.0, mTraceHeightOffset);
	FVector	End = Target->GetActorLocation();

	// WorldStatic만 검사한다 : 기둥 판정(BO_Pillar)·벽. 보이는 기둥 메시는 NoCollision이라 상관없다.
	// 보스·기디언 자신은 무시.
	FHitResult					Hit;
	FCollisionObjectQueryParams	ObjectParams(ECollisionChannel::ECC_WorldStatic);
	FCollisionQueryParams		Params(NAME_None, false, Boss);
	Params.AddIgnoredActor(Target);

	bool	bBlocked = GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, Params);

#if ENABLE_DRAW_DEBUG
	// 초록 = 막힘(생존), 빨강 = 뚫림(즉사). 3초간 표시
	if (Boss->ShouldDrawDebug())
	{
		DrawDebugLine(GetWorld(), Start, bBlocked ? Hit.ImpactPoint : End,
			bBlocked ? FColor::Green : FColor::Red, false, 3.f, 0, 3.f);
	}
#endif

	if (bBlocked)
		UE_LOG(UGDebug, Warning, TEXT("Ultimate : blocked by %s"), *GetNameSafe(Hit.GetActor()));

	return bBlocked;
}

void UGameplayAbility_Pattern_Ultimate::JumpToSection(FName SectionName)
{
	ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInstance = IsValid(Boss) ? Boss->GetMesh()->GetAnimInstance() : nullptr;

	if (IsValid(AnimInstance))
		AnimInstance->Montage_JumpToSection(SectionName, mMontage);
}

void UGameplayAbility_Pattern_Ultimate::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	DestroyFloorDecal();

	// 날아가는 도중에 끊겨도(보스 사망 등) 기둥 충돌을 되돌린다.
	if (ABossBase* PillarBoss = Cast<ABossBase>(GetAvatarActorFromActorInfo()))
	{
		PillarBoss->SetIgnorePillars(false);

		// (11/6) 중앙으로 옮기려고 사라진 채 끊겼으면 다시 보이게 (망령 패턴과 같은 안전장치)
		if (PillarBoss->IsVanished())
			PillarBoss->StartAppear(0.f);
	}

	// 필살기가 끝나면(50%는 Victory까지) 다음 페이즈 시작 → 커지고 강해진다.
	// IsActive() : EndAbility가 두 번 불려도(몽타주 정지 알림 등) 한 번만. 중간에 끊겨도 페이즈는 건너뛰지 않는다.
	if (IsActive() && mTargetPhase > 0)
	{
		ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

		if (IsValid(Boss))
			Boss->EnterPhase(mTargetPhase);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGameplayAbility_Pattern_Ultimate::DestroyFloorDecal()
{
	// 바닥 원 : 부모 함수 (11/4)
	DestroyWarningDecal();

	// 충격 전에 끊기면(보스 사망 등) 빛도 지운다. (충격이 지나간 빛은 이미 손을 놓아서 nullptr)
	if (IsValid(mLight))
		mLight->Destroy();

	mLight = nullptr;
}
