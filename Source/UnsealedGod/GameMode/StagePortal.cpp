// Fill out your copyright notice in the Description page of Project Settings.


#include "StagePortal.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundBase.h"

AStagePortal::AStagePortal()
{
	// 매 프레임 할 일이 없다. (열기·들어가기 모두 이벤트로 처리)
	PrimaryActorTick.bCanEverTick = false;

	mTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	mPortalFX = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("PortalFX"));

	SetRootComponent(mTrigger);
	mPortalFX->SetupAttachment(mTrigger);

	// 들어가는 범위 : 반 크기(가로·세로·높이) = 1m × 1.5m × 2m. 포탈 앞을 지나가면 닿는다.
	mTrigger->SetBoxExtent(FVector(100.f, 150.f, 200.f));

	// "Trigger" 프로필 : 무엇도 막지 않고 겹침(Overlap)만 알려 준다. (걸어서 통과)
	mTrigger->SetCollisionProfileName(TEXT("Trigger"));

	// 처음엔 꺼 둔다. BeginPlay에서 숨기고 충돌도 끈다.
	// bAutoActivate = false : 레벨이 시작될 때 이펙트가 저절로 켜지지 않게 한다. (Open에서 켠다)
	mPortalFX->bAutoActivate = false;

	// 이펙트는 상자 가운데(바닥에서 2m 높이)에 세워서 크게. 크기·방향은 레벨에서 보고 디테일에서 조정한다.
	mPortalFX->SetRelativeScale3D(FVector(3.f));

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		PortalAsset(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Portal/FX/P_Portal_Entrance.P_Portal_Entrance'"));

	if (PortalAsset.Succeeded())
		mPortalFX->SetTemplate(PortalAsset.Object);
}

void AStagePortal::BeginPlay()
{
	Super::BeginPlay();

	// 보스와 싸우는 동안에는 없는 것처럼 : 숨기고 충돌을 끈다.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// 다른 액터를 포탈 모습으로 쓰면 그것도 숨겨 둔다. (그 블루프린트의 Auto Open은 레벨에서 꺼 둔다)
	if (IsValid(mVisualActor))
		mVisualActor->SetActorHiddenInGame(true);

	// OnComponentBeginOverlap : 상자에 무언가 들어오면 알려 주는 델리게이트. (투사체의 OnProjectileStop과 같은 방식)
	mTrigger->OnComponentBeginOverlap.AddDynamic(this, &AStagePortal::OnTriggerBeginOverlap);
}

void AStagePortal::Open()
{
	if (mOpened)
		return;

	mOpened = true;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (IsValid(mVisualActor))
	{
		// 레벨에 놓은 포탈 액터를 보이게 하고, 그 블루프린트의 "열기" 함수를 이름으로 찾아 부른다.
		// FindFunction(이름) : 블루프린트에 그 이름의 함수가 있으면 돌려준다. ProcessEvent : 그 함수를 실행한다. (인자 없는 함수라 nullptr)
		// → C++이 PortalFX 블루프린트를 몰라도(헤더 없이) 함수를 부를 수 있다.
		mVisualActor->SetActorHiddenInGame(false);

		UFunction* OpenFunction = mVisualActor->FindFunction(mVisualOpenFunction);

		if (OpenFunction)
			mVisualActor->ProcessEvent(OpenFunction, nullptr);
		else
			UE_LOG(UGDebug, Warning, TEXT("Portal : %s has no function %s"), *mVisualActor->GetName(),
				*mVisualOpenFunction.ToString());
	}
	else
	{
		mPortalFX->Activate(true);
	}

	if (IsValid(mOpenSound))
		UGameplayStatics::PlaySoundAtLocation(this, mOpenSound, GetActorLocation());

	UE_LOG(UGDebug, Warning, TEXT("Portal : opened (next level %s)"), *mNextLevelName.ToString());
}

void AStagePortal::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어(기디언)만. 미니언 잔해나 투사체가 닿아도 무시한다.
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (!mOpened || mEntering || OtherActor != PlayerPawn)
		return;

	mEntering = true;

	// 입력을 막고(포탈 앞에서 되돌아 나가지 않게) 화면을 검게 만든다.
	// StartCameraFade(시작 투명도, 끝 투명도, 시간, 색, 소리도 줄일지, 끝난 뒤 유지할지)
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (IsValid(PlayerController))
	{
		PlayerPawn->DisableInput(PlayerController);

		if (IsValid(PlayerController->PlayerCameraManager))
			PlayerController->PlayerCameraManager->StartCameraFade(0.f, 1.f, mEnterFadeTime, FLinearColor::Black, true, true);
	}

	GetWorldTimerManager().SetTimer(mTravelTimer, this, &AStagePortal::Travel, FMath::Max(mEnterFadeTime, 0.01f), false);
}

void AStagePortal::Travel()
{
	// OpenLevel : 레벨을 새로 연다. 지금 레벨의 모든 액터(기디언 포함)가 사라지고 새로 만들어지므로
	// HP·MP·물약은 자연스럽게 처음 값으로 돌아간다. (직접 결정 : 스테이지마다 초기화)
	UE_LOG(UGDebug, Warning, TEXT("Portal : travel to %s"), *mNextLevelName.ToString());

	UGameplayStatics::OpenLevel(this, mNextLevelName);
}
