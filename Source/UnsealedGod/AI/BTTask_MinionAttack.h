// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MinionAttack.generated.h"

// ============================================================================
// UBTTask_MinionAttack
// 미니언 비헤이비어 트리의 "공격" 칸. (수업 코드 BTTask_MonsterAttack, 보스 BTTask_RunPattern과 같은 구조)
//   ExecuteTask : 타겟이 공격 거리 안이면 공격 시작 → 진행 중(InProgress)
//                 멀면 실패(Failed) → 선택(Selector) 노드가 다음 칸 "다가가기(Move To)"로 넘어간다
//   TickTask    : 공격 몽타주가 끝났는지 매 프레임 확인 → 끝나면 성공
// 상태(공격 중인지)는 태스크가 아니라 미니언이 들고 있다. (BT 태스크 객체는 여러 미니언이 공유)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UBTTask_MinionAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MinionAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		float DeltaSeconds) override;
};
