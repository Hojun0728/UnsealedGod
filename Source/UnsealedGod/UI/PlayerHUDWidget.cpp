// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerHUDWidget.h"
#include "SkillSlotWidget.h"
#include "../Player/PlayerCharacter.h"
#include "../Boss/BossBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
// TActorIterator : 레벨에 있는 특정 클래스의 액터를 하나씩 돌아보는 반복자
#include "EngineUtils.h"

void UPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// HUD는 보기만 하는 화면이다. 마우스 클릭이 HUD에 걸려 게임 입력을 막지 않게 통과시킨다.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 칸마다 키 글자 (입력 키와 같게)
	if (IsValid(SlotE))		SlotE->SetKeyLabel(TEXT("E"));
	if (IsValid(SlotQ))		SlotQ->SetKeyLabel(TEXT("Q"));
	if (IsValid(SlotF))		SlotF->SetKeyLabel(TEXT("F"));
	if (IsValid(SlotR))		SlotR->SetKeyLabel(TEXT("R"));
	if (IsValid(SlotPotion))	SlotPotion->SetKeyLabel(TEXT("1"));

	// 보스 이름은 보스를 찾을 때 넣는다. (UpdateBoss, 10/26)

	// 보스를 찾기 전에는 보스 묶음을 숨긴다. (Collapsed : 안 보이고 자리도 차지하지 않음)
	if (IsValid(BossPanel))
		BossPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// GetOwningPlayerPawn : 이 HUD를 가진 플레이어 컨트롤러가 조종하는 Pawn (= 기디언)
	APlayerCharacter* Player = Cast<APlayerCharacter>(GetOwningPlayerPawn());

	if (IsValid(Player))
		UpdatePlayer(Player);

	UpdateBoss();

	// 화면 성에 (10/28) : 목표 투명도 = 얼었으면 "최대 → 최소를 진행도만큼", 아니면 0
	if (IsValid(FrozenScreen) && IsValid(Player))
	{
		bool	bFrozen = Player->IsFrozen();
		float	Target = bFrozen ?
			FMath::Lerp(mFrozenScreenMaxOpacity, mFrozenScreenMinOpacity, Player->GetFrozenProgress()) : 0.f;

		// 나타날 때·녹을 때는 빠르게(FadeIn), 사라질 때는 FadeOut 속도로. 속도 = 최대 투명도 ÷ 시간
		float	Time = bFrozen ? mFrozenScreenFadeIn : mFrozenScreenFadeOut;
		float	Speed = Time > 0.f ? mFrozenScreenMaxOpacity / Time : 1000.f;

		// FInterpConstantTo(지금, 목표, 프레임 시간, 속도) : 목표를 향해 일정한 속도로 다가간다 (넘어가지 않음)
		mFrozenScreenOpacity = FMath::FInterpConstantTo(mFrozenScreenOpacity, Target, InDeltaTime, Speed);

		FrozenScreen->SetRenderOpacity(mFrozenScreenOpacity);

		ESlateVisibility	Wanted = mFrozenScreenOpacity > 0.001f ?
			ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

		if (FrozenScreen->GetVisibility() != Wanted)
			FrozenScreen->SetVisibility(Wanted);
	}

	// 얼음 아이콘 톡 튀기 (10/28) : 남은 시간 비율만큼 크게 → 0이 되면 원래 크기
	// SetRenderScale : 배치(자리)는 그대로 두고 그려지는 크기만 바꾼다. (가운데 기준 = 렌더 피벗 0.5, 0.5 기본값)
	if (IsValid(FrozenPrompt) && mPromptPulseRemain > 0.f)
	{
		mPromptPulseRemain = FMath::Max(mPromptPulseRemain - InDeltaTime, 0.f);

		float	Alpha = mPromptPulseTime > 0.f ? mPromptPulseRemain / mPromptPulseTime : 0.f;

		FrozenPrompt->SetRenderScale(FVector2D(FMath::Lerp(1.f, mPromptPulseScale, Alpha)));
	}

	// 서서히 나타나기 (10/21)
	if (mFadingIn)
	{
		mFadeInElapsed += InDeltaTime;

		float	Alpha = mFadeInTime > 0.f ? FMath::Clamp(mFadeInElapsed / mFadeInTime, 0.f, 1.f) : 1.f;

		SetRenderOpacity(Alpha);

		if (Alpha >= 1.f)
			mFadingIn = false;
	}
}

void UPlayerHUDWidget::UpdatePlayer(APlayerCharacter* Player)
{
	float	HP = Player->GetHP();
	float	HPMax = Player->GetHPMax();
	float	MP = Player->GetMP();
	float	MPMax = Player->GetMPMax();

	// SetPercent : 0 ~ 1. 최대치가 0이면 나누지 않는다.
	if (IsValid(PlayerHPBar))
		PlayerHPBar->SetPercent(HPMax > 0.f ? HP / HPMax : 0.f);

	if (IsValid(PlayerMPBar))
		PlayerMPBar->SetPercent(MPMax > 0.f ? MP / MPMax : 0.f);

	// "75 / 100". %.0f : 소수점 없이
	if (IsValid(PlayerHPText))
		PlayerHPText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), HP, HPMax)));

	if (IsValid(PlayerMPText))
		PlayerMPText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), MP, MPMax)));

	UpdateSkillSlot(SlotE, Player, ESkillSlot::Dodge);
	UpdateSkillSlot(SlotQ, Player, ESkillSlot::Q);
	UpdateSkillSlot(SlotF, Player, ESkillSlot::F);
	UpdateSkillSlot(SlotR, Player, ESkillSlot::R);

	if (IsValid(SlotPotion))
		SlotPotion->SetCount(Player->GetPotionCount());

	// 얼음 안내 : 얼어 있는 동안만 보인다. (Collapsed : 안 보이고 자리도 차지하지 않음)
	if (IsValid(FrozenPrompt))
	{
		ESlateVisibility	Wanted = Player->IsFrozen() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

		if (FrozenPrompt->GetVisibility() != Wanted)
			FrozenPrompt->SetVisibility(Wanted);

		// 기디언의 연타 번호가 바뀌었으면 = 방금 한 번 눌렀다 → 톡 튀기 시작 (실제 크기 변화는 NativeTick)
		int32	Serial = Player->GetFrozenMashSerial();

		if (Serial != mSeenMashSerial)
		{
			mSeenMashSerial = Serial;
			mPromptPulseRemain = mPromptPulseTime;
		}
	}
}

void UPlayerHUDWidget::UpdateSkillSlot(USkillSlotWidget* SkillSlot, APlayerCharacter* Player, ESkillSlot SlotType)
{
	if (!IsValid(SkillSlot))
		return;

	float	Remain = 0.f;
	float	Duration = 0.f;
	bool	bEnoughMP = false;

	// 값 3개를 참조(&)로 돌려받는다.
	if (Player->GetSkillSlotState(SlotType, Remain, Duration, bEnoughMP))
		SkillSlot->SetSkillState(Remain, bEnoughMP);
}

void UPlayerHUDWidget::UpdateBoss()
{
	// 아직 보스를 못 찾았으면 레벨에서 찾는다. (보스가 하나라 처음 찾은 것)
	if (!mBoss.IsValid())
	{
		for (TActorIterator<ABossBase> It(GetWorld()); It; ++It)
		{
			mBoss = *It;
			break;
		}

		if (!mBoss.IsValid())
			return;

		if (IsValid(BossPanel))
			BossPanel->SetVisibility(ESlateVisibility::HitTestInvisible);

		// 보스마다 다른 이름 (세바로그 / 오로라). 한 번 찾으면 바뀌지 않으므로 여기서 한 번만 넣는다.
		if (IsValid(BossNameText))
			BossNameText->SetText(mBoss->GetBossName());

		// 보스마다 다른 틀 그림. 보스가 그림을 안 정했으면 WBP에 넣어 둔 그림 그대로.
		// SetBrushFromTexture(그림, 크기 맞추기) : false = WBP에서 정한 이미지 크기(1910×280)를 그대로 쓴다.
		UTexture2D* Frame = mBoss->GetHPBarFrame();

		if (IsValid(BossBarFrame) && IsValid(Frame))
			BossBarFrame->SetBrushFromTexture(Frame, false);
	}

	// TWeakObjectPtr::Get : 아직 살아 있으면 주소, 사라졌으면 nullptr
	ABossBase* Boss = mBoss.Get();
	float	HPMax = Boss->GetHPMax();

	if (IsValid(BossHPBar))
		BossHPBar->SetPercent(HPMax > 0.f ? Boss->GetHP() / HPMax : 0.f);

	if (IsValid(BossPhaseText))
		BossPhaseText->SetText(FText::FromString(FString::Printf(TEXT("PHASE %d"), Boss->GetPhase())));
}

void UPlayerHUDWidget::HideForIntro()
{
	// SetRenderOpacity : 위젯 전체(자식 포함)의 투명도. 0이면 안 보이지만 자리와 값 갱신은 그대로다.
	SetRenderOpacity(0.f);
	mFadingIn = false;
}

void UPlayerHUDWidget::FadeIn(float Time)
{
	mFadeInTime = Time;
	mFadeInElapsed = 0.f;
	mFadingIn = true;
}
