// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_PlayerSkill.h"
#include "GameplayAbility_Burden.generated.h"

// ============================================================================
// UGameplayAbility_Burden
// 기디언 F "버든" : 빠른 저주탄을 쏜다. 맞은 적은 약 4초 동안 0.5초마다 피해를 입는다(GE_Burn).
// 몽타주 : AM_Gideon_Burden (RMB_Intro → RMB_Cast), 기를 모은 어퍼컷이 가장 높이 올라간 순간에 Event.Skill.Fire
//   (10/16 : Q 포털 운석과 모션을 맞바꿨다)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Burden : public UGameplayAbility_PlayerSkill
{
	GENERATED_BODY()

public:
	UGameplayAbility_Burden();

protected:
	// 저주탄이 나갈 손 소켓. 어퍼컷 손. (평타 C와 같은 오른손)
	FName	mSocketName = TEXT("Muzzle_01");

	// "한 번(0.5초마다)" 피해 = 공격력 × 배율 (10 × 0.6 = 6) → 8번 = 48
	float	mTickDamageScale = 0.6f;

	// 손에서 번쩍이는 이펙트.
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mMuzzleParticle;

protected:
	virtual void OnFire(class APlayerCharacter* Player) override;
};
