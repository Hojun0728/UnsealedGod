// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ProjectileBase.h"
#include "PortalBlastProjectile.generated.h"

// ============================================================================
// APortalBlastProjectile
// 기디언의 기본 공격(포털 블라스트) 투사체. (수업 코드의 AWukongProjectile과 같은 역할)
// 부모(AProjectileBase)의 동작을 그대로 쓰고, 생성자에서 속도·이펙트만 기디언 것으로 바꾼다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API APortalBlastProjectile : public AProjectileBase
{
	GENERATED_BODY()

public:
	APortalBlastProjectile();
};
