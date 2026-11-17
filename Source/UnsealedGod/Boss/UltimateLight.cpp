// Fill out your copyright notice in the Description page of Project Settings.


#include "UltimateLight.h"
#include "Components/PointLightComponent.h"

AUltimateLight::AUltimateLight()
{
	PrimaryActorTick.bCanEverTick = true;

	mLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	SetRootComponent(mLight);

	// 움직이는(무버블) 라이트 : 실행 중에 밝기를 바꾸고 실시간 그림자를 만든다.
	mLight->SetMobility(EComponentMobility::Movable);
	mLight->SetCastShadows(true);
	mLight->SetIntensity(0.f);

	// 밝기 단위 : 칸델라 (값이 커야 아레나 끝까지 닿는다)
	mLight->SetIntensityUnits(ELightUnits::Candelas);
}

void AUltimateLight::Setup(const FLinearColor& Color, float Radius, float ChargeIntensity, float FlashIntensity,
	float FadeTime)
{
	mLight->SetLightColor(Color);

	// AttenuationRadius : 빛이 닿는 최대 거리(cm). 아레나 반지름(20m)보다 커야 벽까지 비춘다.
	mLight->SetAttenuationRadius(Radius);

	mChargeIntensity = ChargeIntensity;
	mFlashIntensity = FlashIntensity;
	mFadeTime = FadeTime;
}

void AUltimateLight::StartCharge(float ChargeTime)
{
	mChargeTime = FMath::Max(ChargeTime, 0.01f);
	mElapsed = 0.f;
	mFlashing = false;
}

void AUltimateLight::Flash()
{
	mFlashing = true;
	mElapsed = 0.f;
	mLight->SetIntensity(mFlashIntensity);
}

void AUltimateLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	mElapsed += DeltaTime;

	if (!mFlashing)
	{
		// 차오르기 : 제곱 곡선 → 처음엔 은은하다가 충격 직전에 급격히 밝아진다 (긴장감)
		float	Alpha = FMath::Clamp(mElapsed / mChargeTime, 0.f, 1.f);

		mLight->SetIntensity(mChargeIntensity * Alpha * Alpha);
		return;
	}

	// 번쩍 → 꺼지기. (1 - a)² : 처음에 확 떨어지고 끝이 부드럽다 → "펑!" 하고 터지는 느낌 (10/22 내 판단 : 터질 때만 확)
	float	Alpha = mFadeTime > 0.f ? FMath::Clamp(mElapsed / mFadeTime, 0.f, 1.f) : 1.f;

	mLight->SetIntensity(mFlashIntensity * FMath::Square(1.f - Alpha));

	if (Alpha >= 1.f)
		Destroy();
}
