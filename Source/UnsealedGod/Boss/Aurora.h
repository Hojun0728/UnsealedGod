// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BossBase.h"
#include "Aurora.generated.h"

// ============================================================================
// AAurora
// 스테이지 1 보스 오로라 (봉인의 수호자, 얼음 여왕). (10/26)
// ASevarog와 같은 모양 : ABossBase를 상속받아 메시·동작·수치·패턴 목록만 채운다.
// 패턴 선택·후딜·페이즈·등장·패배 흐름은 부모 그대로 쓴다. BT도 세바로그 것(BT_Sevarog)을 그대로 쓴다.
//   → "보스를 하나 더 붙일 때 새로 짜는 건 패턴 자식 클래스뿐"
// 세바로그와 다른 것 : 2페이즈(필살기 50% 한 번), 페이즈 성장 없음, 미니언 없음, 사망 시 얼음으로 부서짐
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AAurora : public ABossBase
{
	GENERATED_BODY()

public:
	AAurora();

protected:
	// ---------------------------------------------------------------- 사망 : 얼어붙었다가 부서짐
	// 오로라 머티리얼에는 세바로그의 FadeOut 파라미터가 없어서 "검게 깎여 사라지기"를 쓸 수 없다.
	// 대신 쓰러진 몸이 얼음에 덮였다가(mFreezeParticle) mShatterDelay초 뒤 얼음 파편으로 터지며(mShatterParticle) 사라진다.
	UPROPERTY(EditAnywhere, Category = "Aurora|Death")
	TObjectPtr<UParticleSystem>	mFreezeParticle;

	UPROPERTY(EditAnywhere, Category = "Aurora|Death")
	TObjectPtr<UParticleSystem>	mShatterParticle;

	// 부서질 때 소리. 비워 두면 소리 없음.
	UPROPERTY(EditAnywhere, Category = "Aurora|Death")
	TObjectPtr<class USoundBase>	mShatterSound;

	// 얼어붙은 채로 있는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Aurora|Death")
	float	mShatterDelay = 1.f;

	FTimerHandle	mShatterTimer;

	// ---------------------------------------------------------------- 2페이즈 얼음 여제 (10/28)
	// 얼음 여제 스킨 메시를 처음부터 붙여 숨겨 두고, 원래 메시의 자세를 그대로 따라 하게 한다(리더 포즈).
	// 바꾸는 순간 원래 메시는 숨기고 이 메시를 보이게만 한다 → 애니메이션·판정·소켓은 원래 메시가 계속 담당해서 끊기지 않는다.
	// (메시를 SetSkeletalMesh로 갈아 끼우면 애니메이션이 처음부터 다시 시작돼 재생 중인 몽타주가 끊긴다)
	UPROPERTY(VisibleAnywhere, Category = "Aurora|Empress")
	TObjectPtr<USkeletalMeshComponent>	mEmpressMesh;

	// 이미 여제 모습인지
	bool	mEmpressRevealed = false;

	// 변신 소리 (10/29). 오로라는 이 소리가 페이즈 전환 소리를 맡는다 (공용 mPhaseStinger는 비워 둠 → 얼음 지진 폭발과 겹치지 않게)
	UPROPERTY(EditAnywhere, Category = "Aurora|Empress")
	TObjectPtr<class USoundBase>	mTransformSound;

	// 사망 : 얼어붙을 때 소리 (10/29). 부서질 때는 위의 mShatterSound
	UPROPERTY(EditAnywhere, Category = "Aurora|Death")
	TObjectPtr<class USoundBase>	mDeathFreezeSound;

	// ---------------------------------------------------------------- 얼음 조각 (10/28)
	// 원작 감정표현은 몸 "옆"의 얼음 덩어리를 찌르는 동작이라, 정면으로 서면 칼이 기디언 옆 허공으로 나간다.
	// → 이 각도(도)만큼 틀어서 선다. + = 오른쪽. 레벨의 오로라 디테일에서 바꾸면 빌드 없이 바로 확인할 수 있다.
	UPROPERTY(EditAnywhere, Category = "Aurora|Pattern")
	float	mSculptureYawOffset = -90.f;	// 10/28 내 판단 : -90이 맞음 (찌르기가 몸 오른쪽 옆)

	// ---------------------------------------------------------------- 얼음 분신 (10/28)
	// 분신 몸 전체에 칠할 얼음 머티리얼. 팩의 분신 머티리얼(M_Aurora_Decoy_*)은 분신 조각상 메시용이라 오로라 메시에 칠하면 검게 나와서,
	// 얼음 돌진 방패의 얼음(M_Aurora_Chunk_FresFade_Inst)을 쓴다. 레벨의 오로라 디테일에서 바꾸면 빌드 없이 다음 분신부터 바뀐다.
	UPROPERTY(EditAnywhere, Category = "Aurora|Pattern")
	TObjectPtr<UMaterialInterface>	mDecoyMaterial;

public:
	float GetSculptureYawOffset()	const
	{
		return mSculptureYawOffset;
	}

	UMaterialInterface* GetDecoyMaterial()	const
	{
		return mDecoyMaterial;
	}

	// 얼음 여제 모습으로 바꾼다. (얼음 지진 폭발 순간에 패턴이 부른다. 가리는 연출은 패턴이 한다)
	void RevealEmpress();

protected:
	virtual void BeginPlay() override;

	// 2페이즈에 들어왔는데 아직 안 바뀌었으면 바꾼다. (테스트용 "2페이즈부터 시작"이나, 연출이 끊긴 경우의 안전장치)
	virtual void OnPhaseEntered(int32 NewPhase) override;

	// 부모의 사라지기(머티리얼 FadeOut)를 덮어써서 얼어붙기로 바꾼다.
	// override : 부모의 virtual 함수를 이 클래스 방식으로 바꾼다는 표시. (철자가 틀리면 컴파일 오류로 알려 준다)
	// 부모의 Death()는 타이머로 StartDeathFade()를 부를 뿐인데, 실제 객체가 오로라라서 이 함수가 불린다. (가상 함수)
	virtual void StartDeathFade() override;

	// 얼음 파편으로 터지고 숨긴다.
	void Shatter();
};
