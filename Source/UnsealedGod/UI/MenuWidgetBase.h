// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "MenuWidgetBase.generated.h"

// ============================================================================
// UMenuWidgetBase (11/3)
// 버튼이 있는 메뉴 화면(타이틀·일시정지·결과)의 공통 부모.
//   레벨을 바꾸거나 게임을 끄는 버튼은 누름 소리가 끝날 때까지 mActionDelay초 기다렸다가 실행한다.
//   (바로 실행하면 레벨이 바뀌거나 게임이 꺼지는 순간 소리도 같이 끊긴다)
//
// 타이머(SetTimer)가 아니라 위젯의 NativeTick으로 시간을 센다.
//   일시정지 메뉴는 게임이 멈춘 상태에서 눌리는데, 게임이 멈추면 월드 타이머도 멈춘다.
//   위젯은 멈춘 동안에도 Tick이 돌고, 넘어오는 시간(InDeltaTime)도 실제 시간이라 멈춤·히트스톱과 상관없다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 누른 뒤 실제로 실행하기까지 기다리는 시간(초). 누름 소리(S_SFX_UI_Press) 길이와 같게.
	UPROPERTY(EditAnywhere, Category = "Menu")
	float	mActionDelay = 0.3f;

	// 기다리는 중인 동작 : 없음 / 레벨 열기 / 게임 종료
	enum class EPendingAction : uint8
	{
		None,
		OpenLevel,
		Quit,
	};

	EPendingAction	mPendingAction = EPendingAction::None;

	// 열 레벨 이름, (11/5) 레벨 옵션, 남은 시간(초)
	FName	mPendingLevelName;
	FString	mPendingOptions;
	float	mPendingTime = 0.f;

protected:
	// 위젯이 처음 만들어질 때 한 번 : 키보드 포커스를 받을 수 있게 한다. (자식도 Super를 불러야 한다)
	virtual void NativeOnInitialized() override;

	// mActionDelay초 뒤에 레벨을 연다. 기다리는 동안 이 위젯의 모든 버튼은 눌리지 않는다.
	// (11/5) Options : 레벨 옵션 문자열. 다시 시도·RESTART는 "Retry"를 붙여 → 게임 모드가 시작 연출을 짧게 한다
	void DelayOpenLevel(FName LevelName, const FString& Options = TEXT(""));

	// mActionDelay초 뒤에 게임을 끈다. (에디터에서는 플레이만 끝난다)
	void DelayQuit();

	// 이미 다른 동작을 기다리는 중인지. (자식이 키 입력 등을 막을 때 쓴다)
	bool IsActionPending()	const
	{
		return mPendingAction != EPendingAction::None;
	}

	// 매 프레임 : 남은 시간을 줄이다가 0이 되면 실행한다.
	// 자식이 NativeTick을 덮어쓰면 반드시 Super::NativeTick을 불러야 이 시간 세기도 돈다.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// 동작을 예약하고 버튼을 잠근다. (DelayOpenLevel·DelayQuit의 공통 부분)
	void StartPending(EPendingAction Action);

	// 예약한 동작을 실제로 실행한다.
	void RunPending();
};
