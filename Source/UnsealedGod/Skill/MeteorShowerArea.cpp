// Fill out your copyright notice in the Description page of Project Settings.


#include "MeteorShowerArea.h"
#include "../Character/GameCharacter.h"
#include "Sound/SoundBase.h"

AMeteorShowerArea::AMeteorShowerArea()
{
	// 주기마다 할 일은 타이머로 처리하므로 Tick은 끈다.
	PrimaryActorTick.bCanEverTick = false;

	mRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	mPortalFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("PortalFX"));
	mShowerFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ShowerFX"));

	SetRootComponent(mRoot);
	mPortalFX->SetupAttachment(mRoot);
	mShowerFX->SetupAttachment(mRoot);

	// ---------------------------------------------------------------- 이펙트 (기디언 팩의 Meteor 폴더)
	// 이펙트 크기·높이는 실제로 보고 조정한다. (원래 팩의 운석은 반지름이 이보다 작을 수 있다)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		PortalAsset(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Meteor/FX/P_Gideon_Meteor_Portal.P_Gideon_Meteor_Portal'"));

	if (PortalAsset.Succeeded())
		mPortalFX->SetTemplate(PortalAsset.Object);

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ShowerAsset(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Meteor/FX/P_Gideon_Meteor_Shower.P_Gideon_Meteor_Shower'"));

	if (ShowerAsset.Succeeded())
		mShowerFX->SetTemplate(ShowerAsset.Object);

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ImpactAsset(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Meteor/FX/P_Gideon_MeteorBigImpact.P_Gideon_MeteorBigImpact'"));

	if (ImpactAsset.Succeeded())
		mImpactParticle = ImpactAsset.Object;

	// (10/29) 소리 : 착탄마다 10번 → 장판 3초 동안 "삐슝·쾅·우르르"가 이어지는 소리 하나로 바꿈
	// (마법 운석 두 개를 이어 붙이고 돌 떨어지는 소리를 밑에 깐 3초짜리). 착탄마다 내던 칸(mImpactSound)은 비워 둔다
	static ConstructorHelpers::FObjectFinder<USoundBase>
		RainSound(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_MeteorRain.S_SFX_MeteorRain'"));

	if (RainSound.Succeeded())
		mRainSound = RainSound.Object;
}

void AMeteorShowerArea::BeginPlay()
{
	Super::BeginPlay();

	mPulseCount = 0;

	// 장판이 생기는 순간 3초짜리 운석 비 소리를 한 번. 장판(루트)에 붙인다 → 장판이 일찍 지워지면 소리도 함께 끊긴다
	if (IsValid(mRainSound))
		UGameplayStatics::SpawnSoundAttached(mRainSound, mRoot);

	// 전체 횟수 = 지속 시간 ÷ 주기 (3 ÷ 0.3 = 10). RoundToInt : 반올림해서 정수로.
	mPulseTotal = FMath::Max(FMath::RoundToInt(mDuration / mInterval), 1);

	// SetTimer(핸들, 객체, 함수, 주기, 반복 여부, 첫 실행까지 기다릴 시간)
	// 첫 피해도 한 주기 뒤 : 운석이 떨어지기 시작하는 시간을 준다.
	GetWorldTimerManager().SetTimer(mPulseTimer, this, &AMeteorShowerArea::Pulse,
		mInterval, true, mInterval);
}

void AMeteorShowerArea::Pulse()
{
	++mPulseCount;

	// 놓은 사람(기디언)의 범위 공격 함수를 빌려 쓴다. GetOwner()는 AActor*라서 Cast로 바꿔야 한다.
	AGameCharacter* Caster = Cast<AGameCharacter>(GetOwner());

	if (IsValid(Caster) && mDamage > 0.f)
		Caster->ApplyAreaDamage(GetActorLocation(), mRadius, mDamage);

	// 원 안의 아무 곳에 착탄 이펙트 + 소리. (10/26 : 소리가 이펙트와 같은 점에서 나도록 위치를 먼저 정한다)
	// RandPointInCircle : 반지름 안의 랜덤한 2D 점. (원 안에 고르게 퍼진다)
	FVector2D	Random = FMath::RandPointInCircle(mRadius);
	FVector		ImpactLocation = GetActorLocation() + FVector(Random.X, Random.Y, 0.0);

	if (IsValid(mImpactParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mImpactParticle, ImpactLocation,
			FRotator::ZeroRotator, FVector(0.6));
	}

	// PlaySoundAtLocation(월드, 소리, 위치, 볼륨 배율)
	if (IsValid(mImpactSound))
		UGameplayStatics::PlaySoundAtLocation(this, mImpactSound, ImpactLocation, mImpactSoundVolume);

	// 마지막 횟수 : 타이머를 멈추고 이펙트를 끈 뒤, 남은 입자가 사라질 1초 뒤에 삭제한다.
	if (mPulseCount >= mPulseTotal)
	{
		GetWorldTimerManager().ClearTimer(mPulseTimer);

		mPortalFX->DeactivateSystem();
		mShowerFX->DeactivateSystem();

		SetLifeSpan(1.f);
	}
}
