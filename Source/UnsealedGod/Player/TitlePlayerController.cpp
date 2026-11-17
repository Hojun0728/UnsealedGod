// Fill out your copyright notice in the Description page of Project Settings.


#include "TitlePlayerController.h"
#include "../UI/TitleWidget.h"
// ASkeletalMeshActor : 레벨에 놓은 스켈레탈 메시 (타이틀의 세바로그)
#include "Animation/SkeletalMeshActor.h"
#include "Animation/AnimSequence.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"

ATitlePlayerController::ATitlePlayerController()
{
	// 타이틀은 마우스로 버튼을 누르는 화면이라 처음부터 커서를 보인다.
	bShowMouseCursor = true;

	// (11/3) 시작 연출 에셋
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		StartAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Subjugation.Subjugation'"));

	if (StartAnim.Succeeded())
		mStartAnim = StartAnim.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		StartVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Ability_RMB_Engage.Sevarog_Ability_RMB_Engage'"));

	if (StartVoice.Succeeded())
		mStartVoice = StartVoice.Object;
}

void ATitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	TSubclassOf<UTitleWidget>	WidgetClass = LoadClass<UTitleWidget>(nullptr, *mTitleWidgetPath);

	if (!WidgetClass)
	{
		UE_LOG(UGDebug, Warning, TEXT("Title : WBP_Title not found"));
		return;
	}

	// CreateWidget(주인 플레이어 컨트롤러, 위젯 클래스) : 이 컨트롤러가 주인인 위젯을 만든다.
	mTitleWidget = CreateWidget<UTitleWidget>(this, WidgetClass);

	if (!IsValid(mTitleWidget))
		return;

	mTitleWidget->AddToViewport();

	// 입력을 UI 전용으로 : 클릭이 게임 입력으로 새지 않고 버튼에만 간다.
	FInputModeUIOnly	InputMode;
	InputMode.SetWidgetToFocus(mTitleWidget->TakeWidget());
	SetInputMode(InputMode);
}

void ATitlePlayerController::StartGame(FName LevelName)
{
	if (mStarting)
		return;

	mStarting = true;
	mNextLevelName = LevelName;

	// 1. 로고·버튼이 서서히 사라진다. (세바로그 동작을 가리지 않게)
	if (IsValid(mTitleWidget))
		mTitleWidget->HideForStart(mWidgetHideTime);

	SetShowMouseCursor(false);

	// (11/3) 대사 길이에 맞춰 동작을 늦춘다. 손을 움켜쥐는 순간 = 대사가 끝나는 순간.
	// GetDuration : 소리 길이(초). 큐에 대사가 여러 개면 가장 긴 것 → 짧은 대사가 나와도 잘리지 않는다.
	float	PlayRate = mMaxPlayRate;

	if (IsValid(mStartVoice) && mStartVoice->GetDuration() > 0.f)
		PlayRate = FMath::Clamp(mGrabTime / mStartVoice->GetDuration(), mMinPlayRate, mMaxPlayRate);

	// 실제로 움켜쥐는 시각 (느리게 틀면 그만큼 늦어진다)
	float	GrabTime = mGrabTime / PlayRate;

	// 2. 세바로그가 움켜쥔다. 타이틀 레벨의 스켈레탈 메시 액터는 세바로그 하나뿐이라 처음 찾은 것을 쓴다.
	//    애님 블루프린트 없이 레벨에 놓인 메시라서 몽타주 대신 PlayAnimation(동작, 반복 여부)으로 바로 재생한다.
	for (TActorIterator<ASkeletalMeshActor> It(GetWorld()); It; ++It)
	{
		USkeletalMeshComponent* Mesh = It->GetSkeletalMeshComponent();

		if (IsValid(Mesh) && IsValid(mStartAnim))
		{
			Mesh->PlayAnimation(mStartAnim, false);
			Mesh->SetPlayRate(PlayRate);
		}

		break;
	}

	// 대사 (화면 전체에 들리는 2D 소리)
	if (IsValid(mStartVoice))
		UGameplayStatics::PlaySound2D(this, mStartVoice);

	UE_LOG(UGDebug, Warning, TEXT("Title : start (voice %.2fs, play rate %.2f, grab at %.2fs)"),
		IsValid(mStartVoice) ? mStartVoice->GetDuration() : 0.f, PlayRate, GrabTime);

	// 3. 움켜쥐는 순간에 완전히 검어지도록 mFadeDuration초 앞에서 어두워지기 시작, 4. 움켜쥐는 순간 다음 레벨
	GetWorldTimerManager().SetTimer(mFadeTimer, this, &ATitlePlayerController::StartFadeOut, FMath::Max(GrabTime - mFadeDuration, 0.01f), false);
	GetWorldTimerManager().SetTimer(mOpenLevelTimer, this, &ATitlePlayerController::OpenNextLevel, GrabTime, false);
}

void ATitlePlayerController::StartFadeOut()
{
	// StartCameraFade(시작 불투명도, 끝 불투명도, 시간, 색, 소리도 줄일지, 끝난 뒤 그 색으로 유지할지)
	// 유지(true)해야 레벨이 바뀌기 전 한 프레임에 원래 화면이 번쩍 보이지 않는다.
	if (IsValid(PlayerCameraManager))
		PlayerCameraManager->StartCameraFade(0.f, 1.f, mFadeDuration, FLinearColor::Black, false, true);
}

void ATitlePlayerController::OpenNextLevel()
{
	UGameplayStatics::OpenLevel(this, mNextLevelName);
}
