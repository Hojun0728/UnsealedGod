// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_MinionAttack.h"
#include "../Minion/Minion.h"

UBTTask_MinionAttack::UBTTask_MinionAttack()
{
	// BT 에디터에서 보일 노드 이름
	NodeName = TEXT("MinionAttack");

	// TickTask를 호출할 수 있게 한다.
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MinionAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AMinion* Minion = AIController ? AIController->GetPawn<AMinion>() : nullptr;

	if (!IsValid(Minion) || Minion->IsDeath())
		return EBTNodeResult::Failed;

	AActor* Target = Minion->GetTarget();

	if (!IsValid(Target))
		return EBTNodeResult::Failed;

	// 공격 거리 밖이면 실패 → 다가가기로
	float	Distance = FVector::Dist2D(Minion->GetActorLocation(), Target->GetActorLocation());

	if (Distance > Minion->GetAttackRange())
		return EBTNodeResult::Failed;

	// 이동을 멈추고 공격
	AIController->StopMovement();

	if (!Minion->Attack(Target))
		return EBTNodeResult::Failed;

	return EBTNodeResult::InProgress;
}

void UBTTask_MinionAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	AMinion* Minion = AIController ? AIController->GetPawn<AMinion>() : nullptr;

	// 미니언이 없어졌거나 공격 몽타주가 끝났으면 완료
	if (!IsValid(Minion) || !Minion->IsAttacking())
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}
