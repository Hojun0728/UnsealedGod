// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "MenuWidgetBase.h"
#include "PauseWidget.generated.h"

// ============================================================================
// UPauseWidget
// Esc로 여는 일시정지 메뉴의 C++ 부모 클래스. (11/2)
//   [RESUME]  계속하기 : 메뉴를 닫고 게임 재개
//   [RESTART] 다시 시작 : 지금 스테이지를 처음부터 (결과 화면의 다시 시도와 같다)
//   [TITLE]   타이틀로 : 타이틀 레벨(L_Title)을 연다
//   [QUIT]    게임 종료
//
// 결과 화면(UResultWidget)과 같은 구조 : 배치·폰트는 위젯 블루프린트(WBP_Pause), 동작은 여기서 맡는다.
// 게임을 멈추고 푸는 일은 게임 모드(AArenaGameMode)가 한다. 이 위젯은 "버튼이 눌렸다"만 알린다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UPauseWidget : public UMenuWidgetBase
{
	GENERATED_BODY()

protected:
	// ---------------------------------------------------------------- 블루프린트 위젯 (이름이 정확히 같아야 한다)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	RestartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	TitleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	QuitButton;

	// 타이틀 레벨 이름 (WBP_Pause 클래스 디폴트에서 바꿀 수 있다)
	UPROPERTY(EditAnywhere, Category = "Pause")
	FName	mTitleLevelName = TEXT("L_Title");

protected:
	// 위젯이 처음 만들어질 때 한 번 (버튼 클릭 연결). NativeConstruct와 달리
	// 화면에 다시 붙일 때마다 불리지 않아서, 같은 위젯을 여러 번 열어도 클릭이 두 번 연결되지 않는다.
	virtual void NativeOnInitialized() override;

	// 메뉴가 열려 있는 동안 키보드 입력.
	// 메뉴가 열리면 입력이 "UI 전용"이라 게임 입력(Enhanced Input)이 Esc를 받지 못한다.
	// 그래서 메뉴를 닫는 Esc는 위젯이 직접 받는다.
	// FReply : "이 입력을 내가 처리했다(Handled) / 안 했다(Unhandled)"를 돌려준다.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 버튼 클릭 (OnClicked는 다이나믹 델리게이트라 UFUNCTION 필요)
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnTitleClicked();

	UFUNCTION()
	void OnQuitClicked();
};
