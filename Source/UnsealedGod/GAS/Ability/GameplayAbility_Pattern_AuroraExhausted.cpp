// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_AuroraExhausted.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_AuroraExhausted::UGameplayAbility_Pattern_AuroraExhausted()
{
	// 부모 생성자가 먼저 실행되어 세바로그 동작·보이스를 넣어 둔다. 여기서 오로라 것으로 덮어쓴다.

	// 지친 시간(초). 칸 번호 = 페이즈 - 1. 필살기가 2페이즈로 넘기므로 실제로는 2번째 칸.
	// 4초 = 10/28의 "피했을 때 후딜"과 같다 (기디언 물약 약 3.4초를 마실 수 있는 시간)
	mExhaustTimes = { 4.f, 4.f };

	// 오로라의 Stun 계열 (기디언·세바로그 팩에도 같은 이름이 있으니 경로 주의)
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		StartAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Stun_Start.Stun_Start'"));

	if (StartAnim.Succeeded())
		mStartAnim = StartAnim.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		LoopAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Stun_Loop.Stun_Loop'"));

	if (LoopAnim.Succeeded())
		mLoopAnim = LoopAnim.Object;

	// 오로라 팩에는 Stun_End가 없다 → 비워 두면 부모가 반복 동작을 섞으며 서 있는 자세로 돌아간다
	mEndAnim = nullptr;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		StartVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonAurora/Audio/Cues/Aurora_Effort_Frustration.Aurora_Effort_Frustration'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		BreathVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonAurora/Audio/Cues/Aurora_Effort_BreathingLowHealth.Aurora_Effort_BreathingLowHealth'"));

	if (StartVoice.Succeeded())		mStartVoice = StartVoice.Object;
	if (BreathVoice.Succeeded())	mBreathVoice = BreathVoice.Object;
}
