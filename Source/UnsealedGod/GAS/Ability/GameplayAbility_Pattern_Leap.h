// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Leap.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Leap
// 세바로그의 도약 내려찍기 (모든 페이즈, 3페이즈부터 2배속). 플레이어가 멀리 있을 때 거리를 단숨에 좁히는 패턴.
// 원거리에서 짤짤이만 하는 플레이어를 "응징"하는 역할이라 쿨다운이 짧다(3초).
//
// 흐름
//   몽타주 재생 (Swing_In_Air → Jump_Recovery_NonAdditive) ← 부모가 처리
//   → 이륙 노티파이(Event.Boss.LeapTakeoff)
//       ① 착지 지점 = 그 순간 기디언 위치에서 보스 쪽으로 mLandOffset 앞
//       ② 바닥에 예고 데칼 (착지 순간까지 원이 차오름)
//       ③ ApplyRootMotionJumpForce로 포물선 이동 (거리·높이·시간)
//   → 실제로 땅에 닿으면 : 착지 지점 원 안의 플레이어에게 범위 피해 + 데칼 제거
//
// 피해 타이밍을 몽타주 노티파이가 아니라 "실제 착지"로 정한 이유 :
// 몽타주 시간과 이동이 조금 어긋나거나 기둥에 걸려 일찍 떨어져도, 보스가 땅에 닿는 순간 = 피해 순간이 된다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Leap : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Leap();

protected:
	// ---------------------------------------------------------------- 수치
	// 점프 높이(cm). 아레나 벽 높이 4m보다 낮게.
	float	mJumpHeight = 300.f;

	// 공중에 있는 시간(초, "몽타주 시간" 기준). 이륙 노티파이(0.05초) ~ 착지 동작(Jump_Recovery) 시작(1.6초)까지.
	// 실제 시간 = 이 값 ÷ 재생 속도. (1.5배속이면 1.55 / 1.5 ≈ 1.03초) → 속도를 바꿔도 애니메이션과 착지가 맞는다.
	// 이 시간이 플레이어가 예고 데칼을 보고 피할 수 있는 시간이다.
	// (Jump_Start·Jump_Land는 애디티브 애니메이션이라 일반 애니메이션과 한 몽타주에 섞을 수 없어서 뺐다. 10/14)
	float	mAirTime = 1.55f;

	// 착지 후 패턴을 끝낼 때까지의 짧은 경직(초, 실제 시간). 끝나면 착지 동작을 블렌드 아웃하며 바로 다음 패턴(3연타).
	// (10/14 : 착지 동작을 끝까지 + BT Wait까지 기다리면 약 1초 "마"가 떠서 줄임)
	float	mLandDelay = 0.15f;

	// 착지 지점을 기디언 위치에서 보스 쪽으로 얼마나 앞에 둘지(cm).
	// 보스 캡슐(84 × 1.2 ≈ 101) + 기디언 캡슐(34)보다 커야 캡슐끼리 겹치지 않는다.
	float	mLandOffset = 150.f;

	// 착지 범위 반지름(cm) = 예고 데칼 반지름. 데칼 원 = 판정 원.
	float	mImpactRadius = 350.f;

	// 착지 피해 배율 (보스 공격력 × 배율). 10 × 2.5 = 25
	float	mImpactDamageScale = 2.5f;

	// ---------------------------------------------------------------- 실행 중 상태
	// 이미 착지 처리를 했는지. (착지 알림이 두 번 와도 피해는 한 번)
	bool	mLanded = false;

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 어빌리티가 끝날 때 (정상 종료든, 보스 사망 등으로 끊기든) 데칼을 정리한다.
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 몽타주의 Event.Boss.LeapTakeoff 노티파이 → 착지 지점 계산, 데칼, 포물선 이동 시작
	UFUNCTION()
	void OnTakeoffEvent(FGameplayEventData Payload);

	// 포물선 이동이 끝남(땅에 닿음) → 범위 피해 → mLandDelay 뒤 OnLandDelayFinished
	UFUNCTION()
	void OnLanded();

	// 착지 경직이 끝남 → 패턴 종료 (후딜 0이라 바로 다음 패턴)
	UFUNCTION()
	void OnLandDelayFinished();

};
