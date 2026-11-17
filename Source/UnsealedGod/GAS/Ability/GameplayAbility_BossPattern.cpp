// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_BossPattern.h"
#include "../../Boss/BossBase.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
// 정해진 방향·세기로 일정 시간 캐릭터를 미는 GAS 태스크 (CharacterMovement가 벽 충돌까지 처리)
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
// 거리·높이·시간으로 포물선 점프를 시키는 GAS 태스크 (도약·필살기 공용)
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "../../Boss/BossWarningDecal.h"

UGameplayAbility_BossPattern::UGameplayAbility_BossPattern()
{
	// (11/4) 바닥 예고 머티리얼(원)을 미리 불러 둔다. 돌진 계열은 자식 생성자에서 선 머티리얼로 바꾼다.
	// 예고를 쓰지 않는 패턴(3연타 등)도 갖고 있지만, 에셋은 하나라 메모리는 한 번만 쓴다. (같은 에셋을 가리킬 뿐)
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>
		DecalMat(TEXT("/Script/Engine.Material'/Game/UnsealedGod/Boss/M_Decal_Warning.M_Decal_Warning'"));

	if (DecalMat.Succeeded())
		mDecalMaterial = DecalMat.Object;
}

bool UGameplayAbility_BossPattern::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	if (!IsValid(Boss) || !IsValid(Target))
		return false;

	// 바닥 평면(XY) 거리. 높이 차이는 무시한다. (보스는 떠 있고, 기디언은 점프할 수 있으므로)
	float	Distance = FVector::Dist2D(Boss->GetActorLocation(), Target->GetActorLocation());

	return Distance >= mMinRange && Distance <= mMaxRange;
}

void UGameplayAbility_BossPattern::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 이번 재생 속도 : 빠른 페이즈(3페이즈 이상)이고 빠른 속도가 정해져 있으면 그 값, 아니면 평소 값
	// (자식 클래스의 시간 계산(ToRealTime)이 이 값을 쓰므로 가장 먼저 정한다)
	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());

	bool	bFast = IsValid(Boss) && Boss->IsFastPhase();

	mPlayRate = (bFast && mFastPlayRate > 0.f) ? mFastPlayRate : mMontagePlayRate;

	// 부모 : 비용·쿨다운 확정 (실패하면 부모가 이미 EndAbility를 불렀다)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	if (!IsValid(Boss) || !IsValid(mMontage))
	{
		UE_LOG(UGDebug, Warning, TEXT("BossPattern : Invalid Boss or Montage (%s)"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 추적(Move To) 중이었으면 멈춘다. 공격하면서 미끄러지지 않게.
	AAIController* AIController = Cast<AAIController>(Boss->GetController());

	if (IsValid(AIController))
		AIController->StopMovement();

	// 공격 시작 시 타겟 쪽을 본다.
	FaceTarget();

	// ---------------------------------------------------------------- 태스크 1 : 밀기 이벤트 (여러 번)
	// OnlyTriggerOnce = false : 3연타처럼 한 패턴 안에서 밀기가 여러 번 올 수 있다.
	UAbilityTask_WaitGameplayEvent* WaitLunge = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.Lunge")), nullptr, false);

	WaitLunge->EventReceived.AddDynamic(this, &UGameplayAbility_BossPattern::OnLungeEvent);
	WaitLunge->ReadyForActivation();

	// ---------------------------------------------------------------- 태스크 2 : 몽타주 재생·대기
	// 부모(UGameplayAbility_Base) : 끝나거나 끊기면 OnMontageFinished → 어빌리티 종료 (플레이어 스킬과 같은 함수)
	PlayMontageAndEnd(mMontage, mPlayRate);
}

void UGameplayAbility_BossPattern::FaceTarget(float YawOffset)
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	AActor* Target = Boss->GetTarget();

	if (!IsValid(Target))
		return;

	// 보스 → 타겟 방향의 회전값. 위아래(Pitch)는 0으로 두고 좌우(Yaw)만 돌린다.
	FVector		ToTarget = Target->GetActorLocation() - Boss->GetActorLocation();
	FRotator	LookRotation = ToTarget.Rotation();

	// YawOffset을 더해서, 무기가 비스듬히 떨어지는 동작도 타겟 위에 떨어지게 한다.
	Boss->SetActorRotation(FRotator(0.0, LookRotation.Yaw + YawOffset, 0.0));
}

UAbilityTask_ApplyRootMotionJumpForce* UGameplayAbility_BossPattern::CreateJumpToTask(
	const FVector& Destination, float Height, float AirTime)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();

	if (!IsValid(Avatar))
		return nullptr;

	// 수평 방향과 거리 (높이 차이는 무시)
	FVector	ToDestination = Destination - Avatar->GetActorLocation();
	ToDestination.Z = 0.0;

	float		Distance = ToDestination.Size();
	FRotator	Direction = Distance > 1.f ? ToDestination.Rotation() : Avatar->GetActorRotation();

	// 인자 : 소유 어빌리티, 이름, 방향(회전값), 거리, 높이, 시간,
	//        최소 착지 판정 시간, 착지하면 끝낼지, 끝날 때 속도 처리, 끝날 때 속도, 속도 제한, 경로 곡선, 시간 곡선
	// 최소 착지 판정 시간 = 공중 시간의 절반 : 이륙 직후 아직 땅에 붙어 있는 것을 "착지"로 오해하지 않게.
	// bFinishOnLanded = true : 기둥 등에 걸려 일찍 땅에 닿으면 그 순간 끝난다.
	return UAbilityTask_ApplyRootMotionJumpForce::ApplyRootMotionJumpForce(
		this, NAME_None, Direction, Distance, Height, AirTime,
		AirTime * 0.5f, true, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector,
		0.f, nullptr, nullptr);
}

void UGameplayAbility_BossPattern::OnLungeEvent(FGameplayEventData Payload)
{
	// 휘두를 때마다 타겟을 다시 바라본다. (플레이어가 옆으로 움직여도 따라오는 "추적 공격")
	// 노티파이에 각도를 넣었으면 그만큼 더 돌린다. (예 : 3연타 3타는 망치가 비스듬히 떨어짐)
	FaceTarget(Payload.EventMagnitude);

	if (mLungeSpeed <= 0.f || mLungeDuration <= 0.f)
		return;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	// 최대 밀기 거리 = 속도 × 시간
	float	LungeDistance = mLungeSpeed * mLungeDuration;

	// 적정 타격 거리가 있으면 "지금 거리 - 적정 거리"만큼만 민다.
	// Clamp(값, 최소, 최대) : 값을 최소~최대 사이로 자른다. 이미 가까우면 음수 → 0 (밀지 않음).
	AActor* Target = Boss->GetTarget();

	if (mStrikeDistance > 0.f && IsValid(Target))
	{
		float	Distance = FVector::Dist2D(Boss->GetActorLocation(), Target->GetActorLocation());

		LungeDistance = FMath::Clamp(Distance - mStrikeDistance, 0.f, LungeDistance);
	}

	// 1cm 미만이면 밀 필요가 없다.
	if (LungeDistance < 1.f)
		return;

	// 몽타주를 빠르게 재생하면 밀기 시간도 그만큼 줄인다. (1.2배속 → 0.25초 / 1.2 ≈ 0.21초)
	// FMath::Max : 재생 속도가 0이 되어 0으로 나누는 실수를 막는다.
	float	LungeDuration = ToRealTime(mLungeDuration);

	// 시간은 애니메이션에 맞추고 속도를 거리에 맞춘다. → 애니메이션과 밀기의 타이밍이 항상 같다.
	float	LungeSpeed = LungeDistance / LungeDuration;

	// 바라보는 방향으로 일정 시간 민다. (속도 × 시간 = 거리)
	// 인자 : 소유 어빌리티, 이름, 방향, 세기(cm/초), 시간(초), 기존 이동에 더할지, 세기 곡선(없음),
	//        끝날 때 속도 처리, 끝날 때 속도, 속도 제한, 중력 사용
	// SetVelocity + ZeroVector : 밀기가 끝나면 바로 멈춘다. (미끄러지지 않게)
	UAbilityTask_ApplyRootMotionConstantForce* Lunge =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, NAME_None, Boss->GetActorForwardVector(), LungeSpeed, LungeDuration,
			false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector,
			0.f, true);

	Lunge->ReadyForActivation();
}

void UGameplayAbility_BossPattern::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// (11/4) 바닥 예고가 남아 있으면 지운다. 정상 종료·끊김(보스 사망·전투 끝) 모두. 자식은 따로 지우지 않아도 된다.
	DestroyWarningDecal();

	// IsActive() : 아직 실행 중일 때만. (몽타주 정지 알림 등으로 EndAbility가 두 번 불려도 후딜은 한 번만 정한다)
	if (IsActive())
	{
		ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

		if (IsValid(Boss))
		{
			// 빠른 페이즈이고 빠른 후딜이 정해져 있으면(0 이상) 그 값을 쓴다.
			bool	bFast = Boss->IsFastPhase();
			float	Time = (bFast && mFastRecoveryTime >= 0.f) ? mFastRecoveryTime : mRecoveryTime;
			float	Random = (bFast && mFastRecoveryRandom >= 0.f) ? mFastRecoveryRandom : mRecoveryRandom;

			// FRandRange(최소, 최대) : 그 사이의 랜덤 실수. 후딜이 매번 조금씩 달라 리듬을 외우기 어렵게.
			float	Recovery = FMath::FRandRange(FMath::Max(Time - Random, 0.f), Time + Random);

			Boss->StartPatternRecovery(Recovery);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGameplayAbility_BossPattern::SpawnWarningCircle(const FVector& Location, float Radius, float Duration)
{
	// SpawnActor<클래스>(위치, 회전) : 월드에 액터를 새로 만든다. (수업 코드 WukongProjectile과 같은 방식)
	mWarningDecal = GetWorld()->SpawnActor<ABossWarningDecal>(Location, FRotator::ZeroRotator);

	if (IsValid(mWarningDecal))
		mWarningDecal->StartWarning(mDecalMaterial, Radius, Duration);
}

void UGameplayAbility_BossPattern::SpawnWarningBox(const FVector& Center, const FRotator& Rotation,
	float HalfLength, float HalfWidth, float Duration)
{
	// 회전값의 앞(X) 방향 = 사각형의 길이 방향 (ABossWarningDecal::StartWarningBox 설명 참고)
	mWarningDecal = GetWorld()->SpawnActor<ABossWarningDecal>(Center, Rotation);

	if (IsValid(mWarningDecal))
		mWarningDecal->StartWarningBox(mDecalMaterial, HalfLength, HalfWidth, Duration);
}

void UGameplayAbility_BossPattern::DestroyWarningDecal()
{
	if (IsValid(mWarningDecal))
		mWarningDecal->Destroy();

	mWarningDecal = nullptr;
}
