// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void USkillSlotWidget::SetKeyLabel(const FString& Label)
{
	if (IsValid(KeyText))
		KeyText->SetText(FText::FromString(Label));

	// 스킬 칸은 개수 글자를 쓰지 않는다. (물약 칸은 SetCount가 채운다)
	if (IsValid(CountText))
		CountText->SetText(FText::GetEmpty());
}

void USkillSlotWidget::SetSkillState(float Remain, bool bUsable)
{
	bool	bCooling = Remain > 0.f;

	// 남은 초는 올림해서 정수로. (0.3초 남았으면 "1" — 0이 보이는 순간 쓸 수 있게)
	// FMath::CeilToInt : 올림. 2.1 → 3
	if (IsValid(CooldownText))
	{
		CooldownText->SetText(bCooling ? FText::AsNumber(FMath::CeilToInt(Remain)) : FText::GetEmpty());
	}

	// 쿨다운 중이거나 마나가 모자라면 어둡게 덮는다.
	// ESlateVisibility::HitTestInvisible : 보이지만 마우스 클릭은 통과 / Collapsed : 안 보이고 자리도 차지하지 않음
	if (IsValid(Dim))
	{
		Dim->SetVisibility(bCooling || !bUsable ? ESlateVisibility::HitTestInvisible :
			ESlateVisibility::Collapsed);
	}
}

void USkillSlotWidget::SetCount(int32 Count)
{
	if (IsValid(CountText))
		CountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));

	if (IsValid(CooldownText))
		CooldownText->SetText(FText::GetEmpty());

	if (IsValid(Dim))
		Dim->SetVisibility(Count > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void USkillSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 칸마다 고른 그림을 이미지 위젯에 넣는다. 고르지 않았으면 이미지를 숨긴다.
	// SetBrushFromTexture(텍스처) : 이미지 위젯의 그림을 바꾼다.
	if (IsValid(Icon))
	{
		if (IsValid(mIconTexture))
		{
			Icon->SetBrushFromTexture(mIconTexture);
			Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
