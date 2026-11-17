// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Bind.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Bind
// 세바로그의 속박 (10/21, 2페이즈부터). 파훼법은 오직 E(회피 무적).
//
// 흐름 (단계마다 몽타주가 다르다 → 부모의 "몽타주 하나 재생" 흐름을 쓰지 않고 직접 진행. 지침 패턴과 같은 방식)
//   ① 시전 : Subjugation (0.8배속 고정) + 기디언 발밑에 붉은 원이 따라다니며 차오름
//   ② 판정 : 손을 내리치는 노티파이(Event.Boss.BindCast) = 원이 다 찬 순간
//            기디언이 무적(E 회피 중·넘어짐)이면 실패 → Subjugation 끝까지 + 후딜 1초 (반격 틈)
//   ③ 성공 : 기디언에게 Event.Player.Bound → 기디언 속박 어빌리티 (공중에 묶임, 입력 불가)
//   ④ 보스 사라짐 → 기디언 "등 뒤 왼편 45°" 3m에 나타남 → Ultimate_Swing (골프 스윙)
//   ⑤ 치는 노티파이(Event.Boss.BindImpact) : 확정 피해 (막을 수 없음 + Heavy)
//      → 기디언 넘어짐 어빌리티가 이어받는다. 등 뒤에서 맞았으므로 자동으로 Knock_Fwd(앞으로 날아감)
//   ⑥ 도발 (Emote_Pointing 두 조각) : 이 동안 공격하지 않는다 → 기디언이 일어나 정비할 시간
//
// 등 뒤 "왼편"인 이유 : 카메라가 기디언 오른쪽 어깨 너머에 있어서, 바로 뒤에 나타나면 카메라가 보스 몸 안에 들어간다.
// 페이즈 배속을 받지 않는다 : 2배속이면 예고(원이 차는 시간)가 0.55초로 너무 짧아 E 타이밍을 잡을 수 없다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Bind : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Bind();

protected:
	// ---------------------------------------------------------------- 몽타주 (처음 쓸 때 경로로 불러온다)
	// 시전(Subjugation, 손 내리치는 순간 Event.Boss.BindCast)은 부모의 mMontage
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mSwingMontage;

	UPROPERTY()
	TObjectPtr<UAnimMontage>	mTauntMontage;

	// ---------------------------------------------------------------- 수치
	// 손을 내리치는 순간(초, Subjugation 애니메이션 기준). 예고 원이 이 시간에 맞춰 다 찬다.
	float	mCastTime = 0.88f;

	// 기디언 발밑 예고 원 반지름(cm)
	float	mWarningRadius = 150.f;

	// 사라지는 시간 / 사라진 채 기다리는 시간 / 나타나는 시간 (초)
	float	mVanishTime = 0.25f;
	float	mHiddenTime = 0.1f;
	float	mAppearTime = 0.25f;

	// 기디언에게서 얼마나 떨어져 나타날지(cm), 등 뒤에서 왼쪽으로 몇 도 비틀지
	float	mAppearDistance = 300.f;
	float	mAppearAngle = 45.f;

	// 마무리 일격 피해 배율 (보스 공격력 × 배율). 2페이즈 12 × 2.5 = 30, 4페이즈 16 × 2.5 = 40
	// (10/22 : 3.5 → 2.5. 4페이즈 56은 HP의 절반 이상이라 다른 피해와 이어지면 바로 죽음 — 내 판단)
	float	mImpactDamageScale = 2.5f;

	// 판정 구간(초). 손을 내리치는 순간을 가운데로 앞뒤 절반씩, 이 안에 한 번이라도 무적이면 회피 성공.
	// E 무적 구간(약 0.19초)과 합쳐 E를 누를 수 있는 폭 = 0.14 + 0.19 = 0.33초.
	// (10/22 : 처음에는 내리치는 "한 순간"만 검사 → 누를 수 있는 폭 0.19초로 너무 빡빡함 — 내 판단)
	float	mJudgeWindow = 0.14f;

	// 후딜 : 회피당했을 때 (반격 틈) / 성공했을 때 (도발이 끝난 뒤)
	float	mFailRecoveryTime = 1.f;
	float	mSuccessRecoveryTime = 0.3f;

	// 판정 순간 기디언 위치에서 터지는 이펙트 / 일격이 맞는 순간 이펙트
	UPROPERTY()
	TObjectPtr<UParticleSystem>	mBindParticle;

	// (10/29) 소리 : 묶이는 순간 (기디언 몸에서) / 등 뒤에 나타나는 순간 (망령 순간이동과 같은 소리)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mBindSound;

	UPROPERTY()
	TObjectPtr<class USoundBase>	mAppearSound;

	// ---------------------------------------------------------------- 실행 중 상태
	// 속박에 성공했는지 (시전 몽타주가 끝나거나 끊길 때 어떻게 할지 결정)
	bool	mBindSucceeded = false;

	// 판정용 : 기디언이 마지막으로 무적이었던 시각(게임 시간). 무적 태그가 붙거나 떨어질 때마다 갱신.
	float	mLastInvincibleTime = -100.f;

	// 무적 태그 감시 등록 (끝날 때 해제)
	TWeakObjectPtr<UAbilitySystemComponent>	mTargetASC;
	FDelegateHandle	mInvincibleTagHandle;

	// 지금 몇 번째 단계인지. 몽타주가 바뀔 때 앞 몽타주의 "끊김" 알림을 무시하기 위해.
	enum class EBindStage : uint8
	{
		Cast,		// ① 시전 중
		Teleport,	// ③④ 사라짐 ~ 나타남
		Swing,		// ④⑤ 일격
		Taunt		// ⑥ 도발
	};

	EBindStage	mStage = EBindStage::Cast;

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 끝날 때 정리 : 데칼 제거, 사라진 상태면 다시 보이게
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 단계 몽타주 재생 태스크를 만든다. 끝·끊김 알림은 부른 쪽이 단계 함수에 연결한다.
	// (각 단계 함수는 "지금도 그 단계인지" 확인해서, 다음 몽타주에 밀려 끊긴 알림은 무시한다)
	class UAbilityTask_PlayMontageAndWait* PlayStageMontage(UAnimMontage* Montage, float Rate);

	// ② 손을 내리침 → 판정
	UFUNCTION()
	void OnBindCastEvent(FGameplayEventData Payload);

	// 판정 구간 뒤쪽 절반이 끝남 → 마지막 판정
	UFUNCTION()
	void OnJudgeLate();

	// 지금 회피 성공인지 : 지금 무적이거나, 판정 구간 절반 안에 무적이었으면 true
	bool IsDodgingNow()	const;

	// 판정 결과 처리 : 성공(회피) → 후딜 / 실패 → 묶고 사라짐
	void ResolveBind(bool bDodged);

	// 기디언 무적 태그가 붙거나 떨어짐 (ASC 태그 이벤트)
	void OnTargetInvincibleChanged(const FGameplayTag Tag, int32 NewCount);

	void StopWatchingInvincible();

	// ① 시전 몽타주가 끝남 (실패했으면 여기서 종료)
	UFUNCTION()
	void OnCastMontageEnded();

	// ④ 사라짐이 끝남 → 등 뒤에 나타나 일격 시작
	UFUNCTION()
	void OnVanished();

	// ⑤ 일격이 맞는 순간
	UFUNCTION()
	void OnBindImpactEvent(FGameplayEventData Payload);

	// ⑤ 일격 몽타주가 끝남 → 도발
	UFUNCTION()
	void OnSwingMontageEnded();

	// ⑥ 도발이 끝남 → 종료
	UFUNCTION()
	void OnTauntMontageEnded();

};
