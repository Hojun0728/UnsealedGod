// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Potion.h"
#include "../../Player/PlayerCharacter.h"
// 나이아가라 이펙트 생성 (UNiagaraFunctionLibrary::SpawnSystemAttached)
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Potion::UGameplayAbility_Potion()
{
	// 마나·쿨다운 없음. 대신 개수 제한(CanActivateAbility).

	// 몽타주 조각마다 속도를 따로 정해 두었으므로(3배 / 2배) 몽타주 전체 재생 속도는 1.
	// (11/4) 생성자에서 미리 불러온다. (처음 쓸 때 경로로 불러오면 싸움 중에 디스크를 읽느라 멈칫한다)
	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Player/AM_Gideon_Potion.AM_Gideon_Potion'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	mPlayRate = 1.f;

	// 이동 입력은 받는다 (State.Rooted 빼기). 대신 마시는 중에 이동 키·E를 누르면 플레이어가 물약을 취소한다.
	// → "회복하다 위험하면 끊고 피한다, 그만큼 덜 회복" (10/19 직접 설계)
	SetAllowMoveWhileCasting(true);

	static ConstructorHelpers::FObjectFinder<USoundBase>
		HealSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Potion_Heal.S_SFX_Potion_Heal'"));

	if (HealSnd.Succeeded())
		mHealSound = HealSnd.Object;

	// (11/4) 회복 이펙트도 생성자에서 미리 불러온다. (회복 소리와 같은 방식)
	// 10/21에는 "처음 쓸 때 경로로"(LoadObject) 불러왔다. 그러면 첫 회복 순간 게임을 멈추고 디스크에서 읽는다.
	// 레벨이 바뀌면(얼음 궁전 → 세바로그) 다시 읽어야 해서 세바로그전 첫 회복 때 멈칫했다.
	// ConstructorHelpers로 찾으면 어빌리티 클래스가 이 에셋을 붙잡고 있어서 게임 내내 메모리에 있다.
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>
		AuraFX(TEXT("/Script/Niagara.NiagaraSystem'/Game/UnsealedGod/Player/FX/NS_Heal_Aura.NS_Heal_Aura'"));

	if (AuraFX.Succeeded())
		mAuraSystem = AuraFX.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem>
		CrossFX(TEXT("/Script/Niagara.NiagaraSystem'/Game/UnsealedGod/Player/FX/NS_Heal_Cross.NS_Heal_Cross'"));

	if (CrossFX.Succeeded())
		mCrossSystem = CrossFX.Object;
}

bool UGameplayAbility_Potion::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
		return false;

	// const APlayerCharacter* : 이 함수가 const라서 캐릭터를 "읽기만" 한다. (개수 확인만, 줄이는 건 발동할 때)
	const APlayerCharacter* Player = ActorInfo ? Cast<APlayerCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	return IsValid(Player) && Player->GetPotionCount() > 0;
}

void UGameplayAbility_Potion::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	mHealTicksDone = 0;

	// 부모 : 몽타주 확인 → 확정 → 발사 이벤트 대기 + 몽타주 재생
	// 개수는 여기서 줄이지 않는다. 첫 회복(HP 10)이 들어가는 순간 줄인다. (HealTick)
	// → 링이 닿기 전에 이동·E로 끊으면 물약이 그대로 남는다. (10/19 내 기준)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGameplayAbility_Potion::OnFire(APlayerCharacter* Player)
{
	// 링이 닿는 순간 첫 회복, 이후 mHealInterval초마다 나머지 회복
	HealTick();

	if (mHealTicksDone < mHealTickCount)
	{
		Player->GetWorldTimerManager().SetTimer(mHealTimer, this, &UGameplayAbility_Potion::HealTick,
			mHealInterval, true);
	}

	// ---------------------------------------------------------------- 오라 (10/21)
	// 원작 귀환 파티클(파란 계열)은 뺐다 — 빨간 회복 이펙트와 색이 섞여서 (내 판단)
	// SpawnSystemAttached(시스템, 붙일 컴포넌트, 소켓, 위치, 회전, 위치 기준, 끝나면 자동 삭제)
	// 캡슐(루트)에 붙인다 : 메시의 애니메이션(링 동작)에 흔들리지 않고 몸 중심을 따라간다.
	if (IsValid(mAuraSystem))
	{
		mAuraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(mAuraSystem, Player->GetRootComponent(),
			NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	}

	// 회복 소리 : 오라와 같은 곳(캡슐)에 붙인다
	if (IsValid(mHealSound))
		mHealAudio = UGameplayStatics::SpawnSoundAttached(mHealSound, Player->GetRootComponent());
}

void UGameplayAbility_Potion::HealTick()
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());

	if (!IsValid(Player) || Player->IsDeath() || mHealTicksDone >= mHealTickCount)
	{
		if (IsValid(Player))
			Player->GetWorldTimerManager().ClearTimer(mHealTimer);

		return;
	}

	// 첫 회복 순간 물약 하나를 쓴다. (10/19 내 기준)
	if (mHealTicksDone == 0)
		Player->ConsumePotion();

	Player->Heal(mHealPerTick);
	++mHealTicksDone;

	// 십자가 하나 (10/21) : HP가 한 번 찰 때마다 가슴 앞에서 떠올랐다 사라진다.
	// SpawnSystemAtLocation : 붙이지 않고 그 자리에 생성 → 몸이 움직여도 십자가는 제자리에서 떠오른다. 끝나면 자동 삭제.
	if (IsValid(mCrossSystem))
	{
		FVector	Location = Player->GetActorLocation() + FVector(0.0, 0.0, mCrossHeight);

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(Player->GetWorld(), mCrossSystem, Location,
			Player->GetActorRotation());
	}

	if (mHealTicksDone >= mHealTickCount)
	{
		Player->GetWorldTimerManager().ClearTimer(mHealTimer);
		StopAura();
	}
}

void UGameplayAbility_Potion::StopAura()
{
	// Deactivate : 새 입자를 그만 만든다. 이미 나온 빛은 자연스럽게 사라지고, 다 사라지면 컴포넌트가 지워진다(자동 삭제).
	if (IsValid(mAuraComponent))
		mAuraComponent->Deactivate();

	mAuraComponent = nullptr;
}

void UGameplayAbility_Potion::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 끊겼으면(큰 피격) 남은 회복을 멈춘다 → 그때까지 회복한 만큼만 남는다.
	// 정상 종료일 때도 남은 회복이 있으면 마저 준다. (몽타주가 훑는 구간보다 조금 일찍 끝나는 경우 대비)
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;

	if (IsValid(Avatar))
		Avatar->GetWorldTimerManager().ClearTimer(mHealTimer);

	// 오라 끄기 (끊겼으면 즉시 사라져서 "덜 회복했다"가 보인다)
	StopAura();

	// 끊겼으면 소리도 짧게 줄이며 끈다 (회복이 멈춘 걸 소리로도)
	if (bWasCancelled && IsValid(mHealAudio))
		mHealAudio->FadeOut(0.2f, 0.f);

	mHealAudio = nullptr;

	if (IsActive() && !bWasCancelled && mFired)
	{
		while (mHealTicksDone < mHealTickCount)
		{
			APlayerCharacter* Player = Cast<APlayerCharacter>(Avatar);

			if (!IsValid(Player) || Player->IsDeath())
				break;

			Player->Heal(mHealPerTick);
			++mHealTicksDone;
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
