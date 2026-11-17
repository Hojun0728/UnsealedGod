// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

// ============================================================================
// AProjectileBase
// 모든 투사체의 공통 부모 클래스다. (수업 코드의 AProjectileBaseActor와 같은 역할)
// 날아가기(ProjectileMovement), 부딪히기(Sphere 충돌체), 꼬리 이펙트, 수명,
// "맞았을 때 데미지 주고 터지기"까지 공통으로 처리한다.
//
// 자식 클래스(APortalBlastProjectile 등)는 생성자에서 속도·크기·이펙트만 바꾼다.
// 명중 처리를 다르게 하고 싶은 투사체(예 : 범위 폭발하는 Q 운석)는
// virtual 함수인 StopCallback을 override해서 바꾼다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AProjectileBase();

protected:
	// ---------------------------------------------------------------- 컴포넌트
	// 충돌체. 마법탄은 둥글기 때문에 수업 코드의 Box 대신 Sphere를 쓴다.
	// 루트 컴포넌트라서 이 구체의 위치 = 투사체의 위치다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent>	mBody;

	// 투사체를 날려주는 이동 컴포넌트. 속도·중력·튕김을 알아서 계산하고,
	// 무언가에 막혀 멈추면 OnProjectileStop 델리게이트로 알려준다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent>	mMovement;

	// 날아가는 동안 보이는 이펙트(꼬리). 자식 클래스가 템플릿을 정한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UParticleSystemComponent>	mParticle;

	// ---------------------------------------------------------------- 이펙트 에셋
	// 캐릭터(ASC가 있는 대상)에 맞았을 때 터지는 이펙트.
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TObjectPtr<UParticleSystem>		mHitCharacterParticle;

	// 벽·바닥 같은 월드에 맞았을 때 터지는 이펙트.
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TObjectPtr<UParticleSystem>		mHitWorldParticle;

	// 아무것도 못 맞히고 수명이 끝났을 때 사라지는 이펙트.
	UPROPERTY(EditAnywhere, Category = "Projectile")
	TObjectPtr<UParticleSystem>		mNoHitParticle;

	// ---------------------------------------------------------------- 사운드 에셋 (10/26)
	// 명중 이펙트와 같은 자리, 같은 순간에 낸다. 비워 두면 소리 없음. 자식 클래스가 생성자에서 정한다.
	// USoundBase : 사운드 웨이브·사운드 큐·메타사운드의 공통 부모 → 어떤 종류의 소리든 넣을 수 있다.
	UPROPERTY(EditAnywhere, Category = "Projectile|Sound")
	TObjectPtr<class USoundBase>	mHitCharacterSound;

	UPROPERTY(EditAnywhere, Category = "Projectile|Sound")
	TObjectPtr<class USoundBase>	mHitWorldSound;

	// ---------------------------------------------------------------- 수치
	// 수명(초). 허공으로 쏜 투사체가 영원히 남지 않게 한다.
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float	mLifeTime = 3.f;

	// 최대 사거리(cm). 0보다 크면 수명을 "사거리 ÷ 속도"로 바꿔서 이 거리에서 사라진다.
	// 사거리 끝에서는 mNoHitParticle(빗나감 이펙트)이 터져서 플레이어가 사거리를 눈으로 알 수 있다.
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float	mMaxRange = 0.f;

	// 이 투사체의 기본 크기 배율. 충돌 구체와 꼬리 이펙트가 함께 커진다.
	// 자식 클래스가 생성자에서 정한다. (포털 블라스트 = 1.2)
	UPROPERTY(EditAnywhere, Category = "Projectile")
	float	mBaseScale = 1.f;

	// 맞혔을 때 줄 피해량. 발사하는 쪽(캐릭터)이 SetDamage로 넣어준다.
	float	mDamage = 0.f;

	// (10/21) 강타 여부. 캐릭터에 맞히면 히트스톱·흔들림을 요청한다. (평타 막타 C)
	bool	mStrongHit = false;

public:
	void SetDamage(float Damage)
	{
		mDamage = Damage;
	}

	void SetStrongHit(bool bStrongHit)
	{
		mStrongHit = bStrongHit;
	}

	// 최종 크기 = 기본 크기(mBaseScale) × Multiplier.
	// Multiplier는 발사하는 쪽이 넘겨준다. (막타 노티파이 = 1.5 → 1.2 × 1.5 = 1.8배)
	// 발사할 때마다 반드시 호출해야 기본 크기(1.2)가 적용된다. (배율이 1.0이어도)
	void SetProjectileScale(float Multiplier);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// SetLifeSpan으로 정한 수명이 끝나면 엔진이 호출하는 AActor의 virtual 함수다.
	// 여기서는 "못 맞히고 사라지는 이펙트"를 추가하기 위해 덮어쓴다.
	virtual void LifeSpanExpired() override;

public:
	// mMovement의 OnProjectileStop 델리게이트에 등록할 함수.
	// AddDynamic으로 등록하는 델리게이트(다이나믹 델리게이트)는 UFUNCTION()이 꼭 붙어 있어야 한다.
	UFUNCTION()
	void ProjectileStop(const FHitResult& ImpactResult);

	// 투사체가 무언가에 맞아 멈췄을 때의 처리. (수업 코드와 같은 이름)
	// 기본 동작 : 데미지 적용 → 명중 이펙트 → 삭제
	virtual void StopCallback(const FHitResult& ImpactResult);

protected:
	// 맞은 대상에게 GAS로 데미지를 준다. 대상에게 ASC가 없으면(벽 등) 아무것도 하지 않는다.
	// 반환값 : 데미지를 줬으면 true (명중 이펙트를 고를 때 사용)
	bool ApplyDamageToTarget(AActor* Target, const FHitResult& ImpactResult);

	// (10/26) 명중 소리를 Location에서 낸다. Sound가 비어 있으면 아무것도 안 한다. (자식 클래스의 폭발에서도 쓴다)
	void PlayHitSound(class USoundBase* Sound, const FVector& Location);

	// (10/28) 맞은 대상이 정면 방패(오로라 얼음 돌진) 중인 보스이고 정면에 맞았으면 → 피해 없이 이 투사체를 지우고 true.
	// StopCallback 맨 앞에서 부른다. (자식 버든도 피해를 주기 전에 부른다)
	bool TryBlockByFrontGuard(const FHitResult& ImpactResult);
};
