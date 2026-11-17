// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_HammerCombo.h"

UGameplayAbility_Pattern_HammerCombo::UGameplayAbility_Pattern_HammerCombo()
{
	// 사용 거리 : 보스 중심에서 기디언 중심까지 5m 이내.
	// (보스 캡슐 반지름 84cm + 망치 리치 + 밀기 1.5m를 생각한 값. 테스트하며 조정)
	mMinRange = 0.f;
	mMaxRange = 500.f;

	mWeight = 1.f;

	// 재생 속도 1.5배 (10/14 : 원래 속도는 소울류치고 너무 느리고 피하기 쉬웠음. 1.2배도 느려서 1.5배)
	// 판정 막대·밀기 노티파이는 몽타주 시간 기준이라 같이 빨라진다. 밀기 시간도 부모가 속도에 맞춰 줄인다.
	mMontagePlayRate = 1.5f;

	// 빠른 페이즈(3페이즈~) : 더 빠르게(2배속), 반격 틈은 더 짧게(0.5 ± 0.3초). (10/15 : 1.8배속은 체감 차이가 작아서 2배속)
	// 팩의 Swing1~3_FAST_v2는 Slow의 약 3.2배속이라 너무 빨라서 쓰지 않고, 같은 몽타주를 빠르게 재생한다.
	// → 판정 막대·밀기 노티파이가 몽타주 시간 기준이라 그대로 맞는다.
	mFastPlayRate = 2.f;
	mFastRecoveryTime = 0.5f;
	mFastRecoveryRandom = 0.3f;

	// 밀기 : 600cm/초 × 0.25초 = 최대 1.5m
	mLungeSpeed = 600.f;
	mLungeDuration = 0.25f;

	// 적정 타격 거리 : 이 거리보다 멀 때만 그 차이만큼 밀고 들어온다.
	// 10/14 측정 : 망치 머리가 기디언 높이를 지나갈 때 보스 중심에서 약 250~270cm → 250으로 확정.
	mStrikeDistance = 250.f;

	// 기본 공격이라 쿨다운 없음. (패턴 사이의 쉬는 시간은 BT의 Wait가 담당)
	mCoolDownTime = 0.f;

	// 몽타주. 에디터에서 Content/UnsealedGod/Boss/AM_Sevarog_HammerCombo로 만든다.
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_HammerCombo.AM_Sevarog_HammerCombo'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;
}
