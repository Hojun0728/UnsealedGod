// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RunPattern.generated.h"

// ============================================================================
// UBTTask_RunPattern
// 비헤이비어 트리의 "패턴 실행" 칸. (수업 코드의 BTTask_MonsterAttack과 같은 구조)
//   ExecuteTask : 보스에게 쓸 패턴을 고르게 하고 실행 → 진행 중(InProgress)
//   TickTask    : 패턴이 끝났는지 매 프레임 확인 → 끝나면 성공(Succeeded)
// 쓸 수 있는 패턴이 없으면(멀거나 쿨다운) 바로 실패(Failed)
//   → BT의 선택(Selector) 노드가 다음 칸인 "다가가기"로 넘어간다.
//
// 이 태스크는 "어떤 패턴인지" 모른다. 고르는 건 보스(ABossBase::SelectPattern),
// 실제 동작은 각 패턴 어빌리티가 한다. 그래서 패턴이 늘어나도 이 코드와 BT는 그대로다.
//
// 주의 : BT 태스크 객체는 같은 BT를 쓰는 모든 AI가 공유한다(기본 설정).
// 그래서 "지금 실행 중인 패턴" 같은 상태는 태스크가 아니라 보스가 들고 있다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UBTTask_RunPattern : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RunPattern();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		float DeltaSeconds) override;
};
