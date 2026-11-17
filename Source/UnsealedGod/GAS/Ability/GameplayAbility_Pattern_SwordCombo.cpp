// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_SwordCombo.h"

UGameplayAbility_Pattern_SwordCombo::UGameplayAbility_Pattern_SwordCombo()
{
	// 사용 거리 : 3.3m 이내 = 적정 타격 거리 1.8m + 밀기 1.5m. (10/28 : 4m에서는 1타가 밀고 들어와도 닿지 않아 허공을 벴음)
	mMinRange = 0.f;
	mMaxRange = 330.f;

	mWeight = 1.f;

	// 재생 속도 1.5배 (10/28 밸런스 : 1.3배는 쉬웠음). 2페이즈(빠른 페이즈) 1.875배 = 1.5 × 1.25
	// 판정 막대·밀기 노티파이는 몽타주 시간 기준이라 같이 빨라진다.
	mMontagePlayRate = 1.5f;
	mFastPlayRate = 1.875f;

	// 후딜(반격 틈) : 0.5 ± 0.3초, 2페이즈 0.4 ± 0.2초 (10/28 밸런스 : 기본값 0.8 ± 0.4는 너무 길었음)
	mRecoveryTime = 0.5f;
	mRecoveryRandom = 0.3f;
	mFastRecoveryTime = 0.4f;
	mFastRecoveryRandom = 0.2f;

	// 밀기 : 600cm/초 × 0.25초 = 최대 1.5m (세바로그 3연타와 같은 값)
	mLungeSpeed = 600.f;
	mLungeDuration = 0.25f;

	// 적정 타격 거리 : 칼끝이 기디언에게 닿는 거리. 이보다 멀 때만 그 차이만큼 밀고 들어온다.
	// 180은 제안값 (보스 크기 1.3 반영 전 추정). 게임에서 칼끝이 기디언 몸에 닿는 거리를 보고 확정한다.
	mStrikeDistance = 180.f;

	// 기본 공격이라 쿨다운 없음
	mCoolDownTime = 0.f;

	// 몽타주 : Primary_Attack_Slow_A → B → C → Slow_C_Recovery (DefaultSlot). 에디터에서 Boss/Aurora에 만든다.
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_SwordCombo.AM_Aurora_SwordCombo'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;
}
