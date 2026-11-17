// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Ultimate.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Ultimate
// 세바로그의 필살기 "기둥 엄폐 궁극기" (몬스터헌터 월드 베히모스의 에클립틱 메테오 참고)
// 보스 체력 75% · 50% · 25%에서 한 번씩. 우선순위 10이라 조건이 되면 지금 패턴이 끝난 뒤 반드시 나온다.
//
// 몽타주 AM_Sevarog_Ultimate (재생 속도 1.0)
//   Default  : Swing_In_Air       — 0.05초 이륙 노티파이(Event.Boss.LeapTakeoff) → 아레나 중앙으로 포물선 도약
//   Cast     : Ultimate_Targeting — 손 뻗기
//   CastLoop : 기 모으기 (자기 자신으로 반복)
//   Slam     : Recall 0~3.15초    — 2.54초 충격 노티파이(Event.Boss.UltimateImpact). 다음 섹션 없음(끝)
//   Victory  : Victory_Emote      — 50% 필살기만. 어깨 장식 + 망치로 플레이어 가리킴 = 3페이즈(2배속) 선언
//
// 흐름
//   착지 → 바닥 전체 붉은 원(착지 후 mImpactDelay초 동안 차오름)
//   → 착지 후 (mImpactDelay - 충격까지 시간)초 : Slam으로 점프 → 착지 후 정확히 mImpactDelay초에 충격
//   → 충격 : 보스 가슴 → 기디언 몸 중심 라인 트레이스(WorldStatic만). 기둥에 막히면 생존, 아니면 즉사(무적 무시)
//   → 50%면 Slam이 끝나기 직전 Victory로 점프 / 아니면 Slam 끝 = 패턴 끝 → 후딜 2초
//
// 안전지대는 표시하지 않는다. 기둥이 엄폐물이라는 것 자체를 배우는 패턴. (베히모스도 표시 없음, 10/15 결정)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Ultimate : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Ultimate();

protected:
	// ---------------------------------------------------------------- 수치
	float	mJumpHeight = 350.f;

	// 착지 위치가 중앙에서 이 거리(cm)보다 멀면 중앙으로 순간이동 (안전장치)
	float	mCenterTolerance = 150.f;

	// (11/6) 그 순간이동을 망령 패턴처럼 보이게 : 그림자를 남기며 흐려져 사라짐(초) → 중앙에서 나타남(초)
	// (기둥 위에서 출발하면 점프가 금방 끝나 아무 전조 없이 중앙으로 툭 옮겨졌다)
	float	mRelocateVanishTime = 0.2f;
	float	mRelocateAppearTime = 0.3f;

	// 공중 시간(몽타주 시간 기준) = 이륙 노티파이(0.05) ~ Swing_In_Air 끝(1.6)
	float	mAirTime = 1.55f;

	// 착지 후 충격까지(초, 실제 시간). 바닥 원이 차오르는 시간.
	// 10/15 : 5초 → 2.5초 (중앙 도약 공중 1.55초도 예고 역할 + E 순간이동이 있어서 5초는 길었음)
	// 이 값이 Slam의 망치 드는 시간(mSlamImpactTime)보다 길면 남는 시간만큼 손 뻗기·기 모으기가 나온다.
	float	mImpactDelay = 2.5f;

	// Slam 섹션 시작 → 망치가 땅에 닿는 순간 / Slam 섹션 길이 (몽타주 시간)
	// 몽타주에서 Recall 막대를 재생 속도 1.5로 했으므로 원본 값 ÷ 1.5 : 2.54 → 1.69, 3.15 → 2.10
	float	mSlamImpactTime = 1.69f;
	float	mSlamLength = 2.10f;

	// 바닥 붉은 원의 반지름 = 아레나 전체를 덮을 만큼 (중심 → 벽 20m + 여유)
	float	mFloorWarningRadius = 2200.f;

	// 판정 시작 높이 : 보스 캡슐 중심에서 위로 (가슴 높이). 기둥 판정 높이(최소 4m)보다 낮아야 한다.
	float	mTraceHeightOffset = 60.f;

	FName	mSlamSectionName = TEXT("Slam");
	FName	mVictorySectionName = TEXT("Victory");

	// ---------------------------------------------------------------- 실행 중 상태
	// (10/22) 보스 가슴의 붉은 빛 (기둥 그림자 = 안전지대). 충격 때 번쩍이고 스스로 사라진다.
	UPROPERTY()
	TObjectPtr<class AUltimateLight>	mLight;

	// 이번 필살기가 끝나면 들어갈 페이즈 (75% → 2, 50% → 3, 25% → 4)
	int32	mTargetPhase = 0;

	// 이번에 Victory(3페이즈 선언 연출)를 할지 = mVictoryPhase로 올라가는 필살기인지
	bool	mPlayVictory = false;

	// 이 페이즈로 올라가는 필살기만 Victory_Emote를 한다. (50% = 3페이즈 : 2배속 시작, 가장 큰 변화)
	int32	mVictoryPhase = 3;

	bool	mLanded = false;

public:
	// 필살기는 거리와 상관없이, 보스에게 "필살기 대기"가 있을 때만 쓸 수 있다.
	virtual bool CanUsePattern(const AActor* Boss, const AActor* Target) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 이륙 노티파이 → 아레나 중앙으로 점프
	UFUNCTION()
	void OnTakeoffEvent(FGameplayEventData Payload);

	// 중앙 착지 → 바닥 경고 + Slam 타이머
	UFUNCTION()
	void OnLanded();

	// (11/6) 중앙에서 먼 곳에 착지했을 때 : 사라진 뒤 중앙으로 옮겨 나타남 → BeginCharge
	UFUNCTION()
	void OnRelocateVanished();

	// (11/6) 바닥 경고·빛·Slam 타이머 (원래 OnLanded의 뒷부분. 중앙에 있을 때 부른다)
	void BeginCharge();

	// 충격 시각에 맞춰 Slam 섹션으로
	UFUNCTION()
	void OnStartSlam();

	// 충격 노티파이 → 엄폐 판정
	UFUNCTION()
	void OnImpactEvent(FGameplayEventData Payload);

	// (50% 필살기) Slam이 끝나기 직전 → Victory 섹션으로
	UFUNCTION()
	void OnStartVictory();

	// 보스 → 타겟 시선이 WorldStatic(기둥·벽)에 막혔는지
	bool IsTargetCovered(class ABossBase* Boss, AActor* Target) const;

	void JumpToSection(FName SectionName);

	void DestroyFloorDecal();
};
