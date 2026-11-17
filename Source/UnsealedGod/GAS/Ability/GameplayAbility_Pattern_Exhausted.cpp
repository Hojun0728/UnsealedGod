// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Exhausted.h"
#include "../../Boss/BossBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_Exhausted::UGameplayAbility_Pattern_Exhausted()
{
	// 거리와 상관없이 (조건은 "지침 대기"뿐)
	mMinRange = 0.f;
	mMaxRange = 100000.f;

	// 필살기(10)보다 낮고 미니언 소환(5)보다 높다. 지침 대기가 되면 지금 패턴이 끝난 뒤 반드시 지친다.
	mPriority = 8;

	// 지침이 끝나면 바로 다음 행동 (지친 시간 자체가 쉬는 시간)
	mRecoveryTime = 0.3f;
	mRecoveryRandom = 0.f;

	// 세바로그의 Stun 계열 (기디언 팩에도 같은 이름이 있으니 경로 주의)
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		StartAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Stun_Start.Stun_Start'"));

	if (StartAnim.Succeeded())
		mStartAnim = StartAnim.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		LoopAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Stun_Loop.Stun_Loop'"));

	if (LoopAnim.Succeeded())
		mLoopAnim = LoopAnim.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		EndAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Stun_End.Stun_End'"));

	if (EndAnim.Succeeded())
		mEndAnim = EndAnim.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		StartVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Effort_Frustration.Sevarog_Effort_Frustration'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		BreathVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Effort_BreathingLowHealth.Sevarog_Effort_BreathingLowHealth'"));

	if (StartVoice.Succeeded())		mStartVoice = StartVoice.Object;
	if (BreathVoice.Succeeded())	mBreathVoice = BreathVoice.Object;
}

void UGameplayAbility_Pattern_Exhausted::StopBreath()
{
	if (IsValid(mBreathAudio))
		mBreathAudio->FadeOut(0.3f, 0.f);

	mBreathAudio = nullptr;
}

bool UGameplayAbility_Pattern_Exhausted::CanUsePattern(const AActor* Boss, const AActor* Target) const
{
	const ABossBase* BossBase = Cast<ABossBase>(Boss);

	return IsValid(BossBase) && BossBase->IsExhaustPending();
}

void UGameplayAbility_Pattern_Exhausted::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 부모(UGameplayAbility_BossPattern)의 ActivateAbility는 "몽타주 에셋 하나를 재생"하는 흐름이라 건너뛰고,
	// 그 위의 UGameplayAbility_Base(비용·쿨다운 확정)만 부른다.
	// 클래스이름::함수() : 바로 위 부모(Super)가 아니라 지정한 조상 클래스의 버전을 부른다.
	UGameplayAbility_Base::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	ABossBase* Boss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());

	// (11/4) 끝 동작(mEndAnim)은 없어도 된다 (오로라 팩에는 Stun_End가 없다)
	if (!IsValid(Boss) || !IsValid(mStartAnim) || !IsValid(mLoopAnim))
	{
		UE_LOG(UGDebug, Warning, TEXT("Exhausted : Invalid Boss or Animation"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 지침 대기를 꺼내 쓴다. (과부하 횟수 초기화 + 다음 기준 4~6 다시 뽑기)
	Boss->ConsumeExhaust();

	// 그 자리에 멈춘다.
	if (AAIController* AIController = Cast<AAIController>(Boss->GetController()))
		AIController->StopMovement();

	// 이번 페이즈의 지친 시간 → 반복 동작 시간 = 전체 - 시작 - 끝
	int32	Index = FMath::Clamp(Boss->GetPhase() - 1, 0, mExhaustTimes.Num() - 1);
	float	Total = mExhaustTimes[Index];
	float	StartTime = mStartAnim->GetPlayLength() / FMath::Max(mStartAnim->RateScale, 0.01f);
	float	EndTime = IsValid(mEndAnim) ? mEndAnim->GetPlayLength() / FMath::Max(mEndAnim->RateScale, 0.01f) : 0.f;

	mLoopTime = FMath::Max(Total - StartTime - EndTime, 0.f);

	UE_LOG(UGDebug, Warning, TEXT("Boss : EXHAUSTED %.1fs (Phase %d)"), Total, Boss->GetPhase());

	// 화면 글자는 디버그 표시가 켜져 있을 때만 (10/21)
	if (GEngine && Boss->ShouldDrawDebug())
		GEngine->AddOnScreenDebugMessage(8, Total, FColor::Cyan, TEXT("Boss : EXHAUSTED"));

	// 1단계 : 고개를 숙이며 지침 + "크으…" (몸에 붙임)
	float	Time = PlayAnim(mStartAnim, 1, 0.f);

	if (IsValid(mStartVoice))
		UGameplayStatics::SpawnSoundAttached(mStartVoice, Boss->GetMesh());

	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(Time - mBlendTime, 0.05f));
	Wait->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Exhausted::OnStartFinished);
	Wait->ReadyForActivation();
}

float UGameplayAbility_Pattern_Exhausted::PlayAnim(UAnimSequence* Anim, int32 LoopCount, float BlendOut)
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInstance = IsValid(Boss) ? Boss->GetMesh()->GetAnimInstance() : nullptr;

	if (!AnimInstance || !IsValid(Anim))
		return 0.f;

	// 인자 : 애니메이션, 슬롯, 블렌드 인, 블렌드 아웃, 재생 속도, 반복 횟수
	// 10/19 수정 : 시작·반복 동작은 블렌드 아웃 0. 블렌드 아웃이 있으면 끝나기 직전 기본 자세(서 있기)로 섞여서
	//   "잠깐 일어났다가 다시 고개를 숙이는" 것처럼 보였다. 다음 동작이 mBlendTime 일찍 시작해 덮어쓰며 끝난다.
	AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, TEXT("DefaultSlot"), mBlendTime, BlendOut, 1.f, LoopCount);

	return Anim->GetPlayLength() / FMath::Max(Anim->RateScale, 0.01f);
}

bool UGameplayAbility_Pattern_Exhausted::CheckAlive()
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (IsValid(Boss) && !Boss->IsDeath())
		return true;

	StopBreath();

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	return false;
}

void UGameplayAbility_Pattern_Exhausted::OnStartFinished()
{
	if (!CheckAlive())
		return;

	// 2단계 : 지친 자세 유지. 반복 횟수는 넉넉히 주고, 정해진 시간이 지나면 다음 동작이 덮어쓴다.
	PlayAnim(mLoopAnim, 10, 0.f);

	// 헐떡임 (몸에 붙임). 일어날 때(OnLoopFinished) 줄이며 끈다
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

	if (IsValid(Boss) && IsValid(mBreathVoice))
		mBreathAudio = UGameplayStatics::SpawnSoundAttached(mBreathVoice, Boss->GetMesh());

	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(mLoopTime, 0.05f));
	Wait->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Exhausted::OnLoopFinished);
	Wait->ReadyForActivation();
}

void UGameplayAbility_Pattern_Exhausted::OnLoopFinished()
{
	if (!CheckAlive())
		return;

	StopBreath();

	// (11/4) 끝 동작이 없으면 반복 동작을 mNoEndBlendOut초 동안 기본 자세로 섞으며 끝낸다.
	if (!IsValid(mEndAnim))
	{
		ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());

		// StopSlotAnimation(블렌드 아웃 시간, 슬롯) : 이 슬롯의 즉석 몽타주를 멈춘다. 블렌드 아웃 동안 서 있는 자세로 바뀐다.
		if (UAnimInstance* AnimInstance = IsValid(Boss) ? Boss->GetMesh()->GetAnimInstance() : nullptr)
			AnimInstance->StopSlotAnimation(mNoEndBlendOut, TEXT("DefaultSlot"));

		UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, mNoEndBlendOut);
		Wait->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Exhausted::OnEndFinished);
		Wait->ReadyForActivation();
		return;
	}

	// 3단계 : 다시 자세를 잡음
	float	Time = PlayAnim(mEndAnim, 1, mBlendTime);

	UAbilityTask_WaitDelay* Wait = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(Time - mBlendTime, 0.05f));
	Wait->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Exhausted::OnEndFinished);
	Wait->ReadyForActivation();
}

void UGameplayAbility_Pattern_Exhausted::OnEndFinished()
{
	// 부모 EndAbility가 후딜(0.3초)을 보스에게 알려 주고 끝낸다.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
