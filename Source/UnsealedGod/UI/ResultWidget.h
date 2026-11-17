// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "MenuWidgetBase.h"
#include "ResultWidget.generated.h"

// ============================================================================
// UResultWidget
// 전투가 끝났을 때 나오는 결과 화면의 C++ 부모 클래스. (10/19)
//   패배 : "YOU SEALED" / 승리 : "GOD SLAIN"
//   큰 글자가 서서히 나타나고, mButtonDelay초 뒤 아래에 [다시 시도] [게임 종료] 버튼이 나온다.
//
// 배치·폰트·색 같은 "디자인"은 위젯 블루프린트(WBP_Result)에서, "동작"은 여기서 맡는다.
// BindWidget : 블루프린트에 "같은 이름·같은 종류"의 위젯이 있으면 컴파일할 때 이 변수에 자동으로 연결된다.
//   이름이 다르거나 없으면 위젯 블루프린트 컴파일 에러가 나서, 빠뜨린 것을 바로 알 수 있다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UResultWidget : public UMenuWidgetBase
{
	GENERATED_BODY()

protected:
	// ---------------------------------------------------------------- 블루프린트 위젯 (이름이 정확히 같아야 한다)
	// 큰 결과 글자
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	ResultText;

	// 다시 시도 (레벨 처음부터)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	RetryButton;

	// 게임 종료 (에디터에서는 플레이 종료)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton>	QuitButton;

	// ---------------------------------------------------------------- 디자인 값 (위젯 블루프린트의 클래스 디폴트에서 바꾼다)
	UPROPERTY(EditAnywhere, Category = "Result")
	FText	mDefeatText = FText::FromString(TEXT("YOU SEALED"));

	UPROPERTY(EditAnywhere, Category = "Result")
	FText	mVictoryText = FText::FromString(TEXT("GOD SLAIN"));

	UPROPERTY(EditAnywhere, Category = "Result")
	FSlateColor	mDefeatColor = FSlateColor(FLinearColor(0.6f, 0.05f, 0.05f));

	UPROPERTY(EditAnywhere, Category = "Result")
	FSlateColor	mVictoryColor = FSlateColor(FLinearColor(0.9f, 0.75f, 0.35f));

	// (10/26) 결과 화면이 뜰 때 한 번 나는 소리. 비워 두면 소리 없음. (WBP_Result 클래스 디폴트에서 고른다)
	UPROPERTY(EditAnywhere, Category = "Result|Sound")
	TObjectPtr<class USoundBase>	mDefeatSound;

	UPROPERTY(EditAnywhere, Category = "Result|Sound")
	TObjectPtr<class USoundBase>	mVictorySound;

	// 글자가 완전히 나타나는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Result")
	float	mFadeInTime = 1.5f;

	// 화면이 뜨고 몇 초 뒤에 버튼을 보여줄지
	UPROPERTY(EditAnywhere, Category = "Result")
	float	mButtonDelay = 3.f;

	// 화면이 뜬 뒤 지난 시간, 버튼을 이미 보여줬는지
	float	mElapsed = 0.f;
	bool	mButtonsShown = false;

	// (10/26) 스테이지 클리어 모드 : 버튼 없이 글자만 떴다가 사라진다. (다음 스테이지 포탈이 있을 때)
	// 페이드인(mFadeInTime) → mStageClearHoldTime초 유지 → mStageClearFadeOutTime초 동안 사라짐 → 화면에서 떼어 냄
	bool	mStageClear = false;
	float	mStageClearHoldTime = 2.5f;
	float	mStageClearFadeOutTime = 1.f;

public:
	// 승리·패배에 맞춰 글자와 색을 정한다. 화면에 붙이기(AddToViewport) 전에 부른다.
	void SetResult(bool bVictory);

	// (10/26) 스테이지 클리어 모드로 바꾼다. AddToViewport 전에 부른다.
	void SetStageClear(float HoldTime, float FadeOutTime);

	// 스테이지 클리어 화면이 처음부터 끝까지 걸리는 시간(초). 게임 모드가 이 시간 뒤에 포탈을 연다.
	float GetStageClearDuration()	const
	{
		return mFadeInTime + mStageClearHoldTime + mStageClearFadeOutTime;
	}

protected:
	// 위젯이 만들어져 화면에 붙을 때 한 번 (버튼 클릭 연결, 처음엔 글자 투명·버튼 숨김)
	virtual void NativeConstruct() override;

	// 매 프레임 (글자 페이드인, 시간이 되면 버튼 표시)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 버튼을 보여주고 마우스 커서·UI 입력으로 바꾼다.
	void ShowButtons();

	// 버튼 클릭 (OnClicked는 다이나믹 델리게이트라 UFUNCTION 필요)
	UFUNCTION()
	void OnRetryClicked();

	UFUNCTION()
	void OnQuitClicked();
};
