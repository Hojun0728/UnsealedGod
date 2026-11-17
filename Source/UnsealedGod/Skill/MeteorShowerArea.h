// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/Actor.h"
#include "MeteorShowerArea.generated.h"

// ============================================================================
// AMeteorShowerArea
// 기디언 R "운석 비"(궁극기)의 바닥 장판. 월드에 놓여서 정해진 시간 동안 주기마다 범위 피해를 준다.
//
// 어빌리티가 아니라 "액터"로 만든 이유 :
//   어빌리티는 시전 동작(약 3초)이 끝나면 종료되지만, 장판은 그 뒤에도 남아서 피해를 줘야 한다.
//   그래서 어빌리티는 "발사 순간 이 액터를 놓는 것"까지만 하고, 피해는 이 액터가 스스로 처리한다.
//
// 피해는 놓은 사람(Owner = 기디언)의 ApplyAreaDamage를 빌려 쓴다. → 기디언과 다른 편(보스·미니언)만 맞는다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AMeteorShowerArea : public AActor
{
	GENERATED_BODY()

public:
	AMeteorShowerArea();

protected:
	// ---------------------------------------------------------------- 컴포넌트
	// 위치 기준점. (바닥 높이)
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent>		mRoot;

	// 하늘의 포털 이펙트.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent>	mPortalFX;

	// 쏟아지는 운석 이펙트.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent>	mShowerFX;

	// 피해를 줄 때마다 원 안의 아무 곳에 터지는 착탄 이펙트.
	UPROPERTY(EditAnywhere, Category = "Meteor")
	TObjectPtr<UParticleSystem>		mImpactParticle;

	// (10/29) 장판 3초 동안의 운석 비 소리 (생길 때 한 번)
	UPROPERTY(EditAnywhere, Category = "Meteor|Sound")
	TObjectPtr<class USoundBase>	mRainSound;

	// (10/26) 착탄 이펙트와 같은 자리에서 내는 소리. 0.3초마다 10번.
	// 10/29 : 위의 3초짜리 소리로 바꾸면서 비워 둠 (넣으면 착탄마다 겹쳐서 난다)
	UPROPERTY(EditAnywhere, Category = "Meteor|Sound")
	TObjectPtr<class USoundBase>	mImpactSound;

	// 착탄 소리 볼륨 배율. 10번 연속이라 Q 운석 한 방(1.0)보다 작게.
	UPROPERTY(EditAnywhere, Category = "Meteor|Sound")
	float	mImpactSoundVolume = 0.5f;

	// ---------------------------------------------------------------- 수치
	// 반지름(cm), 지속 시간(초), 피해 주기(초). 3초 ÷ 0.3초 = 10번.
	// 10/16 : 반지름 6m → 7.2m (1.2배, 못 움직이는 궁극기의 보상)
	UPROPERTY(EditAnywhere, Category = "Meteor")
	float	mRadius = 720.f;

	UPROPERTY(EditAnywhere, Category = "Meteor")
	float	mDuration = 3.f;

	UPROPERTY(EditAnywhere, Category = "Meteor")
	float	mInterval = 0.3f;

	// 한 번에 주는 피해. 어빌리티가 SetDamage로 넣어준다. (공격력 × 배율)
	float	mDamage = 0.f;

	// 지금까지 피해를 준 횟수 / 전체 횟수
	int32	mPulseCount = 0;
	int32	mPulseTotal = 0;

	FTimerHandle	mPulseTimer;

public:
	void SetDamage(float Damage)
	{
		mDamage = Damage;
	}

protected:
	virtual void BeginPlay() override;

	// 주기마다 한 번 : 범위 피해 + 착탄 이펙트. 마지막 횟수가 끝나면 이펙트를 끄고 사라진다.
	void Pulse();
};
