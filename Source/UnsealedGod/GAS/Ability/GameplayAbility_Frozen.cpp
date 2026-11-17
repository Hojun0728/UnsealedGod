// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Frozen.h"
#include "../../Player/PlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Camera/CameraComponent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Frozen::UGameplayAbility_Frozen()
{
	// 비용·쿨다운 없음.

	// 얼어 있는 동안 붙는 태그. 이동·공격·회피·스킬·물약 입력을 막는다. (APlayerCharacter::CanAct)
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Frozen")));

	// 이미 얼었거나 넘어져·묶여 있으면 다시 발동하지 않는다.
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Frozen")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Knockdown")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Bound")));

	// 얼음 내성 (10/28) : 얼음이 풀린 뒤 mImmuneTime초 동안은 다시 얼지 않는다. (풀리자마자 또 얼어 대처할 수 없던 "얼음 연쇄" 방지)
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.FreezeImmune")));

	// 발동 방법 : 게임플레이 이벤트 (속박·넘어짐과 같은 방식)
	FAbilityTriggerData	Trigger;
	Trigger.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Frozen"));
	Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(Trigger);

	// ---------------------------------------------------------------- 이펙트 (이름으로 고른 기본값 → 게임에서 보고 바꾼다)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		BodyFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Rooted.P_Aurora_Freeze_Rooted'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ScreenFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Ultimate/FX/P_Aurora_ScreenFrost_Frozen.P_Aurora_ScreenFrost_Frozen'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		MashFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Aurora/P_Aurora_Sculpture_Chip.P_Aurora_Sculpture_Chip'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		BreakFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Freeze/FX/P_Aurora_Freeze_Segment_Crumble.P_Aurora_Freeze_Segment_Crumble'"));

	if (BodyFX.Succeeded())		mBodyParticle = BodyFX.Object;
	if (ScreenFX.Succeeded())	mScreenParticle = ScreenFX.Object;
	if (MashFX.Succeeded())		mMashParticle = MashFX.Object;
	if (BreakFX.Succeeded())	mBreakParticle = BreakFX.Object;

	// 소리 (10/29 : 마법 효과음 팩 → AI로 만든 효과음으로 교체)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		FreezeSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_Freeze.S_SFX_Ice_Freeze'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		MashSnd1(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_MashCrack_1.S_SFX_Ice_MashCrack_1'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		MashSnd2(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_MashCrack_2.S_SFX_Ice_MashCrack_2'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		MashSnd3(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_MashCrack_3.S_SFX_Ice_MashCrack_3'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		BreakSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_Break.S_SFX_Ice_Break'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		ShatterSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_Shatter.S_SFX_Ice_Shatter'"));

	if (FreezeSnd.Succeeded())	mFreezeSound = FreezeSnd.Object;
	if (BreakSnd.Succeeded())	mBreakSound = BreakSnd.Object;
	if (ShatterSnd.Succeeded())	mShatterSound = ShatterSnd.Object;

	for (auto* Finder : { &MashSnd1, &MashSnd2, &MashSnd3 })
	{
		if (Finder->Succeeded())
			mMashSounds.Add(Finder->Object);
	}
}

void UGameplayAbility_Frozen::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	mMashCount = 0;

	// 얼음 포박이 얼렸으면 "맞아도 안 깨짐" 표시를 붙인다. (포박은 찌르기를 연타로 줄이는 처벌이라)
	// 다른 얼음은 맞으면 깨진다 : 때린 쪽이 피해 1.5배 + Event.Player.FrozenShatter를 보낸다 (AGameCharacter::ApplyDamageToTarget)
	mNoShatterAdded = false;

	if (TriggerEventData && TriggerEventData->InstigatorTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("State.FrozenNoShatter"))))
	{
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.FrozenNoShatter")));
		mNoShatterAdded = true;
	}

	// HUD 화면 성에 : 방금 얼었으니 진행도 0 (가장 진하게)
	Player->SetFrozenProgress(0.f);

	// ---------------------------------------------------------------- 1. 하던 것 취소 (속박과 같다)
	// Q·F·R 시전, 물약, 회피가 여기서 끊긴다. 자기 자신(this)은 빼고.
	ASC->CancelAbilities(nullptr, nullptr, this);

	// ---------------------------------------------------------------- 2. 그 자세 그대로 멈춤
	// 미끄러지지 않게 이동을 바로 멈추고, 애니메이션을 일시정지한다. = 얼음 조각상
	Player->GetCharacterMovement()->StopMovementImmediately();
	Player->GetMesh()->bPauseAnims = true;

	// 몸 방향도 고정한다. (시점을 돌려도 얼음 속 몸이 돌지 않게)
	Player->SetBodyFollowsCamera(false);

	// ---------------------------------------------------------------- 3. 이펙트·소리
	if (IsValid(mBodyParticle))
		mBodyParticleComponent = UGameplayStatics::SpawnEmitterAttached(mBodyParticle, Player->GetMesh());

	// 화면 성에 : 카메라에 붙인다. FindComponentByClass : 이 액터가 가진 컴포넌트 중 그 종류를 찾는다.
	UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();

	if (IsValid(mScreenParticle) && IsValid(Camera))
		mScreenParticleComponent = UGameplayStatics::SpawnEmitterAttached(mScreenParticle, Camera);

	if (IsValid(mFreezeSound))
		UGameplayStatics::PlaySoundAtLocation(this, mFreezeSound, Player->GetActorLocation());

	// ---------------------------------------------------------------- 4. 연타 기다리기
	// OnlyTriggerOnce = false : 한 번 받고 끝나지 않고, 어빌리티가 끝날 때까지 계속 받는다.
	UAbilityTask_WaitGameplayEvent* WaitMash = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Player.FrozenMash")), nullptr, false);

	WaitMash->EventReceived.AddDynamic(this, &UGameplayAbility_Frozen::OnMash);
	WaitMash->ReadyForActivation();

	// 얼음 깨짐 (한 번) : 맞아서 깨지면 바로 풀려난다
	UAbilityTask_WaitGameplayEvent* WaitShatter = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Player.FrozenShatter")), nullptr, true);

	WaitShatter->EventReceived.AddDynamic(this, &UGameplayAbility_Frozen::OnShatter);
	WaitShatter->ReadyForActivation();

	// ---------------------------------------------------------------- 5. 안전장치 (최대 시간)
	float	MaxTime = TriggerEventData && TriggerEventData->EventMagnitude > 0.f ?
		TriggerEventData->EventMagnitude : mDefaultMaxTime;

	UAbilityTask_WaitDelay* WaitTimeout = UAbilityTask_WaitDelay::WaitDelay(this, MaxTime);
	WaitTimeout->OnFinish.AddDynamic(this, &UGameplayAbility_Frozen::OnTimeout);
	WaitTimeout->ReadyForActivation();
}

void UGameplayAbility_Frozen::OnMash(FGameplayEventData Payload)
{
	++mMashCount;

	// 연타할 때마다 몸에서 얼음 조각이 튄다 → "깨고 있다"는 손맛
	AActor* Avatar = GetAvatarActorFromActorInfo();

	// 진행도를 알린다 → HUD 화면 성에가 이만큼 녹는다 (남은 연타를 숫자 없이 보여 준다)
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(Avatar))
		Player->SetFrozenProgress((float)mMashCount / (float)FMath::Max(mMashRequired, 1));

	if (IsValid(Avatar) && IsValid(mMashParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mMashParticle, Avatar->GetActorLocation());

	// 마지막 연타는 "딱" 대신 풀리는 소리만 (BreakFree)
	if (mMashCount >= mMashRequired)
	{
		BreakFree(mBreakSound);
		return;
	}

	// 금 가는 소리 : 세 개 중 랜덤. RandRange(최소, 최대) : 둘 다 포함
	if (IsValid(Avatar) && !mMashSounds.IsEmpty())
	{
		USoundBase* Sound = mMashSounds[FMath::RandRange(0, mMashSounds.Num() - 1)];

		if (IsValid(Sound))
			UGameplayStatics::PlaySoundAtLocation(this, Sound, Avatar->GetActorLocation());
	}
}

void UGameplayAbility_Frozen::OnShatter(FGameplayEventData Payload)
{
	UE_LOG(UGDebug, Warning, TEXT("Frozen : shattered by a hit"));

	// 맞아서 깨짐 = 무거운 소리 (연타로 풀린 것과 소리로 구분된다)
	BreakFree(mShatterSound);
}

void UGameplayAbility_Frozen::OnTimeout()
{
	BreakFree(mBreakSound);
}

void UGameplayAbility_Frozen::BreakFree(USoundBase* Sound)
{
	if (!IsActive())
		return;

	AActor* Avatar = GetAvatarActorFromActorInfo();

	if (IsValid(Avatar))
	{
		if (IsValid(mBreakParticle))
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mBreakParticle, Avatar->GetActorLocation(),
				Avatar->GetActorRotation());

		if (IsValid(Sound))
			UGameplayStatics::PlaySoundAtLocation(this, Sound, Avatar->GetActorLocation());
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGameplayAbility_Frozen::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsActive())
	{
		UAbilitySystemComponent* OwnerASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

		// 얼음 내성 : 내성 태그를 붙이고 mImmuneTime초 뒤 뗀다. (피해는 그대로 받는다, 얼음만 막는다)
		// CreateWeakLambda(ASC, ...) : ASC가 사라졌으면(레벨 이동 등) 실행하지 않는 안전한 타이머 함수
		if (OwnerASC && mImmuneTime > 0.f && GetWorld())
		{
			FGameplayTag	ImmuneTag = FGameplayTag::RequestGameplayTag(TEXT("State.FreezeImmune"));

			OwnerASC->AddLooseGameplayTag(ImmuneTag);

			FTimerHandle	ImmuneTimer;

			GetWorld()->GetTimerManager().SetTimer(ImmuneTimer,
				FTimerDelegate::CreateWeakLambda(OwnerASC, [OwnerASC, ImmuneTag]()
				{
					OwnerASC->RemoveLooseGameplayTag(ImmuneTag);
				}),
				mImmuneTime, false);
		}

		if (OwnerASC && mNoShatterAdded)
			OwnerASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.FrozenNoShatter")));

		mNoShatterAdded = false;

		// 넘어짐(큰 공격)이 이 어빌리티를 취소하며 이어받아도 여기를 지나므로, 멈춘 애니메이션이 꼭 풀린다.
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr))
		{
			Player->GetMesh()->bPauseAnims = false;

			// (11/4) 죽었으면 몸이 카메라를 따라 돌지 않게 그대로 둔다. (넘어짐과 같은 규칙)
			if (!Player->IsDeath())
				Player->SetBodyFollowsCamera(true);
		}

		if (IsValid(mBodyParticleComponent))
			mBodyParticleComponent->DestroyComponent();

		if (IsValid(mScreenParticleComponent))
			mScreenParticleComponent->DestroyComponent();

		mBodyParticleComponent = nullptr;
		mScreenParticleComponent = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
