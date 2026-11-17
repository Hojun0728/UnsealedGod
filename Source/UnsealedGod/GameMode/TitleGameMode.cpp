// Fill out your copyright notice in the Description page of Project Settings.


#include "TitleGameMode.h"
#include "../Player/TitlePlayerController.h"

ATitleGameMode::ATitleGameMode()
{
	// 조종할 캐릭터가 없다. nullptr이면 PlayerStart가 있어도 아무것도 만들지 않는다.
	DefaultPawnClass = nullptr;

	// 이 레벨에서 쓸 플레이어 컨트롤러. (게임이 시작될 때 이 클래스로 컨트롤러가 만들어진다)
	PlayerControllerClass = ATitlePlayerController::StaticClass();
}
