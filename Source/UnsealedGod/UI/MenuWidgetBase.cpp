// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuWidgetBase.h"
#include "Kismet/KismetSystemLibrary.h"

void UMenuWidgetBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// (11/4) 키보드 포커스를 받을 수 있게 한다.
	// 타이틀·일시정지·결과 모두 입력을 UI 전용으로 바꾸면서 이 위젯에 포커스를 준다(SetWidgetToFocus).
	// 포커스를 못 받는 위젯이면 로그에 Error가 남고 키 입력(일시정지의 Esc)도 못 받는다.
	// (11/2에는 일시정지 위젯에만 있었다 → 공통 부모로 옮겨 새 메뉴도 빠뜨리지 않게)
	SetIsFocusable(true);
}

void UMenuWidgetBase::DelayOpenLevel(FName LevelName, const FString& Options)
{
	if (IsActionPending())
		return;

	mPendingLevelName = LevelName;
	mPendingOptions = Options;
	StartPending(EPendingAction::OpenLevel);
}

void UMenuWidgetBase::DelayQuit()
{
	if (IsActionPending())
		return;

	StartPending(EPendingAction::Quit);
}

void UMenuWidgetBase::StartPending(EPendingAction Action)
{
	mPendingAction = Action;
	mPendingTime = mActionDelay;

	// HitTestInvisible : 보이기는 그대로, 마우스 클릭만 통과시킨다. → 기다리는 동안 다른 버튼이 눌려 동작이 겹치지 않는다.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 기다릴 시간이 없으면 바로 실행
	if (mPendingTime <= 0.f)
		RunPending();
}

void UMenuWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsActionPending())
		return;

	mPendingTime -= InDeltaTime;

	if (mPendingTime <= 0.f)
		RunPending();
}

void UMenuWidgetBase::RunPending()
{
	EPendingAction	Action = mPendingAction;

	// 먼저 비워 둔다. (OpenLevel 도중에 다시 불려도 두 번 실행되지 않게)
	mPendingAction = EPendingAction::None;

	if (Action == EPendingAction::OpenLevel)
	{
		// 새 레벨은 멈춤이 풀린 상태로 시작하고, 플레이어 컨트롤러도 새로 만들어져 입력이 기본값으로 돌아간다.
		// OpenLevel(이름, 절대 이동, 옵션) : 옵션은 새 레벨의 게임 모드가 OptionsString으로 읽는다
		UGameplayStatics::OpenLevel(this, mPendingLevelName, true, mPendingOptions);
	}
	else if (Action == EPendingAction::Quit)
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	}
}
