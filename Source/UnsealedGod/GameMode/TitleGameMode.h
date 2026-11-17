// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/GameModeBase.h"
#include "TitleGameMode.generated.h"

// ============================================================================
// ATitleGameMode (11/2)
// 타이틀 레벨(L_Title)의 게임 규칙. 수업 코드의 시작 화면(Start 레벨)과 같은 역할이다.
//   - 조종할 캐릭터를 만들지 않는다. (DefaultPawnClass = nullptr)
//   - 컨트롤러는 ATitlePlayerController : 마우스 커서와 타이틀 위젯(WBP_Title)을 맡는다.
//
// 화면에 보이는 장면은 레벨에 놓은 카메라 액터가 정한다.
// (카메라 디테일의 "자동 플레이어 활성화 = Player 0" → 게임이 시작되면 이 카메라로 본다)
//
// 프로젝트 기본 게임 모드는 AArenaGameMode라서, L_Title의 월드 세팅 → 게임 모드 오버라이드에서 이 클래스를 고른다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API ATitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATitleGameMode();
};
