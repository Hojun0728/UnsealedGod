// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyController.h"
#include "../Character/GameCharacter.h"

AEnemyController::AEnemyController()
{
}

void AEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 빙의한 Pawn이 우리 캐릭터인지 확인하고, 그 캐릭터가 정한 비헤이비어 트리를 꺼낸다.
	// GetBehaviorTree는 virtual : 보스는 BT_Sevarog, 미니언은 BT_Minion을 돌려준다.
	AGameCharacter* Enemy = Cast<AGameCharacter>(InPawn);

	if (!IsValid(Enemy) || !IsValid(Enemy->GetBehaviorTree()))
	{
		UE_LOG(UGDebug, Warning, TEXT("EnemyController : No Behavior Tree (%s)"), *GetNameSafe(InPawn));
		return;
	}

	// BehaviorTree를 실행한다. (수업 코드와 동일)
	// 트리에 지정된 블랙보드 에셋(BB_Boss)도 이때 자동으로 만들어진다.
	RunBehaviorTree(Enemy->GetBehaviorTree());

	// 타겟 찾기.
	// 레벨에 미리 놓인 보스는 "레벨이 열리자마자" 빙의되는데, 그때는 플레이어가 아직
	// 생성되지 않았을 수 있다. 그래서 바로 한 번 찾아보고, 못 찾으면 0.2초마다 다시 찾는다.
	// SetTimer(핸들, 객체, 함수, 간격, 반복 여부)
	GetWorldTimerManager().SetTimer(mFindTargetTimer, this,
		&AEnemyController::FindTarget, 0.2f, true);

	FindTarget();
}

void AEnemyController::FindTarget()
{
	// 0번 플레이어(싱글 게임이라 기디언)의 Pawn.
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (!IsValid(PlayerPawn) || !Blackboard)
		return;

	// 블랙보드의 "Target" 키에 플레이어를 넣는다. BT의 Move To 노드가 이 키를 읽어서 따라간다.
	// 키 이름과 타입(Object, 베이스 클래스 Actor)이 블랙보드 에셋 BB_Boss와 정확히 같아야 한다.
	Blackboard->SetValueAsObject(TEXT("Target"), PlayerPawn);

	// 찾았으니 타이머를 멈춘다.
	GetWorldTimerManager().ClearTimer(mFindTargetTimer);
}
