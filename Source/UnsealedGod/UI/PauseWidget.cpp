// Fill out your copyright notice in the Description page of Project Settings.


#include "PauseWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../GameMode/ArenaGameMode.h"

void UPauseWidget::NativeOnInitialized()
{
	// 부모 : 키보드 포커스를 받을 수 있게 한다. (포커스가 있어야 NativeOnKeyDown이 Esc를 받는다)
	Super::NativeOnInitialized();

	if (IsValid(ResumeButton))
		ResumeButton->OnClicked.AddDynamic(this, &UPauseWidget::OnResumeClicked);

	if (IsValid(RestartButton))
		RestartButton->OnClicked.AddDynamic(this, &UPauseWidget::OnRestartClicked);

	if (IsValid(TitleButton))
		TitleButton->OnClicked.AddDynamic(this, &UPauseWidget::OnTitleClicked);

	if (IsValid(QuitButton))
		QuitButton->OnClicked.AddDynamic(this, &UPauseWidget::OnQuitClicked);
}

FReply UPauseWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// (11/3) 다시 시작·타이틀·종료를 기다리는 중이면 Esc로 닫지 않는다. (닫히면서 게임이 0.3초 재개되는 것 방지)
	if (IsActionPending())
		return FReply::Handled();

	// 메뉴를 연 키(Esc)를 다시 누르면 닫는다. (소울류·대부분의 게임과 같은 조작)
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnResumeClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPauseWidget::OnResumeClicked()
{
	// 멈춤을 푸는 일은 게임 모드가 한다. (위젯 떼기·게임 재개·마우스 커서 숨기기를 한 곳에서)
	// GetGameMode : 지금 레벨의 게임 모드. AArenaGameMode가 아닌 레벨이면 Cast가 nullptr을 돌려준다.
	AArenaGameMode* GameMode = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this));

	if (IsValid(GameMode))
		GameMode->ResumeGame();
}

void UPauseWidget::OnRestartClicked()
{
	// 지금 레벨을 처음부터 다시 연다. (결과 화면의 다시 시도와 같다)
	// 새 레벨은 멈춤이 풀린 상태로 시작하고, 플레이어 컨트롤러도 새로 만들어져 입력이 게임 기본값으로 돌아간다.
	// (11/3) 누름 소리가 끝나도록 mActionDelay초 뒤에 연다. (게임이 멈춰 있어도 위젯 Tick으로 센다)
	// (11/5) "Retry" : 맵 둘러보기·보스 소개 없이 짧은 등장 연출만 (다시 하는 사람은 이미 봤다)
	DelayOpenLevel(FName(*UGameplayStatics::GetCurrentLevelName(this)), TEXT("Retry"));
}

void UPauseWidget::OnTitleClicked()
{
	DelayOpenLevel(mTitleLevelName);
}

void UPauseWidget::OnQuitClicked()
{
	// 게임 종료. 에디터에서 플레이 중이면 플레이만 끝난다.
	DelayQuit();
}
