// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/Actor.h"
#include "StagePortal.generated.h"

// ============================================================================
// AStagePortal (10/26)
// 다음 스테이지로 가는 포탈. 레벨에 직접 놓아 둔다. (스테이지 1 = L_Arena 동쪽 벽 앞)
//
//   처음 : 숨김 + 충돌 없음 (보스와 싸우는 동안에는 없는 것처럼)
//   보스를 잡으면 : 게임 모드가 승리 문구가 사라진 뒤 Open()을 부른다 → 이펙트·소리와 함께 나타남
//   기디언이 닿으면 : 화면이 검게 변하고 mNextLevelName 레벨을 연다
//
// 게임 모드는 "레벨에 이 포탈이 있는지"로 스테이지 클리어(포탈) / 최종 승리(결과 화면)를 가른다.
// → 마지막 스테이지 레벨에는 포탈을 놓지 않으면 된다. 코드에 "몇 스테이지인지"를 적지 않는다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AStagePortal : public AActor
{
	GENERATED_BODY()

public:
	AStagePortal();

protected:
	// ---------------------------------------------------------------- 컴포넌트
	// 들어가는 범위. 루트라서 이 상자의 위치 = 포탈의 위치다.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<class UBoxComponent>	mTrigger;

	// 포탈 모습 (기디언 팩의 포탈 이펙트). 열릴 때 켠다.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UParticleSystemComponent>	mPortalFX;

	// ---------------------------------------------------------------- 디테일에서 정하는 값
	// 들어가면 열 레벨 이름. (콘텐츠 브라우저의 레벨 에셋 이름 그대로)
	UPROPERTY(EditAnywhere, Category = "Portal")
	FName	mNextLevelName = TEXT("L_Stage2");

	// 열릴 때 나는 소리. 비워 두면 소리 없음.
	UPROPERTY(EditAnywhere, Category = "Portal")
	TObjectPtr<class USoundBase>	mOpenSound;

	// 들어갈 때 화면이 검게 변하는 시간(초). 다 검어지면 레벨을 연다.
	UPROPERTY(EditAnywhere, Category = "Portal")
	float	mEnterFadeTime = 1.f;

	// (10/28) 포탈 모습을 레벨에 놓은 다른 액터로 (Fab PortalFX의 BP_PRT_Portal_Violet 등).
	// 지정하면 : 처음엔 숨겨 두고, 열릴 때 보이게 한 뒤 그 블루프린트의 "Open Portal" 함수를 불러 커지며 열리게 한다.
	// 비워 두면 위의 기디언 팩 파티클(mPortalFX)을 쓴다. (레벨 디테일에서 스포이트로 고른다)
	UPROPERTY(EditAnywhere, Category = "Portal")
	TObjectPtr<AActor>	mVisualActor;

	// 그 블루프린트에서 부를 함수 이름 (PortalFX = OpenPortal)
	UPROPERTY(EditAnywhere, Category = "Portal")
	FName	mVisualOpenFunction = TEXT("OpenPortal");

	// 이미 열렸는지 / 이미 들어갔는지 (두 번 처리되지 않게)
	bool	mOpened = false;
	bool	mEntering = false;

	FTimerHandle	mTravelTimer;

public:
	// 포탈을 연다 : 보이기 + 이펙트 켜기 + 충돌 켜기 + 소리 (게임 모드가 부른다)
	void Open();

protected:
	virtual void BeginPlay() override;

	// 무언가가 상자에 들어왔을 때. (다이나믹 델리게이트에 등록하므로 UFUNCTION 필요)
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 화면이 다 검어진 뒤 다음 레벨을 연다. (타이머로 부른다)
	void Travel();
};
