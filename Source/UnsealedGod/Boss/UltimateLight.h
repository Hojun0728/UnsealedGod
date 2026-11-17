// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/Actor.h"
#include "UltimateLight.generated.h"

// ============================================================================
// AUltimateLight
// 필살기(기둥 엄폐 궁극기)의 붉은 빛. (10/22)
//
// 보스 가슴(= 엄폐 판정 라인 트레이스가 시작되는 점)에 그림자를 만드는 포인트 라이트를 둔다.
// → 기둥 뒤에 "실제 그림자"가 생기고, 그 그림자가 곧 안전지대다. 판정(선이 기둥에 막힘)과 화면(빛이 기둥에 막힘)이 정확히 같다.
//
// 흐름 : StartCharge(시간) — 바닥 원이 차오르는 동안 빛이 점점 밝아진다
//        Flash()          — 충격 순간 가장 밝게 번쩍 → mFadeTime 동안 꺼지고 스스로 사라진다
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AUltimateLight : public AActor
{
	GENERATED_BODY()

public:
	AUltimateLight();

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UPointLightComponent>	mLight;

	// 차오르는 시간(초), 차오른 끝 밝기, 번쩍 밝기, 번쩍 뒤 꺼지는 시간(초)
	float	mChargeTime = 2.5f;
	float	mChargeIntensity = 50000.f;
	float	mFlashIntensity = 400000.f;
	float	mFadeTime = 0.6f;

	float	mElapsed = 0.f;
	bool	mFlashing = false;

public:
	// 값 넣기 (보스의 디테일 값을 패턴이 넘겨준다 → 에디터에서 숫자로 조정)
	void Setup(const FLinearColor& Color, float Radius, float ChargeIntensity, float FlashIntensity, float FadeTime);

	void StartCharge(float ChargeTime);
	void Flash();

	virtual void Tick(float DeltaTime) override;
};
