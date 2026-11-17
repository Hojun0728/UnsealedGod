// Fill out your copyright notice in the Description page of Project Settings.


#include "ArenaGameMode.h"
#include "../Player/Gideon.h"
#include "../Boss/BossBase.h"
#include "../UI/ResultWidget.h"
#include "../UI/PlayerHUDWidget.h"
#include "../UI/PauseWidget.h"
#include "../UI/BossIntroWidget.h"
#include "StagePortal.h"
// AAmbientSound : 레벨의 배경음 액터 / UAudioComponent : 소리를 줄이며 끄기(FadeOut)
#include "Sound/AmbientSound.h"
#include "Components/AudioComponent.h"
// TActorIterator : 레벨에 있는 특정 클래스의 액터를 하나씩 돌아보는 반복자
#include "EngineUtils.h"
// UCameraShakeBase : 카메라 흔들림 블루프린트(CS_Light·CS_Heavy)의 부모 클래스
#include "Camera/CameraShakeBase.h"
#include "Sound/SoundBase.h"
// (11/5) 레벨에 놓은 시퀀스(시작 연출 카메라)를 찾아 재생
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
// (11/5) 보스 뒤 배경 띠 : 레벨 후처리 볼륨에 머티리얼을 얹는다
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
// (11/5) 맵 둘러보기 카메라 (시퀀스가 없을 때)
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

AArenaGameMode::AArenaGameMode()
{
	// DefaultPawnClass : 게임이 시작될 때 PlayerStart 위치에 생성되어 플레이어가 조종할 Pawn 클래스.
	// StaticClass() : 객체가 아니라 "AGideon이라는 클래스 정보"를 반환한다.
	DefaultPawnClass = AGideon::StaticClass();

	// (11/5) 맵 둘러보기 카메라를 매 프레임 옮기려고 Tick을 켠다 (둘러보기 중이 아니면 Tick은 아무것도 안 한다)
	PrimaryActorTick.bCanEverTick = true;
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	// (11/4) 타격감 표의 흔들림·소리를 미리 불러 둔다. (화면이 검은 페이드인 동안이라 멈칫해도 안 보인다)
	PreloadFeedbackAssets();

	// HUD 위젯 블루프린트를 불러와 화면에 붙인다. (결과 화면과 같은 방식 : 경로로 불러와서 재시작 없이)
	TSubclassOf<UPlayerHUDWidget>	WidgetClass = LoadClass<UPlayerHUDWidget>(nullptr, *mHUDWidgetPath);
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// (10/22) 입력 모드를 항상 "게임 전용"으로 되돌린다.
	// 결과 화면이 버튼을 누르게 하려고 UI 전용(FInputModeUIOnly)으로 바꾸는데, 이 설정은 게임 뷰포트에 "게임 입력 무시"를 켠다.
	// 게임 뷰포트는 레벨을 다시 열어도 그대로 남기 때문에, 다시 시도하면 키보드·마우스가 전부 먹히지 않았다.
	// → 레벨이 시작될 때마다 여기서 게임 전용으로 되돌리고 커서를 숨긴다.
	if (IsValid(PlayerController))
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);

		// (11/3) 검은 화면에서 시작해 서서히 밝아진다. (타이틀 시작 연출·스테이지 포탈이 화면을 검게 만든 뒤 레벨을 연다)
		// StartCameraFade(시작 불투명도 1 = 검정, 끝 0 = 원래 화면, 시간, 색, 소리도, 끝난 뒤 유지 안 함)
		if (IsValid(PlayerController->PlayerCameraManager))
			PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, mLevelFadeInTime, FLinearColor::Black, false, false);
	}

	// (10/26) 다음 스테이지 포탈이 레벨에 있는지 찾아 둔다. 있으면 승리 = 스테이지 클리어.
	for (TActorIterator<AStagePortal> It(GetWorld()); It; ++It)
	{
		mStagePortal = *It;
		break;
	}

	if (!WidgetClass || !IsValid(PlayerController))
	{
		UE_LOG(UGDebug, Warning, TEXT("HUD : WBP_HUD not found"));
		return;
	}

	mHUDWidget = CreateWidget<UPlayerHUDWidget>(PlayerController, WidgetClass);

	if (IsValid(mHUDWidget))
		mHUDWidget->AddToViewport();

	// ---------------------------------------------------------------- 등장 연출 (10/21)
	// OptionsString : 레벨을 열 때 붙인 옵션 문자열. HasOption(옵션 문자열, 이름) : 그 옵션이 있는지.
	// (11/5) 다시 시도·RESTART는 "?Retry"를 붙여서 연다 → 시작 연출(맵 둘러보기·보스 소개) 없이 짧은 연출만.
	// (10/22까지 쓰던 "SkipIntro" 옵션은 아무도 안 붙여서 지웠다)
	mRetry = UGameplayStatics::HasOption(OptionsString, TEXT("Retry"));

	// HUD는 연출 동안 숨긴다. (첫 프레임부터 숨겨야 깜빡이지 않는다)
	if (IsValid(mHUDWidget))
		mHUDWidget->HideForIntro();

	// SetTimerForNextTick : 다음 프레임에 부른다. 레벨의 모든 액터 BeginPlay(보스 AI 시작 포함)가 끝난 뒤.
	GetWorldTimerManager().SetTimerForNextTick(this, &AArenaGameMode::StartIntro);
}

void AArenaGameMode::StartIntro()
{
	// 처음 들어왔으면 시작 연출. 다시 시도거나, 레벨에 시퀀스가 없으면 짧은 연출.
	if (!mRetry && TryStartCinematic())
		return;

	StartShortIntro();
}

void AArenaGameMode::StartShortIntro()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	// 레벨의 보스 (하나)
	ABossBase* Boss = nullptr;

	for (TActorIterator<ABossBase> It(GetWorld()); It; ++It)
	{
		Boss = *It;
		break;
	}

	float	BossTime = IsValid(Boss) ? Boss->StartIntro() : 0.f;

	// 기디언은 보스 쪽을 바라보며 시작한다.
	float	PlayerTime = 0.f;

	if (IsValid(Player))
		PlayerTime = Player->StartIntro(IsValid(Boss) ? Boss->GetActorLocation() : Player->GetActorLocation() + Player->GetActorForwardVector());

	// 둘 중 긴 동작이 끝나면 전투 시작. (몽타주가 없으면 짧은 대체 시간)
	float	IntroTime = FMath::Max(BossTime, PlayerTime);

	if (IntroTime <= 0.f)
		IntroTime = mIntroFallbackTime;

	UE_LOG(UGDebug, Warning, TEXT("Intro : %.2fs (Boss %.2f, Player %.2f)"), IntroTime, BossTime, PlayerTime);

	GetWorldTimerManager().SetTimer(mIntroTimer, this, &AArenaGameMode::EndIntro, IntroTime, false);
}

void AArenaGameMode::EndIntro()
{
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
		Player->EndIntro();

	EndBossIntro();

	if (IsValid(mHUDWidget))
		mHUDWidget->FadeIn(mHUDFadeInTime);
}

void AArenaGameMode::EndBossIntro()
{
	for (TActorIterator<ABossBase> It(GetWorld()); It; ++It)
		It->EndIntro();
}

// ============================================================================ 시작 연출 (11/5)

bool AArenaGameMode::TryStartCinematic()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	mIntroBoss = nullptr;

	for (TActorIterator<ABossBase> It(GetWorld()); It; ++It)
	{
		mIntroBoss = *It;
		break;
	}

	// 레벨에 놓은 시퀀스 중 태그 "Intro"가 붙은 것 (다른 시퀀스가 생겨도 헷갈리지 않게)
	mIntroSequence = nullptr;

	for (TActorIterator<ALevelSequenceActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(mIntroSequenceTag))
		{
			mIntroSequence = *It;
			break;
		}
	}

	ULevelSequencePlayer* SequencePlayer = IsValid(mIntroSequence) ? mIntroSequence->GetSequencePlayer() : nullptr;
	TSubclassOf<UBossIntroWidget>	WidgetClass = LoadClass<UBossIntroWidget>(nullptr, *mIntroWidgetPath);
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// (11/5) 시퀀스는 없어도 된다 (없으면 코드가 계산한 경로로 둘러본다)
	if (!IsValid(Player) || !IsValid(mIntroBoss) || !WidgetClass || !IsValid(PlayerController))
	{
		UE_LOG(UGDebug, Warning, TEXT("Intro : cinematic skipped (widget %d, boss %d)"),
			WidgetClass != nullptr, IsValid(mIntroBoss));
		return false;
	}

	mCinematicPlaying = true;

	// ① 보스는 숨기고 멈춘다(빈 아레나를 먼저). 기디언은 입력만 잠그고 보스 쪽을 본다(카메라가 돌아올 때를 위해).
	mIntroBoss->HoldForIntro(true);
	Player->LockForIntro(mIntroBoss->GetActorLocation());

	mIntroWidget = CreateWidget<UBossIntroWidget>(PlayerController, WidgetClass);

	if (IsValid(mIntroWidget))
		mIntroWidget->AddToViewport(5);

	// (11/5) 보스 뒤 배경 띠 머티리얼을 미리 만들어 둔다 (멈추는 순간 불러오면 그 순간 멈칫한다)
	PrepareIntroBand();

	float	Duration = 0.f;

	if (SequencePlayer)
	{
		// ---------------------------------------------------------------- 시퀀스가 있으면 시퀀스 (손으로 다듬은 카메라가 우선)
		// 끝에서 일시정지 : 시퀀스가 끝나도 마지막 카메라 구도를 그대로 둔다 → 그 구도에서 보스가 나타난다.
		// (꺼져 있으면 끝나는 순간 카메라가 기디언에게 튀어 돌아간다. 에디터 체크를 깜빡해도 되게 코드가 켠다)
		FMovieSceneSequencePlaybackSettings	Settings = mIntroSequence->PlaybackSettings;
		Settings.bPauseAtEnd = true;
		Settings.LoopCount.Value = 0;
		SequencePlayer->SetPlaybackSettings(Settings);
		SequencePlayer->Play();

		// GetDuration : 시퀀스 길이 (프레임 + 프레임 레이트) → AsSeconds로 초
		Duration = SequencePlayer->GetDuration().AsSeconds();
	}
	else
	{
		// ---------------------------------------------------------------- 없으면 코드가 계산한 경로 (11/5)
		StartFlyoverCamera(PlayerController, Player);

		Duration = mFlyoverDuration;
	}

	UE_LOG(UGDebug, Warning, TEXT("Intro : cinematic start (%s, flyover %.2fs)"),
		SequencePlayer ? TEXT("sequence") : TEXT("code path"), Duration);

	GetWorldTimerManager().SetTimer(mStageCardTimer, this, &AArenaGameMode::ShowStageCard, mStageCardDelay, false);
	GetWorldTimerManager().SetTimer(mCinematicTimer, this, &AArenaGameMode::OnFlyoverFinished,
		FMath::Max(Duration, 0.01f), false);

	return true;
}

// (11/5) 보스를 화면 왼쪽 1/3에 두려면 카메라를 오른쪽으로 몇 도 틀어야 하는지 (둘러보기 끝·따라가기·카드 공용)
// static 함수 : 이 파일 안에서만 쓰는 도우미 (게임 모드의 멤버 값을 쓰지 않는다)
static float GetThirdsYaw(float FieldOfView)
{
	return FMath::RadiansToDegrees(FMath::Atan(FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f)) / 3.f));
}

void AArenaGameMode::StartFlyoverCamera(APlayerController* PlayerController, APlayerCharacter* Player)
{
	const FIntroFlyoverSettings&	S = mIntroBoss->GetIntroFlyover();

	FVector	Center = mIntroBoss->GetArenaCenter();
	FVector	BossLocation = mIntroBoss->GetActorLocation();

	// ---------------------------------------------------------------- 마지막 구도 (보스 키의 비율로)
	// 보스 키 = 캡슐 높이(반높이 × 2, 크기 배율 적용 후). 발바닥 = 액터 위치(캡슐 가운데) - 반높이
	float	HalfHeight = mIntroBoss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float	BossHeight = HalfHeight * 2.f;
	FVector	BossFeet = BossLocation - FVector(0.0, 0.0, HalfHeight);

	FVector	ToPlayer = Player->GetActorLocation() - BossLocation;
	ToPlayer.Z = 0.0;

	FVector	Dir = ToPlayer.IsNearlyZero() ? mIntroBoss->GetActorForwardVector() : ToPlayer.GetSafeNormal();
	FVector	FinalPosition = BossFeet + Dir * (BossHeight * S.FinalDistanceRatio) + FVector(0.0, 0.0, BossHeight * S.FinalHeightRatio);

	// 바라보는 점 : 보스 발바닥에서 키 × FinalLookRatio 위. 카메라보다 높으면 살짝 올려다보는 구도(보스가 커 보인다)
	FVector	LookPoint = BossFeet + FVector(0.0, 0.0, BossHeight * S.FinalLookRatio);

	// 보스를 화면 왼쪽 1/3에 : 보스를 바라본 방향에서 오른쪽으로 Delta도 돌린다.
	// 화면 가로 1/3 지점 = 가운데에서 왼쪽으로 화면 폭의 1/6. 가운데~가장자리가 시야각의 절반이므로
	// Delta = atan( tan(시야각/2) × (1/6) ÷ (1/2) ) = atan( tan(시야각/2) / 3 )
	float		Delta = GetThirdsYaw(S.FieldOfView);
	FRotator	FinalRotation = (LookPoint - FinalPosition).Rotation();
	FinalRotation.Yaw += Delta;

	// (11/5) 올려다보며 도착 (세바로그 : 하늘의 망치 쪽, 웅크린 첫 자세는 화면 아래 밖). Pitch + = 위로
	FinalRotation.Pitch += S.FinalLookUpDegrees;

	FVector	FinalTarget = FinalPosition + FinalRotation.Vector() * 500.0;

	// ---------------------------------------------------------------- 돌기 구간 (중심을 축으로, 끝 = 마지막 구도와 같은 방향)
	// Atan2(y, x) : 중심에서 본 마지막 카메라의 방향(라디안). 돌기는 OrbitDegrees만큼 "앞"에서 시작해 이 방향에서 끝난다.
	FVector	FromCenter = FinalPosition - Center;
	float	EndAngle = FMath::RadiansToDegrees(FMath::Atan2(FromCenter.Y, FromCenter.X));
	float	StartAngle = EndAngle - S.OrbitDegrees;
	FVector	LookCenter = Center + FVector(0.0, 0.0, 100.0);

	mFlyoverPositions.Reset();
	mFlyoverTargets.Reset();

	// 점 하나 = (시간, 카메라 위치, 바라보는 점).
	// AddPoint(시간, 값) 뒤 InterpMode = CIM_CurveAuto : 점 사이를 엔진이 기울기를 자동으로 맞춘 부드러운 곡선으로 잇는다.
	// [&] 람다 : 이 함수 안에서만 쓰는 작은 함수. &는 바깥 변수(곡선들)를 복사하지 않고 그대로 쓴다는 뜻
	auto	AddKey = [&](float Time, const FVector& Position, const FVector& Target)
	{
		mFlyoverPositions.Points[mFlyoverPositions.AddPoint(Time, Position)].InterpMode = CIM_CurveAuto;
		mFlyoverTargets.Points[mFlyoverTargets.AddPoint(Time, Target)].InterpMode = CIM_CurveAuto;
	};

	if (IsValid(S.FocusActor))
	{
		// ------------------------------------------------------------ (11/5) 석상 방식 : 앞 → 옆 → 등 뒤 어깨 위 → 아레나
		// GetActorBounds(충돌 부품만?, 중심, 반크기) : 액터 전체를 감싸는 상자. false = 충돌 없는 장식 메시도 포함 (석상은 NoCollision)
		FVector	Origin, Extent;

		S.FocusActor->GetActorBounds(false, Origin, Extent);

		float	H = Extent.Z * 2.f;				// 석상 키
		float	Base = Origin.Z - Extent.Z;		// 발바닥
		float	Top = Origin.Z + Extent.Z;		// 머리 끝

		// 앞 = 석상에서 아레나 중심 쪽 (석상이 아레나를 바라본다). 오른쪽 = 앞을 위에서 볼 때 시계 방향 90도
		FVector	Front = Center - Origin;

		Front.Z = 0.0;
		Front = Front.GetSafeNormal();

		FVector	Right = Front.RotateAngleAxis(90.f, FVector::UpVector);
		FVector	Foot = FVector(Origin.X, Origin.Y, Base);
		FVector	Face = FVector(Origin.X, Origin.Y, Top - H * 0.15f);
		float	D = S.Duration;

		// 0초 : 앞 왼쪽 아래에서 얼굴을 올려다봄
		AddKey(0.f, Foot + Front * (H * S.FocusFrontRatio) - Right * (H * S.FocusSlideRatio) + FVector(0.0, 0.0, H * 0.2f), Face);

		// 33% : 오른쪽으로 스윽 미끄러지며 조금 올라감 (계속 얼굴)
		AddKey(D * 0.33f, Foot + Front * (H * S.FocusFrontRatio * 0.9f) + Right * (H * S.FocusSlideRatio) + FVector(0.0, 0.0, H * 0.45f), Face);

		// 52% : 옆으로 돌아 머리 높이 (머리를 봄)
		AddKey(D * 0.52f, Foot + Right * (H * 0.9f) + Front * (H * 0.15f) + FVector(0.0, 0.0, H * 1.05f), Face);

		// 68% : 등 뒤 어깨 위 → 어깨 너머로 아레나 중심 (석상의 시선)
		AddKey(D * 0.68f, FVector(Origin.X, Origin.Y, Top) - Front * (H * 0.5f) + Right * (H * 0.35f) + FVector(0.0, 0.0, H * S.FocusOverHeightRatio), LookCenter);

		mFlyoverDuration = D;
	}
	else
	{
		// ------------------------------------------------------------ 돌기 방식 (오로라) : 점 9개 (8칸), 시간·각도·거리·높이를 0 → 1 비율로
		const int32	Steps = 8;

		for (int32 i = 0; i <= Steps; ++i)
		{
			float	Alpha = static_cast<float>(i) / Steps;
			float	Angle = FMath::DegreesToRadians(FMath::Lerp(StartAngle, EndAngle, Alpha));
			float	Radius = FMath::Lerp(S.StartDistance, S.OrbitEndDistance, Alpha);
			float	Height = FMath::Lerp(S.StartHeight, S.OrbitEndHeight, Alpha);

			AddKey(S.OrbitEndTime * Alpha, Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height), LookCenter);
		}

		mFlyoverDuration = FMath::Max(S.Duration, S.OrbitEndTime + 0.1f);
	}

	// 마지막 점 : 보스 앞 구도 (두 방식 공용)
	AddKey(mFlyoverDuration, FinalPosition, FinalTarget);

	// AutoSetTangents : CurveAuto 점들의 기울기를 실제로 계산한다 (점을 다 넣은 뒤 한 번)
	mFlyoverPositions.AutoSetTangents();
	mFlyoverTargets.AutoSetTangents();

	// ---------------------------------------------------------------- 카메라 만들기 → 화면을 이 카메라로
	mFlyoverCamera = GetWorld()->SpawnActor<ACameraActor>(mFlyoverPositions.Points[0].OutVal, FRotator::ZeroRotator);

	if (!IsValid(mFlyoverCamera))
		return;

	mFlyoverCamera->GetCameraComponent()->SetFieldOfView(S.FieldOfView);
	mFlyoverCamera->GetCameraComponent()->bConstrainAspectRatio = false;

	mFlyoverElapsed = 0.f;
	mFlyoverActive = true;

	UpdateFlyoverCamera(0.f);

	// SetViewTarget : 이제부터 이 카메라로 화면을 그린다 (레벨이 검은 화면에서 밝아지는 중이라 바로 바꿔도 티가 안 난다)
	PlayerController->SetViewTarget(mFlyoverCamera);
}

void AArenaGameMode::UpdateFlyoverCamera(float DeltaTime)
{
	if (!mFlyoverActive || !IsValid(mFlyoverCamera))
		return;

	mFlyoverElapsed = FMath::Min(mFlyoverElapsed + DeltaTime, mFlyoverDuration);

	// 천천히 출발 → 천천히 도착 : 실제 시간 비율을 SmoothStep으로 휘어서 곡선 위의 시간으로 쓴다
	float	Alpha = FMath::SmoothStep(0.f, 1.f, mFlyoverElapsed / FMath::Max(mFlyoverDuration, 0.01f));
	float	CurveTime = Alpha * mFlyoverDuration;

	// Eval(시간, 기본값) : 곡선 위의 그 시간 값
	FVector	Position = mFlyoverPositions.Eval(CurveTime, FVector::ZeroVector);
	FVector	Target = mFlyoverTargets.Eval(CurveTime, FVector::ZeroVector);

	mFlyoverCamera->SetActorLocationAndRotation(Position, (Target - Position).Rotation());

	// 끝에 닿으면 그 구도로 멈춘다 (그 구도에서 보스가 나타난다)
	if (mFlyoverElapsed >= mFlyoverDuration)
		mFlyoverActive = false;
}

void AArenaGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateFlyoverCamera(DeltaSeconds);
	UpdateTrackCamera(DeltaSeconds);
	UpdateCardCamera(DeltaSeconds);
}

FVector AArenaGameMode::GetIntroBoneLocation(FName BoneName, float FallbackRatio) const
{
	// GetBoneIndex : 뼈 이름이 스켈레톤에 없으면 INDEX_NONE(-1). 있으면 그 뼈의 지금 월드 위치를 쓴다
	USkeletalMeshComponent* Mesh = mIntroBoss->GetMesh();

	if (IsValid(Mesh) && Mesh->GetBoneIndex(BoneName) != INDEX_NONE)
		return Mesh->GetBoneLocation(BoneName);

	// 대신 : 발바닥 + 키 × 비율
	float	HalfHeight = mIntroBoss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	return mIntroBoss->GetActorLocation() + FVector(0.0, 0.0, -HalfHeight + HalfHeight * 2.f * FallbackRatio);
}

void AArenaGameMode::UpdateTrackCamera(float DeltaTime)
{
	if (mIntroCamera != EIntroCamera::Track || !IsValid(mFlyoverCamera) || !IsValid(mIntroBoss))
		return;

	const FIntroFlyoverSettings&	S = mIntroBoss->GetIntroFlyover();

	// 자리 : 보스 위치 + 둘러보기 끝 구도의 거리 → 보스가 등장 동작 중 걸어가면 카메라도 같이 옮겨 간다
	FVector	DesiredLocation = mIntroBoss->GetActorLocation() + mTrackOffset;
	FVector	Pelvis = GetIntroBoneLocation(S.PelvisBone, 0.5f);

	// (11/5) 먼저 따라갈 소켓(세바로그 망치) : 떨어지는 동안은 빠르게 쫓고, 바닥 근처에 닿으면 골반 따라가기로 넘어간다
	// DoesSocketExist : 소켓 이름이든 뼈 이름이든 있으면 true. GetSocketLocation도 둘 다 받는다
	USkeletalMeshComponent*	Mesh = mIntroBoss->GetMesh();

	// (11/6 부드럽게) 따라가기는 "임계 감쇠 스프링"으로 : FMath::CriticallyDampedSmoothing(값, 속도, 목표, 목표 속도, 시간, 부드러움 시간)
	// VInterpTo는 출발하는 순간 최고 속도라 "툭" 움직이고, 목표가 바뀌면(망치 → 골반) 속도가 꺾인다.
	// 스프링은 속도를 기억해서 천천히 출발 → 따라감 → 넘치지 않고 멈춤. 부드러움 시간 = 2 ÷ 기존 속도 값 (같은 정도로 따라감)
	auto	SmoothTime = [](float Speed) { return 2.f / FMath::Max(Speed, 0.1f); };
	FVector	LookGoal = mTrackLookPoint;
	FVector	LookGoalRate = FVector::ZeroVector;		// (11/6) 목표가 움직이는 속도 (떨어지는 망치)
	float	LookSmooth = SmoothTime(S.TrackLookSpeed);

	mTrackLeadTime += DeltaTime;

	float	HalfHeight = mIntroBoss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float	FeetZ = mIntroBoss->GetActorLocation().Z - HalfHeight;

	// (11/6) 망치 뼈 찾기 : 소켓 하나(HammerCenter)는 떨어지는 망치를 따라가지 않았다 (손 쪽에 붙은 듯).
	// 출발 전에는 후보 중 "가장 높은 것"을 보고, 머리 위에서 처음 보인 그 뼈를 진짜 망치로 정해 그 뒤로는 그 뼈만 따른다.
	// (계속 최댓값을 쓰면 망치가 꽂힌 뒤 손 쪽 뼈가 더 높아져서 "아직 안 닿음"이 되고, 후보가 바뀌는 순간 높이가 튀었다)
	bool	bLeadFound = false;
	float	LeadZ = 0.f;
	FName	LeadName = NAME_None;

	if (mTrackLeading && IsValid(Mesh))
	{
		if (mTrackLeadArmed)
		{
			if (Mesh->DoesSocketExist(mTrackLeadBone))
			{
				LeadZ = Mesh->GetSocketLocation(mTrackLeadBone).Z;
				bLeadFound = true;
			}
		}
		else
		{
			for (const FName& Name : S.LeadSockets)
			{
				if (!Mesh->DoesSocketExist(Name))
					continue;

				float	Z = Mesh->GetSocketLocation(Name).Z;

				if (!bLeadFound || Z > LeadZ)
				{
					LeadZ = Z;
					LeadName = Name;
				}

				bLeadFound = true;
			}
		}
	}

	if (bLeadFound)
	{
		FVector	Lead(Pelvis.X, Pelvis.Y, LeadZ);
		float	LandZ = FeetZ + HalfHeight * 2.f * S.LeadLandRatio;

		// (11/6 끊김 수정) 출발 조건 : 망치가 머리 위(키 × 1.2)에서 한 번 보인 뒤부터 따라간다.
		// 등장 동작 첫 프레임에는 애니메이션이 아직 몸에 적용되기 전이라 망치가 대기 자세의 손(낮은 곳)에 있다
		// → 그 값을 읽고 카메라가 한 프레임 만에 내려가 버렸다 (녹화 4.43 → 4.47초). 그전까지 카메라는 하늘을 본 채 그대로
		if (!mTrackLeadArmed && Lead.Z > FeetZ + HalfHeight * 2.f * 1.2f)
		{
			mTrackLeadArmed = true;
			mTrackLeadBone = LeadName;
			mTrackArmedTime = 0.f;
			mTrackPrevLeadValid = false;

			UE_LOG(UGDebug, Warning, TEXT("Intro : lead bone = %s"), *LeadName.ToString());
		}

		if (mTrackLeadArmed)
		{
			mTrackArmedTime += DeltaTime;

			// (11/5 흔들림 수정) 좌우(X·Y)는 보스 골반에 고정하고 "높이만" 망치를 따른다 = 카메라는 고개를 숙이는 움직임만.
			// 높이도 지금보다 낮아질 때만(FMath::Min) → 돌며 잠깐 올라가도 카메라는 가만히, 떨어질 때만 따라 내려간다
			LookGoal = FVector(Pelvis.X, Pelvis.Y, FMath::Min(mTrackLookPoint.Z, Lead.Z));
			LookSmooth = SmoothTime(S.LeadFollowSpeed);

			// (11/6 따라잡기) 망치는 화면 전체를 0.25초 만에 지나간다 → 늦게 쫓는 스프링으로는 못 따라잡았다 (녹화 2.27 → 2.54초).
			// 망치의 낙하 속도(이번 프레임 높이 - 지난 프레임 높이 ÷ 시간)를 스프링에 "목표 속도"로 알려 주면,
			// 스프링이 그만큼 앞을 보고 움직여서 늦지 않고 같은 속도로 함께 내려간다. 내려갈 때만 (올라가는 건 무시)
			// 비정상 값(한 프레임에 높이가 튐) 방지 : 초당 60m보다 빠른 낙하는 60m로 자른다
			if (mTrackPrevLeadValid && DeltaTime > UE_KINDA_SMALL_NUMBER)
				LookGoalRate.Z = FMath::Clamp((Lead.Z - mTrackPrevLeadZ) / DeltaTime, -6000.f, 0.f);

			// 땅에 닿음, 또는 안전장치 : 따라가기 시작 1.5초가 지나면 무조건 골반으로
			if (Lead.Z < LandZ || mTrackArmedTime > 1.5f)
			{
				// (11/10) 망치 착지 "쾅"(흔들림·소리)은 보스가 등장 동작 시각(mIntroImpactTime)으로 직접 낸다
				// → 다시 시도(짧은 연출, 이 카메라 없음)에서도 같이 나고, 소리와 흔들림이 정확히 같은 순간
				mTrackLeading = false;
			}
		}
		else if (mTrackLeadTime > 3.f)
		{
			// 안전장치 : 3초 안에 망치가 높이 보이지 않으면(소켓 이름이 틀린 경우 등) 골반 따라가기로
			// (녹화 : 따라가기 시작 1.3초 뒤에야 망치가 하늘에 보여서 1.5초는 빠듯했다)
			mTrackLeading = false;
		}

		// 다음 프레임의 낙하 속도 계산용
		mTrackPrevLeadZ = Lead.Z;
		mTrackPrevLeadValid = true;
	}
	else
	{
		mTrackLeading = false;
	}

	if (!mTrackLeading)
	{
		// (11/5 부드럽게) 회전을 직접 쫓지 않고 "바라보는 점"을 천천히 골반 쪽으로 옮긴다.
		// 걸음마다 생기는 골반의 자잘한 흔들림은 따라잡기 전에 되돌아가서 화면에 거의 안 보인다
		LookGoal = Pelvis;
	}

	FMath::CriticallyDampedSmoothing(mTrackLookPoint, mTrackLookVelocity, LookGoal, LookGoalRate, DeltaTime, LookSmooth);

	// (11/6) 바닥 아래는 보지 않는다 : 바라보는 점이 보스 발바닥 아래로 내려가면 거기서 멈추고 아래로 가던 속도도 없앤다
	// (속도를 앞질러 반영하다 넘쳐서 화면이 바닥만 보던 문제)
	if (mTrackLookPoint.Z < FeetZ)
	{
		mTrackLookPoint.Z = FeetZ;
		mTrackLookVelocity.Z = FMath::Max(mTrackLookVelocity.Z, 0.f);
	}

	// 카메라 자리도 같은 스프링으로 (보스가 걸어가면 천천히 출발해 따라붙는다)
	FVector	Location = mFlyoverCamera->GetActorLocation();

	FMath::CriticallyDampedSmoothing(Location, mTrackMoveVelocity, DesiredLocation, FVector::ZeroVector, DeltaTime, SmoothTime(S.TrackMoveSpeed));

	FRotator	Rotation = (mTrackLookPoint - Location).Rotation();

	// 왼쪽 1/3 구도 유지
	Rotation.Yaw += GetThirdsYaw(S.FieldOfView);

	mFlyoverCamera->SetActorLocationAndRotation(Location, Rotation);
}

FVector AArenaGameMode::GetCameraLookPoint(float Distance) const
{
	// 카메라 회전에서 "왼쪽 1/3 틀기"를 되돌린 방향으로 Distance만큼 앞 = 지금 화면이 보스로 삼고 있는 점.
	// 따라가기·클로즈업을 시작할 때 이 점에서 출발하면 첫 프레임에 화면이 튀지 않는다
	FRotator	Rotation = mFlyoverCamera->GetActorRotation();

	Rotation.Yaw -= GetThirdsYaw(mIntroBoss->GetIntroFlyover().FieldOfView);

	return mFlyoverCamera->GetActorLocation() + Rotation.Vector() * Distance;
}

void AArenaGameMode::StartCardCamera()
{
	if (!IsValid(mFlyoverCamera) || !IsValid(mIntroBoss))
		return;

	const FIntroFlyoverSettings&	S = mIntroBoss->GetIntroFlyover();

	// ---------------------------------------------------------------- 멈춘 순간의 실제 몸 (머리·골반 뼈)
	float	HalfHeight = mIntroBoss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Head = GetIntroBoneLocation(S.HeadBone, 0.9f);
	FVector	Pelvis = GetIntroBoneLocation(S.PelvisBone, 0.5f);

	// 상체 길이 = 머리 - 골반 높이. 뼈가 이상하게 잡혀도(웅크린 자세 등) 너무 작아지지 않게 키의 20%는 보장
	float	Upper = FMath::Max(Head.Z - Pelvis.Z, HalfHeight * 2.f * 0.2f);

	// ---------------------------------------------------------------- 거리 : 상체가 화면 높이의 CardFill만큼 차도록
	// 세로 시야각 : tan(세로/2) = tan(가로/2) ÷ 화면 비율(가로 ÷ 세로)
	float	Aspect = 16.f / 9.f;
	int32	ViewX = 0, ViewY = 0;

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		PlayerController->GetViewportSize(ViewX, ViewY);

	if (ViewX > 0 && ViewY > 0)
		Aspect = static_cast<float>(ViewX) / ViewY;

	float	TanHalfV = FMath::Tan(FMath::DegreesToRadians(S.FieldOfView * 0.5f)) / Aspect;

	// 화면에 보일 높이 = 상체 ÷ CardFill → 그 높이가 다 들어가는 거리 = (높이 / 2) ÷ tan(세로/2)
	float	Visible = Upper / FMath::Max(S.CardFill, 0.1f);
	float	Distance = (Visible * 0.5f) / FMath::Max(TanHalfV, 0.01f);

	// ---------------------------------------------------------------- 방향 : 보스 정면에서 CardOrbitDegrees만큼 (보스의 오른쪽 어깨 쪽)
	// RotateAngleAxis(각도, 축) : 벡터를 위쪽(Z) 축으로 돌린다. + 각도 = 위에서 볼 때 시계 방향 (언리얼 기준)
	FVector	Forward = mIntroBoss->GetActorForwardVector();

	Forward.Z = 0.0;
	Forward = Forward.GetSafeNormal().RotateAngleAxis(S.CardOrbitDegrees, FVector::UpVector);

	// 바라보는 점 = 골반 위 상체의 60% (머리 위 여백을 조금 두려고 가운데보다 약간 위)
	// 카메라 높이 = 골반 위 상체의 45% → 약간 아래에서 올려다본다 (보스가 커 보인다)
	mCardLookPoint = FVector(Pelvis.X, Pelvis.Y, Pelvis.Z + Upper * 0.6f);
	mCardLocation = FVector(Pelvis.X, Pelvis.Y, Pelvis.Z + Upper * 0.45f) + Forward * Distance;

	// (11/5 부드럽게) 출발 : 지금 카메라 자리와 지금 바라보는 점. 호의 중심 = 골반 (위에서 본 위치)
	mCardPivot = FVector(Pelvis.X, Pelvis.Y, 0.0);
	mCardStartLocation = mFlyoverCamera->GetActorLocation();
	mCardStartLook = GetCameraLookPoint(FVector::Dist(mCardStartLocation, mCardLookPoint));
	mCardElapsed = 0.f;

	mIntroCamera = EIntroCamera::Card;
}

void AArenaGameMode::UpdateCardCamera(float DeltaTime)
{
	if (mIntroCamera != EIntroCamera::Card || !IsValid(mFlyoverCamera) || !IsValid(mIntroBoss))
		return;

	const FIntroFlyoverSettings&	S = mIntroBoss->GetIntroFlyover();

	mCardElapsed += DeltaTime;

	// ① 밀고 들어가기 (11/5 부드럽게) : SmootherStep 곡선 t³(6t² - 15t + 10)
	// 출발·도착 모두 속도 0에서 시작·끝 → "툭" 끊기는 느낌이 없다 (감속 곡선은 출발이 최고 속도라 튀어 보였다)
	float	Push = FMath::Clamp(mCardElapsed / FMath::Max(S.CardPushTime, 0.01f), 0.f, 1.f);
	float	Ease = Push * Push * Push * (Push * (Push * 6.f - 15.f) + 10.f);

	// 위치 : 직선이 아니라 골반을 중심으로 호를 그린다. 각도·거리·높이를 따로 섞는다.
	// Atan2(y, x) : 중심에서 본 방향(라디안). FindDeltaAngleRadians : 두 각도 사이의 "짧은 쪽" 차이 (-180~180도)
	FVector	StartOffset = mCardStartLocation - mCardPivot;
	FVector	EndOffset = mCardLocation - mCardPivot;

	float	StartAngle = FMath::Atan2(StartOffset.Y, StartOffset.X);
	float	Angle = StartAngle + FMath::FindDeltaAngleRadians(StartAngle, FMath::Atan2(EndOffset.Y, EndOffset.X)) * Ease;
	float	Radius = FMath::Lerp(StartOffset.Size2D(), EndOffset.Size2D(), Ease);
	float	Height = FMath::Lerp(StartOffset.Z, EndOffset.Z, Ease);

	FVector	Location = mCardPivot + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height);

	// ② 천천히 다가가기 : 도착한 뒤 카드가 끝날 때까지 바라보는 점 쪽으로 거리의 CardDrift만큼 (일정한 속도)
	float	DriftTime = S.CardDelay + mBossCardHoldTime - S.CardPushTime;
	float	Drift = FMath::Clamp((mCardElapsed - S.CardPushTime) / FMath::Max(DriftTime, 0.01f), 0.f, 1.f);

	Location += (mCardLookPoint - mCardLocation) * (S.CardDrift * Drift);

	// 회전 : 바라보는 점을 출발 점 → 상체 점으로 같이 옮기고, 그 점을 바라본 뒤 왼쪽 1/3만큼 튼다
	// (위치와 회전을 같은 곡선으로 움직여서 보스가 화면에서 미끄러지지 않는다)
	FVector		LookPoint = FMath::Lerp(mCardStartLook, mCardLookPoint, Ease);
	FRotator	Rotation = (LookPoint - Location).Rotation();

	Rotation.Yaw += GetThirdsYaw(S.FieldOfView);

	mFlyoverCamera->SetActorLocationAndRotation(Location, Rotation);
}

void AArenaGameMode::ShowStageCard()
{
	if (IsValid(mIntroWidget) && IsValid(mIntroBoss))
		mIntroWidget->ShowStageCard(mIntroBoss->GetStageName(), mIntroBoss->GetStageIcon());
}

void AArenaGameMode::OnFlyoverFinished()
{
	if (IsValid(mIntroWidget))
		mIntroWidget->HideStageCard();

	if (!IsValid(mIntroBoss))
	{
		ReturnCameraToPlayer(mCameraReturnTime);
		return;
	}

	// ② 보스가 나타나며 등장 동작. 멈출 순간이 동작보다 길면 동작 끝에서 멈춘다.
	float	MontageTime = mIntroBoss->StartIntro();

	if (MontageTime <= 0.f)
	{
		ReturnCameraToPlayer(mCameraReturnTime);
		return;
	}

	// (11/5) 코드 경로 카메라면 등장 동작 동안 보스를 따라간다 (둘러보기 끝 자리에서 보스까지의 거리를 유지)
	if (IsValid(mFlyoverCamera))
	{
		mFlyoverActive = false;
		mTrackOffset = mFlyoverCamera->GetActorLocation() - mIntroBoss->GetActorLocation();

		// 바라보는 점은 지금 화면이 보던 곳에서 출발 → 둘러보기에서 따라가기로 넘어가는 순간 튀지 않는다
		mTrackLookPoint = GetCameraLookPoint(FVector::Dist(mFlyoverCamera->GetActorLocation(), mIntroBoss->GetActorLocation()));
		mIntroCamera = EIntroCamera::Track;

		// 먼저 따라갈 소켓이 정해져 있으면(세바로그 망치) 그것부터. (11/6) 출발 조건·스프링 속도는 처음부터
		mTrackLeading = mIntroBoss->GetIntroFlyover().LeadSockets.Num() > 0;
		mTrackLeadArmed = false;
		mTrackLeadTime = 0.f;
		mTrackPrevLeadValid = false;
		mTrackLookVelocity = FVector::ZeroVector;
		mTrackMoveVelocity = FVector::ZeroVector;
	}

	float	FreezeTime = FMath::Clamp(mIntroBoss->GetIntroFreezeTime(), 0.f, MontageTime);

	GetWorldTimerManager().SetTimer(mCinematicTimer, this, &AArenaGameMode::OnBossFreeze,
		FMath::Max(FreezeTime, 0.01f), false);
}

void AArenaGameMode::OnBossFreeze()
{
	if (!IsValid(mIntroBoss))
		return;

	// 보스만 멈춘다 (정지 화면) + 카드 쾅. 카드는 위젯 Tick(실제 시간)으로 움직여서 멈춘 보스와 상관없다.
	mIntroBoss->SetIntroFrozen(true);

	// (11/5) 카메라 : 멈춘 몸의 머리·골반으로 상체 클로즈업 자리를 계산해 밀고 들어간다 (코드 경로 카메라일 때만)
	StartCardCamera();

	// 카메라가 움직이는 중에 카드가 쾅 나오면 정신없어서, 거의 도착할 때(CardDelay초 뒤)부터 띠 → 이름 → 부제.
	// 시퀀스 카메라라면(밀기 없음) 바로
	float	CardDelay = (mIntroCamera == EIntroCamera::Card) ? mIntroBoss->GetIntroFlyover().CardDelay : 0.f;

	if (CardDelay > 0.f)
		GetWorldTimerManager().SetTimer(mCinematicTimer, this, &AArenaGameMode::OnBossCardShow, CardDelay, false);
	else
		OnBossCardShow();
}

void AArenaGameMode::OnBossCardShow()
{
	if (!IsValid(mIntroBoss))
		return;

	// (11/5) 보스 뒤 배경 띠 (보스 실루엣 마스크 켜기 + 후처리에 띠 얹기)
	StartIntroBand();

	if (IsValid(mIntroWidget))
		mIntroWidget->ShowBossCard(mIntroBoss->GetIntroName(), mIntroBoss->GetIntroTitle(),
			mIntroBoss->GetIntroNameImage(), mIntroBoss->GetIntroTitleImage(), mIntroBoss->GetIntroTintColor());

	GetWorldTimerManager().SetTimer(mCinematicTimer, this, &AArenaGameMode::OnBossCardEnd, mBossCardHoldTime, false);
}

void AArenaGameMode::OnBossCardEnd()
{
	if (IsValid(mIntroWidget))
		mIntroWidget->HideBossCard();

	// 보스는 남은 동작을 마저 하고, 그동안 카메라가 기디언 뒤로 돌아온다.
	if (IsValid(mIntroBoss))
		mIntroBoss->SetIntroFrozen(false);

	// 클로즈업 자리에 멈춘 채 기디언 카메라로 섞여 간다
	mIntroCamera = EIntroCamera::None;

	ReturnCameraToPlayer(mCameraReturnTime);
}

void AArenaGameMode::ReturnCameraToPlayer(float BlendTime)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

	// 둘러보기 동안 마우스로 시점이 돌아갔을 수 있어서, 돌아오기 전에 다시 보스 쪽으로 맞춘다.
	if (IsValid(Player) && IsValid(mIntroBoss))
		Player->LockForIntro(mIntroBoss->GetActorLocation());

	// SetViewTargetWithBlend(새 대상, 시간, 곡선) : 지금 카메라(시퀀스의 마지막 구도)에서 기디언 카메라로 부드럽게 옮겨 간다.
	// VTBlend_Cubic : 천천히 출발해 빨라졌다가 천천히 도착
	if (IsValid(PlayerController) && IsValid(Player))
		PlayerController->SetViewTargetWithBlend(Player, BlendTime, EViewTargetBlendFunction::VTBlend_Cubic);

	// (11/6) FinishCinematic이 값(보스 준비 시간)을 받게 바뀌어서, 타이머는 "함수 + 넘길 값(0 = 준비 시간 없음)"을 묶은 델리게이트로 부른다
	// FTimerDelegate::CreateUObject(객체, 함수, 넘길 값) : 나중에 부를 때 그 값을 붙여서 부른다
	GetWorldTimerManager().SetTimer(mCinematicTimer, FTimerDelegate::CreateUObject(this, &AArenaGameMode::FinishCinematic, 0.f),
		FMath::Max(BlendTime, 0.01f), false);
}

void AArenaGameMode::FinishCinematic(float BossDelay)
{
	if (!mCinematicPlaying)
		return;

	mCinematicPlaying = false;

	GetWorldTimerManager().ClearTimer(mStageCardTimer);
	GetWorldTimerManager().ClearTimer(mCinematicTimer);

	// 시퀀스를 멈춘다. 카메라는 이미 기디언에게 돌아와 있다. (건너뛰기면 이 순간 기디언 카메라로 바로 바뀐다)
	if (IsValid(mIntroSequence) && mIntroSequence->GetSequencePlayer())
		mIntroSequence->GetSequencePlayer()->Stop();

	// (11/5) 코드 경로 카메라 : 아직 그 카메라로 보고 있으면(건너뛰기) 기디언 카메라로 바로 바꾼 뒤 지운다.
	// 보고 있는 카메라를 먼저 지우면 화면이 엉뚱한 곳(월드 원점)으로 튈 수 있어서 순서가 중요하다.
	mFlyoverActive = false;
	mIntroCamera = EIntroCamera::None;

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (IsValid(PlayerController) && IsValid(PlayerPawn) && PlayerController->GetViewTarget() != PlayerPawn)
		PlayerController->SetViewTarget(PlayerPawn);

	if (IsValid(mFlyoverCamera))
		mFlyoverCamera->Destroy();

	mFlyoverCamera = nullptr;

	if (IsValid(mIntroWidget))
		mIntroWidget->RemoveFromParent();

	mIntroWidget = nullptr;

	StopIntroBand();

	mSkipping = false;

	// ③ 입력·AI 풀기 + HUD (짧은 연출과 같은 끝). 보스 EndIntro가 숨김·멈춤도 되돌린다.
	if (BossDelay <= 0.f)
	{
		EndIntro();
		return;
	}

	// (11/6) 건너뛰기 : 기디언·HUD 먼저, 보스는 BossDelay초 동안 무적·제자리 (자리 잡을 시간) → EndBossIntro
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
		Player->EndIntro();

	if (IsValid(mHUDWidget))
		mHUDWidget->FadeIn(mHUDFadeInTime);

	GetWorldTimerManager().SetTimer(mBossGraceTimer, this, &AArenaGameMode::EndBossIntro, BossDelay, false);
}

void AArenaGameMode::PrepareIntroBand()
{
	mIntroBandMaterial = nullptr;

	// 띠 그림이 없는 보스면 띠 없이 (카드만)
	if (!IsValid(mIntroBoss) || !IsValid(mIntroBoss->GetIntroBandTexture()))
		return;

	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, *mIntroBandMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);

	if (!IsValid(BaseMaterial))
	{
		UE_LOG(UGDebug, Warning, TEXT("Intro : M_PP_IntroBand not found"));
		return;
	}

	// 레벨 전체에 걸리는(무한 범위) 후처리 볼륨. 없으면 하나 만든다.
	// bUnbound : 볼륨 상자 밖에서도 적용 (= 레벨 전체)
	mIntroPostProcess = nullptr;

	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		if (It->bUnbound)
		{
			mIntroPostProcess = *It;
			break;
		}
	}

	if (!IsValid(mIntroPostProcess))
	{
		mIntroPostProcess = GetWorld()->SpawnActor<APostProcessVolume>();

		if (IsValid(mIntroPostProcess))
			mIntroPostProcess->bUnbound = true;
	}

	// 다이내믹 인스턴스 : 실행 중에 파라미터(띠 그림·투명도)를 바꿀 수 있는 머티리얼 복사본 (경고 데칼과 같은 방식)
	mIntroBandMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	mIntroBandMaterial->SetTextureParameterValue(TEXT("BandTexture"), mIntroBoss->GetIntroBandTexture());
	mIntroBandMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.f);
}

void AArenaGameMode::StartIntroBand()
{
	if (!IsValid(mIntroBandMaterial) || !IsValid(mIntroPostProcess) || !IsValid(mIntroBoss))
		return;

	// 보스 실루엣을 커스텀 뎁스에 그린다 → 후처리가 "보스가 있는 픽셀"을 알 수 있다
	mIntroBoss->SetIntroMaskEnabled(true);

	// AddOrUpdateBlendable(머티리얼, 가중치) : 후처리 볼륨에 머티리얼을 얹는다. 투명도는 위젯이 카드와 같이 올린다.
	mIntroPostProcess->AddOrUpdateBlendable(mIntroBandMaterial, 1.f);

	if (IsValid(mIntroWidget))
		mIntroWidget->SetBandMaterial(mIntroBandMaterial);
}

void AArenaGameMode::StopIntroBand()
{
	if (IsValid(mIntroPostProcess) && IsValid(mIntroBandMaterial))
		mIntroPostProcess->Settings.RemoveBlendable(mIntroBandMaterial);

	if (IsValid(mIntroBoss))
		mIntroBoss->SetIntroMaskEnabled(false);

	mIntroBandMaterial = nullptr;
}

void AArenaGameMode::SkipIntro()
{
	if (!mCinematicPlaying || mSkipping)
		return;

	mSkipping = true;

	// (11/6) 바로 끊지 않고 0.25초 동안 검게 → 다 어두워지면 정리(OnSkipFadedOut).
	// 예약해 둔 다음 단계(맵 카드·보스 등장·카드)는 지금 지운다 → 어두워지는 동안 연출이 더 진행되지 않게
	GetWorldTimerManager().ClearTimer(mStageCardTimer);
	GetWorldTimerManager().ClearTimer(mCinematicTimer);
	// StartCameraFade(시작, 끝, 시간, 색, 소리도, 끝난 뒤 유지) : 0 → 1 = 화면이 검게. 유지(true)해야 정리하는 동안 검은 채로 남는다
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		PlayerController->PlayerCameraManager->StartCameraFade(0.f, 1.f, mSkipFadeOutTime, FLinearColor::Black, false, true);

	UE_LOG(UGDebug, Warning, TEXT("Intro : skipped"));

	GetWorldTimerManager().SetTimer(mSkipTimer, this, &AArenaGameMode::OnSkipFadedOut, FMath::Max(mSkipFadeOutTime, 0.01f), false);
}

void AArenaGameMode::OnSkipFadedOut()
{
	// 검은 화면 뒤에서 정리 : 보스는 등장 동작을 멈추고 제자리에 보이게 + 기디언 쪽을 본다, 기디언은 보스 쪽을 본다
	if (IsValid(mIntroBoss))
	{
		mIntroBoss->StopIntroMontage();
		mIntroBoss->SetIntroFrozen(false);
		mIntroBoss->SetActorHiddenInGame(false);

		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			// 위에서 본 방향만 (Z = 0) → 보스가 앞뒤로 기울지 않게
			FVector	ToPlayer = PlayerPawn->GetActorLocation() - mIntroBoss->GetActorLocation();

			ToPlayer.Z = 0.0;

			if (!ToPlayer.IsNearlyZero())
				mIntroBoss->SetActorRotation(ToPlayer.Rotation());
		}
	}

	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
	{
		if (IsValid(mIntroBoss))
			Player->LockForIntro(mIntroBoss->GetActorLocation());
	}

	// 기디언·HUD는 바로, 보스는 준비 시간 뒤 (보스마다 다름. 0이면 바로)
	FinishCinematic(IsValid(mIntroBoss) ? mIntroBoss->GetIntroSkipDelay() : 0.f);

	// 1 → 0 = 다시 밝아짐 (유지 안 함)
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, mSkipFadeInTime, FLinearColor::Black, false, false);
}

void AArenaGameMode::OnPlayerDeath(APlayerCharacter* Player)
{
	if (mBattleEnded)
		return;

	EndBattle(false, mDefeatResultDelay);
}

void AArenaGameMode::OnBossDeath(ABossBase* Boss)
{
	if (mBattleEnded)
		return;

	// 마지막 일격 : 전투가 끝나기 "전에" 부른다. (PlayHitFeedback은 전투가 끝나면 무시하기 때문)
	PlayHitFeedback(EHitFeedback::BossDeath);

	EndBattle(true, mVictoryResultDelay);
}

void AArenaGameMode::EndBattle(bool bPlayerWon, float ResultDelay)
{
	mBattleEnded = true;
	mPlayerWon = bPlayerWon;

	// 레벨의 모든 캐릭터(기디언·보스·미니언)에게 알린다.
	// 부모 포인터(AGameCharacter*)로 부르지만 virtual이라 각자 덮어쓴 반응이 실행된다.
	// TActorIterator<T>(월드) : 월드에 있는 T 타입 액터를 처음부터 끝까지 하나씩 가리킨다.
	for (TActorIterator<AGameCharacter> It(GetWorld()); It; ++It)
	{
		AGameCharacter* Character = *It;

		if (IsValid(Character))
			Character->OnBattleEnd(bPlayerWon);
	}

	// 소울류 문법 (10/26) : 끝나는 순간 음악을 빼서 정적을 만든다. (승리·패배 모두)
	FadeOutAmbience();

	GetWorldTimerManager().SetTimer(mResultTimer, this, &AArenaGameMode::ShowResult,
		FMath::Max(ResultDelay, 0.01f), false);
}

void AArenaGameMode::FadeOutAmbience()
{
	// AAmbientSound : 레벨에 놓는 배경음 액터. 안의 오디오 컴포넌트가 실제로 소리를 낸다.
	for (TActorIterator<AAmbientSound> It(GetWorld()); It; ++It)
	{
		UAudioComponent* Audio = It->GetAudioComponent();

		if (IsValid(Audio))
			Audio->FadeOut(mAmbienceFadeTime, 0.f);
	}
}

void AArenaGameMode::OnStageClearEnd()
{
	// 기디언이 다시 움직일 수 있게 → 포탈까지 걸어간다. (HUD는 결과 화면 때 숨긴 그대로 둔다)
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
		Player->ResumeAfterVictory();

	if (IsValid(mStagePortal))
		mStagePortal->Open();

	// 다시 걸어 다니는 구간 → 일시정지 허용 (PauseGame)
	mWalkingToPortal = true;
}

void AArenaGameMode::ShowResult()
{
	// 결과 화면이 뜨면 HUD는 숨긴다. (Collapsed : 안 보이고 자리도 차지하지 않음)
	if (IsValid(mHUDWidget))
		mHUDWidget->SetVisibility(ESlateVisibility::Collapsed);

	if (!mResultWidgetClass)
		mResultWidgetClass = LoadClass<UResultWidget>(nullptr, *mResultWidgetPath);

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// 위젯 블루프린트가 아직 없으면 화면 글자로 대신 알린다.
	if (!mResultWidgetClass || !IsValid(PlayerController))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(10, 10.f, FColor::Red,
				mPlayerWon ? TEXT("GOD SLAIN (WBP_Result 없음)") : TEXT("YOU SEALED (WBP_Result 없음)"));
		}

		return;
	}

	// CreateWidget<T>(주인 플레이어 컨트롤러, 위젯 클래스) : 위젯 객체를 만든다. 아직 화면에는 없다.
	UResultWidget* Widget = CreateWidget<UResultWidget>(PlayerController, mResultWidgetClass);

	if (!IsValid(Widget))
		return;

	Widget->SetResult(mPlayerWon);

	// (10/26) 다음 스테이지 포탈이 있는 레벨에서 이겼으면 : 버튼 없이 글자만 떴다 사라지고, 그 뒤 포탈이 열린다.
	bool	bStageClear = mPlayerWon && IsValid(mStagePortal);

	if (bStageClear)
		Widget->SetStageClear(mStageClearHoldTime, mStageClearFadeOutTime);

	// 화면에 붙인다. (붙는 순간 NativeConstruct → 페이드인 시작)
	Widget->AddToViewport();

	if (bStageClear)
	{
		GetWorldTimerManager().SetTimer(mStageClearTimer, this, &AArenaGameMode::OnStageClearEnd,
			Widget->GetStageClearDuration(), false);
	}
}

// ============================================================================ 타격감 (10/21)

void AArenaGameMode::PlayHitFeedback(EHitFeedback Type, const FVector& Location, bool bHit, USoundBase* OverrideSound)
{
	// 전투가 끝난 뒤(결과 연출 중)에는 흔들거나 멈추지 않는다.
	if (mBattleEnded)
		return;

	// 사건마다 "히트스톱 몇 초 + 어떤 흔들림 몇 배"를 고른다. 수치는 헤더의 표(멤버 변수)에 있다.
	// switch : 값에 따라 여러 갈래 중 하나로 간다. 각 case 끝의 break가 없으면 다음 case까지 이어서 실행된다.
	switch (Type)
	{
	case EHitFeedback::PlayerHitLight:
		PlayCameraShake(false, mShakePlayerLight);

		// 때린 쪽이 소리를 정했으면 그것 (세바로그 망치), 아니면 표의 기본 소리
		if (IsValid(OverrideSound))
			UGameplayStatics::PlaySound2D(this, OverrideSound);
		else
			PlayFeedbackSound(mSoundPathPlayerHitLight);
		break;

	case EHitFeedback::PlayerHitHeavy:
		StartHitStop(mHitStopPlayerHeavy);
		PlayCameraShake(true, mShakePlayerHeavy);

		if (IsValid(OverrideSound))
			UGameplayStatics::PlaySound2D(this, OverrideSound);
		else
			PlayFeedbackSound(mSoundPathPlayerHitHeavy);
		break;

	case EHitFeedback::StrongShotHit:
		StartHitStop(mHitStopStrongShot);
		PlayCameraShake(false, mShakeStrongShot);
		break;

	case EHitFeedback::MeteorExplode:
		// 흔들림은 폭발만으로, 히트스톱은 실제로 맞혔을 때만
		if (bHit)
			StartHitStop(mHitStopMeteor);

		PlayCameraShakeAt(false, mShakeMeteor, Location, mMeteorShakeInner, mMeteorShakeOuter);
		break;

	case EHitFeedback::LeapLand:
		// 맞았다면 기디언 쪽에서 PlayerHitHeavy가 따로 온다. 여기는 "땅이 울리는" 흔들림만.
		PlayCameraShakeAt(true, mShakeLeapLand, Location, mLeapShakeInner, mLeapShakeOuter);
		PlayFeedbackSound(mSoundPathLeapLand, Location);
		break;

	case EHitFeedback::UltimateImpact:
		// 기둥 뒤에 숨어 있어도 크게 들려야 하므로 2D (위치 없음)
		PlayCameraShake(true, mShakeUltimate);
		PlayFeedbackSound(mSoundPathUltimateImpact);
		break;

	case EHitFeedback::CryoseismBlast:
		// 원 밖으로 피했어도 크게 들려야 하므로 2D (필살기 충격과 같은 이유)
		PlayCameraShake(true, mShakeUltimate);
		PlayFeedbackSound(mSoundPathCryoseismBlast);
		break;

	case EHitFeedback::IntroSlam:
		// (11/6) 카메라가 보스 가까이 있는 연출이라 기디언과의 거리로 줄이지 않는다. 소리는 등장 동작에 이미 있어서 흔들림만
		PlayCameraShake(true, mShakeLeapLand);
		break;

	case EHitFeedback::BossDeath:
		StartHitStop(mHitStopBossDeath);
		PlayCameraShake(true, mShakeBossDeath);
		break;
	}
}

USoundBase* AArenaGameMode::FindFeedbackSound(const FString& Path)
{
	if (Path.IsEmpty())
		return nullptr;

	// 처음 쓰는 경로면 불러와서 기억한다. 못 찾으면 nullptr을 기억해서 다음부터는 다시 찾지 않는다.
	// LOAD_NoWarn | LOAD_Quiet : 아직 임포트 안 한 소리여도 로그에 경고를 쌓지 않는다.
	// (게임 모드는 플레이할 때마다 새로 만들어지므로, 에디터에서 소리를 임포트한 뒤 다시 플레이하면 찾는다)
	if (!mLoadedSounds.Contains(Path))
		mLoadedSounds.Add(Path, LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet));

	return mLoadedSounds[Path];
}

void AArenaGameMode::PreloadFeedbackAssets()
{
	// 흔들림 블루프린트 클래스 (10/21까지는 처음 흔들 때 불러왔다)
	mLightShakeClass = LoadClass<UCameraShakeBase>(nullptr, *mLightShakePath);
	mHeavyShakeClass = LoadClass<UCameraShakeBase>(nullptr, *mHeavyShakePath);

	// 타격감 표의 소리 5개. FindFeedbackSound가 불러서 기억해 두면, 실제로 칠 때는 기억한 것을 바로 쓴다.
	// { &a, &b } : 멤버 변수들의 주소를 묶은 목록. 복사하지 않고 원래 문자열을 그대로 읽는다.
	for (const FString* Path : { &mSoundPathPlayerHitLight, &mSoundPathPlayerHitHeavy, &mSoundPathLeapLand,
		&mSoundPathUltimateImpact, &mSoundPathCryoseismBlast })
	{
		FindFeedbackSound(*Path);
	}
}

void AArenaGameMode::PlayFeedbackSound(const FString& Path, const FVector& Location)
{
	USoundBase* Sound = FindFeedbackSound(Path);

	if (!IsValid(Sound))
		return;

	// PlaySound2D : 위치 없이 귀에 바로 (거리·좌우 계산 없음) / PlaySoundAtLocation : 그 자리에서
	if (Location.IsZero())
		UGameplayStatics::PlaySound2D(this, Sound);
	else
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location);
}

void AArenaGameMode::StartHitStop(float Duration)
{
	if (Duration <= 0.f)
		return;

	// GetRealTimeSeconds : 시간 배율과 상관없이 흐르는 실제 시간.
	// 이미 진행 중인 히트스톱이 이번 것보다 늦게 끝나면 그대로 둔다. (짧은 요청이 긴 멈춤을 끊지 않게)
	double	Now = GetWorld()->GetRealTimeSeconds();
	double	EndTime = Now + Duration;

	if (EndTime <= mHitStopEndRealTime)
		return;

	mHitStopEndRealTime = EndTime;

	// SetGlobalTimeDilation : 월드 전체의 시간 배율. 0.05면 모든 움직임·애니메이션·타이머가 20배 느려진다.
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), mHitStopDilation);

	// 타이머도 느려진 시간으로 흐른다. 그래서 "실제 Duration초"를 기다리려면 Duration × 배율만큼 예약한다.
	//   예) 0.1초 × 0.05 = 0.005초(게임 시간) → 실제로는 약 0.1초 뒤에 끝난다.
	// 같은 핸들로 다시 SetTimer하면 이전 예약은 취소되고 새로 시작한다.
	GetWorldTimerManager().SetTimer(mHitStopTimer, this, &AArenaGameMode::EndHitStop,
		Duration * mHitStopDilation, false);
}

void AArenaGameMode::EndHitStop()
{
	mHitStopEndRealTime = 0.0;

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
}

void AArenaGameMode::PlayCameraShake(bool bHeavy, float Scale)
{
	if (Scale <= 0.f)
		return;

	// 흔들림 클래스는 레벨 시작 때 불러 둔다 (PreloadFeedbackAssets). 에셋이 없으면 흔들림만 건너뛴다.
	TSubclassOf<UCameraShakeBase>	ShakeClass = bHeavy ? mHeavyShakeClass : mLightShakeClass;
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (!ShakeClass || !IsValid(PlayerController))
		return;

	// ClientStartCameraShake(흔들림 클래스, 세기 배율) : 이 플레이어의 카메라만 흔든다.
	PlayerController->ClientStartCameraShake(ShakeClass, Scale);
}

void AArenaGameMode::PlayCameraShakeAt(bool bHeavy, float Scale, const FVector& Location,
	float Inner, float Outer)
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (!IsValid(PlayerPawn))
		return;

	// 사건 위치 ↔ 플레이어 거리로 세기를 줄인다.
	//   Inner 안 : 1,  Inner~Outer : 1 → 0으로 직선 감소,  Outer 밖 : 0
	// GetMappedRangeValueClamped(입력 범위, 출력 범위, 값) : 값을 입력 범위에서 출력 범위로 비례 변환하고 끝에서 자른다.
	float	Distance = FVector::Dist(PlayerPawn->GetActorLocation(), Location);
	float	Falloff = FMath::GetMappedRangeValueClamped(FVector2D(Inner, Outer), FVector2D(1.f, 0.f), Distance);

	PlayCameraShake(bHeavy, Scale * Falloff);
}

// ============================================================================
// 일시정지 (11/2)
// ============================================================================
void AArenaGameMode::TogglePause()
{
	// IsGamePaused : 지금 게임이 멈춰 있는지 (SetGamePaused로 바꾼 상태)
	if (UGameplayStatics::IsGamePaused(GetWorld()))
		ResumeGame();
	else
		PauseGame();
}

void AArenaGameMode::PauseGame()
{
	// 결과 화면이 뜬 뒤(승리·패배·스테이지 클리어)에는 열지 않는다. 결과 화면의 버튼과 겹치지 않게.
	// (11/4) 단, 스테이지 클리어 글자가 사라진 뒤 포탈까지 걸어가는 동안은 연다. 다시 움직이는 구간이라
	//   (11/3까지는 mBattleEnded만 봐서 오로라를 잡은 뒤에는 Esc가 안 먹었다)
	if (mBattleEnded && !mWalkingToPortal)
		return;

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (!IsValid(PlayerController))
		return;

	// 처음 열 때만 위젯을 만든다. (결과 화면처럼 처음 쓸 때 경로로 불러온다)
	if (!IsValid(mPauseWidget))
	{
		TSubclassOf<UPauseWidget>	WidgetClass = LoadClass<UPauseWidget>(nullptr, *mPauseWidgetPath);

		if (!WidgetClass)
		{
			UE_LOG(UGDebug, Warning, TEXT("Pause : WBP_Pause not found"));
			return;
		}

		mPauseWidget = CreateWidget<UPauseWidget>(PlayerController, WidgetClass);

		if (!IsValid(mPauseWidget))
			return;
	}

	// SetGamePaused(true) : 게임 시간을 멈춘다. 캐릭터·AI·타이머·애니메이션·월드 소리가 모두 멈추고,
	// 위젯(UI)과 UI 소리만 계속 움직인다.
	UGameplayStatics::SetGamePaused(GetWorld(), true);

	// AddToViewport(ZOrder) : 숫자가 클수록 위에 그려진다. HUD(기본 0)보다 위에.
	mPauseWidget->AddToViewport(10);

	// 마우스로 버튼을 누를 수 있게 커서를 보이고, 입력을 UI 전용으로 바꾼다.
	// SetWidgetToFocus : 키보드 포커스를 메뉴에 준다. (그래야 메뉴의 NativeOnKeyDown이 Esc를 받는다)
	PlayerController->SetShowMouseCursor(true);

	FInputModeUIOnly	InputMode;
	InputMode.SetWidgetToFocus(mPauseWidget->TakeWidget());
	PlayerController->SetInputMode(InputMode);
}

void AArenaGameMode::ResumeGame()
{
	// RemoveFromParent : 화면에서 떼어 낸다. mPauseWidget이 계속 가리키고 있어서 지워지지 않고 다음에 다시 쓴다.
	if (IsValid(mPauseWidget))
		mPauseWidget->RemoveFromParent();

	UGameplayStatics::SetGamePaused(GetWorld(), false);

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (IsValid(PlayerController))
	{
		// 게임 입력으로 되돌린다. (GameOnly : 포커스를 게임 화면으로 돌려 시점 회전·공격이 다시 먹힌다)
		PlayerController->SetShowMouseCursor(false);
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}
