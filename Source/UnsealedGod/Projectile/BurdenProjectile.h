// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProjectileBase.h"
#include "BurdenProjectile.generated.h"

// ============================================================================
// ABurdenProjectile
// 기디언 F "버든"의 투사체. 적에게 맞으면 지속 피해(GE_Burn)를 건다.
//
// 피해 자체는 GAS의 주기 효과(GE_Burn)가 알아서 0.5초마다 준다.
// 이 투사체는 맞은 뒤에도 바로 사라지지 않고, 몸을 숨긴 채 도트 시간만큼 남아서
// 대상에 붙인 도트 이펙트를 끝날 때 꺼 주는 역할을 한다.
// mDamage = "한 번(0.5초마다)" 깎는 양.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API ABurdenProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	ABurdenProjectile();

protected:
	// 맞은 대상의 몸에 붙는 도트 이펙트.
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TObjectPtr<UParticleSystem>		mDoTParticle;

	// 대상에 붙여 둔 도트 이펙트. 도트가 끝나면 끈다.
	// 대상(미니언)이 먼저 죽어 삭제되면 이 컴포넌트도 같이 삭제되므로 쓰기 전에 IsValid로 확인한다.
	UPROPERTY()
	TObjectPtr<UParticleSystemComponent>	mDoTComponent;

	// 적에게 맞아서 도트를 걸었는지. (수명이 끝날 때 "빗나감 이펙트"를 낼지 정한다)
	bool	mAttached = false;

public:
	virtual void StopCallback(const FHitResult& ImpactResult) override;

protected:
	virtual void LifeSpanExpired() override;

	// 대상에게 GE_Burn을 건다. 걸었으면 true.
	bool ApplyBurn(AActor* Target, const FHitResult& ImpactResult);
};
