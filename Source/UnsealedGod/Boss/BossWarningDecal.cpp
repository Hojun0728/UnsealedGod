// Fill out your copyright notice in the Description page of Project Settings.


#include "BossWarningDecal.h"

ABossWarningDecal::ABossWarningDecal()
{
	// Progress를 매 프레임 올려야 하므로 Tick을 켠다.
	PrimaryActorTick.bCanEverTick = true;

	mRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(mRoot);

	mDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	mDecal->SetupAttachment(mRoot);

	// 데칼은 자신의 X축 방향으로 투영된다. Pitch -90 → X축이 아래를 보게 해서 바닥에 찍는다. (수업 코드와 동일)
	mDecal->SetRelativeRotation(FRotator(-90.0, 0.0, 0.0));
}

void ABossWarningDecal::StartWarning(UMaterialInterface* Material, float Radius, float Duration)
{
	// DecalSize는 "반" 크기다. (X = 투영 깊이, Y·Z = 가로·세로 반지름)
	// 깊이 200 : 바닥이 조금 울퉁불퉁하거나 기울어도 위아래 2m 안이면 찍힌다.
	mDecal->DecalSize = FVector(200.0, Radius, Radius);

	ApplyMaterial(Material, Duration);
}

void ABossWarningDecal::StartWarningBox(UMaterialInterface* Material, float HalfLength,
	float HalfWidth, float Duration)
{
	// 데칼은 Pitch -90으로 아래를 보고 있다. 이때 데칼의 축이 월드에서 어디를 향하는지 :
	//   데칼 X(투영 방향) → 아래, 데칼 Y → 액터의 오른쪽(Y), 데칼 Z(원래 위쪽) → 액터의 앞(X)
	// 그래서 DecalSize의 Y = 좌우 반 폭, Z = 앞뒤 반 길이.
	mDecal->DecalSize = FVector(200.0, HalfWidth, HalfLength);

	ApplyMaterial(Material, Duration);
}

void ABossWarningDecal::ApplyMaterial(UMaterialInterface* Material, float Duration)
{
	mDuration = FMath::Max(Duration, 0.01f);
	mElapsed = 0.f;

	if (!IsValid(Material))
	{
		UE_LOG(UGDebug, Warning, TEXT("BossWarningDecal : Material is null (M_Decal_Warning이 있는지 확인)"));
		return;
	}

	mDecal->SetDecalMaterial(Material);

	// 다이내믹 머티리얼 인스턴스를 만들어 데칼에 적용한다. 이후 이 인스턴스의 파라미터만 바꾼다.
	mDynamicMaterial = mDecal->CreateDynamicMaterialInstance();

	if (IsValid(mDynamicMaterial))
		mDynamicMaterial->SetScalarParameterValue(TEXT("Progress"), 0.f);
}

void ABossWarningDecal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	mElapsed += DeltaTime;

	// 흐른 비율 0 ~ 1. 1이 되는 순간 공격이 떨어진다.
	float	Progress = FMath::Clamp(mElapsed / mDuration, 0.f, 1.f);

	if (IsValid(mDynamicMaterial))
		mDynamicMaterial->SetScalarParameterValue(TEXT("Progress"), Progress);
}
