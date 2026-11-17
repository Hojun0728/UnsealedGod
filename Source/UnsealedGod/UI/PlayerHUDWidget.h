// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

// 앞쪽 선언 : 이런 열거형이 있다고만 알린다. (정의는 PlayerCharacter.h) 열거형은 바탕 타입(uint8)까지 같게 적어야 한다.
enum class ESkillSlot : uint8;

// ============================================================================
// UPlayerHUDWidget
// 전투 화면(HUD)의 C++ 부모 클래스. (10/21, 임시 HUD — 디자인은 11/2 Figma 작업 때 입힌다)
//   왼쪽 위   : 기디언 HP·MP 바 + 숫자
//   왼쪽 아래 : 스킬 칸 [E][Q] / [F][R] / 물약 [1]
//   아래 가운데 : 보스 이름 · HP 바(75·50·25% 눈금은 블루프린트에서 이미지로) · PHASE
//
// 값은 HUD가 매 프레임(NativeTick) 기디언·보스에게서 "읽어 간다".
//   수업 코드는 HP가 바뀔 때 캐릭터가 HUD 함수를 불러 "밀어 넣었다". (캐릭터가 HUD를 알아야 함)
//   여기서는 캐릭터가 HUD를 몰라도 된다. 애님 인스턴스가 매 프레임 캐릭터 값을 읽어 가는 것과 같은 방식.
//   쿨다운은 시간이 계속 흐르는 값이라 어차피 매 프레임 확인해야 해서, HP·MP도 같은 방식으로 통일했다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// ---------------------------------------------------------------- 블루프린트 위젯 (이름이 정확히 같아야 한다)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UProgressBar>	PlayerHPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UProgressBar>	PlayerMPBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	PlayerHPText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	PlayerMPText;

	// 보스 이름·바·PHASE를 담은 묶음(캔버스·세로 박스 등 아무 패널). 보스를 찾기 전에는 숨긴다.
	// UWidget : 모든 위젯의 부모 클래스. 어떤 종류의 패널이든 연결된다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWidget>	BossPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UProgressBar>	BossHPBar;

	// (10/26) 보스 체력바 틀 이미지. 보스마다 그림을 바꿔 끼운다. (WBP의 이미지 이름을 BossBarFrame으로 + 변수 여부 체크)
	// BindWidgetOptional : WBP에 이 이름의 위젯이 없어도 컴파일 오류가 나지 않는다(그때는 nullptr → 틀을 안 바꿈).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage>	BossBarFrame;

	// (10/28) 얼음에 갇혔을 때 조준점 위의 얼음 WASD 아이콘(T_UI_FrozenPrompt). 얼어 있는 동안만 보이고, 연타할 때마다 톡 튄다.
	// UWidget : 이미지든 글자든 받는다. (WBP에 없으면 안내 없음)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UWidget>	FrozenPrompt;

	// 톡 튀기 : 연타 한 번마다 mPromptPulseScale배로 커졌다가 mPromptPulseTime초 동안 원래 크기로
	UPROPERTY(EditAnywhere, Category = "HUD")
	float	mPromptPulseScale = 1.15f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float	mPromptPulseTime = 0.12f;

	// 지금까지 본 연타 번호 (기디언의 번호가 바뀌면 새 연타), 남은 튀기 시간
	int32	mSeenMashSerial = 0;
	float	mPromptPulseRemain = 0.f;

	// (10/28) 얼었을 때 화면 가장자리 성에 (T_UI_FrozenScreen, 화면 전체). HUD보다 아래 Z 순서에 둔다.
	// 얼면 mFrozenScreenFadeIn초 동안 최대 투명도까지 → 연타 진행도만큼 최소 투명도 쪽으로 녹음 → 풀리면 mFrozenScreenFadeOut초 동안 사라짐
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UWidget>	FrozenScreen;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float	mFrozenScreenMaxOpacity = 0.85f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float	mFrozenScreenMinOpacity = 0.25f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float	mFrozenScreenFadeIn = 0.2f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float	mFrozenScreenFadeOut = 0.3f;

	// 지금 화면 성에 투명도 (목표값을 향해 매 프레임 조금씩 다가간다)
	float	mFrozenScreenOpacity = 0.f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock>	BossPhaseText;

	// 스킬·물약 칸 (WBP_SkillSlot을 5개 놓는다)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USkillSlotWidget>	SlotE;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USkillSlotWidget>	SlotQ;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USkillSlotWidget>	SlotF;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USkillSlotWidget>	SlotR;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USkillSlotWidget>	SlotPotion;

	// ---------------------------------------------------------------- 디자인 값 (위젯 블루프린트의 클래스 디폴트에서 바꾼다)
	// (10/26) 보스 이름은 보스(ABossBase::mBossName)가 갖고, HUD는 보스를 찾을 때 읽어 온다.

	// (10/21) 등장 연출이 끝날 때 서서히 나타나기 : 전체 투명도 0 → 1
	float	mFadeInTime = 0.f;
	float	mFadeInElapsed = 0.f;
	bool	mFadingIn = false;

	// 찾아 둔 보스. TWeakObjectPtr : 보스가 사라져도 위험하지 않은 포인터 (사라지면 자동으로 무효가 된다)
	TWeakObjectPtr<class ABossBase>	mBoss;

public:
	// (10/21) 등장 연출 동안 숨겨 두기 / Time초 동안 서서히 나타나기
	void HideForIntro();
	void FadeIn(float Time);

protected:
	// 화면에 붙을 때 한 번 (키 글자·보스 이름, 마우스 클릭 통과)
	virtual void NativeConstruct() override;

	// 매 프레임 : 기디언·보스 값을 읽어 바·글자·칸을 갱신한다.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void UpdatePlayer(class APlayerCharacter* Player);
	void UpdateBoss();

	// 스킬 칸 하나 갱신 (플레이어에게 쿨다운·마나를 물어본다)
	void UpdateSkillSlot(class USkillSlotWidget* SkillSlot, class APlayerCharacter* Player, ESkillSlot SlotType);
};
