// Fill out your copyright notice in the Description page of Project Settings.


#include "ResultWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"

void UResultWidget::SetResult(bool bVictory)
{
	// 결과 소리 (10/26) : 화면 소리라 위치 없이 2D로. (글자 위젯이 없어도 소리는 낸다)
	USoundBase* Sound = bVictory ? mVictorySound : mDefeatSound;

	if (IsValid(Sound))
		UGameplayStatics::PlaySound2D(this, Sound);

	if (!IsValid(ResultText))
		return;

	ResultText->SetText(bVictory ? mVictoryText : mDefeatText);
	ResultText->SetColorAndOpacity(bVictory ? mVictoryColor : mDefeatColor);
}

void UResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	mElapsed = 0.f;
	mButtonsShown = false;

	// 처음엔 글자를 투명하게, 버튼은 숨긴다. (Hidden : 안 보이지만 자리는 차지 → 배치가 흔들리지 않는다)
	if (IsValid(ResultText))
		ResultText->SetRenderOpacity(0.f);

	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(ESlateVisibility::Hidden);
		RetryButton->OnClicked.AddDynamic(this, &UResultWidget::OnRetryClicked);
	}

	if (IsValid(QuitButton))
	{
		QuitButton->SetVisibility(ESlateVisibility::Hidden);
		QuitButton->OnClicked.AddDynamic(this, &UResultWidget::OnQuitClicked);
	}
}

void UResultWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	mElapsed += InDeltaTime;

	// 글자 페이드인 : 0 → 1 (다크소울의 사망 글자처럼 천천히)
	if (IsValid(ResultText) && mFadeInTime > 0.f)
		ResultText->SetRenderOpacity(FMath::Clamp(mElapsed / mFadeInTime, 0.f, 1.f));

	// (10/26) 스테이지 클리어 : 버튼 없이, 유지 시간이 지나면 글자가 사라지고 위젯도 화면에서 떼어 낸다.
	if (mStageClear)
	{
		float	FadeOutStart = mFadeInTime + mStageClearHoldTime;

		if (mElapsed >= FadeOutStart)
		{
			float	Alpha = mStageClearFadeOutTime > 0.f ? (mElapsed - FadeOutStart) / mStageClearFadeOutTime : 1.f;

			if (IsValid(ResultText))
				ResultText->SetRenderOpacity(FMath::Clamp(1.f - Alpha, 0.f, 1.f));

			// RemoveFromParent : 화면(뷰포트)에서 떼어 낸다. 더 이상 아무도 가리키지 않으면 나중에 정리된다.
			if (Alpha >= 1.f)
				RemoveFromParent();
		}

		return;
	}

	if (!mButtonsShown && mElapsed >= mButtonDelay)
		ShowButtons();
}

void UResultWidget::SetStageClear(float HoldTime, float FadeOutTime)
{
	mStageClear = true;
	mStageClearHoldTime = HoldTime;
	mStageClearFadeOutTime = FadeOutTime;
}

void UResultWidget::ShowButtons()
{
	mButtonsShown = true;

	if (IsValid(RetryButton))
		RetryButton->SetVisibility(ESlateVisibility::Visible);

	if (IsValid(QuitButton))
		QuitButton->SetVisibility(ESlateVisibility::Visible);

	// 마우스로 버튼을 누를 수 있게 커서를 보이고, 입력을 UI 전용으로 바꾼다.
	// (게임 입력이 살아 있으면 클릭이 시점 회전·공격으로 먹힌다)
	APlayerController* PlayerController = GetOwningPlayer();

	if (IsValid(PlayerController))
	{
		PlayerController->SetShowMouseCursor(true);

		FInputModeUIOnly	InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		PlayerController->SetInputMode(InputMode);
	}
}

void UResultWidget::OnRetryClicked()
{
	// 지금 레벨을 처음부터 다시 연다. 새 레벨에서는 플레이어 컨트롤러도 새로 만들어져
	// 마우스 커서·입력 모드가 게임 기본값으로 돌아간다.
	// (10/22) 다시 시도도 등장 동작부터 (내 판단 : 처음과 똑같이)
	// (11/5) 단, 맵 둘러보기·보스 소개는 처음 들어올 때만 → "Retry" 옵션 (소울류는 다시 하는 일이 많아서)
	// (11/3) 누름 소리가 끝나도록 부모(UMenuWidgetBase)가 mActionDelay초 뒤에 연다.
	DelayOpenLevel(FName(*UGameplayStatics::GetCurrentLevelName(this)), TEXT("Retry"));
}

void UResultWidget::OnQuitClicked()
{
	// 게임 종료. 에디터에서 플레이 중이면 플레이만 끝난다. (11/3) 누름 소리가 끝난 뒤에
	DelayQuit();
}
