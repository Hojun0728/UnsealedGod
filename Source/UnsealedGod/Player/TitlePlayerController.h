// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/PlayerController.h"
#include "TitlePlayerController.generated.h"

// ============================================================================
// ATitlePlayerController (11/2)
// 타이틀 화면의 플레이어 컨트롤러. (수업 코드 AStartPlayerController와 같은 구조)
//   - 마우스 커서를 보이고, 입력을 UI 전용으로 바꾼다. (버튼만 누를 수 있게)
//   - 타이틀 위젯(WBP_Title)을 만들어 화면에 붙인다.
//
// 수업 코드는 생성자에서 ConstructorHelpers로 위젯 클래스를 찾지만,
// 이 프로젝트의 HUD·결과 화면처럼 BeginPlay에서 경로로 불러온다.
// (위젯 블루프린트를 빌드 뒤에 만들어도 에디터 재시작 없이 찾는다)
//
// (11/3) 시작 연출 : START를 누르면 바로 넘어가지 않고
//   로고·버튼이 사라짐 → 세바로그가 손을 뻗어 움켜쥠(Subjugation) + 대사 → 움켜쥐는 순간 화면이 검게 → 첫 스테이지
// ============================================================================
UCLASS()
class UNSEALEDGOD_API ATitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATitlePlayerController();

protected:
	FString	mTitleWidgetPath = TEXT("/Game/UnsealedGod/UI/WBP_Title.WBP_Title_C");

	// 만든 위젯을 들고 있는다. (UPROPERTY : 가비지 컬렉터가 지우지 않게)
	UPROPERTY()
	TObjectPtr<class UTitleWidget>	mTitleWidget;

	// ---------------------------------------------------------------- 시작 연출 (11/3)
	// 세바로그 동작 : 손을 뻗어 움켜쥔다 (속박 패턴과 같은 동작 → 게임 속 잡기의 예고편)
	UPROPERTY()
	TObjectPtr<class UAnimSequence>	mStartAnim;

	// 움켜쥘 때 대사
	UPROPERTY()
	TObjectPtr<class USoundBase>	mStartVoice;

	// 로고·버튼이 사라지는 시간(초)
	float	mWidgetHideTime = 0.3f;

	// 동작에서 손을 움켜쥐는 순간(초, 재생 속도 1 기준). Subjugation 29프레임 = 1.0초
	float	mGrabTime = 1.f;

	// (11/3) 대사가 잘리지 않게 동작을 대사 길이에 맞춰 늦춘다 : 재생 속도 = mGrabTime / 대사 길이 (이 범위로 제한)
	float	mMinPlayRate = 0.4f;
	float	mMaxPlayRate = 1.f;

	// 완전히 검어지는 데 걸리는 시간(초). 움켜쥐는 순간에 끝나도록 그만큼 앞에서 시작한다.
	float	mFadeDuration = 0.5f;

	// 연출이 끝나면 열 레벨
	FName	mNextLevelName;

	// 이미 시작했는지 (두 번 눌려도 한 번만)
	bool	mStarting = false;

	FTimerHandle	mFadeTimer;
	FTimerHandle	mOpenLevelTimer;

public:
	// START 버튼이 부른다. 시작 연출을 재생하고, 끝나면 LevelName 레벨을 연다.
	void StartGame(FName LevelName);

protected:
	virtual void BeginPlay() override;

	// 화면을 검게 (타이머로 부른다)
	void StartFadeOut();

	// 다음 레벨 열기 (타이머로 부른다)
	void OpenNextLevel();
};
