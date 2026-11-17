// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Animation/AnimInstance.h"
#include "PlayerAnimInstance.generated.h"

// ============================================================================
// UPlayerAnimInstance
// 애님 블루프린트(ABP_Gideon)의 C++ 부모 클래스다. (수업 코드와 같은 구조)
// C++에서는 "애니메이션을 고르는 데 필요한 값"(속도, 방향, 공중 여부)을 계산하고,
// 블루프린트(애님 그래프)에서는 그 값을 보고 어떤 애니메이션을 재생할지 정한다.
//
// 이렇게 나누는 이유 : 계산은 C++이 빠르고 정확하며,
// 애니메이션 연결(블렌드스페이스, 스테이트 머신)은 블루프린트가 눈으로 보며 만들기 쉽다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPlayerAnimInstance();

protected:
	// BlueprintReadOnly : 블루프린트(애님 그래프)에서 읽을 수 있게 한다. (쓰기는 C++에서만)
	// = 0.f : 변수를 선언하면서 초기값을 넣는다.

	// 이동 속도 (cm/초). 블렌드스페이스의 "속도" 축에 연결한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float	mMoveSpeed = 0.f;

	// 이동 방향 (-180 ~ 180도). 캐릭터가 바라보는 방향 기준이다.
	//   0 = 앞으로, 90 = 오른쪽, -90 = 왼쪽, 180 또는 -180 = 뒤로
	// 블렌드스페이스의 "방향" 축에 연결해서 옆걸음·뒷걸음 애니메이션을 고른다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float	mMoveDirection = 0.f;

	// 공중에 떠 있는지. 점프 스테이트로 넘어갈지 판단할 때 쓴다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool	mAir = false;

	// 이동 키를 누르고 있는지(가속 중인지). 멈출 때 자연스럽게 Idle로 넘어갈 때 쓴다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool	mAccelerating = false;

	// 사망했는지. true가 되면 ABP_Gideon이 사망 애니메이션(Death_Fwd)으로 바꾼다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool	mDeath = false;

	// 조준 위아래 각도 (-90 ~ 90도). 위를 보면 +, 아래를 보면 -.
	// 에임 오프셋(AO_Blendspace)의 Pitch 축에 연결해서 상체가 조준 방향으로 기울게 한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float	mAimPitch = 0.f;

	// ---------------------------------------------------------------- 기본 공격 (수업 코드와 같은 콤보 구조)
	// EditAnywhere : ABP_Gideon의 "클래스 디폴트"에서 몽타주와 값을 지정한다.

	// 공격 몽타주. A·B·C 세 섹션이 들어 있다.
	UPROPERTY(EditAnywhere, Category = "Attack")
	TObjectPtr<UAnimMontage>	mAttackMontage;

	// 순서대로 재생할 섹션 이름들. 몽타주의 섹션 이름과 정확히 같아야 한다.
	UPROPERTY(EditAnywhere, Category = "Attack")
	TArray<FName>		mAttackSectionNames;

	// A→B→C를 다 쏜 뒤 다시 A를 쏘기까지 쉬는 시간(초).
	// 10/19 : 0.5 → 0.15 (3타마다 끊기는 게 답답하다는 테스트 의견)
	UPROPERTY(EditAnywhere, Category = "Attack")
	float	mComboCooldown = 0.15f;

	// 공격 몽타주 전체 재생 속도. 동작과 Combo 노티파이(다음 타 입력 시점)가 같이 빨라진다.
	// 10/19 : 1.0 → 1.4 (평타가 느려 답답함. 1.5를 넘으면 동작이 경망스러워 보일 수 있음)
	// 몽타주 안의 조각 속도(C 0.9)와 곱해진다 → C는 1.4 × 0.9 = 1.26배
	UPROPERTY(EditAnywhere, Category = "Attack")
	float	mAttackPlayRate = 1.4f;

	// [테스트용] 재생한 섹션·몽타주 종료를 화면에 표시한다. (10/21 기본 꺼짐)
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool	mShowAttackDebug = false;

	// 다음에 재생할 섹션 번호. (0 = A, 1 = B, 2 = C)
	int32	mAttackSectionIndex = 0;

	// 마지막으로 재생한 섹션 번호. 몽타주가 끝났을 때 "C까지 다 쐈는지" 판단하는 데 쓴다.
	int32	mLastAttackSection = -1;

	// 다음 공격 입력을 받을 수 있는지. 섹션을 재생하면 false, Combo 노티파이에서 true가 된다.
	bool	mAttackCombo = true;

	// 이 시간(게임 시작 후 흐른 초)이 지나야 다시 A부터 쏠 수 있다. 콤보 쿨타임용.
	float	mNextComboTime = 0.f;

public:
	// 애님 인스턴스가 처음 만들어질 때 한 번 호출된다.
	virtual void NativeInitializeAnimation() override;

	// 게임이 시작될 때 한 번 호출된다. 몽타주 종료 델리게이트를 여기서 등록한다. (수업 코드와 동일)
	virtual void NativeBeginPlay() override;

	// 매 프레임 호출된다. 여기서 위의 변수들을 갱신한다. (액터의 Tick과 비슷)
	// DeltaSeconds : 지난 프레임부터 흐른 시간(초).
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	// 공격 몽타주의 다음 섹션을 재생한다. 지금 쏠 수 없으면(콤보 대기, 쿨타임) 아무것도 하지 않는다.
	// 반환값 : 실제로 재생했으면 true
	bool PlayAttack();

	// 재생 중인 몽타주를 모두 멈춘다. (사망 시 사용)
	void StopAllMontage();

public:	// Notify
	// 노티파이 함수 생성방법 : void AnimNotify_노티파이이름() 으로 함수를 만든다. (수업 코드와 동일)
	// 몽타주에 "Combo"라는 이름의 노티파이를 놓으면, 그 프레임에 이 함수가 자동으로 호출된다.
	// A·B 섹션 끝부분에만 놓는다. C에는 놓지 않는다 → C가 끝나야 다음 공격(쿨타임 후 A)이 가능하다.
	UFUNCTION()
	void AnimNotify_Combo();

protected:
	// 몽타주 재생이 끝났을 때 호출된다. (OnMontageEnded 델리게이트, AddDynamic이라 UFUNCTION 필요)
	// Interrupted : 끝까지 재생되지 않고 다른 재생으로 중간에 끊겼으면 true
	UFUNCTION()
	void MontageEnd(UAnimMontage* Montage, bool Interrupted);
};
