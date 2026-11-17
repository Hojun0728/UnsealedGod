// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_TornSpace.h"
#include "../../Player/PlayerCharacter.h"
// GAS가 제공하는 "어빌리티 태스크" : 어빌리티 안에서 기다려야 하는 일(몽타주 끝, 이벤트 도착)을 처리한다.
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_TornSpace::UGameplayAbility_TornSpace()
{
	// ---------------------------------------------------------------- 비용·쿨다운 (부모가 처리)
	mMPCost = 20.f;
	mCoolDownTime = 1.5f;
	mCoolDownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.TornSpace"));

	// ---------------------------------------------------------------- 태그
	// ActivationOwnedTags : 이 어빌리티가 켜져 있는 동안 캐릭터(ASC)에게 자동으로 붙는 태그.
	// 어빌리티가 끝나면(EndAbility) GAS가 자동으로 떼 준다 → 떼는 걸 깜빡하는 버그가 생기지 않는다.
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dodging")));

	// ActivationBlockedTags : 캐릭터에게 이 태그가 붙어 있으면 이 어빌리티는 발동되지 않는다.
	// (11/4) R 시전 중(State.NoDodge)에만 회피할 수 없다. 궁극기 = 각을 재서 쓰는 리스크.
	// (10/16에는 Q·F·R 모두 State.Casting으로 막았다 → Q·F는 회피로 끊을 수 있게 바꿈)
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.NoDodge")));

	// CancelAbilitiesWithTag : 이 어빌리티가 "발동에 성공하는 순간" 이 태그를 가진 어빌리티를 취소한다.
	// Q·F(Ability.Skill.DodgeCancel)를 쓰는 중에 E → 스킬 동작을 끊고 바로 순간이동.
	// 마나 부족·쿨다운으로 회피가 실패하면 GAS가 취소도 하지 않는다 → "회피가 실제로 나갈 때만" 스킬이 끊긴다.
	// (물약처럼 먼저 끊고 회피를 시도하면, 회피가 실패해도 스킬만 날아간다)
	// 이미 쓴 마나·쿨다운은 돌려주지 않는다. 발사한 운석·저주탄은 독립된 액터라 그대로 날아간다.
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.DodgeCancel")));

	// ---------------------------------------------------------------- 에셋
	// 회피 몽타주. 에디터에서 Content/UnsealedGod/Player/AM_Gideon_TornSpace로 만든다.
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_TornSpace.AM_Gideon_TornSpace'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	// 기디언 팩의 Portal 폴더 이펙트.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		EnterFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Portal/FX/P_Portal_Teleport_Enter.P_Portal_Teleport_Enter'"));

	if (EnterFX.Succeeded())
		mEnterParticle = EnterFX.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ExitFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Portal/FX/P_Portal_Teleport_Exit.P_Portal_Teleport_Exit'"));

	if (ExitFX.Succeeded())
		mExitParticle = ExitFX.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		TeleportSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Gideon_Teleport_1.S_SFX_Gideon_Teleport_1'"));

	if (TeleportSnd.Succeeded())
		mTeleportSound = TeleportSnd.Object;
}

void UGameplayAbility_TornSpace::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 부모 : 비용(마나 20)·쿨다운(1.5초) 확정. 실패하면 부모가 이미 EndAbility를 불렀다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	// 어빌리티를 실행한 몸(Avatar)을 플레이어 캐릭터로 형변환한다.
	APlayerCharacter* PlayerChar = Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get());

	if (!IsValid(PlayerChar) || !IsValid(mMontage))
	{
		UE_LOG(UGDebug, Warning, TEXT("TornSpace : Invalid Character or Montage"));

		// bWasCancelled = true : 정상 종료가 아니라 취소됐다는 의미.
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ---------------------------------------------------------------- 도착 지점
	mDestination = PlayerChar->CalcDodgeDestination(mDistance);

	// 출발 지점에 포털 이펙트.
	if (IsValid(mEnterParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(PlayerChar->GetWorld(), mEnterParticle,
			PlayerChar->GetActorLocation(), PlayerChar->GetActorRotation());
	}

	// 회피는 누르는 즉시 반응해야 하므로 순간이동 이벤트가 아니라 발동 순간에 튼다.
	// 캡슐(루트)에 붙인다 → 6m 순간이동해도 소리가 출발 자리에 남지 않는다
	if (IsValid(mTeleportSound))
		UGameplayStatics::SpawnSoundAttached(mTeleportSound, PlayerChar->GetRootComponent());

	// ---------------------------------------------------------------- 태스크 1 : Teleport 이벤트 기다리기
	// 몽타주의 "Send Gameplay Event (Event.Dodge.Teleport)" 노티파이가 보낸 이벤트를 기다린다.
	// OnlyTriggerOnce = true : 한 번 받으면 태스크가 끝난다.
	UAbilityTask_WaitGameplayEvent* WaitTeleport = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Dodge.Teleport")), nullptr, true);

	WaitTeleport->EventReceived.AddDynamic(this, &UGameplayAbility_TornSpace::OnTeleportEvent);

	// ReadyForActivation : 태스크를 실제로 시작한다. (만들기만 하고 이걸 안 부르면 동작하지 않는다)
	WaitTeleport->ReadyForActivation();

	// ---------------------------------------------------------------- 태스크 2 : 몽타주 재생하고 끝날 때까지 기다리기
	// 몽타주는 DefaultSlot(전신)을 쓰고, 공격 몽타주(UpperBody)와 같은 그룹이라
	// 공격 중에 회피하면 엔진이 공격 몽타주를 자동으로 멈춘다. → "공격을 회피로 캔슬"
	// 부모(UGameplayAbility_Base) : 완료·블렌드아웃·중단·취소 중 어느 것이 와도 어빌리티를 끝낸다.
	PlayMontageAndEnd(mMontage, mPlayRate);
}

void UGameplayAbility_TornSpace::OnTeleportEvent(FGameplayEventData Payload)
{
	// GetAvatarActorFromActorInfo : 이 어빌리티를 실행 중인 몸(캐릭터)
	AActor* Avatar = GetAvatarActorFromActorInfo();

	if (!IsValid(Avatar))
		return;

	// 도착 지점으로 즉시 이동한다.
	// 두 번째 인자 false : 이동 경로에서 충돌 검사를 하지 않는다(이미 계산할 때 막히는 곳을 피했다).
	// ETeleportType::TeleportPhysics : "순간이동"이라고 알려서 물리·이동 속도가 튀지 않게 한다.
	Avatar->SetActorLocation(mDestination, false, nullptr, ETeleportType::TeleportPhysics);

	// 도착 지점에 포털 이펙트.
	if (IsValid(mExitParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(Avatar->GetWorld(), mExitParticle,
			mDestination, Avatar->GetActorRotation());
	}
}

void UGameplayAbility_TornSpace::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 안전장치 : 무적 구간(노티파이 스테이트) 도중에 몽타주가 끊겨 NotifyEnd가 불리지 않았더라도
	// 회피가 끝나면 무적 태그를 확실히 0개로 만든다. (무적이 영원히 남는 버그 방지)
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->SetLooseGameplayTagCount(
			FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")), 0);
	}

	// 안전장치 2 : 숨김 구간(Hide Mesh Window) 도중에 끊겼더라도 메시를 다시 보이게 한다.
	// ActorInfo->SkeletalMeshComponent : ASC가 기억하고 있는 아바타의 스켈레탈 메시 (기디언의 메시)
	if (ActorInfo && ActorInfo->SkeletalMeshComponent.IsValid())
	{
		ActorInfo->SkeletalMeshComponent->SetVisibility(true, true);
	}

	// 부모 : 태스크 정리, ActivationOwnedTags(State.Dodging) 제거, 어빌리티 종료 알림
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
