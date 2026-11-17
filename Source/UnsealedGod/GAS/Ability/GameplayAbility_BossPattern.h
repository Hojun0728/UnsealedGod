// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_Base.h"
#include "GameplayAbility_BossPattern.generated.h"

// ============================================================================
// UGameplayAbility_BossPattern
// 보스 패턴의 공통 부모 클래스다. (설계 : "패턴 부모 클래스 + 패턴별 자식 클래스")
// 패턴 하나 = GAS 어빌리티 하나. 쿨다운·비용은 할아버지(UGameplayAbility_Base)가 처리한다.
//
// 이 클래스가 하는 일 (모든 패턴 공통)
//   - 사용 조건 : 거리(최소·최대). virtual CanUsePattern으로 자식이 조건을 더할 수 있다.
//   - 가중치 : 여러 패턴을 쓸 수 있을 때 얼마나 자주 뽑힐지 (ABossBase::SelectPattern이 사용)
//   - 실행 : 이동 멈춤 → 타겟 쪽으로 돌기 → 몽타주 재생하고 끝날 때까지 대기 → 종료
//   - 밀기 : 몽타주의 Event.Boss.Lunge 노티파이마다 타겟 쪽으로 돌고 앞으로 짧게 밀기
// 자식 클래스(3연타, 도약, 돌진 …)는 생성자에서 몽타주·수치만 정하고,
// 특별한 동작이 필요하면 ActivateAbility 등을 override한다.
//
// UCLASS(Abstract) : 이 클래스 자체는 패턴으로 쓰지 않는다(자식만 쓴다)는 표시.
// ============================================================================
UCLASS(Abstract)
class UNSEALEDGOD_API UGameplayAbility_BossPattern : public UGameplayAbility_Base
{
	GENERATED_BODY()

public:
	UGameplayAbility_BossPattern();

protected:
	// ---------------------------------------------------------------- 자식이 생성자에서 정하는 값
	// 이 패턴의 몽타주 (보스의 DefaultSlot에서 재생)
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mMontage;

	float	mMontagePlayRate = 1.f;

	// 사용 거리 (보스 중심 → 타겟 중심, cm). 이 범위 안에 있을 때만 쓸 수 있다.
	float	mMinRange = 0.f;
	float	mMaxRange = 500.f;

	// 뽑힐 가중치. 다른 패턴보다 2배 자주 나오게 하려면 2.
	float	mWeight = 1.f;

	// 우선순위. 쓸 수 있는 후보 중 우선순위가 가장 높은 것들만 남기고 가중치 랜덤을 한다.
	// 일반 패턴 0, 필살기 10 → 필살기 조건이 되면 지금 패턴이 끝난 뒤 반드시 필살기. (페이즈 전환·그로기도 같은 방식 예정)
	int32	mPriority = 0;

	// 밀기 : 속도(cm/초) × 시간(초) = 최대 이동 거리. 속도 0이면 밀지 않는다.
	float	mLungeSpeed = 0.f;
	float	mLungeDuration = 0.25f;

	// 적정 타격 거리 (보스 중심 → 타겟 중심, cm). 무기가 가장 잘 맞는 거리.
	// 0보다 크면 밀기 거리를 "지금 거리 - 적정 거리"로 줄인다. (0 ~ 최대 이동 거리)
	//   → 멀면 쫓아 들어오고, 이미 가까우면 더 파고들지 않는다. (10/14 : 붙은 상태에서 밀면 망치가 머리 위로 지나갔음)
	// 0이면 항상 최대 거리만큼 민다.
	float	mStrikeDistance = 0.f;

	// 후딜(초) : 패턴이 끝난 뒤 다음 행동까지 쉬는 시간 = 플레이어의 반격 틈. 실제 값은 시간 ± 랜덤 편차.
	// 패턴마다 다르다 → 3연타는 0.8 ± 0.4, 도약은 0 (착지하자마자 3연타로 이어짐)
	// (10/14 : BT의 Wait 노드가 모든 패턴 뒤에 똑같이 쉬던 것을 패턴별 값으로 옮김)
	float	mRecoveryTime = 0.8f;
	float	mRecoveryRandom = 0.4f;

	// 빠른 페이즈(보스의 mFastPhase 이상, 기본 3페이즈 = 50% 필살기 이후) 값. 0·음수(기본)면 평소 값을 그대로 쓴다.
	// 예) 3연타 : 재생 속도 1.5 → 2.0, 후딜 0.8±0.4 → 0.5±0.3
	float	mFastPlayRate = 0.f;
	float	mFastRecoveryTime = -1.f;
	float	mFastRecoveryRandom = -1.f;

	// ---------------------------------------------------------------- 실행 중 값
	// 이번 실행의 재생 속도. 시작할 때 페이즈를 보고 정한다. (자식은 시간 계산에 이 값을 쓴다)
	float	mPlayRate = 1.f;

	// ---------------------------------------------------------------- 바닥 예고 데칼 (11/4 : 패턴 9개가 각자 들고 있던 것을 부모로)
	// 예고 머티리얼. 생성자에서 미리 불러 둔다. 기본 = 원(M_Decal_Warning), 돌진 계열은 자식 생성자에서 선(M_Decal_WarningLine)으로 바꾼다.
	// (11/3까지는 패턴마다 경로를 들고 있다가 쓸 때마다 LoadObject로 찾았다)
	UPROPERTY()
	TObjectPtr<UMaterialInterface>	mDecalMaterial;

	// 지금 바닥에 있는 예고 데칼. 부모 EndAbility가 자동으로 지운다.
	UPROPERTY()
	TObjectPtr<class ABossWarningDecal>	mWarningDecal;

public:
	// 지금 이 패턴을 쓸 수 있는 거리인지. (쿨다운·태그 조건은 GAS의 CanActivateAbility가 따로 판단)
	// const 함수 : 패턴의 "기본 객체(CDO)"에서 호출하므로 아무 값도 바꾸지 않는다.
	// 자식이 override해서 조건을 더할 수 있다. (예 : 궁극기는 중앙까지의 거리도 확인)
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const;

	float GetWeight()	const
	{
		return mWeight;
	}

	int32 GetPriority()	const
	{
		return mPriority;
	}

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 패턴이 끝날 때 보스에게 후딜(쉬는 시간)을 알려 준다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// "몽타주 시간"(노티파이·섹션 위치 기준)을 실제 시간으로 바꾼다 = 몽타주 시간 ÷ 이번 재생 속도
	// FMath::Max : 재생 속도가 0이 되어 0으로 나누는 실수를 막는다.
	float ToRealTime(float MontageTime)	const
	{
		return MontageTime / FMath::Max(mPlayRate, 0.01f);
	}

	// 보스를 타겟 쪽으로 바로 돌린다. (좌우 회전만)
	// YawOffset : 타겟 방향에 더할 각도(도). + = 오른쪽. 무기가 정면이 아닌 쪽으로 떨어지는 동작을 보정할 때 쓴다.
	// = 0.f : "기본 인자". 호출할 때 생략하면 0으로 들어간다. → FaceTarget()도, FaceTarget(-30.f)도 가능
	void FaceTarget(float YawOffset = 0.f);

	// 보스를 Destination(수평 위치)까지 포물선으로 뛰게 하는 태스크를 만든다. (도약·필살기 공용)
	// Height : 점프 높이(cm), AirTime : 공중 시간(초, 실제 시간)
	// 돌려받은 태스크에 OnFinish(착지)를 연결한 뒤 ReadyForActivation()을 불러야 시작된다.
	// class UAbilityTask_ApplyRootMotionJumpForce* : 전방 선언 + 포인터 반환 (헤더 include는 cpp에서)
	class UAbilityTask_ApplyRootMotionJumpForce* CreateJumpToTask(const FVector& Destination,
		float Height, float AirTime);

	// (11/4) 바닥에 원 예고를 만든다. Location : 바닥 위치, Radius : 반지름(cm, 판정과 같게), Duration : 다 차기까지 실제 시간(초)
	void SpawnWarningCircle(const FVector& Location, float Radius, float Duration);

	// (11/4) 바닥에 사각형 예고를 만든다. Rotation의 앞(X) 방향이 길이 방향 (돌진 경로). HalfLength·HalfWidth : 반 길이·반 폭(cm)
	void SpawnWarningBox(const FVector& Center, const FRotator& Rotation, float HalfLength, float HalfWidth, float Duration);

	// (11/4) 예고 데칼을 지운다. 없으면 아무것도 안 한다 → 여러 번 불러도 안전. 부모 EndAbility가 자동으로 부른다.
	void DestroyWarningDecal();

	// (11/4) 몽타주가 끝났을 때(OnMontageFinished)는 부모(UGameplayAbility_Base)로 옮겼다. (플레이어 스킬·회피와 공용)

	// 몽타주의 Event.Boss.Lunge 노티파이 → 타겟 쪽으로 돌고 앞으로 밀기
	// Payload.EventMagnitude = 돌 때 더할 각도(도). 노티파이 디테일의 Event Magnitude로 정한다.
	UFUNCTION()
	virtual void OnLungeEvent(FGameplayEventData Payload);
};
