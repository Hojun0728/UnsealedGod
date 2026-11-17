// Fill out your copyright notice in the Description page of Project Settings.


#include "TitleWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Player/TitlePlayerController.h"

void UTitleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(StartButton))
		StartButton->OnClicked.AddDynamic(this, &UTitleWidget::OnStartClicked);

	if (IsValid(QuitButton))
		QuitButton->OnClicked.AddDynamic(this, &UTitleWidget::OnQuitClicked);
}

void UTitleWidget::OnStartClicked()
{
	// 첫 스테이지를 연다. 새 레벨은 AArenaGameMode가 맡고, BeginPlay에서 입력을 게임 전용으로 되돌린다.
	// (11/3) 바로 열지 않고 시작 연출(세바로그 움켜쥐기 → 페이드아웃)을 거친다.
	// 연출 순서는 컨트롤러가 맡는다. 연출이 1초라 누름 소리(0.3초)도 끝까지 난다.
	// GetOwningPlayer : 이 위젯을 만든 플레이어 컨트롤러 (ATitlePlayerController라서 Cast가 성공한다)
	ATitlePlayerController* Controller = Cast<ATitlePlayerController>(GetOwningPlayer());

	if (IsValid(Controller))
		Controller->StartGame(mFirstLevelName);
	else
		DelayOpenLevel(mFirstLevelName);
}

void UTitleWidget::OnQuitClicked()
{
	// 게임 종료. 에디터에서 플레이 중이면 플레이만 끝난다.
	DelayQuit();
}

void UTitleWidget::HideForStart(float Duration)
{
	mHideDuration = FMath::Max(Duration, 0.01f);
	mHideTime = mHideDuration;

	// 보이기는 그대로(사라지는 중), 클릭만 막는다. → 연출 도중 QUIT 등이 눌리지 않는다.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTitleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (mHideTime <= 0.f)
		return;

	mHideTime = FMath::Max(mHideTime - InDeltaTime, 0.f);

	// 남은 시간 비율 = 불투명도 (1 → 0)
	SetRenderOpacity(mHideTime / mHideDuration);
}
