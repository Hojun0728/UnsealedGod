// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_Charge.h"
#include "../../Boss/BossBase.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UGameplayAbility_Pattern_Charge::UGameplayAbility_Pattern_Charge()
{
	// 사용 거리 : 4m ~ 12m. 6m 이상에서는 도약과 겹치지만 "직전 패턴 제외" 규칙으로 번갈아 나온다.
	mMinRange = 400.f;
	mMaxRange = 1200.f;

	mWeight = 1.f;

	mCoolDownTime = 5.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.BossCharge"));

	// 3연타·도약과 같은 템포. (준비 동작만 몽타주에서 재생 속도 0.5로 늘려 둠 → 실제 약 0.47초)
	mMontagePlayRate = 1.5f;

	// 빠른 페이즈(3페이즈~) 2배속 (10/15). 준비(예고) 시간도 같이 짧아진다 : 0.70 ÷ 2 = 0.35초
	mFastPlayRate = 2.f;

	// 목표 지점보다 이만큼 앞에서 멈추고 내려찍는다. (3연타와 같은 적정 타격 거리)
	mStrikeDistance = 250.f;

	// 밀기(Lunge)는 쓰지 않는다. 이동은 돌진 시작 이벤트의 질주가 담당.
	mLungeSpeed = 0.f;

	// 후딜은 기본값(0.8 ± 0.4) = 마무리 망치 뒤 반격 틈

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_Charge.AM_Sevarog_Charge'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	// (11/4) 예고 머티리얼을 부모 기본값(원)에서 선(사각형 경로)으로 바꾼다. 얼음 돌진(자식)도 이것을 그대로 쓴다.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>
		LineMat(TEXT("/Script/Engine.Material'/Game/UnsealedGod/Boss/M_Decal_WarningLine.M_Decal_WarningLine'"));

	if (LineMat.Succeeded())
		mDecalMaterial = LineMat.Object;
}

void UGameplayAbility_Pattern_Charge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 같은 어빌리티 객체를 재사용하므로 지난 돌진의 상태를 비운다.
	mPassThroughPawns = false;
	mChargeDistance = 0.f;

	// 부모 : 쿨다운 확정 → 멈춤 → 기디언 쪽 바라보기 → 몽타주 재생
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive || !IsActive())
		return;

	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Target))
		return;

	// ---------------------------------------------------------------- 방향·거리 고정
	// 이 순간의 기디언 위치로 고정한다. 준비 중에 옆으로 피하면 보스는 원래 경로로 지나간다.
	FVector	BossLocation = Boss->GetActorLocation();
	FVector	ToTarget = Target->GetActorLocation() - BossLocation;
	ToTarget.Z = 0.0;

	mChargeDirection = ToTarget.GetSafeNormal();

	// 목표 지점 mStrikeDistance 앞에서 멈춘다. (0 ~ 최대 거리)
	mChargeDistance = FMath::Clamp(ToTarget.Size() - mStrikeDistance, 0.f, mMaxChargeDistance);

	// ---------------------------------------------------------------- 사각형 예고 데칼
	// 보스 위치부터 "멈추는 지점 + 망치가 닿는 곳"까지. 사각형의 가운데에 놓고 질주 방향으로 돌린다.
	float	TotalLength = mChargeDistance + mPathExtraLength;
	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Center = BossLocation + mChargeDirection * (TotalLength * 0.5f) - FVector(0.0, 0.0, HalfHeight);

	// 부모 함수 (11/4) : 선 머티리얼로 사각형 예고. 준비 시간(실제 시간) 동안 차오르고, 다 차는 순간 출발
	SpawnWarningBox(Center, mChargeDirection.Rotation(), TotalLength * 0.5f, mPathHalfWidth, ToRealTime(mWindupTime));

	// ---------------------------------------------------------------- 돌진 시작 노티파이 대기
	UAbilityTask_WaitGameplayEvent* WaitCharge = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Boss.ChargeStart")), nullptr, true);

	WaitCharge->EventReceived.AddDynamic(this, &UGameplayAbility_Pattern_Charge::OnChargeStartEvent);
	WaitCharge->ReadyForActivation();
}

void UGameplayAbility_Pattern_Charge::OnChargeStartEvent(FGameplayEventData Payload)
{
	// 이미 가까워서 달릴 거리가 없으면 바로 내려찍는다.
	if (mChargeDistance < 1.f)
	{
		StartSwing();
		return;
	}

	// 달리는 동안은 기디언을 몸으로 막지 않고 "뚫고" 지나간다. (판정은 몽타주의 몸통 Melee Hit Window)
	SetPassThroughPawns(true);

	// 고정한 방향으로 일정 속도로 민다. 시간 = 거리 ÷ 속도
	// 끝날 때 속도 0 (SetVelocity + ZeroVector) → 도착하면 미끄러지지 않고 바로 선다.
	float	ChargeTime = mChargeDistance / mChargeSpeed;

	UAbilityTask_ApplyRootMotionConstantForce* Charge =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, NAME_None, mChargeDirection, mChargeSpeed, ChargeTime,
			false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector,
			0.f, true);

	Charge->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Charge::OnChargeFinished);
	Charge->ReadyForActivation();
}

void UGameplayAbility_Pattern_Charge::OnChargeFinished()
{
	StartSwing();
}

void UGameplayAbility_Pattern_Charge::StartSwing()
{
	// 몸 충돌을 원래대로
	SetPassThroughPawns(false);

	ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	// 무한 반복 중인 Run 섹션에서 Swing 섹션으로 넘긴다.
	// Montage_JumpToSection(섹션 이름, 몽타주) : 재생 중인 몽타주의 다른 섹션으로 바로 이동
	UAnimInstance* AnimInstance = Boss->GetMesh()->GetAnimInstance();

	if (IsValid(AnimInstance))
		AnimInstance->Montage_JumpToSection(mSwingSectionName, mMontage);

	// 망치가 내려오는 순간 예고 데칼을 지운다. (실제 시간 = 몽타주 시간 ÷ 재생 속도)
	float	HitDelay = ToRealTime(mSwingHitTime);

	UAbilityTask_WaitDelay* WaitHit = UAbilityTask_WaitDelay::WaitDelay(this, HitDelay);

	WaitHit->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_Charge::OnSwingHitTime);
	WaitHit->ReadyForActivation();
}

void UGameplayAbility_Pattern_Charge::OnSwingHitTime()
{
	DestroyWarningDecal();
}

void UGameplayAbility_Pattern_Charge::SetPassThroughPawns(bool bPassThrough)
{
	if (mPassThroughPawns == bPassThrough)
		return;

	ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (!IsValid(Boss))
		return;

	mPassThroughPawns = bPassThrough;

	// Pawn 채널에 대한 반응 : Overlap(겹침, 막지 않음) ↔ Block(막음, 원래 값)
	// Overlap이어도 몸통 판정(Pawn 오브젝트 검사)은 그대로 기디언을 찾는다.
	Boss->GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn,
		bPassThrough ? ECollisionResponse::ECR_Overlap : ECollisionResponse::ECR_Block);
}

void UGameplayAbility_Pattern_Charge::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 달리는 도중에 끊겨도(보스 사망 등) 몸 충돌을 꼭 되돌린다. (바닥 예고는 부모 EndAbility가 지운다)
	SetPassThroughPawns(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
