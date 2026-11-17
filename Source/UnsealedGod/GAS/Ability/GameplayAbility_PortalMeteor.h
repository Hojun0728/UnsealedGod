// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_PlayerSkill.h"
#include "GameplayAbility_PortalMeteor.generated.h"

// ============================================================================
// UGameplayAbility_PortalMeteor
// 기디언 Q "포털 운석" : 손짓으로 조준점 하늘에 포털을 열고, 운석 하나를 수직으로 떨어뜨린다.
// 땅에 닿으면 범위 폭발. (한 방 딜)
// 몽타주 : AM_Gideon_PortalMeteor (CosmicRift_Start → CosmicRift_Execute), "오라이" 순간에 Event.Skill.Fire
//   10/16 : 처음엔 어퍼컷(RMB)으로 손에서 쏘려 했지만, 코즈믹 리프트 모션이 원래 "멀리 운석을 부르는" 동작이라
//           F 버든과 모션을 맞바꿨다.
// 흐름은 부모(UGameplayAbility_PlayerSkill)가 처리하고, 여기서는 OnFire(운석 떨어뜨리기)만 채운다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_PortalMeteor : public UGameplayAbility_PlayerSkill
{
	GENERATED_BODY()

public:
	UGameplayAbility_PortalMeteor();

protected:
	// 조준할 수 있는 최대 수평 거리(cm).
	float	mMaxDistance = 1500.f;

	// 포털이 열리는 높이(cm). 바닥에서 8m 위 → 운석 속도 20m/s로 약 0.4초 뒤 착탄.
	float	mDropHeight = 800.f;

	// 폭발 피해 = 공격력 × 배율 (10 × 4 = 40)
	float	mDamageScale = 4.f;

	// 하늘에 열리는 포털 이펙트.
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mPortalParticle;

protected:
	virtual void OnFire(class APlayerCharacter* Player) override;
};
