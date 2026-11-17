// Fill out your copyright notice in the Description page of Project Settings.


#include "BossIntroWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void UBossIntroWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// HitTestInvisible : 보이기만 하고 마우스는 통과. 연출 카드라 누를 것이 없다.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	mStageValue = mStageTarget = 0.f;
	mBossElapsed = 0.f;
	mBossFade = 0.f;
	mBossShowing = false;

	ApplyStageCard();
	ApplyBossCard();
}

void UBossIntroWidget::ShowStageCard(const FText& StageName, UTexture2D* Icon)
{
	if (IsValid(StageNameText))
		StageNameText->SetText(StageName);

	// 아이콘이 없는 맵이면 아이콘 칸을 접는다 (Collapsed : 자리도 차지하지 않음 → 글자가 왼쪽으로 붙는다)
	if (IsValid(StageIcon))
	{
		if (IsValid(Icon))
		{
			StageIcon->SetBrushFromTexture(Icon);
			StageIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			StageIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	mStageTarget = 1.f;
}

void UBossIntroWidget::HideStageCard()
{
	mStageTarget = 0.f;
}

void UBossIntroWidget::ShowBossCard(const FText& BossName, const FText& BossTitle, UTexture2D* NameImage,
	UTexture2D* TitleImage, const FLinearColor& TintColor)
{
	// 이름·부제 : 그림이 있으면 그림을 보이고 글자는 접는다. 없으면 반대. (같은 규칙이라 아래 ShowImageOrText 하나로)
	ShowImageOrText(BossNameImage, NameImage, BossNameText, BossName);
	ShowImageOrText(BossTitleImage, TitleImage, BossTitleText, BossTitle);

	// 화면 덧칠 : 비네트 그림(흰색)에 보스 색을 곱한다. SetColorAndOpacity = 이미지 색조
	if (IsValid(BossBackdrop))
		BossBackdrop->SetColorAndOpacity(TintColor);

	// 처음부터 다시 : 띠 → 이름 쾅 → 부제 쾅 (시차는 mBossElapsed로 센다)
	mBossElapsed = 0.f;
	mBossFade = 1.f;
	mBossShowing = true;

	// 카드가 나오면 건너뛰기 안내는 치운다 (카드에 시선이 가도록)
	if (IsValid(SkipHint))
		SkipHint->SetVisibility(ESlateVisibility::Hidden);
}

void UBossIntroWidget::ShowImageOrText(UImage* ImageWidget, UTexture2D* Texture, UTextBlock* TextWidget, const FText& Text)
{
	bool	bUseImage = IsValid(ImageWidget) && IsValid(Texture);

	if (IsValid(ImageWidget))
	{
		// SetBrushFromTexture(그림) : 블루프린트에서 정한 크기는 그대로 두고 그림만 바꾼다
		if (bUseImage)
			ImageWidget->SetBrushFromTexture(Texture);

		ImageWidget->SetVisibility(bUseImage ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (IsValid(TextWidget))
	{
		TextWidget->SetText(Text);
		TextWidget->SetVisibility(bUseImage ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UBossIntroWidget::HideBossCard()
{
	// 셋이 함께 mBossOutTime초 동안 흐려진다 (NativeTick)
	mBossShowing = false;
}

void UBossIntroWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// FInterpConstantTo(지금, 목표, 시간, 속도) : 목표 쪽으로 "1초에 속도만큼" 일정하게 다가간다. 넘치지 않는다.
	// 속도 = 1 ÷ 걸리는 시간 → 0에서 1까지 정확히 그 시간이 걸린다.
	float	StageSpeed = 1.f / FMath::Max(mStageTarget > mStageValue ? mStageInTime : mStageOutTime, 0.01f);

	mStageValue = FMath::FInterpConstantTo(mStageValue, mStageTarget, InDeltaTime, StageSpeed);

	// 보스 카드 : 나타나는 동안은 시간을 세서 시차를 만들고, 사라질 때는 전체 투명도만 내린다
	if (mBossShowing)
		mBossElapsed += InDeltaTime;
	else
		mBossFade = FMath::FInterpConstantTo(mBossFade, 0.f, InDeltaTime, 1.f / FMath::Max(mBossOutTime, 0.01f));

	ApplyStageCard();
	ApplyBossCard();
}

void UBossIntroWidget::ApplyStageCard()
{
	if (!IsValid(StageCard))
		return;

	// 감속 곡선 : 1 - (1 - t)³. 빠르게 들어오다가 제자리에서 부드럽게 멈춘다.
	float	Ease = 1.f - FMath::Pow(1.f - mStageValue, 3.f);

	StageCard->SetRenderOpacity(mStageValue);

	// SetRenderTranslation : 배치(레이아웃)는 그대로 두고 그려지는 위치만 옮긴다. 왼쪽 밖(-거리)에서 제자리(0)로.
	StageCard->SetRenderTranslation(FVector2D(-mStageSlideDistance * (1.f - Ease), 0.f));
}

void UBossIntroWidget::ApplyBossCard()
{
	// ① 배경 띠 : 0초부터 mBandInTime초 동안 깔린다. 사라질 때는 전체 투명도(mBossFade)를 따른다
	float	Band = FMath::Clamp(mBossElapsed / FMath::Max(mBandInTime, 0.01f), 0.f, 1.f) * mBossFade;

	if (IsValid(BossBackdrop))
		BossBackdrop->SetRenderOpacity(Band);

	// 보스 뒤 배경 띠 = 후처리 머티리얼의 Opacity 파라미터
	if (IsValid(mBandMaterial))
		mBandMaterial->SetScalarParameterValue(TEXT("Opacity"), Band);

	// 이름·부제를 담은 패널 전체 투명도 = 사라질 때 함께 흐려지는 값
	if (IsValid(BossCard))
		BossCard->SetRenderOpacity(mBossFade);

	// ② 이름 쾅 (mNameDelay초부터) ③ 부제 쾅 (mTitleDelay초부터)
	float	InTime = FMath::Max(mBossInTime, 0.01f);

	ApplySlam(BossNameImage, BossNameText, FMath::Clamp((mBossElapsed - mNameDelay) / InTime, 0.f, 1.f));
	ApplySlam(BossTitleImage, BossTitleText, FMath::Clamp((mBossElapsed - mTitleDelay) / InTime, 0.f, 1.f));
}

void UBossIntroWidget::ApplySlam(UWidget* ImageWidget, UWidget* TextWidget, float Value)
{
	// 감속 곡선으로 1.25배 → 1배. 다 들어온 뒤(1)에는 제 크기 그대로 (사라질 때도)
	float	Ease = 1.f - FMath::Pow(1.f - Value, 3.f);
	float	Scale = FMath::Lerp(mBossSlamScale, 1.f, Ease);

	// 그림·글자 중 접혀 있는 쪽에 적용해도 안 보이니 상관없다. 둘 다 같은 값으로
	for (UWidget* Widget : { ImageWidget, TextWidget })
	{
		if (!IsValid(Widget))
			continue;

		Widget->SetRenderOpacity(Value);
		Widget->SetRenderScale(FVector2D(Scale, Scale));
	}
}
