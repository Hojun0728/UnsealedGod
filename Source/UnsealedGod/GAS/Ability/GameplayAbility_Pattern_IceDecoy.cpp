// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_IceDecoy.h"
#include "../../Boss/Aurora.h"
#include "../../Boss/IceDecoy.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "EngineUtils.h"

UGameplayAbility_Pattern_IceDecoy::UGameplayAbility_Pattern_IceDecoy()
{
	// 0~14m (10/28 밸런스 : 원거리에서도 얼음 폭탄이 걸어와 1페이즈가 바빠지게. 처음엔 6m 안에 붙었을 때만)
	mMinRange = 0.f;
	mMaxRange = 1400.f;

	mWeight = 1.f;

	mCoolDownTime = 16.f;	// 10/28 밸런스 : 12 → 10 → 16초 (사용 거리 14m라 너무 자주 나왔음)
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossIceDecoy"));

	mMontagePlayRate = 1.f;
	mLungeSpeed = 0.f;

	// 착지 후 잠깐 (분신이 걸어가는 동안 오로라는 다음 패턴을 준비)
	mRecoveryTime = 0.5f;
	mRecoveryRandom = 0.2f;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_IceDecoy.AM_Aurora_IceDecoy'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;
}

bool UGameplayAbility_Pattern_IceDecoy::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	if (!Super::CanUsePattern(Boss, Target))
		return false;

	// TActorIterator : 레벨의 분신을 하나씩 본다. 숨지 않은(부서지지 않은) 분신이 있으면 쓰지 않는다. (분신은 한 번에 하나)
	for (TActorIterator<AIceDecoy> It(Boss->GetWorld()); It; ++It)
	{
		if (!It->IsHidden())
			return false;
	}

	return true;
}

void UGameplayAbility_Pattern_IceDecoy::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 부모 : 멈춤 → 기디언 쪽 바라보기 → 몽타주 재생 (뒤로 공중제비이므로 기디언을 본 채 뒤로 넘어간다)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive())
		return;

	UAbilityTask_WaitGameplayEvent* WaitTakeoff = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.LeapTakeoff")), nullptr, true);

	WaitTakeoff->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_IceDecoy::OnTakeoffEvent);
	WaitTakeoff->ReadyForActivation();
}

void UGameplayAbility_Pattern_IceDecoy::OnTakeoffEvent(FGameplayEventData Payload)
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Boss) || !IsValid(Target))
		return;

	FVector	BossLocation = Boss->GetActorLocation();
	FVector	ToTarget = Target->GetActorLocation() - BossLocation;
	ToTarget.Z = 0.0;

	FVector	Direction = ToTarget.GetSafeNormal();

	// ---------------------------------------------------------------- 1. 분신 생성 (오로라 자리, 기디언 쪽으로 살짝 앞)
	// (10/28) 1페이즈 1개 / 2페이즈 2개 : 좌우로 갈라져 나오고(하나가 둘로 갈라지는 그림), 걷는 속도를 조금 달리해 도착이 어긋나게 한다.
	// AlwaysSpawn : 겹쳐도 무조건 만든다. 바로 아래에서 서로 부딪히지 않게 해 둔다.
	FActorSpawnParameters	SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector	Center = BossLocation + Direction * mDecoyForwardOffset;

	// Right : 기디언 쪽 방향에 수직인 오른쪽. (위쪽 축과 외적 = 두 방향 모두에 수직인 방향)
	FVector	Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();

	bool	bTwin = Boss->GetPhase() >= 2;
	int32	Count = bTwin ? 2 : 1;

	for (int32 i = 0; i < Count; ++i)
	{
		// 2개면 왼쪽(-1)·오른쪽(+1)으로 mTwinSideOffset씩. 1개면 가운데.
		float	Side = bTwin ? (i == 0 ? -1.f : 1.f) : 0.f;

		AIceDecoy* Decoy = GetWorld()->SpawnActor<AIceDecoy>(Center + Right * (Side * mTwinSideOffset),
			Direction.Rotation(), SpawnParams);

		if (!IsValid(Decoy))
			continue;

		// 폭발 피해 = 보스의 지금 공격력 × 배율 (2페이즈 1.2배도 따라간다)
		Decoy->SetAttackFrom(Boss->GetAttack());

		// 얼음 머티리얼 : 오로라 디테일의 값 (빌드 없이 바꿔 볼 수 있게)
		if (AAurora* Aurora = Cast<AAurora>(Boss))
			Decoy->SetIceMaterial(Aurora->GetDecoyMaterial());

		// 2개면 걷는 속도를 다르게 (380 / 420). 1개면 기본값(400) 그대로.
		if (bTwin && mTwinWalkSpeeds.IsValidIndex(i))
			Decoy->SetWalkSpeed(mTwinWalkSpeeds[i]);

		// MoveIgnoreActorAdd : 움직일 때 이 액터와는 부딪히지 않는다. (오로라가 뛰어오를 때 분신 몸에 걸리지 않게)
		Boss->MoveIgnoreActorAdd(Decoy);
		Decoy->MoveIgnoreActorAdd(Boss);
	}

	// ---------------------------------------------------------------- 2. 뒤로 도약 (기디언 반대쪽)
	// 부모의 공용 포물선 점프 (세바로그 도약·필살기와 같은 함수). 공중 시간은 실제 시간으로.
	UAbilityTask_ApplyRootMotionJumpForce* Jump =
		CreateJumpToTask(BossLocation - Direction * mRetreatDistance, mJumpHeight, ToRealTime(mAirTime));

	if (Jump)
		Jump->ReadyForActivation();
}
