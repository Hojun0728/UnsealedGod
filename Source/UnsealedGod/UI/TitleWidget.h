// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "MenuWidgetBase.h"
#include "TitleWidget.generated.h"

// ============================================================================
// UTitleWidget (11/2)
// 타이틀 화면의 C++ 부모 클래스. (수업 코드 UStartWidget과 같은 역할)
//   [START] 첫 스테이지(L_IcePalace)를 연다
//   [QUIT]  게임 종료
//
// 로고(제목 + 부제) 그림과 배치는 위젯 블루프린트(WBP_Title)에서 정한다. 여기서는 버튼 동작만 맡는다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UTitleWidget : public UMenuWidgetBase
{
	GENERATED_BODY()

protected:
	// ---------------------------------------------------------------- 블루프린트 위젯 (이름이 정확히 같아야 한다)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	QuitButton;

	// START를 누르면 열 레벨 (WBP_Title 클래스 디폴트에서 바꿀 수 있다)
	UPROPERTY(EditAnywhere, Category = "Title")
	FName	mFirstLevelName = TEXT("L_IcePalace");

	// (11/3) 시작 연출 동안 로고·버튼이 서서히 사라진다. (남은 시간, 전체 시간)
	float	mHideTime = 0.f;
	float	mHideDuration = 0.f;

public:
	// 시작 연출이 부른다. Duration초 동안 서서히 투명해지고 클릭도 막힌다.
	void HideForStart(float Duration);

protected:
	// 위젯이 처음 만들어질 때 한 번 (버튼 클릭 연결)
	virtual void NativeOnInitialized() override;

	// 매 프레임 (사라지기). 부모(UMenuWidgetBase)의 시간 세기도 돌도록 Super를 먼저 부른다.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnQuitClicked();
};
