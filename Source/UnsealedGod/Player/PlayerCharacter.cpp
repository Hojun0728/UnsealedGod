// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "PlayerAnimInstance.h"
#include "../GAS/BaseAttributeSet.h"
#include "../GAS/Effect/GameplayEffect_ManaRegen.h"
#include "../GAS/Effect/GameplayEffect_Heal.h"
#include "../GAS/Ability/GameplayAbility_Base.h"
#include "../Projectile/ProjectileBase.h"
#include "../GameMode/ArenaGameMode.h"
// DrawDebugSphere 같은 디버그 그리기 함수가 들어있다.
#include "DrawDebugHelpers.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// 부모(AGameCharacter) 생성자가 먼저 실행되어 ASC·AttributeSet은 이미 만들어져 있다.

	// ---------------------------------------------------------------- 컴포넌트 생성
	// CreateDefaultSubobject<타입>(이름) : 생성자에서 컴포넌트를 만든다.
	// 반환값은 만들어진 컴포넌트의 주소(포인터)이고, 그 주소를 멤버 변수에 저장해 둔다.
	mArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	mCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));

	// ---------------------------------------------------------------- 카메라 설정
	// 수업 코드와 다른 부분 : 수업은 SpringArm을 메시에 붙이고 마우스로 Arm을 직접 돌렸다(RPG 방식).
	// 이 게임은 화면 중앙으로 조준하는 슈터 방식이라 "컨트롤러 회전"을 기준으로 삼는다.
	//   마우스 → 컨트롤러 회전 → (1) 카메라가 따라 돈다 (2) 캐릭터 몸도 같은 방향을 본다.

	// SpringArm을 캡슐(루트 컴포넌트)에 붙이고, 카메라를 SpringArm 끝에 붙인다.
	mArm->SetupAttachment(GetCapsuleComponent());
	mCamera->SetupAttachment(mArm);

	// 캐릭터와 카메라 사이 거리.
	mArm->TargetArmLength = 350.f;

	// SocketOffset : SpringArm 끝(카메라 위치)을 옆·위로 옮긴다.
	// Y 60 = 오른쪽으로, Z 60 = 위로. → 캐릭터 오른쪽 어깨 너머로 보는 시점이 된다.
	// 캐릭터가 화면 중앙을 가리지 않아서 조준점이 잘 보인다.
	mArm->SocketOffset = FVector(0.0, 60.0, 60.0);

	// SpringArm이 컨트롤러 회전(마우스)을 따라 돌게 한다.
	mArm->bUsePawnControlRotation = true;

	// 캐릭터 몸의 좌우 회전(Yaw)도 컨트롤러를 따라가게 한다.
	// → 캐릭터가 항상 카메라가 보는 방향을 바라본다. 옆·뒤로 이동하면 옆걸음·뒷걸음이 된다.
	// 위아래(Pitch)는 몸이 기울면 이상하므로 따라가지 않는다. (조준 높이는 나중에 에임 오프셋으로)
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 이동 방향으로 몸을 돌리는 기능을 끈다. (위에서 몸 방향을 카메라 기준으로 정했기 때문)
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 점프 높이와 공중 조작감.
	// JumpZVelocity : 점프할 때 위로 튀어오르는 속도. 클수록 높이 뛴다.
	// GravityScale  : 이 캐릭터에게만 적용되는 중력 배율. (월드 중력 980cm/s² × 배율)
	// AirControl    : 공중에서 방향을 바꿀 수 있는 정도 (0 = 못 바꿈, 1 = 땅과 같음).
	//
	// 체공 시간 = 2 × 점프속도 ÷ 중력,  최고 높이 = 점프속도² ÷ (2 × 중력)
	//   이전 : 속도 500, 중력 980  → 체공 약 1.02초, 높이 약 128cm  ("달에 온 느낌")
	//   2차  : 속도 700, 중력 1960 → 체공 약 0.71초, 높이 약 125cm  (아직 김)
	//   3차  : 속도 700, 중력 2940 → 체공 약 0.48초, 높이 약 83cm   (조금 낮음)
	//   지금 : 속도 767, 중력 2940 → 체공 약 0.52초, 높이 약 100cm  (3차 높이의 1.2배)
	// 높이는 속도의 제곱에 비례하므로, 높이를 1.2배 하려면 속도는 √1.2 ≈ 1.095배만 올린다.
	// 액션 게임은 보통 현실보다 중력을 세게 줘서 짧고 단단한 점프를 만든다.
	GetCharacterMovement()->JumpZVelocity = 767.f;
	GetCharacterMovement()->GravityScale = 3.f;
	GetCharacterMovement()->AirControl = 0.35f;

	// 작은 피격 몽타주. (11/4 : 실행 중 로드 → 생성자에서 미리. 레벨마다 첫 피격 순간 멈칫하지 않게)
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		HitReactAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_HitReact.AM_Gideon_HitReact'"));

	if (HitReactAsset.Succeeded())
		mHitReactMontage = HitReactAsset.Object;

	// 등장 연출 (10/21)
	mIntroMontagePath = TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_LevelStart.AM_Gideon_LevelStart'");
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	// 부모(AGameCharacter)의 BeginPlay : GAS 초기화 (주인 지정 → 값 감지 등록 → 초기값)
	// 이 안에서 virtual 함수 BindAttributeDelegates·InitAttributes가 불리는데,
	// 실제 객체가 플레이어라서 아래에 있는 플레이어 버전(MP 포함)이 실행된다.
	Super::BeginPlay();

	// 메시가 생성한 AnimInstance를 얻어와 우리 클래스(UPlayerAnimInstance)로 형변환한다.
	// Cast<T>(객체) : 언리얼 객체의 안전한 형변환. 실제 타입이 T가 아니면 nullptr을 반환한다.
	mAnimInst = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());

	// ---------------------------------------------------------------- 입력 매핑 등록
	// Pawn종류는 자신에게 빙의된 컨트롤러를 반환하는 기능을 제공한다.
	TObjectPtr<APlayerController> PlayerController =
		Cast<APlayerController>(GetController());

	// UObject타입 객체가 유효한 객체인지를 체크한다. (nullptr이거나 삭제 대기 중이면 false)
	if (IsValid(PlayerController))
	{
		// PlayerController가 가지고 있는 InputSystem을 얻어온다.
		TObjectPtr<UEnhancedInputLocalPlayerSubsystem>	InputSystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());

		// InputMappingContext를 가지고 있는 CDO를 얻어온다.
		// const UGideonInput* : "가리키는 대상을 바꿀 수 없는" 포인터다. CDO는 공용 기본값이라 읽기만 한다.
		const UGideonInput* InputCDO = GetDefault<UGideonInput>();

		// InputMappingContext를 InputSystem에 등록한다. (두 번째 인자 0 = 우선순위)
		if (IsValid(InputSystem) && IsValid(InputCDO->mContext))
			InputSystem->AddMappingContext(InputCDO->mContext, 0);
	}

	// ---------------------------------------------------------------- 어빌리티 등록
	// GiveAbility : ASC에 "이 캐릭터는 이 스킬을 쓸 수 있다"고 등록한다. 등록해야 발동(TryActivate)할 수 있다.
	// FGameplayAbilitySpec(클래스, 레벨) : 등록할 어빌리티 정보.
	if (mDodgeAbilityClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mDodgeAbilityClass, 1));

	// 스킬 Q·F·R도 같은 방법으로 등록한다.
	if (mSkillQClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mSkillQClass, 1));

	if (mSkillFClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mSkillFClass, 1));

	if (mSkillRClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mSkillRClass, 1));

	// 넘어짐 어빌리티. 키로 쓰는 게 아니라 "Event.Player.Knockdown" 이벤트가 오면 GAS가 알아서 발동한다.
	if (mKnockdownAbilityClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mKnockdownAbilityClass, 1));

	// 속박 (10/21). 넘어짐처럼 "Event.Player.Bound" 이벤트로 발동한다.
	if (mBoundAbilityClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mBoundAbilityClass, 1));

	// 얼음에 갇힘 (10/28). "Event.Player.Frozen" 이벤트로 발동한다.
	if (mFrozenAbilityClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mFrozenAbilityClass, 1));

	// 회복 물약 (1번 키)
	if (mPotionAbilityClass)
		mASC->GiveAbility(FGameplayAbilitySpec(mPotionAbilityClass, 1));
}

bool APlayerCharacter::HasStateTag(const TCHAR* TagName) const
{
	// HasMatchingGameplayTag : ASC에 이 태그가 붙어 있는지 확인한다.
	// const TCHAR* : 언리얼 문자열 상수(TEXT("..."))의 주소. 글자들을 "읽기만" 한다.
	return mASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TagName));
}

bool APlayerCharacter::CanAct() const
{
	if (mDeath)
		return false;

	// 회피 어빌리티가 켜져 있는 동안 붙는 태그. (ActivationOwnedTags)
	if (HasStateTag(TEXT("State.Dodging")))
		return false;

	// 넘어져 있는 동안(넘어짐 어빌리티의 ActivationOwnedTags)은 아무것도 못 한다.
	if (HasStateTag(TEXT("State.Knockdown")))
		return false;

	// 보스 속박에 묶여 있는 동안(속박 어빌리티의 ActivationOwnedTags)도 아무것도 못 한다. (10/21)
	if (HasStateTag(TEXT("State.Bound")))
		return false;

	// 얼음에 갇혀 있는 동안도 (10/28). 이동 키는 MoveKey가 따로 "연타"로 받는다.
	if (HasStateTag(TEXT("State.Frozen")))
		return false;

	// 등장 연출 중 (10/21)
	if (mIntroPlaying)
		return false;

	// 전투가 끝났으면(승리 동작 중) 움직이거나 공격하지 않는다. 시점은 계속 돌릴 수 있다.
	if (mBattleEnded)
		return false;

	return true;
}

void APlayerCharacter::ResumeAfterVictory()
{
	if (mDeath)
		return;

	// CanAct가 다시 true가 된다 → 이동·점프·스킬 입력이 다시 먹힌다.
	mBattleEnded = false;
}

void APlayerCharacter::OnBattleEnd(bool bPlayerWon)
{
	// 부모 : "전투 끝" 표시 → CanAct가 false가 된다
	Super::OnBattleEnd(bPlayerWon);

	// 패배면 이미 쓰러지는 중이라 할 일이 없다.
	if (mDeath || !bPlayerWon)
		return;

	// 하던 스킬·회피·넘어짐을 모두 멈춘다. 그 자리에서 미끄러지지 않게 이동도 멈춘다.
	mASC->CancelAllAbilities();
	GetCharacterMovement()->StopMovementImmediately();

	if (IsValid(mAnimInst))
		mAnimInst->StopAllMontage();

	// 승리 동작. 끝나면 대기 자세로 돌아간다.
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && IsValid(mVictoryAnim))
		AnimInstance->PlaySlotAnimationAsDynamicMontage(mVictoryAnim, TEXT("DefaultSlot"), 0.25f, 0.4f);
}

void APlayerCharacter::OnDamaged(float Damage, AActor* Attacker, bool bHeavy)
{
	// 죽는 피해였으면 사망 동작이 우선이다. (HP가 0이 되는 순간 이미 Death()가 불렸다)
	if (mDeath)
		return;

	// 넘어져 있는 동안은 무적이라 보통 여기 오지 않지만, 필살기 같은 막을 수 없는 피해가 오면 반응하지 않는다.
	if (HasStateTag(TEXT("State.Knockdown")))
		return;

	// 타격감 (10/21) : 큰 피격 = 히트스톱 + 강한 흔들림, 작은 피격 = 약한 흔들림만.
	// 수치는 게임 모드의 표가 정한다. 여기서는 "무슨 일이 일어났는지"만 알린다.
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();

	// (10/29) 피격 소리는 때린 쪽이 정한다 : 세바로그 망치 = 무거운 소리, 그 외 = 표의 기본 소리 (nullptr)
	// const AGameCharacter* : 때린 캐릭터의 값을 "읽기만" 한다
	USoundBase* HitSound = nullptr;

	if (const AGameCharacter* AttackerCharacter = Cast<AGameCharacter>(Attacker))
		HitSound = AttackerCharacter->GetHitSoundOnTarget(bHeavy);

	if (IsValid(GameMode))
		GameMode->PlayHitFeedback(bHeavy ? EHitFeedback::PlayerHitHeavy : EHitFeedback::PlayerHitLight,
			FVector::ZeroVector, true, HitSound);

	if (bHeavy)
	{
		// 큰 공격 : 넘어짐 어빌리티를 이벤트로 발동한다. 이벤트에 공격자를 담아 보내 날아갈 방향을 알려 준다.
		// SendGameplayEventToActor(받을 액터, 태그, 이벤트 정보) : 받는 액터의 ASC에서
		// 이 태그를 AbilityTriggers로 등록한 어빌리티가 발동된다.
		FGameplayEventData	Payload;
		Payload.Instigator = Attacker;
		Payload.EventMagnitude = Damage;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this,
			FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Knockdown")), Payload);
		return;
	}

	// 작은 공격 : 하던 동작 위에 움찔만 덧씌운다.
	PlayHitReact(Attacker);
}

void APlayerCharacter::PlayHitReact(AActor* Attacker)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(mHitReactMontage) || !AnimInstance)
		return;

	// ---------------------------------------------------------------- 맞은 방향 고르기
	// 나 → 공격자 방향을, 내 앞 방향·오른쪽 방향과 내적(Dot)해서 앞뒤·좌우 성분을 구한다.
	//   Forward 성분이 크면 앞/뒤, Right 성분이 크면 좌/우. (각도로 치면 45도씩 나눈 4구역)
	FName	Section = TEXT("Front");

	if (IsValid(Attacker))
	{
		FVector	ToAttacker = Attacker->GetActorLocation() - GetActorLocation();
		ToAttacker.Z = 0.f;
		ToAttacker = ToAttacker.GetSafeNormal();

		float	ForwardAmount = FVector::DotProduct(ToAttacker, GetActorForwardVector());
		float	RightAmount = FVector::DotProduct(ToAttacker, GetActorRightVector());

		// FMath::Abs : 절댓값. 앞뒤 성분과 좌우 성분 중 더 큰 쪽으로 정한다.
		if (FMath::Abs(ForwardAmount) >= FMath::Abs(RightAmount))
			Section = ForwardAmount >= 0.f ? TEXT("Front") : TEXT("Back");
		else
			Section = RightAmount >= 0.f ? TEXT("Right") : TEXT("Left");
	}

	// 처음부터 다시 재생하고 해당 방향 섹션으로 점프한다. (연속으로 맞으면 다시 움찔)
	// HitReact 슬롯은 다른 그룹이라 평타·스킬 몽타주를 멈추지 않는다.
	AnimInstance->Montage_Play(mHitReactMontage);
	AnimInstance->Montage_JumpToSection(Section, mHitReactMontage);
}

FVector APlayerCharacter::CalcDodgeDestination(float Distance) const
{
	// 마지막으로 들어온 이동 입력 방향. (MoveKey에서 AddMovementInput으로 넣은 월드 방향)
	// 이동 키를 안 누르고 있으면 (0, 0, 0)이다.
	FVector	Direction = GetLastMovementInputVector();

	// 기상 회피 : 넘어진 동안은 이동 입력이 막혀 있어서, MoveKey가 기억해 둔 방향을 쓴다.
	// (방금(0.15초 안) 누르고 있던 방향만. 키를 뗀 지 오래면 뒤로)
	if (Direction.IsNearlyZero() && GetWorld()->GetTimeSeconds() - mWakeDodgeInputTime < 0.15f)
		Direction = mWakeDodgeDirection;

	Direction.Z = 0.f;

	// 이동 입력이 없으면 뒤로. (소울류의 백스텝)
	// IsNearlyZero : 거의 0인지 확인한다. (float 오차 때문에 == 0 대신 사용)
	if (Direction.IsNearlyZero())
		Direction = -GetActorForwardVector();

	Direction.Normalize();

	FVector	Start = GetActorLocation();
	FVector	End = Start + Direction * Distance;

	// 내 캡슐 모양으로 Start → End를 쓸어 보면서(스윕) 벽·기둥에 막히는지 확인한다.
	// 반높이를 30cm 줄인다 : 캡슐 바닥이 바닥 면에 붙어 있어서 그대로 쓸면 출발하자마자 바닥에 걸리고,
	// 살짝 올라온 중앙 문양 같은 낮은 턱에도 걸리기 때문이다. 벽·기둥은 훨씬 높아서 그대로 막힌다.
	float	Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	float	HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FCollisionShape	Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight - 30.f);

	// WorldStatic 오브젝트(숨긴 벽·기둥 같은 움직이지 않는 것)만 검사한다.
	// 보스(Pawn)는 검사하지 않는다 → 보스를 뚫고 뒤로 순간이동할 수 있다.
	FCollisionObjectQueryParams	ObjectParams(ECollisionChannel::ECC_WorldStatic);
	FCollisionQueryParams		Params(NAME_None, false, this);

	FHitResult	Hit;

	bool	bHit = GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity,
		ObjectParams, Shape, Params);

	if (bHit)
	{
		// Hit.Location : 스윕에서는 "막힌 순간의 캡슐 중심 위치".
		// 벽에 딱 붙으면 살짝 파고들 수 있어서 5cm 덜 간다.
		return Hit.Location - Direction * 5.f;
	}

	return End;
}

void APlayerCharacter::BindAttributeDelegates()
{
	// 부모 : HP 감지 등록
	Super::BindAttributeDelegates();

	// 플레이어 전용 : MP 감지 등록 (화면 표시용)
	mASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetMPAttribute())
		.AddUObject(this, &APlayerCharacter::OnMPChanged);
}

void APlayerCharacter::InitAttributes()
{
	// 부모 : HPMax → HP → 공격력
	Super::InitAttributes();

	// 플레이어 전용 : 마나. (최대치를 먼저 넣어야 MP가 0으로 잘리지 않는다)
	mAttributeSet->SetMPMax(mInitMP);
	mAttributeSet->SetMP(mInitMP);

	// 마나 회복 효과를 자기 자신에게 건다. (Infinite라서 한 번만 걸면 계속 동작한다)
	TMap<FGameplayTag, float>	Values;
	Values.Add(FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.MPRegen")), mMPRegen);

	ApplyEffectToSelf(UGameplayEffect_ManaRegen::StaticClass(), Values);
}

void APlayerCharacter::SetupPlayerInputComponent(
	UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// InputComponent에 InputAction을 등록하고 해당 InputAction이 동작될때
	// 호출할 함수를 등록한다.
	// UInputComponent 타입을 UEnhancedInputComponent 타입으로 변환한다.
	TObjectPtr<UEnhancedInputComponent>	Input =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (IsValid(Input))
	{
		// InputAction을 가지고 있는 CDO를 얻어온다.
		const UGideonInput* InputCDO = GetDefault<UGideonInput>();

		// 1번인자 : 바인딩할 InputAction을 지정한다.
		// 2번인자 : 이 Action에 지정된 Key가 어떻게 반응할지 지정한다.
		//   Triggered : 누르고 있는 동안 매 프레임 (이동, 시점처럼 계속 들어오는 입력)
		//   Started   : 누른 순간 한 번
		//   Completed : 뗀 순간 한 번
		// 3번인자 : 이 Action이 동작할 때 호출할 함수를 가지고 있는 UObject 객체를 지정한다.
		// 4번인자 : 이 Action이 동작할 때 호출할 함수의 주소를 지정한다.
		Input->BindAction(InputCDO->FindAction(TEXT("Move")),
			ETriggerEvent::Triggered, this, &APlayerCharacter::MoveKey);

		// Completed : 이동 키를 모두 뗀 순간. 연타 감지용으로 지난 입력을 비운다. (10/28)
		Input->BindAction(InputCDO->FindAction(TEXT("Move")),
			ETriggerEvent::Completed, this, &APlayerCharacter::MoveReleaseKey);

		Input->BindAction(InputCDO->FindAction(TEXT("Look")),
			ETriggerEvent::Triggered, this, &APlayerCharacter::LookKey);

		Input->BindAction(InputCDO->FindAction(TEXT("Jump")),
			ETriggerEvent::Started, this, &APlayerCharacter::JumpKey);

		Input->BindAction(InputCDO->FindAction(TEXT("Jump")),
			ETriggerEvent::Completed, this, &APlayerCharacter::JumpReleaseKey);

		// 회복 물약 (1번 키). 입력 에셋(IA_Potion)이 아직 없으면 연결하지 않는다.
		if (UInputAction* Potion = InputCDO->FindAction(TEXT("Potion")))
			Input->BindAction(Potion, ETriggerEvent::Started, this, &APlayerCharacter::PotionKey);

		// 기본 공격은 Triggered : 누르고 있는 동안 매 프레임 호출된다 → 누르고 있으면 연사.
		// 실제로 쏠 수 있는지(콤보 대기·쿨타임)는 애님 인스턴스의 PlayAttack이 판단한다.
		Input->BindAction(InputCDO->FindAction(TEXT("Attack")),
			ETriggerEvent::Triggered, this, &APlayerCharacter::AttackKey);

		// 회피는 누른 순간 한 번만.
		Input->BindAction(InputCDO->FindAction(TEXT("Dodge")),
			ETriggerEvent::Started, this, &APlayerCharacter::DodgeKey);

		// 스킬 Q·F·R도 누른 순간 한 번만.
		// 입력 에셋(IA_SkillQ 등)을 아직 안 만들었으면 FindAction이 nullptr을 돌려주므로, 있을 때만 연결한다.
		if (UInputAction* SkillQ = InputCDO->FindAction(TEXT("SkillQ")))
			Input->BindAction(SkillQ, ETriggerEvent::Started, this, &APlayerCharacter::SkillQKey);

		if (UInputAction* SkillF = InputCDO->FindAction(TEXT("SkillF")))
			Input->BindAction(SkillF, ETriggerEvent::Started, this, &APlayerCharacter::SkillFKey);

		if (UInputAction* SkillR = InputCDO->FindAction(TEXT("SkillR")))
			Input->BindAction(SkillR, ETriggerEvent::Started, this, &APlayerCharacter::SkillRKey);

		// (11/2) 일시정지 (Esc). 메뉴를 "닫는" Esc는 메뉴 위젯이 직접 받는다. (메뉴가 열리면 입력이 UI 전용이라서)
		if (UInputAction* Pause = InputCDO->FindAction(TEXT("Pause")))
			Input->BindAction(Pause, ETriggerEvent::Started, this, &APlayerCharacter::PauseKey);
	}
}

void APlayerCharacter::PauseKey(const FInputActionValue& Value)
{
	// 멈추고 푸는 일은 게임 모드가 한다. (HUD·결과 화면과 같은 곳에서 화면 위젯을 관리)
	AArenaGameMode* GameMode = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this));

	if (IsValid(GameMode))
		GameMode->TogglePause();
}

void APlayerCharacter::MoveReleaseKey(const FInputActionValue& Value)
{
	mLastMoveAxis = FVector2D::ZeroVector;
}

void APlayerCharacter::MoveKey(const FInputActionValue& Value)
{
	// ---------------------------------------------------------------- 얼음에 갇힘 : 이동 대신 연타 (10/28)
	// 이동 입력은 누르고 있는 동안 매 프레임 들어온다. 그래서 지난 입력과 비교해
	// "0이었다가 눌렸거나(새로 누름) 방향이 뒤집힌(A↔D 번갈아)" 순간만 한 번으로 센다.
	FVector2D	MoveAxis = Value.Get<FVector2D>();

	if (HasStateTag(TEXT("State.Frozen")))
	{
		// 람다 : 이 함수 안에서만 쓰는 작은 함수. [](인자) { 내용 } 꼴. 앞뒤 축과 좌우 축에 똑같이 쓴다.
		auto IsNewPress = [](float Now, float Last)
		{
			return FMath::Abs(Now) > 0.5f &&
				(FMath::Abs(Last) < 0.5f || FMath::Sign(Now) != FMath::Sign(Last));
		};

		if (IsNewPress(MoveAxis.X, mLastMoveAxis.X) || IsNewPress(MoveAxis.Y, mLastMoveAxis.Y))
		{
			// 얼음 어빌리티에게 "한 번 눌렀다"를 알린다. 숫자는 어빌리티가 센다.
			FGameplayEventData	Payload;
			Payload.Instigator = this;

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this,
				FGameplayTag::RequestGameplayTag(TEXT("Event.Player.FrozenMash")), Payload);

			++mFrozenMashSerial;
		}

		mLastMoveAxis = MoveAxis;
		return;
	}

	mLastMoveAxis = MoveAxis;

	// 죽었거나 회피 중이면 이동하지 않는다.
	// 제자리 시전 스킬(State.Rooted)을 쓰는 동안에도 이동하지 않는다.
	// 기상 회피 대기 중(무릎 꿇음)에는 움직이지 않고, 누르고 있는 방향만 기억한다. (10/19)
	// → E를 누르면 그 방향으로 회피. 넘어진 동안 몸은 카메라를 안 따라가므로 "카메라 기준" 방향으로 계산한다.
	if (HasStateTag(TEXT("State.CanWakeDodge")))
	{
		FVector2D	WakeAxis = Value.Get<FVector2D>();
		FRotator	CameraYaw(0.0, GetControlRotation().Yaw, 0.0);

		mWakeDodgeDirection = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X) * WakeAxis.X +
			FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y) * WakeAxis.Y;
		mWakeDodgeInputTime = GetWorld()->GetTimeSeconds();
		return;
	}

	if (!CanAct() || HasStateTag(TEXT("State.Rooted")))
		return;

	// 물약을 마시는 중이면 끊고 움직인다. (회복은 끊기 전까지 들어간 만큼만)
	CancelPotion();

	// Axis2D 입력값을 꺼낸다. X = 앞(+)/뒤(-), Y = 오른쪽(+)/왼쪽(-)
	FVector2D	Axis = Value.Get<FVector2D>();

	// 캐릭터 몸이 항상 카메라 방향을 보고 있으므로(bUseControllerRotationYaw),
	// 캐릭터의 앞·오른쪽 방향이 곧 카메라 기준 앞·오른쪽 방향이다.
	FVector	Forward = GetActorForwardVector();
	FVector	Right = GetActorRightVector();

	FVector	MoveDir = Forward * Axis.X + Right * Axis.Y;

	// 방향별 속도 : 입력의 "앞 성분"(-1 ~ 1)으로 속도를 섞는다.
	//   앞쪽(0 ~ 1) : 옆 → 앞 속도 / 뒤쪽(-1 ~ 0) : 옆 → 뒤 속도. 대각선은 그 중간.
	// GetSafeNormal : 길이 1로 만든 벡터 (W+D면 X ≈ 0.71 → 앞과 옆의 중간 속도)
	// Lerp(A, B, 비율) : A와 B 사이에서 비율만큼 간 값
	FVector2D	InputDir = Axis.GetSafeNormal();
	float		ForwardAmount = InputDir.X;

	float	Speed = ForwardAmount >= 0.f ?
		FMath::Lerp(mMoveSpeedSide, mMoveSpeedForward, ForwardAmount) :
		FMath::Lerp(mMoveSpeedSide, mMoveSpeedBack, -ForwardAmount);

	GetCharacterMovement()->MaxWalkSpeed = Speed;

	// 이동 방향 벡터를 정규화한다. (길이를 1로 만든다)
	// 정규화하지 않으면 대각선 이동(W+D)의 길이가 약 1.41이 되어 더 빨라진다.
	MoveDir.Normalize();

	AddMovementInput(MoveDir, 1.f);
}

void APlayerCharacter::LookKey(const FInputActionValue& Value)
{
	// 등장 연출 중에는 시점도 고정 (보스가 솟아오르는 장면을 놓치지 않게, 10/21)
	if (mIntroPlaying)
		return;

	FVector2D	Axis = Value.Get<FVector2D>();

	// 컨트롤러를 좌우(Yaw)·상하(Pitch)로 회전시킨다.
	// SpringArm(카메라)과 캐릭터 몸(Yaw만)이 이 회전을 따라간다.
	// 마우스 위아래가 반대로 느껴지면 IMC_Gideon의 마우스 매핑에서 "부정(Negate)" 모디파이어를 켜고 끈다.
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void APlayerCharacter::JumpKey(const FInputActionValue& Value)
{
	// (11/5) 시작 연출 중이면 점프 대신 연출 건너뛰기. (건너뛸 게 없으면 게임 모드가 무시한다)
	if (mIntroPlaying)
	{
		if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
			GameMode->SkipIntro();

		return;
	}

	// 점프가 가능한 상태일 경우 점프한다. (공중에 있으면 CanJump가 false)
	// 스킬 시전 중에는 점프하지 않는다.
	if (CanAct() && !HasStateTag(TEXT("State.Casting")) && CanJump())
		Jump();
}

void APlayerCharacter::JumpReleaseKey(const FInputActionValue& Value)
{
	// 점프 키를 뗐다고 알려준다.
	// 지금은 점프 높이가 고정이지만, 나중에 "길게 누르면 더 높이" 기능을 쓸 때 필요하다.
	StopJumping();
}

void APlayerCharacter::PotionKey(const FInputActionValue& Value)
{
	// 남은 개수·시전 중 여부는 물약 어빌리티(CanActivateAbility·ActivationBlockedTags)가 확인한다.
	TryActivateSkill(mPotionAbilityClass);
}

bool APlayerCharacter::CancelPotion()
{
	if (!mPotionAbilityClass)
		return false;

	// FindAbilitySpecFromClass : ASC에 등록된 어빌리티 중 이 클래스의 정보(Spec)를 찾는다.
	// 반환형 FGameplayAbilitySpec* : ASC 안에 있는 정보의 "주소". 없으면 nullptr.
	FGameplayAbilitySpec* Spec = mASC->FindAbilitySpecFromClass(mPotionAbilityClass);

	// IsActive() : 지금 실행 중인지 (마시는 중인지)
	if (!Spec || !Spec->IsActive())
		return false;

	// 취소 → 물약 어빌리티의 EndAbility(취소)가 남은 회복 타이머를 멈추고 State.Casting을 뗀다.
	mASC->CancelAbilityHandle(Spec->Handle);

	return true;
}

void APlayerCharacter::AddPotion(int32 Count)
{
	mPotionCount = FMath::Max(mPotionCount + Count, 0);
}

bool APlayerCharacter::ConsumePotion()
{
	if (mPotionCount <= 0)
		return false;

	--mPotionCount;

	return true;
}

bool APlayerCharacter::GetSkillSlotState(ESkillSlot Slot, float& OutRemain, float& OutDuration,
	bool& OutEnoughMP) const
{
	OutRemain = 0.f;
	OutDuration = 0.f;
	OutEnoughMP = false;

	// 슬롯 → 어빌리티 클래스. (영웅 생성자가 정해 둔 클래스)
	TSubclassOf<UGameplayAbility>	SkillClass;

	switch (Slot)
	{
	case ESkillSlot::Dodge:	SkillClass = mDodgeAbilityClass;	break;
	case ESkillSlot::Q:		SkillClass = mSkillQClass;			break;
	case ESkillSlot::F:		SkillClass = mSkillFClass;			break;
	case ESkillSlot::R:		SkillClass = mSkillRClass;			break;
	}

	if (!SkillClass)
		return false;

	// GetDefaultObject : 클래스의 기본 객체(CDO). 생성자에서 정한 마나 비용·쿨다운 태그가 들어 있다. 읽기만 한다.
	const UGameplayAbility_Base* Skill = SkillClass->GetDefaultObject<UGameplayAbility_Base>();

	if (!Skill)
		return false;

	OutEnoughMP = GetMP() >= Skill->GetMPCost();

	if (!Skill->GetCoolDownTag().IsValid())
		return true;

	// "이 쿨다운 태그를 가진 효과"를 찾는 조건. 쿨다운 GE는 어빌리티가 붙인 태그(DynamicGrantedTags)를 가진다.
	// MakeQuery_MatchAnyOwningTags : 효과가 가진 태그 중 하나라도 맞으면 찾는다.
	FGameplayEffectQuery	Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
		FGameplayTagContainer(Skill->GetCoolDownTag()));

	// 찾은 효과마다 (남은 시간, 전체 시간) 한 쌍. 보통 하나뿐이지만, 여러 개면 가장 오래 남은 것.
	// TPair<A, B> : 값 두 개를 묶은 것. Key = 첫 번째(남은 시간), Value = 두 번째(전체 시간).
	TArray<TPair<float, float>>	Times = mASC->GetActiveEffectsTimeRemainingAndDuration(Query);

	for (const TPair<float, float>& Time : Times)
	{
		if (Time.Key > OutRemain)
		{
			OutRemain = Time.Key;
			OutDuration = Time.Value;
		}
	}

	return true;
}

void APlayerCharacter::LockForIntro(const FVector& LookAt)
{
	mIntroPlaying = true;

	// ---------------------------------------------------------------- 시점 : 보스 쪽
	// SetControlRotation : 컨트롤러 회전을 직접 정한다. 카메라(스프링 암)와 몸(Yaw)이 이 회전을 따라간다.
	// 위아래는 살짝 내려다보게(-8도) 해서 바닥에서 솟아오르는 보스가 화면 가운데쯤 오게 한다.
	if (AController* PlayerController = GetController())
	{
		FVector	ToTarget = LookAt - GetActorLocation();

		PlayerController->SetControlRotation(FRotator(-8.0, ToTarget.Rotation().Yaw, 0.0));
	}
}

float APlayerCharacter::StartIntro(const FVector& LookAt)
{
	// (11/5) 잠금·시점은 LockForIntro와 같다 (시작 연출의 맵 둘러보기에서도 쓰려고 나눔)
	LockForIntro(LookAt);

	// ---------------------------------------------------------------- 동작
	if (!IsValid(mIntroMontage) && !mIntroMontagePath.IsEmpty())
		mIntroMontage = LoadObject<UAnimMontage>(nullptr, *mIntroMontagePath);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(mIntroMontage) || !AnimInstance)
	{
		UE_LOG(UGDebug, Warning, TEXT("Player : AM_Gideon_LevelStart not found"));
		return 0.f;
	}

	// Montage_Play의 반환값 = 재생 길이(초). 실패하면 0.
	return AnimInstance->Montage_Play(mIntroMontage);
}

void APlayerCharacter::EndIntro()
{
	mIntroPlaying = false;
}

void APlayerCharacter::Heal(float Amount)
{
	if (mDeath || Amount <= 0.f)
		return;

	// GE_Heal : HP에 더하기. 최대 HP를 넘는 부분은 AttributeSet이 잘라 준다.
	TMap<FGameplayTag, float>	Values;
	Values.Add(FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Heal")), Amount);

	ApplyEffectToSelf(UGameplayEffect_Heal::StaticClass(), Values);
}

void APlayerCharacter::AttackKey(const FInputActionValue& Value)
{
	// 회피 중·스킬 시전 중에는 공격하지 않는다.
	if (!CanAct() || HasStateTag(TEXT("State.Casting")))
		return;

	Attack();
}

void APlayerCharacter::DodgeKey(const FInputActionValue& Value)
{
	if (!mDodgeAbilityClass || mDeath)
		return;

	// ---------------------------------------------------------------- 기상 회피 (10/19)
	// 넘어져 무릎을 꿇은 뒤(State.CanWakeDodge)에는 E로 넘어짐을 끊고 회피로 빠져나간다.
	// 순서 : 회피를 "먼저" 발동해 보고, 성공했을 때만 넘어짐을 취소한다.
	//   → 마나 부족·쿨다운이면 회피가 실패하는데, 그때 넘어짐만 끊기면 무적 없이 서 있게 되므로.
	// (회피 어빌리티는 State.Casting만 막히고 State.Knockdown에는 막히지 않아서, 넘어진 상태에서도 발동 시도가 된다)
	if (HasStateTag(TEXT("State.CanWakeDodge")))
	{
		if (mASC->TryActivateAbilityByClass(mDodgeAbilityClass))
		{
			if (FGameplayAbilitySpec* Spec = mASC->FindAbilitySpecFromClass(mKnockdownAbilityClass))
				mASC->CancelAbilityHandle(Spec->Handle);
		}

		return;
	}

	// 죽었거나 이미 회피 중이면 무시한다. (마나·쿨다운은 어빌리티가 따로 확인한다)
	if (!CanAct())
		return;

	// 물약을 마시는 중이면 먼저 끊는다. (마시는 중 이동 키로도 끊기는 물약 규칙과 같다)
	CancelPotion();

	// TryActivateAbilityByClass : 등록된 어빌리티 중 이 클래스를 찾아 발동을 시도한다.
	// 마나 부족·쿨다운 중이면 GAS가 알아서 발동하지 않고 false를 돌려준다.
	// (11/4) Q·F 시전 중이면 회피 어빌리티의 CancelAbilitiesWithTag가 스킬을 끊는다. R 시전 중(State.NoDodge)에는 발동하지 않는다.
	mASC->TryActivateAbilityByClass(mDodgeAbilityClass);
}

void APlayerCharacter::SkillQKey(const FInputActionValue& Value)
{
	TryActivateSkill(mSkillQClass);
}

void APlayerCharacter::SkillFKey(const FInputActionValue& Value)
{
	TryActivateSkill(mSkillFClass);
}

void APlayerCharacter::SkillRKey(const FInputActionValue& Value)
{
	TryActivateSkill(mSkillRClass);
}

void APlayerCharacter::TryActivateSkill(TSubclassOf<UGameplayAbility> SkillClass)
{
	// 죽었거나 회피 중이면 무시한다.
	// 마나 부족·쿨다운·이미 다른 스킬 시전 중(State.Casting)은 GAS가 알아서 막는다.
	if (!CanAct() || !SkillClass)
		return;

	mASC->TryActivateAbilityByClass(SkillClass);
}

void APlayerCharacter::Attack()
{
	// 공격 몽타주의 다음 섹션(A→B→C)을 재생한다.
	// 투사체는 여기서 바로 쏘지 않는다. 손을 뻗는 프레임에 있는 Fire 노티파이가 FireProjectile을 호출한다.
	// → 애니메이션과 투사체 발사 타이밍이 항상 맞는다.
	if (IsValid(mAnimInst))
		mAnimInst->PlayAttack();
}

FVector APlayerCharacter::GetAimTarget() const
{
	// 카메라 위치에서 카메라가 보는 방향(= 화면 중앙)으로 선을 쏜다.
	FVector	Start = mCamera->GetComponentLocation();
	FVector	End = Start + mCamera->GetForwardVector() * mAimDistance;

	FHitResult	Hit;

	// FCollisionQueryParams(이름, 복잡한 충돌 사용 여부, 무시할 액터)
	// 자기 자신은 무시한다. 카메라가 어깨 뒤에 있어서 선이 내 몸을 먼저 지나갈 수 있기 때문이다.
	FCollisionQueryParams	Params(NAME_None, false, this);

	// Visibility 채널로 선을 쏜다.
	// 숨긴 판정 벽·기둥(BlockAll)과 보스는 Visibility를 막고, 플레이어 투사체는 무시한다
	// (PlayerProjectile 프로필에서 Visibility = Ignore). 그래서 날아가는 내 탄에 조준이 걸리지 않는다.
	bool	bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End,
		ECollisionChannel::ECC_Visibility, Params);

	// 맞은 게 있으면 맞은 지점, 없으면(하늘) 최대 거리 지점.
	return bHit ? Hit.ImpactPoint : End;
}

void APlayerCharacter::FireProjectile(FName SocketName, float DamageScale, float ProjectileScale,
	bool bStrongHit)
{
	if (mDeath || !mProjectileClass)
		return;

	// 실제 생성은 SpawnProjectile이 한다. (Q·F 스킬도 같은 함수를 쓴다)
	// 피해량 = 공격력 × 배율. (A·B 10, C 15) / 크기 = 투사체 기본 크기 × 배율. (A·B 1.2배, C 1.8배)
	AProjectileBase* Projectile = SpawnProjectile(mProjectileClass, SocketName,
		mAttributeSet->GetAttack() * DamageScale, ProjectileScale);

	// 강타 표시는 투사체가 들고 날아간다. 맞는 순간 투사체가 게임 모드에 알린다. (10/21)
	if (IsValid(Projectile))
		Projectile->SetStrongHit(bStrongHit);

	// 손에서 번쩍이는 이펙트. 소켓에 붙여서 생성하면 손을 따라 움직인다.
	// 막타(배율 1.5)는 손 이펙트도 그만큼 크게 한다.
	// 인자 순서 : 이펙트, 붙일 컴포넌트, 소켓, 위치 오프셋, 회전 오프셋, 크기
	if (IsValid(mMuzzleParticle))
	{
		UGameplayStatics::SpawnEmitterAttached(mMuzzleParticle, GetMesh(), SocketName,
			FVector::ZeroVector, FRotator::ZeroRotator, FVector(ProjectileScale));
	}
}

AProjectileBase* APlayerCharacter::SpawnProjectile(TSubclassOf<AProjectileBase> ProjectileClass,
	FName SocketName, float Damage, float ProjectileScale)
{
	if (mDeath || !ProjectileClass)
		return nullptr;

	// 손(소켓)의 월드 위치.
	FVector	MuzzleLocation = GetMesh()->GetSocketLocation(SocketName);

	// 화면 중앙이 가리키는 지점.
	FVector	Target = GetAimTarget();

#if ENABLE_DRAW_DEBUG
	// [테스트용] 조준 지점에 빨간 구(반지름 20cm)를 1초 동안 그린다.
	// #if ENABLE_DRAW_DEBUG : 디버그 그리기가 가능한 빌드(에디터·개발 빌드)에서만 이 코드를 넣는다.
	//                         출시용(Shipping) 빌드에서는 이 부분이 아예 컴파일되지 않는다.
	if (mDrawAimDebug)
	{
		DrawDebugSphere(GetWorld(), Target, 20.f, 8, FColor::Red, false, 1.f);

		// 어느 소켓에서 발사했는지도 화면에 띄운다. (Key 5)
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(5, 2.f, FColor::Green,
				FString::Printf(TEXT("Fire : %s"), *SocketName.ToString()));
		}
	}
#endif

	// 손 → 목표 방향.
	FVector	Direction = Target - MuzzleLocation;

	// 예외 처리 : 목표가 손보다 뒤에 있으면(카메라와 손 사이에 벽이 있는 경우 등)
	// 뒤로 쏘게 되므로, 그때는 카메라가 보는 방향 그대로 쏜다.
	// Dot(내적) : 두 방향이 같은 쪽이면 +, 반대쪽이면 -.
	if (FVector::DotProduct(Direction, mCamera->GetForwardVector()) <= 0.f)
		Direction = mCamera->GetForwardVector();

	// 방향 벡터 → 회전값. 투사체는 자신의 앞(X축) 방향으로 날아간다.
	return SpawnProjectileAt(ProjectileClass, MuzzleLocation, Direction.Rotation(), Damage,
		ProjectileScale);
}

AProjectileBase* APlayerCharacter::SpawnProjectileAt(TSubclassOf<AProjectileBase> ProjectileClass,
	const FVector& Location, const FRotator& Rotation, float Damage, float ProjectileScale)
{
	if (mDeath || !ProjectileClass)
		return nullptr;

	// 스폰 옵션.
	FActorSpawnParameters	SpawnParams;

	// Owner·Instigator : "누가 쐈는지". 투사체가 발사자와 부딪히지 않게 하고(IgnoreActorWhenMoving),
	// 데미지를 줄 때 발사자의 ASC를 찾는 데 쓴다.
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	// 손이 벽에 붙어 있어 생성 위치가 막혀 있어도 무조건 생성한다. (생성되자마자 벽에 맞아 터진다)
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// SpawnActor<부모 타입>(실제 클래스, 위치, 회전, 옵션)
	// ProjectileClass에 자식 클래스(APortalBlastProjectile 등)가 들어 있어도 부모 포인터로 받을 수 있다.
	// → 부모 포인터로 자식 객체를 다루는 것. 이게 virtual 함수가 의미를 가지는 이유다.
	AProjectileBase* Projectile = GetWorld()->SpawnActor<AProjectileBase>(
		ProjectileClass, Location, Rotation, SpawnParams);

	if (IsValid(Projectile))
	{
		Projectile->SetDamage(Damage);

		// 발사할 때마다 반드시 호출해야 투사체의 기본 크기가 적용된다. (배율이 1.0이어도)
		Projectile->SetProjectileScale(ProjectileScale);
	}

	return Projectile;
}

FVector APlayerCharacter::GetAimGroundLocation(float MaxDistance) const
{
	FVector	Origin = GetActorLocation();
	FVector	Aim = GetAimTarget();

	// 내 위치 → 조준 지점의 "수평" 방향과 거리. (높이 차이는 빼고 계산한다)
	FVector	Offset = Aim - Origin;
	Offset.Z = 0.f;

	// GetClampedToMaxSize : 길이가 MaxDistance보다 길면 그 길이로 줄인 벡터. (방향은 그대로)
	Offset = Offset.GetClampedToMaxSize(MaxDistance);

	// 조준선이 기둥·벽 옆면에 맞았으면 그 지점이 벽 "표면"이라, 내 쪽으로 50cm 당겨서 벽 안쪽 바닥을 고른다.
	if (Offset.Size() > 50.f)
		Offset -= Offset.GetSafeNormal() * 50.f;

	FVector	Point = Origin + Offset;

	// 그 지점에서 아래로 선을 쏴서 바닥 높이를 찾는다.
	// 시작 높이를 "내 위치 + 2m"로 한 이유 : 기둥(5m 이상) 꼭대기 위에서 쏘면 기둥 위에 장판이 생기기 때문이다.
	FVector	Start = Point + FVector(0.0, 0.0, 200.0);
	FVector	End = Point - FVector(0.0, 0.0, 1000.0);

	FCollisionObjectQueryParams	ObjectParams(ECollisionChannel::ECC_WorldStatic);
	FCollisionQueryParams		Params(NAME_None, false, this);

	FHitResult	Hit;

	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, Params))
		return Hit.ImpactPoint;

	// 바닥을 못 찾으면 내 발 높이에 둔다.
	Point.Z = Origin.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	return Point;
}

void APlayerCharacter::OnHPChanged(const FOnAttributeChangeData& Data)
{
	// (10/21) 화면 표시는 HUD(UPlayerHUDWidget)가 매 프레임 직접 읽어 간다. 예전 디버그 글자는 삭제.

	// 부모 : 체력이 0이 되면 Death()
	Super::OnHPChanged(Data);
}

void APlayerCharacter::OnMPChanged(const FOnAttributeChangeData& Data)
{
	// (10/21) MP 표시는 HUD가 매 프레임 읽어 간다. (예전 디버그 글자 삭제)
}

void APlayerCharacter::Death()
{
	// 부모 : mDeath = true, 이동 정지, 캡슐 충돌 끄기
	Super::Death();

	UE_LOG(UGDebug, Warning, TEXT("Player Death"));

	// 입력을 통째로 막지는 않는다(DisableInput 사용 안 함). 마우스 시점은 계속 돌릴 수 있게 하기 위함이다.
	// 대신 이동·점프·공격·회피 함수가 각자 처음에 CanAct()로 막는다.
	// (나중에 11/3 패배 연출에서 카메라가 보스를 비출 때는 그 연출 동안만 시점 입력을 막을 예정)

	// 몸이 컨트롤러(마우스) 방향을 따라 도는 설정을 끈다.
	// 이걸 안 끄면 시점을 돌릴 때 쓰러진 몸이 마우스를 따라 빙글빙글 돈다.
	bUseControllerRotationYaw = false;

	// 공격 중에 죽었으면 상체 공격 동작을 멈춘다.
	if (IsValid(mAnimInst))
		mAnimInst->StopAllMontage();

	// 게임 모드에 알린다 → 모든 캐릭터에게 "패배"(보스 영혼 뽑기·미니언 둘러싸기) + 결과 화면 예약 (10/19)
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();

	if (IsValid(GameMode))
		GameMode->OnPlayerDeath(this);

	// 쓰러지는 동작은 애니메이션(Death_Fwd)으로 보여준다.
	// UPlayerAnimInstance가 매 프레임 IsDeath()를 읽어서 mDeath를 갱신하고,
	// ABP_Gideon의 "bool로 포즈 블렌딩" 노드가 mDeath가 true면 사망 애니메이션으로 바꾼다.
	//
	// 수업 코드의 래그돌(물리로 쓰러뜨리기)을 쓰지 않는 이유 :
	// 파라곤 기디언의 물리 에셋으로 래그돌을 켜면 관절이 비정상적으로 늘어나 보였다.
	// 또 소울류 보스전에서는 정해진 사망 동작이 연출(패배 화면 전환 타이밍)을 맞추기도 쉽다.
}
