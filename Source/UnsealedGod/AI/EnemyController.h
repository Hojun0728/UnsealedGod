// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "EnemyController.generated.h"

// ============================================================================
// AEnemyController
// 적(보스·미니언)의 "두뇌". 빙의(Possess)한 캐릭터의 비헤이비어 트리를 실행한다.
// (수업 코드의 AMonsterController와 같은 역할)
// 10/15 : 원래 ABossController였는데, 미니언도 똑같이 "BT 실행 + Target = 플레이어"라서 일반화했다.
//         어떤 BT를 쓸지는 캐릭터가 정한다 (AGameCharacter::GetBehaviorTree, virtual).
//
// 수업 코드와 다른 점 : 시야 감지(AIPerception)를 쓰지 않는다.
// 아레나에 플레이어가 한 명뿐이고 적은 항상 플레이어를 알고 있으므로,
// 블랙보드의 Target에 플레이어를 바로 넣는다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyController();

protected:
	// 타겟(플레이어)을 찾을 때까지 반복하는 타이머.
	// FTimerHandle : 타이머를 나중에 멈추기 위해 들고 있는 "번호표".
	FTimerHandle	mFindTargetTimer;

protected:
	// 컨트롤러가 Pawn(보스·미니언)에 빙의했을 때 엔진이 호출한다.
	virtual void OnPossess(APawn* InPawn) override;

	// 블랙보드의 Target에 플레이어를 넣는다. 찾으면 타이머를 멈춘다.
	void FindTarget();
};
