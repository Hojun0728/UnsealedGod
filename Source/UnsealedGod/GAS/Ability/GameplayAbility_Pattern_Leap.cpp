// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Leap.h"
#include "../../Boss/BossBase.h"
#include "../../GameMode/ArenaGameMode.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
// 정해진 거리·높이·시간으로 캐릭터를 포물선으로 이동시키는 GAS 태스크
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionJumpForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UGameplayAbility_Pattern_Leap::UGameplayAbility_Pattern_Leap()
{
	// 사용 거리 : 6m 이상 떨어졌을 때. (3연타는 5m 이내라 겹치지 않는다)
	// 최대 30m : 아레나(지름 40m) 어디로 도망가도 쫓아온다.
	mMinRange = 600.f;
	mMaxRange = 3000.f;

	mWeight = 1.f;

	// 쿨다운 3초. 쿨다운은 "도약 시작"부터 세므로, 공중 약 1.7초 + 착지 후 쉬기(BT Wait)를 빼면
	// 착지 후 약 1초만 지나면 다시 쓸 수 있다. → 계속 거리를 벌리면 계속 날아온다. (10/14 직접 결정)
	mCoolDownTime = 3.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossLeap"));

	// 밀기(Lunge)는 쓰지 않는다. 이동은 이륙 이벤트의 점프가 담당.
	mLungeSpeed = 0.f;

	// 재생 속도 1.5배 (10/14 : 원래 속도는 너무 느렸음). 공중 시간도 이 속도로 나눠서 맞춘다.
	mMontagePlayRate = 1.5f;

	// 빠른 페이즈(3페이즈~) 2배속 (10/15). 공중 시간·예고 원도 같이 짧아진다 : 1.55 ÷ 2 ≈ 0.78초
	mFastPlayRate = 2.f;

	// 후딜 0 : 착지하자마자 다음 패턴(가까우니까 3연타)으로 이어진다. 도약은 "응징" 패턴이라 쉬지 않는다.
	mRecoveryTime = 0.f;
	mRecoveryRandom = 0.f;

	// 몽타주. 에디터에서 Content/UnsealedGod/Boss/AM_Sevarog_Leap로 만든다.
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_Leap.AM_Sevarog_Leap'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;
}

void UGameplayAbility_Pattern_Leap::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 같은 어빌리티 객체를 재사용하므로(InstancedPerActor) 지난 도약의 상태를 먼저 비운다.
	mLanded = false;

	// 부모 : 쿨다운 확정 → 멈춤 → 바라보기 → 몽타주 재생
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 부모 단계에서 실패해 이미 끝났으면 더 하지 않는다.
	// IsActive() : 이 어빌리티가 지금 실행 중인지 (EndAbility가 불렸으면 false)
	if (!mAbilityActive || !IsActive())
		return;

	// 이륙 노티파이를 한 번만 기다린다. (OnlyTriggerOnce = true)
	UAbilityTask_WaitGameplayEvent* WaitTakeoff = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.LeapTakeoff")), nullptr, true);

	WaitTakeoff->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Leap::OnTakeoffEvent);
	WaitTakeoff->ReadyForActivation();
}

void UGameplayAbility_Pattern_Leap::OnTakeoffEvent(FGameplayEventData Payload)
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	AActor* Target = Boss->GetTarget();

	if (!IsValid(Target))
		return;

	// ---------------------------------------------------------------- ① 착지 지점
	// 보스 → 기디언 수평 방향과 거리. (높이 차이는 무시)
	FVector	BossLocation = Boss->GetActorLocation();
	FVector	ToTarget = Target->GetActorLocation() - BossLocation;
	ToTarget.Z = 0.0;

	// GetSafeNormal : 길이 1인 방향 벡터. 길이가 0이면(같은 위치) 0벡터를 돌려줘서 계산이 터지지 않는다.
	FVector	Direction = ToTarget.GetSafeNormal();

	// 기디언 몸 위에 떨어지지 않도록 mLandOffset만큼 앞에 착지한다.
	float	JumpDistance = FMath::Max(ToTarget.Size() - mLandOffset, 0.f);

	// 착지 지점의 바닥 높이 = 보스 캡슐 바닥. (아레나 바닥은 평평하다)
	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	LandGround = BossLocation + Direction * JumpDistance - FVector(0.0, 0.0, HalfHeight);

	// (11/6) 착지 지점이 기둥 안이면(기디언이 기둥에 붙어 있을 때) 보스 쪽으로 당긴 빈 바닥에 착지.
	// 날아가는 동안 기둥을 통과하므로, 기둥 안에 내려앉으면 충돌이 몸을 밀어내 기둥 위로 올라가 버렸다
	LandGround = Boss->FindOpenGround(LandGround, BossLocation);

	// 착지할 방향을 바라본다. (공중 회전 애니메이션이 끝나면 이쪽을 보고 서 있게)
	FaceTarget();

	// 실제 공중 시간 = 몽타주 기준 시간 ÷ 재생 속도
	float	AirTime = ToRealTime(mAirTime);

	// ---------------------------------------------------------------- ② 예고 데칼
	// 부모 함수 (11/4) : 미리 불러 둔 머티리얼로 원 예고. 패턴이 끝나면 부모 EndAbility가 지운다
	SpawnWarningCircle(LandGround, mImpactRadius, AirTime);

	// ---------------------------------------------------------------- ③ 포물선 이동
	// 부모의 공용 함수로 점프 태스크를 만든다. (필살기도 같은 함수로 중앙까지 뛴다)
	// 기둥 등에 걸려 일찍 땅에 닿으면 그 순간 끝난다 → 그 순간 피해.
	UAbilityTask_ApplyRootMotionJumpForce* Jump =
		CreateJumpToTask(LandGround, mJumpHeight, AirTime);

	if (!Jump)
		return;

	// 날아가는 동안 기둥을 통과한다. (경로에 기둥이 있으면 기둥 위에 착지하던 버그, 10/15)
	Boss->SetIgnorePillars(true);

	// OnFinish : 시간이 다 됐거나 땅에 닿아서 이동이 끝났을 때 (둘 중 먼저 오는 쪽)
	Jump->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Leap::OnLanded);
	Jump->ReadyForActivation();
}

void UGameplayAbility_Pattern_Leap::OnLanded()
{
	if (mLanded)
		return;

	mLanded = true;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (IsValid(Boss))
	{
		// 착지했으니 기둥 충돌을 원래대로. (기둥과 겹친 채 착지했으면 엔진이 밖으로 밀어낸다)
		Boss->SetIgnorePillars(false);

		// 실제로 떨어진 위치의 바닥을 중심으로 범위 피해. ("보이는 대로")
		float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FVector	Ground = Boss->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

		// 10/16 : ApplyAreaDamage가 "배율"이 아니라 "피해량"을 받도록 바뀌어서 공격력을 곱해 넘긴다.
		// 마지막 true : 큰 공격 → 맞은 기디언이 넘어진다.
		Boss->ApplyAreaDamage(Ground, mImpactRadius, Boss->GetAttack() * mImpactDamageScale, true);

		// 타격감 (10/21) : 피해도 착지 위치 기준으로 땅이 울린다. (맞았으면 기디언 쪽 큰 피격이 따로 온다)
		AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();

		if (IsValid(GameMode))
			GameMode->PlayHitFeedback(EHitFeedback::LeapLand, Ground);

		// (10/26) 착지 소리도 위 LeapLand 사건 하나로 게임 모드 표에서 같이 난다.
	}

	DestroyWarningDecal();

	// 착지 경직 mLandDelay 뒤에 패턴을 끝낸다. (착지 동작을 끝까지 보여 주지 않고 바로 다음 패턴으로)
	// WaitDelay : 정해진 시간 뒤에 OnFinish를 알려 주는 GAS 태스크
	UAbilityTask_WaitDelay* LandDelay = UAbilityTask_WaitDelay::WaitDelay(this, mLandDelay);

	LandDelay->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Leap::OnLandDelayFinished);
	LandDelay->ReadyForActivation();
}

void UGameplayAbility_Pattern_Leap::OnLandDelayFinished()
{
	// 어빌리티가 끝나면 몽타주 재생 태스크가 몽타주를 멈춘다(블렌드 아웃 0.25초).
	// 후딜이 0이라 BT가 바로 다음 패턴을 고른다 → 블렌드 아웃 중에 3연타가 시작되어 자연스럽게 이어진다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGameplayAbility_Pattern_Leap::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 착지 전에 끊겼으면(보스 사망 등) 기둥 충돌을 되돌린다. (바닥 예고는 부모 EndAbility가 지운다)
	if (ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo()))
		Boss->SetIgnorePillars(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
