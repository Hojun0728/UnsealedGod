// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_RunPattern.h"
#include "../BossBase.h"
#include "../../GAS/Ability/GameplayAbility_BossPattern.h"

UBTTask_RunPattern::UBTTask_RunPattern()
{
	// BT 에디터에서 보일 노드 이름
	NodeName = TEXT("RunPattern");

	// TickTask를 호출할 수 있게 한다. (수업 코드와 동일)
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_RunPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	// AIController를 얻어온다.
	AAIController* AIController = OwnerComp.GetAIOwner();

	if (!AIController)
		return EBTNodeResult::Failed;

	ABossBase* Boss = AIController->GetPawn<ABossBase>();

	if (!IsValid(Boss) || Boss->IsDeath())
		return EBTNodeResult::Failed;

	// 블랙보드에서 Target을 얻어온다.
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!BlackboardComp)
		return EBTNodeResult::Failed;

	AActor* Target = Cast<AActor>(BlackboardComp->GetValueAsObject(TEXT("Target")));

	// 보스에게 지금 쓸 패턴을 고르게 한다. (거리·쿨다운·페이즈 조건 + 가중치 랜덤)
	TSubclassOf<UGameplayAbility_BossPattern> Pattern = Boss->SelectPattern(Target);

	// 쓸 수 있는 패턴이 없다 → 실패 → BT가 "다가가기"로 넘어간다.
	if (!Pattern)
		return EBTNodeResult::Failed;

	if (!Boss->ActivatePattern(Pattern))
		return EBTNodeResult::Failed;

	// 패턴이 끝날 때까지 이 태스크는 계속 진행 중이다. (끝났는지는 TickTask에서 확인)
	return EBTNodeResult::InProgress;
}

void UBTTask_RunPattern::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ABossBase* Boss = AIController ? AIController->GetPawn<ABossBase>() : nullptr;

	// 보스가 사라졌으면 바로 끝낸다.
	// FinishLatentTask : InProgress로 남겨 둔 태스크를 "이제 끝났다"고 BT에 알린다.
	if (!IsValid(Boss))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// 패턴(어빌리티)이 끝나고 + 그 패턴의 후딜(쉬는 시간)까지 끝나야 완료.
	// 후딜 길이는 패턴마다 다르다. (3연타 0.8±0.4초, 도약 0초) → BT에 Wait 노드가 필요 없다.
	if (!Boss->IsPatternActive() && !Boss->IsInPatternRecovery())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
