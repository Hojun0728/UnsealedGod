// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Input/Input.h"
#include "../Character/GameCharacter.h"
#include "PlayerCharacter.generated.h"

// (10/21) HUD 스킬 칸이 "어느 스킬"인지 고를 때 쓰는 번호. (E 회피, Q·F·R 스킬)
enum class ESkillSlot : uint8
{
	Dodge,
	Q,
	F,
	R
};

// ============================================================================
// APlayerCharacter
// 플레이어 캐릭터의 공통 부모 클래스다. (수업 코드의 PlayerCharacterGAS와 같은 역할)
// GAS·체력·사망 같은 "모든 캐릭터 공통"은 부모 AGameCharacter가 처리하고,
// 여기서는 카메라, 입력, 마나, 공격, 회피처럼 "플레이어만의 것"을 담당한다.
// 실제 영웅(AGideon)은 이 클래스를 상속받아 메시·애니메이션·스킬만 채운다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API APlayerCharacter : public AGameCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// ---------------------------------------------------------------- 컴포넌트
	// VisibleAnywhere : 에디터 디테일 패널에서 볼 수 있게 한다. (컴포넌트 포인터 자체는 못 바꿈)

	// 카메라를 캐릭터 뒤쪽에 일정 거리로 붙잡아두는 "셀카봉" 역할.
	// 벽에 가로막히면 자동으로 짧아져서 카메라가 벽을 뚫지 않는다.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent>	mArm;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent>	mCamera;

	// SkeletalMeshComponent가 생성한 AnimInstance 객체의 주소를 저장하기 위한 변수.
	UPROPERTY()
	TObjectPtr<class UPlayerAnimInstance>	mAnimInst;

	// ---------------------------------------------------------------- 초기 수치 (플레이어 전용)
	// 체력(mInitHP)·공격력(mInitAttack)은 부모 AGameCharacter에 있다.
	// EditAnywhere : 에디터에서 값을 바꿀 수 있게 한다. Category : 디테일 패널에서 묶여 보일 그룹 이름.
	UPROPERTY(EditAnywhere, Category = "Stat")
	float	mInitMP = 100.f;

	// 초당 마나 회복량.
	UPROPERTY(EditAnywhere, Category = "Stat")
	float	mMPRegen = 5.f;

	// ---------------------------------------------------------------- 방향별 이동 속도 (cm/초)
	// 소울류처럼 뒷걸음질이 가장 느리다. 앞 600 / 옆 550 / 뒤 500 (10/15 : 400은 너무 답답해서 500)
	// 보스 걷기는 페이즈마다 500~550이라, 2페이즈부터는 뒷걸음질만으로 점점 따라잡힌다.
	UPROPERTY(EditAnywhere, Category = "Move")
	float	mMoveSpeedForward = 600.f;

	UPROPERTY(EditAnywhere, Category = "Move")
	float	mMoveSpeedSide = 550.f;

	UPROPERTY(EditAnywhere, Category = "Move")
	float	mMoveSpeedBack = 500.f;

	// ---------------------------------------------------------------- 기본 공격
	// 기본 공격으로 발사할 투사체 클래스. 영웅(자식 클래스)이 생성자에서 정한다.
	// TSubclassOf<AProjectileBase> : AProjectileBase나 그 자식 "클래스"만 담을 수 있다.
	UPROPERTY(EditAnywhere, Category = "Attack")
	TSubclassOf<class AProjectileBase>	mProjectileClass;

	// 발사할 때 손에서 번쩍이는 이펙트.
	UPROPERTY(EditAnywhere, Category = "Attack")
	TObjectPtr<UParticleSystem>		mMuzzleParticle;

	// 화면 중앙 조준 거리(cm). 이 거리 안에서 아무것도 안 맞으면 이 거리의 지점을 목표로 삼는다.
	UPROPERTY(EditAnywhere, Category = "Attack")
	float	mAimDistance = 10000.f;

	// [테스트용] 발사할 때 조준 지점에 디버그 구를 그린다. (10/21 HUD 조준점이 생겨서 기본 꺼짐)
	UPROPERTY(EditAnywhere, Category = "Attack")
	bool	mDrawAimDebug = false;

	// ---------------------------------------------------------------- 회피 (E)
	// 회피 스킬로 쓸 어빌리티 클래스. 영웅(자식 클래스)이 생성자에서 정한다. (기디언 = 토른 스페이스)
	// 게임이 시작되면 BeginPlay에서 ASC에 등록(GiveAbility)한다.
	UPROPERTY(EditAnywhere, Category = "Dodge")
	TSubclassOf<class UGameplayAbility>	mDodgeAbilityClass;

	// ---------------------------------------------------------------- 스킬 (Q·F·R)
	// 영웅(자식 클래스)이 생성자에서 정하고, BeginPlay에서 회피와 같이 ASC에 등록한다.
	// (기디언 : Q 포털 운석, F 버든, R 운석 비)
	UPROPERTY(EditAnywhere, Category = "Skill")
	TSubclassOf<class UGameplayAbility>	mSkillQClass;

	UPROPERTY(EditAnywhere, Category = "Skill")
	TSubclassOf<class UGameplayAbility>	mSkillFClass;

	UPROPERTY(EditAnywhere, Category = "Skill")
	TSubclassOf<class UGameplayAbility>	mSkillRClass;

	// ---------------------------------------------------------------- 피격 (10/16)
	// 큰 공격에 맞았을 때 넘어지는 어빌리티. 영웅이 생성자에서 정한다. (게임플레이 이벤트로 발동)
	UPROPERTY(EditAnywhere, Category = "Hit")
	TSubclassOf<class UGameplayAbility>	mKnockdownAbilityClass;

	// (10/21) 보스 속박에 묶였을 때의 어빌리티. 영웅이 생성자에서 정한다. (게임플레이 이벤트로 발동)
	UPROPERTY(EditAnywhere, Category = "Hit")
	TSubclassOf<class UGameplayAbility>	mBoundAbilityClass;

	// (10/28) 오로라의 얼음에 갇혔을 때의 어빌리티. 영웅이 생성자에서 정한다. (게임플레이 이벤트로 발동)
	UPROPERTY(EditAnywhere, Category = "Hit")
	TSubclassOf<class UGameplayAbility>	mFrozenAbilityClass;

	// 지난번 이동 입력. 얼어 있을 때 "새로 누른 키"만 연타로 세려고 기억한다. 키를 다 떼면 0으로.
	FVector2D	mLastMoveAxis = FVector2D::ZeroVector;

	// 연타할 때마다 1씩 오르는 번호. HUD가 바뀐 것을 보고 아이콘을 톡 튀긴다. (숫자 자체는 의미 없음)
	int32	mFrozenMashSerial = 0;

	// 얼음 탈출 진행도 (0 = 방금 얼음, 1 = 다 깸). 얼음 어빌리티가 정하고, HUD 화면 성에가 이만큼 녹는다.
	float	mFrozenProgress = 0.f;

	// 작은 피격 몽타주 (애디티브, 섹션 Front·Back·Left·Right, HitReact 슬롯). (11/4) 생성자에서 미리 불러온다.
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mHitReactMontage;

	// ---------------------------------------------------------------- 등장 연출 (10/21)
	// 게임 시작 때 무릎 꿇었다 일어나 손을 터는 동작 (원작 LevelStart_Montage를 복제해 슬롯만 DefaultSlot으로). 처음 쓸 때 경로로 불러온다.
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mIntroMontage;

	FString	mIntroMontagePath;

	// 등장 연출 중인지. 이동·공격·스킬·회피(CanAct)와 시점 회전(LookKey)을 막는다.
	bool	mIntroPlaying = false;

	// ---------------------------------------------------------------- 기상 회피 (10/19)
	// 무릎 꿇고 있는 동안 누르고 있는 이동 방향과 그 시각. E를 누르면 이 방향으로 회피한다.
	FVector	mWakeDodgeDirection = FVector::ZeroVector;
	float	mWakeDodgeInputTime = -100.f;

	// ---------------------------------------------------------------- 회복 물약 (10/19)
	// 마시는 어빌리티. 영웅이 생성자에서 정한다. (기디언 = UGameplayAbility_Potion, Recall 동작)
	UPROPERTY(EditAnywhere, Category = "Potion")
	TSubclassOf<class UGameplayAbility>	mPotionAbilityClass;

	// 남은 물약 개수. 시작 1개, 보스가 2·3·4페이즈에 들어갈 때마다 +1 (안 쓴 건 이어진다)
	UPROPERTY(EditAnywhere, Category = "Potion")
	int32	mPotionCount = 1;

	// ---------------------------------------------------------------- 승리 (10/19)
	// 보스를 쓰러뜨렸을 때의 승리 동작. 영웅이 생성자에서 정한다. (기디언 = Emote_Taunt_PraiseMe_T1)
	UPROPERTY(EditAnywhere, Category = "Result")
	TObjectPtr<UAnimSequence>	mVictoryAnim;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 부모의 HP 감지에 MP 감지를 더한다.
	virtual void BindAttributeDelegates() override;

	// 부모의 HP·공격력 초기값에 MP·마나 회복을 더한다.
	virtual void InitAttributes() override;

	// 화면에 HP를 표시하고, 부모의 처리(0이면 사망)를 이어서 한다.
	virtual void OnHPChanged(const FOnAttributeChangeData& Data) override;

public:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// ---------------------------------------------------------------- 입력 처리 함수
	// 키가 눌렸을 때 Enhanced Input이 호출해 주는 함수들이다.
	// const FInputActionValue& : 입력값(축 값, 눌림 여부)을 "복사 없이" 읽기 전용으로 받는다.
	//   &(참조) = 원본을 그대로 가리킨다(복사 비용 없음). const = 읽기만 하고 바꾸지 않는다.
	void MoveKey(const FInputActionValue& Value);
	void MoveReleaseKey(const FInputActionValue& Value);	// (10/28) 이동 키를 모두 뗐을 때 (연타 감지용)
	void LookKey(const FInputActionValue& Value);
	void JumpKey(const FInputActionValue& Value);
	void JumpReleaseKey(const FInputActionValue& Value);
	void PotionKey(const FInputActionValue& Value);

	void AttackKey(const FInputActionValue& Value);
	void DodgeKey(const FInputActionValue& Value);
	void SkillQKey(const FInputActionValue& Value);
	void SkillFKey(const FInputActionValue& Value);
	void SkillRKey(const FInputActionValue& Value);
	void PauseKey(const FInputActionValue& Value);	// (11/2) Esc : 일시정지 메뉴 열기

	// 스킬 키 3개의 공통 처리 : 등록된 스킬 어빌리티의 발동을 시도한다.
	void TryActivateSkill(TSubclassOf<class UGameplayAbility> SkillClass);

	// ASC에 이 태그가 붙어 있는지. (State.Casting 등. 이름 문자열로 간단히 확인하는 도우미 함수)
	bool HasStateTag(const TCHAR* TagName)	const;

	// 작은 피격 : 공격자 방향에 맞는 HitReact 섹션(Front·Back·Left·Right)을 재생한다.
	void PlayHitReact(AActor* Attacker);

public:
	// 지금 행동 입력을 받을 수 있는지.
	// 죽었거나 회피 중(State.Dodging)·넘어짐(State.Knockdown)이면 false. 보스 속박(State.Bound)도 포함 (10/21).
	// 스킬 시전 중 제한은 입력마다 다르다 (10/16)
	//   State.Rooted  : 이동만 막는다 (MoveKey)
	//   State.Casting : 기본 공격·점프를 막는다 (다른 스킬은 어빌리티의 ActivationBlockedTags가 막는다)
	//   (11/4) 회피는 Q·F를 끊고 나간다. R 시전 중에만 State.NoDodge가 막는다
	bool CanAct()	const;

	// 플레이어 편. (부모의 기본값 false를 덮어쓴다 → 범위 공격이 보스·미니언만 맞힌다)
	virtual bool IsPlayerTeam()	const override
	{
		return true;
	}

	// 피격 반응. 큰 공격이면 넘어지고(Knockdown 어빌리티), 아니면 맞은 방향으로 움찔한다(HitReact).
	virtual void OnDamaged(float Damage, AActor* Attacker, bool bHeavy) override;

	// 전투 끝 (10/19). 승리면 하던 스킬을 멈추고 승리 동작(Emote_Taunt_PraiseMe_T1). 이후 행동 입력은 CanAct가 막는다.
	virtual void OnBattleEnd(bool bPlayerWon) override;

	// (10/26) 스테이지 클리어 : 승리 글자가 사라지면 다시 움직일 수 있게 한다. (포탈까지 걸어가기)
	// "전투 끝" 표시(mBattleEnded)만 되돌린다. 게임 모드 쪽 전투 끝은 그대로라 결과가 두 번 나지 않는다.
	void ResumeAfterVictory();

	// ---------------------------------------------------------------- 회복 물약
	// 물약 개수 더하기 (보스 EnterPhase가 +1), 하나 쓰기 (물약 어빌리티가 발동할 때. 없으면 false)
	void AddPotion(int32 Count);
	bool ConsumePotion();

	// 마시는 중이면 물약을 끊는다. (이동 키·E를 누르면 부른다. 끊었으면 true)
	bool CancelPotion();

	int32 GetPotionCount()	const
	{
		return mPotionCount;
	}

	// (10/28) 오로라의 얼음에 갇혀 있는지. HUD가 "WASD 연타!" 안내를 켜고 끌 때 읽는다.
	bool IsFrozen()	const
	{
		return HasStateTag(TEXT("State.Frozen"));
	}

	int32 GetFrozenMashSerial()	const
	{
		return mFrozenMashSerial;
	}

	void SetFrozenProgress(float Progress)
	{
		mFrozenProgress = FMath::Clamp(Progress, 0.f, 1.f);
	}

	float GetFrozenProgress()	const
	{
		return mFrozenProgress;
	}

	// (10/21) HUD 스킬 칸 정보. 슬롯의 어빌리티 클래스에서 쿨다운 태그·마나 비용을 읽고, ASC에 남은 쿨다운을 물어본다.
	// OutRemain : 남은 쿨다운(초, 없으면 0), OutDuration : 전체 쿨다운(초), OutEnoughMP : 지금 마나로 쓸 수 있는지
	// float& : 참조 — 함수가 이 변수에 값을 "써서 돌려준다". (값 여러 개를 한 번에 돌려주는 방법)
	// 반환값 : 슬롯에 스킬이 없으면 false
	bool GetSkillSlotState(ESkillSlot Slot, float& OutRemain, float& OutDuration, bool& OutEnoughMP)	const;

	// HP 회복 (GE_Heal). 최대 HP를 넘지 않는다.
	void Heal(float Amount);

	// (10/21) 등장 연출 시작 : 동작 재생 + 시점을 LookAt 쪽으로 돌려 고정 + 입력 잠금. 반환값 = 동작 길이(초, 없으면 0)
	float StartIntro(const FVector& LookAt);

	// 등장 연출 끝 : 입력 풀기
	void EndIntro();

	// (11/5) 동작 없이 입력만 잠그고 시점을 LookAt 쪽으로 돌린다. (맵 둘러보기·보스 소개 동안. 카메라가 돌아올 때 보스를 보고 있게)
	// 잠긴 동안 스페이스바(점프 키) = 연출 건너뛰기
	void LockForIntro(const FVector& LookAt);

	// 몸이 카메라(컨트롤러) 방향을 따라 도는지. 넘어져 날아가는 동안만 끈다.
	void SetBodyFollowsCamera(bool bFollow)
	{
		bUseControllerRotationYaw = bFollow;
	}

	// 회피 도착 지점을 계산한다. 회피 어빌리티(UGameplayAbility_TornSpace)가 호출한다.
	// 이동 키를 누르고 있으면 그 방향, 아니면 뒤로 Distance만큼.
	// 가는 길에 벽·기둥(숨긴 판정 오브젝트)이 있으면 그 바로 앞에서 멈춘다.
	FVector CalcDodgeDestination(float Distance)	const;

	// 기본 공격 입력이 들어왔을 때. 기본 동작은 공격 몽타주의 다음 섹션 재생.
	// virtual이라 영웅마다 다른 공격 방식을 넣을 수 있다. (수업 코드의 Attack과 같은 역할)
	virtual void Attack();

	// 투사체를 실제로 발사한다. 공격 몽타주의 Fire 노티파이(UAnimNotify_Fire)가 호출한다.
	// SocketName : 투사체가 나갈 손 소켓, DamageScale : 피해 배율, ProjectileScale : 크기 배율
	// = 1.f : 기본 인자. 호출할 때 생략하면 1.0이 들어간다.
	// bStrongHit : (10/21) 강타 — 맞히면 히트스톱·흔들림 (막타 C)
	void FireProjectile(FName SocketName, float DamageScale, float ProjectileScale = 1.f,
		bool bStrongHit = false);

	// 손(소켓)에서 화면 중앙 방향으로 투사체 하나를 생성한다. (기본 공격과 Q·F 스킬이 같이 쓴다)
	// ProjectileClass : 생성할 투사체 클래스, Damage : 피해량(공격력이 이미 곱해진 값), ProjectileScale : 크기 배율
	// 반환값 : 생성된 투사체 (실패하면 nullptr). 부모 포인터(AProjectileBase*)로 자식 투사체를 받는다.
	class AProjectileBase* SpawnProjectile(TSubclassOf<class AProjectileBase> ProjectileClass,
		FName SocketName, float Damage, float ProjectileScale = 1.f);

	// 원하는 위치·방향에서 투사체를 생성한다. (Q 포털 운석 : 조준점 하늘에서 아래로)
	// SpawnProjectile(손에서 조준점으로)도 방향을 계산한 뒤 결국 이 함수를 부른다.
	// const FRotator& : 회전값을 복사하지 않고 읽기 전용으로 빌려 쓴다.
	class AProjectileBase* SpawnProjectileAt(TSubclassOf<class AProjectileBase> ProjectileClass,
		const FVector& Location, const FRotator& Rotation, float Damage, float ProjectileScale = 1.f);

	// 화면 중앙이 가리키는 월드 위치를 구한다. (카메라에서 정면으로 선을 쏴서 처음 맞은 곳)
	// 함수 뒤 const : 이 함수는 캐릭터의 멤버 변수를 바꾸지 않는다.
	FVector GetAimTarget()	const;

	// 화면 중앙이 가리키는 "바닥" 위치. 수평 거리가 MaxDistance보다 멀면 그 거리까지만. (R 운석 비 위치)
	FVector GetAimGroundLocation(float MaxDistance)	const;

	// 사망 처리. 부모(이동 정지·충돌 끄기)에 플레이어 전용 처리(몸 회전 끄기·공격 동작 멈춤)를 더한다.
	virtual void Death() override;

protected:
	// MP가 바뀔 때 GAS가 호출해 주는 함수. (등록은 BindAttributeDelegates에서)
	// 델리게이트 : "어떤 일이 생기면 이 함수를 불러줘"라고 등록해두는 언리얼의 콜백(방송) 시스템이다.
	void OnMPChanged(const FOnAttributeChangeData& Data);
};
