// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Knockdown.h"
#include "../../Player/PlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UGameplayAbility_Knockdown::UGameplayAbility_Knockdown()
{
	// 비용·쿨다운 없음.

	// 넘어져 있는 동안 붙는 태그. 이동·공격·회피·스킬 입력을 막는다. (APlayerCharacter::CanAct)
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Knockdown")));

	// 이미 넘어져 있으면 다시 발동하지 않는다. (무적이라 실제로는 거의 없지만 안전장치)
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Knockdown")));

	// ---------------------------------------------------------------- 발동 방법 : 게임플레이 이벤트
	// AbilityTriggers : "이 태그의 이벤트가 오면 이 어빌리티를 발동한다"는 등록.
	// 이벤트에 공격자(Instigator)를 담아 보낼 수 있어서, 발동하자마자 날아갈 방향을 알 수 있다.
	// (TryActivateAbilityByClass는 정보를 같이 넘길 방법이 없다)
	FAbilityTriggerData	Trigger;
	Trigger.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Knockdown"));
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	// ---------------------------------------------------------------- 애니메이션 (11/4 : 실행 중 로드 → 생성자에서 미리)
	// 10/16~11/3에는 처음 넘어질 때 경로로 불러왔다. 그 순간(히트스톱 중) 디스크를 읽느라 멈칫할 수 있었다.
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		KnockAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_Knock.AM_Gideon_Knock'"));

	if (KnockAsset.Succeeded())
		mKnockMontage = KnockAsset.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		StandUpAsset(TEXT("/Script/Engine.AnimSequence'/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Stun_End.Stun_End'"));

	if (StandUpAsset.Succeeded())
		mStandUpAnim = StandUpAsset.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		FallAsset(TEXT("/Script/Engine.AnimSequence'/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Stun_Start.Stun_Start'"));

	if (FallAsset.Succeeded())
		mFallAnim = FallAsset.Object;
}

void UGameplayAbility_Knockdown::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	APlayerCharacter* Player = Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!IsValid(Player) || !ASC || !IsValid(mKnockMontage) || !IsValid(mStandUpAnim))
	{
		UE_LOG(UGDebug, Warning, TEXT("Knockdown : Invalid Character or Animation"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	mStandingUp = false;
	mInvincibleAdded = false;

	// ---------------------------------------------------------------- 1. 하던 것 취소
	// CancelAbilities(태그, 제외 태그, 제외할 어빌리티) : 실행 중인 어빌리티를 모두 취소한다. 자기 자신(this)만 빼고.
	// Q·F·R 시전과 E 회피가 여기서 끊긴다. (마나는 이미 썼으므로 돌려주지 않는다)
	ASC->CancelAbilities(nullptr, nullptr, this);

	// ---------------------------------------------------------------- 2. 무적
	// ActivationOwnedTags로 넣지 않고 여기서 직접 붙이는 이유 :
	// 위에서 회피를 취소하면 회피의 EndAbility가 무적 태그 개수를 "0으로" 만든다(무적이 남는 버그 방지 코드).
	// 발동할 때 자동으로 붙은 태그였다면 그때 같이 지워진다. 그래서 취소가 끝난 "뒤"에 붙인다.
	// 필살기는 "막을 수 없는" 피해라 이 무적을 무시하고 즉사시킨다.
	ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));
	mInvincibleAdded = true;

	// ---------------------------------------------------------------- 3. 방향 : 공격자 반대쪽
	// TriggerEventData->Instigator : 이벤트에 담겨 온 공격자. const AActor* (읽기 전용 포인터)
	const AActor* Attacker = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;

	// 공격자 → 나 방향 = 날아갈 방향. 공격자를 모르면 내 뒤쪽으로.
	FVector	AwayDirection = -Player->GetActorForwardVector();

	if (IsValid(Attacker))
		AwayDirection = Player->GetActorLocation() - Attacker->GetActorLocation();

	AwayDirection.Z = 0.f;
	AwayDirection = AwayDirection.GetSafeNormal();

	if (AwayDirection.IsNearlyZero())
		AwayDirection = -Player->GetActorForwardVector();

	// 카메라를 따라 몸이 도는 설정을 잠시 꺼야, 넘어진 동안 시점을 돌려도 몸이 안 돈다.
	Player->SetBodyFollowsCamera(false);

	// ---------------------------------------------------------------- 4. 맞은 방향에 따라 동작 고르기 (10/19)
	// 공격자 → 나 방향(AwayDirection)과 내 앞 방향의 내적 > 0.5 = 공격자가 내 뒤쪽 120도 안에 있다.
	// (나 → 공격자 방향과 앞 방향의 내적 < -0.5 와 같은 뜻. 내적 0.5 = 60도)
	bool	bHitFromBack = FVector::DotProduct(AwayDirection, Player->GetActorForwardVector()) > 0.5f;

	// 밀려나는 시간. 등 뒤는 날아가는 시간 내내, 앞·옆은 짧게 (무릎 꿇으며 밀림)
	float	PushTime = mAirTime;

	if (bHitFromBack)
	{
		// 등 뒤 : Knock_Fwd("등 뒤에서 맞아 앞으로 날아가는" 자세) → 몸이 날아갈 방향을 보게 돌린다.
		// 이미 거의 등지고 있어서 회전은 최대 60도 정도만 생긴다. (10/16 : 정면에서 180도 도는 문제)
		Player->SetActorRotation(FRotator(0.0, AwayDirection.Rotation().Yaw, 0.0));

		UAbilityTask_PlayMontageAndWait* PlayKnock =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, mKnockMontage, 1.f);

		// 사망 등으로 끊기면 종료. (일어나기로 넘어갈 때 끊기는 건 OnKnockMontageInterrupted 안에서 무시)
		PlayKnock->OnInterrupted.AddDynamic(this, &UGameplayAbility_Knockdown::OnKnockMontageInterrupted);
		PlayKnock->OnCancelled.AddDynamic(this, &UGameplayAbility_Knockdown::OnKnockMontageInterrupted);
		PlayKnock->ReadyForActivation();
	}
	else
	{
		// 앞·옆 : 몸을 돌리지 않고 제자리에서 무릎 꿇기(Stun_Start). Stun_End와 이어지는 원래 한 세트 동작.
		// Stun_Start(29.143fps)·Stun_End(29.4fps)도 프레임 레이트가 달라 즉석 몽타주로 재생한다.
		// 재생 시간을 날아가는 시간(mAirTime)에 맞춘다 (내 판단 : Knock_Fwd 쪽 시간이 좋음).
		//   재생 속도 = 길이 ÷ (에셋 속도 스케일 × 원하는 시간)  → 1.17초 ÷ 0.6초 ≈ 1.95배
		UAnimInstance* AnimInstance = Player->GetMesh()->GetAnimInstance();

		if (AnimInstance && IsValid(mFallAnim))
		{
			// 10/19 수정 : 끝나는 시간을 "mAirTime + Stun_End 블렌드 인 시간"으로, 블렌드 아웃은 0.
			//   정확히 mAirTime에 끝나게 하고 블렌드 아웃 0.1을 주면, 끝나기 직전 기본 자세(서 있기)로 섞여 돌아갔다가
			//   Stun_End가 다시 무릎을 꿇어서 "두 번 넘어지는" 것처럼 보였다.
			//   이렇게 하면 Stun_End가 섞여 들어오는 동안에도 무릎 자세가 유지되고, Stun_End가 덮어쓰며 끝난다.
			float	FallTime = mAirTime + mStandUpBlendIn;
			float	FallRate = mFallAnim->GetPlayLength() /
				FMath::Max(mFallAnim->RateScale * FallTime, 0.01f);

			AnimInstance->PlaySlotAnimationAsDynamicMontage(mFallAnim, TEXT("DefaultSlot"),
				0.1f, 0.f, FallRate);
		}

		PushTime = mFrontPushTime;
	}

	// 루트 모션 힘 : 보스 돌진과 같은 태스크. 정해진 방향·속도로 정해진 시간 동안 캐릭터를 민다.
	// 끝나면 속도 0 (미끄러지지 않고 그 자리에 멈춤), 중력 적용
	UAbilityTask_ApplyRootMotionConstantForce* Push =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, NAME_None, AwayDirection, mPushSpeed, PushTime,
			false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector,
			0.f, true);

	Push->ReadyForActivation();

	// 날아가는 시간 뒤 무릎 꿇기로
	UAbilityTask_WaitDelay* WaitAir = UAbilityTask_WaitDelay::WaitDelay(this, mAirTime);
	WaitAir->OnFinish.AddDynamic(this, &UGameplayAbility_Knockdown::OnAirFinished);
	WaitAir->ReadyForActivation();
}

void UGameplayAbility_Knockdown::OnAirFinished()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInstance = IsValid(Player) ? Player->GetMesh()->GetAnimInstance() : nullptr;

	if (!AnimInstance || Player->IsDeath())
	{
		OnStandUpFinished();
		return;
	}

	mStandingUp = true;

	// 기상 회피 가능 (10/19) : 무릎이 땅에 닿은 지금부터 E를 누르면 넘어짐을 끊고 회피로 빠져나갈 수 있다.
	// 플레이어의 DodgeKey가 이 태그를 보고 판단한다. 넘어지는 중(공중)에는 붙어 있지 않아서 E가 안 된다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.CanWakeDodge")));
		mWakeDodgeTagAdded = true;
	}

	// Stun_End를 즉석 몽타주로 재생한다. 날아가는 몽타주와 같은 DefaultSlot이라 그 몽타주는 밀려나며 섞인다.
	// 블렌드 인 0.15초 : 공중 자세 → 무릎 꿇은 자세로 내려오는 시간 ("쿵" 착지처럼 보이게)
	AnimInstance->PlaySlotAnimationAsDynamicMontage(mStandUpAnim, TEXT("DefaultSlot"),
		mStandUpBlendIn, 0.25f, mStandUpPlayRate);

	// 실제 재생 시간 = 길이 ÷ (재생 속도 × 에셋의 속도 스케일). Stun_End는 에셋 속도 스케일 1.4로 설정해 둠.
	// RateScale : 애니메이션 에셋 디테일의 "속도 스케일" 값
	float	Rate = FMath::Max(mStandUpPlayRate * mStandUpAnim->RateScale, 0.01f);
	float	StandUpTime = mStandUpAnim->GetPlayLength() / Rate;

	UAbilityTask_WaitDelay* WaitStandUp = UAbilityTask_WaitDelay::WaitDelay(this, StandUpTime);
	WaitStandUp->OnFinish.AddDynamic(this, &UGameplayAbility_Knockdown::OnStandUpFinished);
	WaitStandUp->ReadyForActivation();
}

void UGameplayAbility_Knockdown::OnKnockMontageInterrupted()
{
	// 일어나기로 넘어가면서 날아가는 몽타주가 밀려난 것 → 정상이므로 무시
	if (mStandingUp)
		return;

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGameplayAbility_Knockdown::OnStandUpFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGameplayAbility_Knockdown::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// IsActive() : 아직 끝나지 않았을 때만 정리한다. (EndAbility가 두 번 불려도 무적을 두 번 떼지 않게)
	if (IsActive() && ActorInfo)
	{
		// 붙인 무적 하나를 뗀다. (RemoveLooseGameplayTag : 개수를 1 줄인다)
		// 기상 회피 태그도 뗀다.
		if (mWakeDodgeTagAdded && ActorInfo->AbilitySystemComponent.IsValid())
		{
			mWakeDodgeTagAdded = false;

			ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(
				FGameplayTag::RequestGameplayTag(TEXT("State.CanWakeDodge")));
		}

		if (mInvincibleAdded && ActorInfo->AbilitySystemComponent.IsValid())
		{
			mInvincibleAdded = false;

			ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(
				FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));
		}

		// 몸이 다시 카메라 방향을 따라가게. 단, 죽었으면 그대로 둔다. (쓰러진 몸이 시점을 따라 돌지 않게)
		APlayerCharacter* Player = Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get());

		if (IsValid(Player) && !Player->IsDeath())
			Player->SetBodyFollowsCamera(true);
	}

	// 부모 : 태스크 정리, State.Knockdown 제거
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
