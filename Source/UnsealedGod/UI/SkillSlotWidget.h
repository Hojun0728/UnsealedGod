// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "SkillSlotWidget.generated.h"

// ============================================================================
// USkillSlotWidget
// HUD의 스킬·물약 칸 하나. (10/21, 임시 HUD — 디자인은 11/2 Figma 작업 때 입힌다)
//   KeyText      : 키 글자 (E·Q·F·R·1). HUD가 정해 준다.
//   CooldownText : 남은 쿨다운 초 (쿨다운이 아니면 빈칸)
//   CountText    : 물약 개수 (×2). 스킬 칸에는 없어도 된다 (BindWidgetOptional)
//   Dim          : 칸을 덮는 어두운 반투명 이미지. 쓸 수 없을 때(쿨다운·마나 부족·물약 0개)만 보인다.
//
// 같은 칸을 5번 쓰려고 칸 하나를 위젯 블루프린트(WBP_SkillSlot)로 따로 만들고,
// HUD(WBP_HUD)에 5개를 놓는다. (수업 코드의 SlotWidget과 같은 생각)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API USkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// ---------------------------------------------------------------- 블루프린트 위젯 (이름이 정확히 같아야 한다)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	KeyText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	CooldownText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UImage>	Dim;

	// (10/21) 스킬 그림. 없어도 된다(Optional). Background와 KeyText 사이에 둔다.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage>	Icon;

	// 이 칸에 넣을 그림. WBP_HUD에서 칸(SlotE 등)을 클릭하면 디테일에 "Icon Texture"로 나온다. 칸마다 다르게 고른다.
	UPROPERTY(EditAnywhere, Category = "Slot", meta = (DisplayName = "Icon Texture"))
	TObjectPtr<UTexture2D>	mIconTexture;

	// BindWidgetOptional : 블루프린트에 없어도 컴파일 에러가 나지 않는다. (없으면 nullptr)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock>	CountText;

protected:
	// 디자이너 화면에서도 불리는 초기화. 여기서 아이콘을 넣어야 WBP_HUD 디자이너에서 바로 보인다.
	virtual void NativePreConstruct() override;

public:
	// 키 글자 (HUD가 처음에 한 번 정한다)
	void SetKeyLabel(const FString& Label);

	// 스킬 칸 갱신. Remain : 남은 쿨다운(초), bUsable : 마나가 충분한지
	void SetSkillState(float Remain, bool bUsable);

	// 물약 칸 갱신. 0개면 어둡게.
	void SetCount(int32 Count);
};
