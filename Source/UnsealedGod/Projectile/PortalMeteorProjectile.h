// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProjectileBase.h"
#include "PortalMeteorProjectile.generated.h"

// ============================================================================
// APortalMeteorProjectile
// 기디언 Q "포털 운석"의 투사체. 맞으면 그 자리에서 범위 폭발한다. (한 방 딜)
//
// 부모(AProjectileBase)의 기본 명중 처리는 "맞은 대상 하나에게 피해"다.
// 운석은 "맞은 지점 주변 전체에 피해"라서 virtual 함수 StopCallback을 덮어쓴다.
// 사거리 끝까지 아무것도 못 맞혀도 그 자리에서 터진다. (LifeSpanExpired도 덮어쓴다)
// 폭발 피해는 SetDamage로 받은 값(mDamage)을 그대로 쓴다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API APortalMeteorProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	APortalMeteorProjectile();

protected:
	// 폭발 반지름(cm). 수평 거리로 판정한다.
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float	mExplosionRadius = 250.f;

public:
	virtual void StopCallback(const FHitResult& ImpactResult) override;

protected:
	virtual void LifeSpanExpired() override;

	// Location을 중심으로 범위 피해를 준다. (이펙트는 부르는 쪽에서)
	void Explode(const FVector& Location);
};
