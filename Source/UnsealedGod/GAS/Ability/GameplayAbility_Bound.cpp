// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Bound.h"
#include "../../Player/PlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UGameplayAbility_Bound::UGameplayAbility_Bound()
{
	// 비용·쿨다운 없음.

	// 묶여 있는 동안 붙는 태그. 이동·공격·회피·스킬·물약 입력을 막는다. (APlayerCharacter::CanAct)
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Bound")));

	// 이미 묶였거나 넘어져 있으면 다시 발동하지 않는다. (속박 패턴이 무적 여부를 먼저 확인하지만 안전장치)
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Bound")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Knockdown")));

	// 발동 방법 : 게임플레이 이벤트 (넘어짐과 같은 방식. 이벤트에 보스를 담아 보낸다)
	FAbilityTriggerData	Trigger;
	Trigger.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Bound"));
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		BoundAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_Bound.AM_Gideon_Bound'"));

	if (BoundAsset.Succeeded())
		mBoundMontage = BoundAsset.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		BoundFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonSevarog/FX/Particles/Abilities/Subjugate/FX/P_SubjugateSwirls.P_SubjugateSwirls'"));

	if (BoundFX.Succeeded())
		mBoundParticle = BoundFX.Object;
}

void UGameplayAbility_Bound::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	APlayerCharacter* Player = Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!IsValid(Player) || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	mInvincibleAdded = false;

	// ---------------------------------------------------------------- 1. 하던 것 취소 (넘어짐과 같다)
	// Q·F·R 시전, 물약, 회피가 여기서 끊긴다. 자기 자신(this)은 빼고.
	ASC->CancelAbilities(nullptr, nullptr, this);

	// ---------------------------------------------------------------- 2. 무적 (취소가 끝난 "뒤"에 붙인다 — 넘어짐 주석 참고)
	// 보스의 마무리 일격은 "막을 수 없는" 피해라 이 무적을 뚫는다.
	ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));
	mInvincibleAdded = true;

	// 묶인 동안 시점을 돌려도 몸이 돌지 않게. (보스는 "묶인 순간 기디언의 등 뒤"에 나타나서 친다 → 몸 방향이 고정돼야 함)
	Player->SetBodyFollowsCamera(false);

	// ---------------------------------------------------------------- 3. 동작 + 이펙트
	if (IsValid(mBoundMontage))
	{
		// 끝나거나 끊겨도 어빌리티는 끝내지 않는다. (보스의 일격 → 넘어짐이 이어받거나, 안전장치 시간에 풀린다)
		UAbilityTask_PlayMontageAndWait* PlayBound =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, mBoundMontage, 1.f);

		PlayBound->ReadyForActivation();
	}
	else
	{
		UE_LOG(UGDebug, Warning, TEXT("Bound : AM_Gideon_Bound not found"));
	}

	// SpawnEmitterAttached(이펙트, 붙일 컴포넌트) : 메시를 따라 움직이는 이펙트. 돌려받은 컴포넌트로 나중에 지운다.
	if (IsValid(mBoundParticle))
		mBoundParticleComponent = UGameplayStatics::SpawnEmitterAttached(mBoundParticle, Player->GetMesh());

	// ---------------------------------------------------------------- 4. 안전장치
	UAbilityTask_WaitDelay* WaitTimeout = UAbilityTask_WaitDelay::WaitDelay(this, mMaxBoundTime);
	WaitTimeout->OnFinish.AddDynamic(this, &UGameplayAbility_Bound::OnBoundTimeout);
	WaitTimeout->ReadyForActivation();
}

void UGameplayAbility_Bound::OnBoundTimeout()
{
	// 몽타주를 멈추고 서 있는 자세로 돌아간다.
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UAnimInstance* AnimInstance = Player->GetMesh()->GetAnimInstance())
			AnimInstance->Montage_Stop(0.25f, mBoundMontage);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGameplayAbility_Bound::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsActive())
	{
		// 무적 해제. (넘어짐이 이어받는 경우 넘어짐이 자기 무적을 따로 붙인다)
		UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

		if (ASC && mInvincibleAdded)
			ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));

		mInvincibleAdded = false;

		// 묶임 이펙트 제거
		if (IsValid(mBoundParticleComponent))
			mBoundParticleComponent->DestroyComponent();

		mBoundParticleComponent = nullptr;

		// 몸이 다시 카메라를 따라 돈다. (넘어짐이 이어받으면 넘어짐이 다시 끈다)
		// (11/4) 단, 죽었으면 그대로 둔다. 쓰러진 몸이 시점을 따라 돌지 않게 (넘어짐과 같은 규칙)
		APlayerCharacter* Player = Cast<APlayerCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

		if (IsValid(Player) && !Player->IsDeath())
			Player->SetBodyFollowsCamera(true);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
