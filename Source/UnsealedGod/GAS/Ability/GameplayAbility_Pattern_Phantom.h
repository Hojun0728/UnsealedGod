// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayAbility_BossPattern.h"
#include "GameplayAbility_Pattern_Phantom.generated.h"

// ============================================================================
// UGameplayAbility_Pattern_Phantom
// 세바로그의 망령 순간이동 (10/21, 3페이즈부터). 멀리서 쏘기만 하는 플레이어의 옆을 기습한다.
//
// 흐름
//   ① Speedburst (제자리에서 흔들리는 동작) mBlinkTime초 = 사라지기 예고
//   ② 검게 깎여 사라짐 (사망 페이드와 같은 머티리얼 효과) → 숨김 + 충돌 끔
//   ③ 기디언 왼쪽 또는 오른쪽(랜덤) 3m 바닥에 예고 원 mAppearWarnTime초 (다 차면 등장)
//   ④ 나타나서 기디언을 보며 Swing1 (AM_Sevarog_Phantom, 판정은 몽타주의 Melee Hit Window ×2.0 Heavy)
//   ⑤ 끝나면 후딜 = 반격 틈
//
// 옆인 이유 : 기디언 바로 뒤는 카메라 자리라 보스 몸이 화면을 가린다. 예고 원이 보여야 공정하다.
// Heavy (직접 결정) : 옆에서 맞으니 넘어짐 규칙상 제자리 무릎(Stun_Start) → 기상 회피 가능. 예고 원을 꼭 봐야 하는 패턴.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayAbility_Pattern_Phantom : public UGameplayAbility_BossPattern
{
	GENERATED_BODY()

public:
	UGameplayAbility_Pattern_Phantom();

protected:
	// ---------------------------------------------------------------- 애셋
	// 휘두르기 몽타주는 부모의 mMontage (11/4 : 생성자에서 미리 불러온다)

	// 사라지기 전 흔들림 (즉석 몽타주로 재생 — 몽타주 에셋을 만들지 않아도 된다)
	UPROPERTY()
	TObjectPtr<UAnimSequence>	mBlinkAnim;

	// (10/29) 나타나는 순간 "훅" (사라질 때는 디졸브가 보이므로 소리 없이)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mAppearSound;

	// ---------------------------------------------------------------- 수치 (초·cm, 실제 시간)
	float	mBlinkTime = 0.5f;
	float	mVanishTime = 0.3f;

	// 예고 원이 다 차는 시간 = 기디언이 보고 반응할 시간
	// (10/21 : 0.6 → 0.3 (느려서 긴장감이 떨어짐), 10/22 : → 0.45 (0.3은 못 피함) — 내 판단. 나타난 뒤 판정까지 약 0.22초가 더 있다)
	float	mAppearWarnTime = 0.45f;
	float	mAppearTime = 0.2f;

	// 기디언 옆 몇 cm에 나타날지, 예고 원 반지름
	float	mAppearDistance = 300.f;
	float	mWarningRadius = 200.f;

	// ---------------------------------------------------------------- 실행 중 상태
	// 나타날 바닥 위치 (사라질 때 정해서 예고 원을 그 자리에 둔다)
	FVector	mAppearSpot = FVector::ZeroVector;

	// 휘두르기 단계인지 (흔들림 즉석 몽타주가 밀려 끝나는 것과 구분)
	bool	mSwinging = false;

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// ① 흔들림이 끝남 → ② 사라짐
	UFUNCTION()
	void OnBlinkFinished();

	// ② 사라짐이 끝남 → ③ 나타날 자리 정하고 예고 원
	UFUNCTION()
	void OnVanished();

	// ③ 예고 원이 다 참 → ④ 나타나서 휘두르기
	UFUNCTION()
	void OnAppear();

	// ④ 휘두르기 몽타주가 끝남 → 종료
	UFUNCTION()
	void OnSwingMontageEnded();

	// 기디언 옆(좌우 랜덤) 나타날 바닥 위치. 벽·기둥에 막히면 반대쪽, 그것도 막히면 막힌 곳 조금 앞.
	FVector FindAppearSpot(class ABossBase* Boss, AActor* Target)	const;

};
