// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "GameplayEffect_ManaRegen.generated.h"

// ============================================================================
// UGameplayEffect_ManaRegen  (수업 코드에 없는 새 클래스)
// 1초마다 MP를 회복시키는 GameplayEffect다.
// 게임 시작 시 플레이어에게 한 번 적용해두면, 따로 제거하기 전까지 계속 동작한다.
// 회복량은 "GameplayEffect.MPRegen" 태그로 넘겨준다(SetByCaller).
//
// Tick에서 직접 MP를 더하는 방법도 있지만, GE로 만들면
// "마나 회복 정지 디버프" 같은 것도 GE를 제거하는 것만으로 처리할 수 있다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayEffect_ManaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGameplayEffect_ManaRegen();
};
