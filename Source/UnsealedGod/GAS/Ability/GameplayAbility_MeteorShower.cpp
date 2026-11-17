// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_MeteorShower.h"
#include "../../Player/PlayerCharacter.h"
#include "../../Skill/MeteorShowerArea.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

UGameplayAbility_MeteorShower::UGameplayAbility_MeteorShower()
{
	// ---------------------------------------------------------------- 비용·쿨다운
	mMPCost = 50.f;
	mCoolDownTime = 60.f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.MeteorShower"));

	// ---------------------------------------------------------------- 몽타주
	// Blackhole_Start(3.83초) → 2배속 약 1.9초, 이어서 BlackHole_Loop(2.17초) → 2배속 약 1.1초 = 공중 약 3초
	// (11/4) 생성자에서 미리 불러온다. (처음 쓸 때 경로로 불러오면 싸움 중에 디스크를 읽느라 멈칫한다)
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_MeteorShower.AM_Gideon_MeteorShower'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	mPlayRate = 2.f;

	// 시전 중 이동 허용 (10/19 테스트 : 이동 불가는 리스크가 너무 큼). 무적은 여전히 없음.
	SetAllowMoveWhileCasting(true);

	// (11/4) 공중에 떠 있는 동안 회피 불가. Q·F와 달리 E로 끊을 수 없다 = 궁극기의 리스크.
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.NoDodge")));

	// 공중 유지 동작. 시작 몽타주가 끝나면 OnMontageFinished가 즉석 몽타주로 이어 재생한다.
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		LoopAsset(TEXT("/Script/Engine.AnimSequence'/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/BlackHole_Loop.BlackHole_Loop'"));

	if (LoopAsset.Succeeded())
		mLoopAnim = LoopAsset.Object;
}

void UGameplayAbility_MeteorShower::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 이번 시전은 아직 루프 전. (어빌리티 객체를 재사용하므로 시작할 때마다 되돌린다)
	mLoopStarted = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGameplayAbility_MeteorShower::OnFire(APlayerCharacter* Player)
{
	// 조준한 바닥 (최대 15m).
	FVector	Location = Player->GetAimGroundLocation(mMaxDistance);

	// Owner = 기디언 : 장판이 기디언의 범위 공격 함수로 피해를 준다.
	FActorSpawnParameters	SpawnParams;
	SpawnParams.Owner = Player;
	SpawnParams.Instigator = Player;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 장판의 BeginPlay(타이머 시작)는 SpawnActor 안에서 바로 실행되지만,
	// 첫 피해는 0.3초 뒤라서 아래 SetDamage가 그보다 먼저 들어간다.
	AMeteorShowerArea* Area = Player->GetWorld()->SpawnActor<AMeteorShowerArea>(
		AMeteorShowerArea::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);

	if (IsValid(Area))
		Area->SetDamage(Player->GetAttack() * mPulseDamageScale);
}

void UGameplayAbility_MeteorShower::OnMontageFinished()
{
	// 이미 루프 중이면 무시한다. (시작 몽타주의 "완료" 알림이 루프 도중에 늦게 올 수 있다)
	if (mLoopStarted)
		return;

	// (11/4) 넘어짐·속박·얼음으로 취소돼서 끊겼으면 루프·발사 없이 "취소로" 끝낸다. (부모 버전 = 종료)
	// (11/3까지는 취소도 "몽타주 끝"으로 받아서, 발사 전에 넘어져도 아래 안전장치가 운석을 쐈다)
	// 이미 쓴 마나·쿨다운은 돌려주지 않는다. (Q·F를 E로 끊을 때와 같은 규칙)
	if (mCancelling)
	{
		Super::OnMontageFinished();
		return;
	}

	mLoopStarted = true;

	APlayerCharacter* Player = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());

	UAnimInstance* AnimInstance = IsValid(Player) ? Player->GetMesh()->GetAnimInstance() : nullptr;

	// 죽었거나(사망으로 몽타주가 끊김) 루프 애니메이션이 없으면 그냥 끝낸다. (부모 버전 = 종료)
	if (!IsValid(Player) || Player->IsDeath() || !IsValid(mLoopAnim) || !AnimInstance)
	{
		Super::OnMontageFinished();
		return;
	}

	// 안전장치 : 루프로 넘어가는데 아직 발사를 안 했으면(노티파이가 블렌드아웃 구간에 있어 씹힌 경우) 지금 발사한다.
	// 정상이라면 노티파이(3.43초)가 블렌드아웃 시작(몽타주 블렌드 아웃 0.1초 → 3.63초)보다 먼저 와서 여기는 건너뛴다.
	if (!mFired)
	{
		UE_LOG(UGDebug, Warning, TEXT("MeteorShower : Fire event missed, firing at loop start"));
		OnFireEvent(FGameplayEventData());
	}

	// 애니메이션 하나를 즉석 몽타주로 재생한다. (미니언 사망 동작과 같은 함수)
	// 인자 : 애니메이션, 슬롯, 블렌드 인, 블렌드 아웃, 재생 속도
	// 시작 몽타주와 같은 DefaultSlot이라, 시작 동작이 블렌드아웃되는 동안 자연스럽게 이어진다.
	// 블렌드 인 0.1초 = 시작 몽타주의 블렌드 아웃 시간과 맞춘다.
	AnimInstance->PlaySlotAnimationAsDynamicMontage(mLoopAnim, TEXT("DefaultSlot"),
		0.1f, 0.25f, mPlayRate);

	// 루프 길이(2배속이면 약 1.1초)만큼 기다린 뒤 끝낸다.
	// 즉석 몽타주는 태스크가 아니라서 "끝났다"는 알림을 받기 번거롭다 → 길이만큼 기다리는 태스크를 쓴다.
	float	LoopTime = mLoopAnim->GetPlayLength() / mPlayRate;

	UAbilityTask_WaitDelay* WaitLoop = UAbilityTask_WaitDelay::WaitDelay(this, LoopTime);
	WaitLoop->OnFinish.AddDynamic(this, &UGameplayAbility_MeteorShower::OnLoopFinished);
	WaitLoop->ReadyForActivation();
}

void UGameplayAbility_MeteorShower::OnLoopFinished()
{
	// 부모 버전 = 어빌리티 종료 (State.Casting·Rooted가 떨어진다)
	Super::OnMontageFinished();
}
