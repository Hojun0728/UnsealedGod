// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BossBase.h"
#include "Sevarog.generated.h"

// ============================================================================
// ASevarog
// 보스 세바로그. ABossBase를 상속받아 메시·애니메이션·수치만 채운다. (AGideon과 같은 역할)
// 앞으로 패턴 목록(3연타, 도약, 돌진, 궁극기 …)을 여기에 등록한다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API ASevarog : public ABossBase
{
	GENERATED_BODY()

public:
	ASevarog();
};
