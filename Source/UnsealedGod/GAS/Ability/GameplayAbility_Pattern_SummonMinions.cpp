// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_SummonMinions.h"
#include "../../Boss/BossBase.h"
#include "../../Minion/Minion.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_SummonMinions::UGameplayAbility_Pattern_SummonMinions()
{
	// 거리 조건 없음 (보스의 "소환 대기"로만 판단)
	mMinRange = 0.f;
	mMaxRange = 100000.f;

	// 일반 패턴(0)보다 높고 필살기(10)보다 낮다. → 조건이 되면 지금 패턴 다음에 반드시, 필살기가 겹치면 필살기 먼저
	mPriority = 5;

	mCoolDownTime = 0.f;

	mMontagePlayRate = 1.f;
	mLungeSpeed = 0.f;

	// 소환 후 짧게 쉬고 바로 공격 재개 (미니언과 보스가 함께 압박)
	mRecoveryTime = 0.3f;
	mRecoveryRandom = 0.f;

	mMinionClass = AMinion::StaticClass();

	// 소환 몽타주 (Cast, 왼손을 펼침) = 부모의 mMontage. (11/4 : 실행 중 경로로 → 생성자에서 미리)
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_Summon.AM_Sevarog_Summon'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		SummonSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Summon.S_SFX_Summon'"));

	if (SummonSnd.Succeeded())
		mSummonSound = SummonSnd.Object;
}

bool UGameplayAbility_Pattern_SummonMinions::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	const ABossBase* BossBase = Cast<ABossBase>(Boss);

	return IsValid(BossBase) && BossBase->CanSummonMinions();
}

void UGameplayAbility_Pattern_SummonMinions::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// "소환 대기"를 꺼내 쓴다. (몇 마리인지)
	mSummonCount = 0;

	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());

	if (IsValid(Boss))
		mSummonCount = Boss->ConsumePendingSummon();

	// 부모 : 멈춤 → 바라보기 → 몽타주 재생
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive())
		return;

	UE_LOG(UGDebug, Warning, TEXT("Summon start : %d minions"), mSummonCount);

	UAbilityTask_WaitGameplayEvent* WaitSummon = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.Summon")), nullptr, true);

	WaitSummon->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_SummonMinions::OnSummonEvent);
	WaitSummon->ReadyForActivation();
}

void UGameplayAbility_Pattern_SummonMinions::OnSummonEvent(FGameplayEventData Payload)
{
	if (!mMinionClass || mSummonCount <= 0)
		return;

	// 소환 지점 찾기 (액터 태그 "MinionSpawn"). 없으면 보스 주변에 소환한다.
	TArray<AActor*>	SpawnPoints;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("MinionSpawn"), SpawnPoints);

	AActor*	Boss = GetAvatarActorFromActorInfo();

	if (IsValid(Boss) && IsValid(mSummonSound))
		UGameplayStatics::PlaySoundAtLocation(this, mSummonSound, Boss->GetActorLocation());

	// 소환할 때 겹치면 조금 옮겨서라도 반드시 소환한다.
	FActorSpawnParameters	SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < mSummonCount; ++i)
	{
		// 지점을 돌아가며 쓴다 (i % 지점 수 : 0,1,2,3,0,1,…)
		FVector	Base = SpawnPoints.IsEmpty() ?
			(IsValid(Boss) ? Boss->GetActorLocation() : FVector::ZeroVector) :
			SpawnPoints[i % SpawnPoints.Num()]->GetActorLocation();

		// 같은 지점에 여러 마리가 나오면 겹치지 않게 수평으로 흩뜨린다. 바닥에 묻히지 않게 조금 위에서.
		FVector2D	Offset = FMath::RandPointInCircle(mScatterRadius);
		FVector		Location = Base + FVector(Offset.X, Offset.Y, 100.0);

		// 보스(=플레이어 쪽)를 바라보며 등장
		FRotator	Rotation = IsValid(Boss) ? (Boss->GetActorLocation() - Location).Rotation() : FRotator::ZeroRotator;
		Rotation.Pitch = 0.0;

		GetWorld()->SpawnActor<AMinion>(mMinionClass, Location, Rotation, SpawnParams);
	}

	UE_LOG(UGDebug, Warning, TEXT("Summoned %d minions at %d spawn points"), mSummonCount, SpawnPoints.Num());

	mSummonCount = 0;
}
