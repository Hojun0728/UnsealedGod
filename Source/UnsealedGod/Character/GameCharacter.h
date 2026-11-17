// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/Character.h"
#include "GameCharacter.generated.h"

// ============================================================================
// AGameCharacter
// 플레이어·보스·미니언의 공통 부모 클래스다.
// 셋 다 "GAS로 체력을 관리하고, 체력이 0이 되면 죽는다"는 점이 같아서 그 부분을 여기 모았다.
// (수업 코드는 PlayerCharacterGAS와 MonsterBaseGAS에 같은 코드가 두 번 들어 있었다)
//
//   AGameCharacter
//    ├─ APlayerCharacter → AGideon   : 카메라·입력·공격·회피를 더한다
//    ├─ ABossBase → ASevarog         : 패턴·페이즈를 더한다 (예정)
//    └─ (미니언)                      : 보스에게 이동·흡수 (예정)
//
// 죽었을 때 할 일은 셋이 다르다(패배 / 승리 연출 / 사라짐).
// 그래서 Death()를 virtual로 두고, 자식마다 override해서 덧붙인다.
//
// IAbilitySystemInterface : GAS가 "이 액터의 ASC가 어디 있는지" 물어볼 때 쓰는 인터페이스다.
//   인터페이스 = 순수가상함수만 있는 부모 클래스. 상속받으면 그 함수를 반드시 구현해야 한다.
//   (C++은 부모를 여러 개 상속받을 수 있다 : ACharacter + IAbilitySystemInterface)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AGameCharacter : public ACharacter,
	public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGameCharacter();

protected:
	// ---------------------------------------------------------------- GAS
	// GAS의 본체. 체력(AttributeSet)과 효과(GameplayEffect)·스킬(GameplayAbility)을 관리한다.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent>	mASC;

	// 체력·마나·공격력 수치. ASC에 등록해서 사용한다.
	// class UBaseAttributeSet : "전방 선언". 이런 클래스가 있다고만 알려주고,
	// 실제 헤더 include는 cpp에서 한다. 헤더끼리 서로 include해서 꼬이는 것을 막고 컴파일도 빨라진다.
	UPROPERTY()
	TObjectPtr<class UBaseAttributeSet>	mAttributeSet;

	// ---------------------------------------------------------------- 초기 수치 (자식이 생성자에서 바꾼다)
	UPROPERTY(EditAnywhere, Category = "Stat")
	float	mInitHP = 100.f;

	// 공격력. 공격 피해 = 공격력 × 공격별 배율.
	UPROPERTY(EditAnywhere, Category = "Stat")
	float	mInitAttack = 10.f;

	// 사망 처리가 두 번 실행되지 않게 막는 변수.
	bool	mDeath = false;

	// 공중에서 죽었을 때 true. 땅에 닿으면(Landed) 사망 마무리를 하고 false로 돌린다.
	bool	mDeathLandingPending = false;

	// 전투가 끝났는지 (플레이어 사망 또는 보스 사망). 끝나면 공격·스킬 같은 전투 행동을 하지 않는다.
	bool	mBattleEnded = false;

	// ---------------------------------------------------------------- 근접 판정 (보스 망치·미니언 칼 공용)
	// 10/15 : 보스에만 있던 판정을 미니언도 쓰도록 공통 부모로 옮겼다.
	// 이번 휘두르기에서 이미 맞은 대상. 한 번 휘두를 때 한 번만 맞게 하기 위함이다.
	// TWeakObjectPtr : "약한 포인터". 대상이 삭제되면 자동으로 무효가 되어 위험한 주소를 쓰지 않는다.
	//   (목록에 들고만 있을 뿐 "사용 중"으로 붙잡아 두지 않으므로 GC를 막지 않는다)
	TArray<TWeakObjectPtr<AActor>>	mMeleeHitActors;

	// 지난 프레임의 판정 점 위치들 (손 → 끝 순서). 지난 위치 → 지금 위치로 쓸어서(스윕) 빠른 무기도 놓치지 않는다.
	// 비어 있으면 "이번 휘두르기의 첫 프레임"이라는 뜻이다.
	TArray<FVector>	mLastMeleePoints;

	// [테스트용] 판정 구·범위·필살기 엄폐 선·보스 상태 글자를 화면에 그린다. (초록 = 판정 중, 빨강 = 맞음)
	// (10/21) 기본 꺼짐. 버그를 볼 때 레벨의 캐릭터 디테일 → Debug에서 켠다.
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool	mDrawMeleeDebug = false;

	// ---------------------------------------------------------------- 피격 소리 (10/29)
	// "이 캐릭터에게 맞았을 때" 맞은 쪽이 내는 소리 = 때리는 무기의 성질. 자식 생성자가 정한다.
	// 비워 두면 게임 모드 표의 기본 피격 소리 (오로라 검·미니언·분신은 비워 둠, 세바로그 망치만 무거운 소리)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mHitSoundOnTargetLight;

	UPROPERTY()
	TObjectPtr<class USoundBase>	mHitSoundOnTargetHeavy;

protected:
	// 게임 시작 시 GAS를 초기화한다. 순서가 중요하다.
	//   1. InitAbilityActorInfo : ASC에 주인(Owner)과 몸(Avatar) 지정
	//   2. BindAttributeDelegates : 값 변화 감지 등록 (초기값보다 먼저 해야 시작 값도 감지된다)
	//   3. InitAttributes : 초기값 넣기
	virtual void BeginPlay() override;

	// 값이 바뀔 때 호출될 함수를 등록한다. 기본 : HP.
	// 플레이어는 override해서 MP도 등록한다.
	virtual void BindAttributeDelegates();

	// 초기값을 넣는다. 기본 : HP·공격력.
	// 플레이어는 override해서 MP·마나 회복을 더한다.
	virtual void InitAttributes();

	// HP가 바뀔 때 GAS가 호출한다. 기본 : 0 이하가 되면 Death().
	// 자식은 override해서 화면 표시·피격 반응 등을 덧붙이고, 마지막에 Super::OnHPChanged를 부른다.
	virtual void OnHPChanged(const FOnAttributeChangeData& Data);

	// GameplayEffect를 자기 자신에게 적용하는 공통 함수.
	// TSubclassOf<UGameplayEffect> : "UGameplayEffect 또는 그 자식 클래스"만 담을 수 있는 클래스 타입 변수.
	// SetByCallerValues : SetByCaller로 넘길 "태그 → 값" 목록. GE의 Modifier 수만큼 넣어준다.
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass,
		const TMap<FGameplayTag, float>& SetByCallerValues);

	// 사망 마무리 : 이동 모드 없음 + 캡슐 충돌 끄기. 땅에 있을 때만 해야 한다.
	// (공중에서 하면 중력이 없어져 공중에 멈추고, 충돌만 끄면 바닥을 뚫고 떨어진다)
	void FinishDeathOnGround();

	// ACharacter의 virtual 함수 : 떨어지다가 땅에 닿는 순간 엔진이 호출한다.
	// 공중에서 죽었으면 여기서 사망 마무리를 한다.
	virtual void Landed(const FHitResult& Hit) override;

public:
	// IAbilitySystemInterface에서 이 함수는 순수가상함수로 되어 있기 때문에 반드시 재정의해야 한다.
	// GAS 내부 프레임워크에서 이 함수를 호출하여 AbilitySystemComponent를 얻어온다.
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 사망 처리. 기본 : 이동 정지, 충돌 끄기.
	// 자식은 override해서 자기만의 처리(패배 연출, 승리 연출, 사라짐)를 덧붙인다.
	virtual void Death();

	// 다른 캐릭터에게 GAS로 데미지를 준다. (GameplayEffect_Damage, 기디언 투사체와 같은 방식)
	// 대상에게 ASC가 없으면 아무것도 하지 않는다. 데미지를 줬으면 true.
	// 무적(State.Invincible)은 대상의 AttributeSet이 알아서 막는다.
	// const FHitResult& Hit : 어디에 맞았는지 (이펙트 위치 등에 쓸 수 있도록 Context에 담는다)
	// bUnblockable : true면 무적(회피)으로 막을 수 없는 피해 (필살기 즉사). = false는 "기본 인자" → 생략하면 false
	// bHeavy : true면 "큰 공격" (맞은 플레이어가 넘어진다. 도약 착지·돌진 마무리·3연타 3타)
	bool ApplyDamageToTarget(AActor* Target, float Damage, const FHitResult& Hit,
		bool bUnblockable = false, bool bHeavy = false);

	// 피해를 "실제로" 받은 뒤 AttributeSet(PostGameplayEffectExecute)이 부른다. (무적으로 막힌 피해는 안 온다)
	// Damage : 받은 피해량(양수), Attacker : 공격한 캐릭터(없을 수 있음), bHeavy : 큰 공격인지
	// 기본은 아무것도 안 한다. 플레이어가 덮어써서 피격 반응(움찔·넘어짐)을 한다. (나중에 보스 그로기·미니언 경직도 여기)
	virtual void OnDamaged(float Damage, AActor* Attacker, bool bHeavy)
	{
	}

	// 전투가 끝났을 때 게임 모드(AArenaGameMode)가 레벨의 모든 캐릭터에게 부른다. (10/19)
	// bPlayerWon : true = 보스 처치(승리), false = 기디언 사망(패배)
	// 기본은 "전투 끝" 표시만 한다. 캐릭터마다 덮어써서 자기 반응을 더한다. (자식은 마지막에 Super 호출)
	//   보스 : 영혼 뽑기 → 박수 / 미니언 : 쓰러진 기디언을 둘러싸고 공격 또는 부서짐 / 기디언 : 승리 동작
	// 게임 모드는 상대가 누군지 몰라도 이 함수 하나만 부르면 된다. (OnDamaged·IsPlayerTeam과 같은 방식)
	virtual void OnBattleEnd(bool bPlayerWon)
	{
		mBattleEnded = true;
	}

	bool IsBattleEnded()	const
	{
		return mBattleEnded;
	}

	// 바닥 원 범위 공격 : Center(바닥 높이) 기준 수평 반지름 Radius 안의 "적"에게 Damage만큼 피해.
	// 10/16 : 보스에만 있던 함수를 공통 부모로 옮겼다. 보스가 쓰면 기디언이, 기디언이 쓰면 보스·미니언이 맞는다.
	// 예고 데칼과 같은 원을 쓰도록 "수평 거리"로 판정한다. 맞힌 대상이 있으면 true.
	// const FVector& : FVector를 복사하지 않고 원본을 "읽기 전용"으로 빌려 쓴다. (복사 비용 절약 + 수정 방지)
	bool ApplyAreaDamage(const FVector& Center, float Radius, float Damage, bool bHeavy = false);

	// 현재 공격력. (공격 피해 = 공격력 × 공격별 배율)
	float GetAttack()	const;

	// (10/21) HUD가 매 프레임 읽어 가는 값. 캐릭터는 HUD를 몰라도 된다. (애님 인스턴스가 값을 읽어 가는 것과 같은 방식)
	float GetHP()		const;
	float GetHPMax()	const;
	float GetMP()		const;
	float GetMPMax()	const;

	// ---------------------------------------------------------------- 편 (팀)
	// 플레이어 편이면 true. 기본은 false(적 편 : 보스·미니언), APlayerCharacter가 덮어써서 true를 돌려준다.
	// virtual 함수 : 부모 포인터(AGameCharacter*)로 불러도 "실제 객체"의 버전이 실행된다.
	//   → 범위 공격 코드는 상대가 기디언인지 미니언인지 몰라도, 이 함수 하나로 편을 구분할 수 있다.
	virtual bool IsPlayerTeam()	const
	{
		return false;
	}

	// Other가 나와 다른 편이고 살아 있으면 true. (범위 공격 대상 고르기)
	// const AGameCharacter* : 상대를 "읽기만" 한다. (상대의 값을 바꾸지 않는다는 약속)
	bool IsHostileTo(const AGameCharacter* Other)	const;

	bool IsDeath()	const
	{
		return mDeath;
	}

	// ---------------------------------------------------------------- AI (보스·미니언)
	// 이 캐릭터가 쓸 비헤이비어 트리. AEnemyController가 빙의할 때 꺼내서 실행한다.
	// virtual : 기본은 없음(nullptr, 플레이어). 보스·미니언이 덮어써서 자기 BT를 돌려준다.
	virtual UBehaviorTree* GetBehaviorTree() const
	{
		return nullptr;
	}

	// 블랙보드의 Target (플레이어). AI가 아니거나 없으면 nullptr.
	AActor* GetTarget()	const;

	// ---------------------------------------------------------------- 근접 판정
	// 근접 판정 구간 : 시작 / 매 프레임 / 끝 (UAnimNotifyState_MeleeHit가 호출)
	// TickMeleeHit : StartSocket(손) → EndSocket(무기 끝) 사이를 PointCount개의 점으로 나눠 각각 검사한다.
	//   StartSocket이 None이면 EndSocket 한 점만 검사한다. 플레이어만 맞힌다.
	//   bHeavy : 큰 공격 (노티파이의 Heavy 체크)
	//   FreezeTime : 0보다 크면 맞은 플레이어가 그 시간(최대)만큼 얼어붙는다 (10/28, 오로라 얼음 돌진. 노티파이의 Freeze Time)
	void BeginMeleeHit();
	void TickMeleeHit(FName StartSocketName, FName EndSocketName, int32 PointCount,
		float Radius, float DamageScale, bool bHeavy = false, float FreezeTime = 0.f);
	void EndMeleeHit();

	// [테스트용] 판정·디버그 그림을 그릴지
	bool ShouldDrawDebug()	const
	{
		return mDrawMeleeDebug;
	}

	// 이 캐릭터에게 맞은 대상이 낼 피격 소리 (없으면 nullptr → 기본 소리)
	class USoundBase* GetHitSoundOnTarget(bool bHeavy)	const
	{
		return bHeavy ? mHitSoundOnTargetHeavy : mHitSoundOnTargetLight;
	}
};
