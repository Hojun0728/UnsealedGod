// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "BossIntroWidget.generated.h"

// ============================================================================
// UBossIntroWidget (11/5)
// 시작 연출의 카드 두 장 + 건너뛰기 안내. 디자인은 WBP_BossIntro, 동작(나타나기·사라지기)은 여기서.
//   맵 카드   : 맵 둘러보기 동안 왼쪽 아래. 아이콘 + 맵 이름 (마리오카트의 코스 소개)
//               왼쪽에서 미끄러져 들어오고, 둘러보기가 끝나면 왼쪽으로 빠진다.
//   보스 카드 : 보스 등장 동작이 멈추는 순간. 영어 이름 크게 + 한글 칭호 (보더랜드의 보스 소개)
//               살짝 크게 시작해 제자리로 "쾅" 들어오고(0.15초), 유지 뒤 서서히 사라진다.
//   건너뛰기  : 구석의 "SPACE — SKIP" (선택)
// 순서(언제 띄우고 언제 지울지)는 게임 모드(AArenaGameMode)가 정한다.
// 애니메이션은 NativeTick의 실제 시간으로 센다 → 보스만 멈춘 동안(CustomTimeDilation 0)에도 카드는 움직인다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UBossIntroWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// ---------------------------------------------------------------- 블루프린트 위젯 (이름이 정확히 같아야 한다)
	// 맵 카드 전체 (배경·아이콘·글자를 담은 패널). 이 패널을 통째로 움직인다
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidget>	StageCard;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	StageNameText;

	// 맵 아이콘. 없어도 된다(BindWidgetOptional). 보스에 아이콘이 정해져 있지 않으면 숨긴다
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage>	StageIcon;

	// 보스 카드 전체 (이름·칭호를 담은 패널)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidget>	BossCard;

	// 보스 이름 : 로고 그림(BossNameImage)이 있으면 그림, 없으면 글자(BossNameText). 둘 다 선택형 (11/5 보더랜드식 로고)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage>	BossNameImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock>	BossNameText;

	// 부제 : 그림(BossTitleImage, AI)이 있으면 그림, 없으면 글자(BossTitleText). 둘 다 선택형
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage>	BossTitleImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock>	BossTitleText;

	// (11/5) 화면 가장자리 색 덧칠 (비네트 그림, 전체 화면). 보스 카드와 같이 나타나고 사라진다. 크기는 바뀌지 않는다
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage>	BossBackdrop;

	// "SPACE — SKIP" 안내. 없어도 된다
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UWidget>	SkipHint;

	// ---------------------------------------------------------------- 디자인 값 (WBP_BossIntro 클래스 디폴트에서 바꾼다)
	// 맵 카드 : 나타나는·사라지는 시간(초), 미끄러져 들어오는 거리(왼쪽으로 몇 px 밖에서 시작하는지)
	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mStageInTime = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mStageOutTime = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mStageSlideDistance = 500.f;

	// 보스 카드 (11/5 시차 : 띠 → 이름 쾅 → 부제 쾅). 시각은 보스가 멈춘 순간부터(초)
	// 배경 띠가 다 깔리는 시간
	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mBandInTime = 0.2f;

	// 이름·부제가 쾅 들어오기 시작하는 시각
	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mNameDelay = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mTitleDelay = 0.45f;

	// 한 번 쾅 들어오는 시간, 시작 크기(1.25배 → 1배), 셋이 함께 사라지는 시간
	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mBossInTime = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mBossSlamScale = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Intro")
	float	mBossOutTime = 0.3f;

	// (11/5) 보스 뒤 배경 띠의 후처리 머티리얼. 띠의 진행 값으로 "Opacity" 파라미터를 올리고 내린다
	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic>	mBandMaterial;

	// ---------------------------------------------------------------- 진행 상태 (0 = 안 보임, 1 = 다 보임)
	float	mStageValue = 0.f;
	float	mStageTarget = 0.f;

	// 보스 카드 : 멈춘 뒤 흐른 시간(초, 시차 계산용), 전체 투명도(사라질 때 1 → 0), 나타나는 중인지
	float	mBossElapsed = 0.f;
	float	mBossFade = 0.f;
	bool	mBossShowing = false;

public:
	// 맵 카드 : 글자·아이콘을 넣고 들어온다 / 빠진다
	void ShowStageCard(const FText& StageName, class UTexture2D* Icon);
	void HideStageCard();

	// 보스 카드 : 이름(로고 그림 또는 글자)·칭호를 넣고 쾅 들어온다 + 화면 덧칠 색 / 사라진다
	void ShowBossCard(const FText& BossName, const FText& BossTitle, class UTexture2D* NameImage,
		class UTexture2D* TitleImage, const FLinearColor& TintColor);

	// (11/5) 배경 띠 머티리얼을 넘겨받는다. 카드가 나타나고 사라질 때 같이 흐려진다 (없으면 nullptr)
	void SetBandMaterial(class UMaterialInstanceDynamic* BandMaterial)
	{
		mBandMaterial = BandMaterial;
	}
	void HideBossCard();

protected:
	// 화면에 붙을 때 : 카드 둘 다 숨긴 상태로 시작, 클릭은 모두 통과
	virtual void NativeConstruct() override;

	// 매 프레임 : 진행 값을 목표 쪽으로 옮기고 카드의 투명도·위치·크기에 반영
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 그림이 있으면 그림, 없으면 글자 (보스 이름·부제 공용)
	void ShowImageOrText(class UImage* ImageWidget, class UTexture2D* Texture, class UTextBlock* TextWidget, const FText& Text);

	// 진행 값(0~1)을 맵 카드·보스 카드 모양으로
	void ApplyStageCard();
	void ApplyBossCard();

	// (11/5) 이름·부제 하나를 쾅 모양으로 : 진행 값(0~1)만큼 나타나고, 들어오는 동안 1.25배 → 1배.
	// 이름·부제 모두 그림이 보이면 그림, 아니면 글자 쪽에 적용한다
	void ApplySlam(class UWidget* ImageWidget, class UWidget* TextWidget, float Value);
};
