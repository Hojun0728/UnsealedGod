// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_PlayerSkill.h"
#include "../../Player/PlayerCharacter.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UGameplayAbility_PlayerSkill::UGameplayAbility_PlayerSkill()
{
	// 시전 중 태그. (클래스 위 설명 참고)
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Casting")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Rooted")));

	// 이미 다른 스킬을 시전 중이거나 회피 중이면 발동하지 않는다.
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Casting")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dodging")));
}

void UGameplayAbility_PlayerSkill::SetAllowMoveWhileCasting(bool bAllow)
{
	FGameplayTag	RootedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Rooted"));

	if (bAllow)
		ActivationOwnedTags.RemoveTag(RootedTag);
	else
		ActivationOwnedTags.AddTag(RootedTag);
}

void UGameplayAbility_PlayerSkill::SetDodgeCancelable()
{
	// 애셋 태그 = "이 어빌리티가 무엇인가"를 나타내는 이름표. (ActivationOwnedTags처럼 캐릭터에게 붙는 태그가 아니다)
	// UE 5.5부터 AbilityTags를 직접 고치지 않고 GetAssetTags로 복사 → 추가 → SetAssetTags(생성자 전용)로 넣는다.
	// GetAssetTags()는 const 참조(읽기 전용)를 돌려주므로, 값으로 복사해 받은 Tags에 추가한다.
	FGameplayTagContainer	Tags = GetAssetTags();
	Tags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Skill.DodgeCancel")));
	SetAssetTags(Tags);
}

void UGameplayAbility_PlayerSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 몽타주가 없으면 마나·쿨다운을 쓰기 "전에" 끝낸다. (부모의 CommitAbility보다 먼저 확인)
	// 순서를 반대로 하면 몽타주가 없는데 마나만 빠지는 문제가 생긴다.
	if (!IsValid(mMontage))
	{
		UE_LOG(UGDebug, Warning, TEXT("%s : Montage not found"), *GetName());

		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 부모 : 비용·쿨다운 확정. 실패하면 부모가 이미 EndAbility를 불렀다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!mAbilityActive)
		return;

	mFired = false;

	// ---------------------------------------------------------------- 태스크 1 : 발사 이벤트 기다리기
	// 세 스킬이 같은 태그(Event.Skill.Fire)를 쓴다. 한 번에 한 스킬만 시전할 수 있어서(State.Casting) 섞이지 않는다.
	UAbilityTask_WaitGameplayEvent* WaitFire = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, FGameplayTag::RequestGameplayTag(TEXT("Event.Skill.Fire")), nullptr, true);

	WaitFire->EventReceived.AddDynamic(this, &UGameplayAbility_PlayerSkill::OnFireEvent);
	WaitFire->ReadyForActivation();

	// ---------------------------------------------------------------- 태스크 2 : 몽타주 재생하고 끝날 때까지 기다리기
	// DefaultSlot(전신)이라 기본 공격 몽타주(UpperBody, 같은 그룹)는 엔진이 자동으로 멈춘다.
	// 부모(UGameplayAbility_Base) : 끝나거나 끊기면 OnMontageFinished → 어빌리티 종료
	PlayMontageAndEnd(mMontage, mPlayRate);
}

void UGameplayAbility_PlayerSkill::OnFireEvent(FGameplayEventData Payload)
{
	APlayerCharacter* Player = Cast<APlayerCharacter>(GetAvatarActorFromActorInfo());

	// 시전 도중에 죽었으면 발사하지 않는다.
	// 이미 발사했으면 다시 발사하지 않는다. (R : 루프 시작 때 대신 발사한 뒤 노티파이가 늦게 오는 경우)
	if (!IsValid(Player) || Player->IsDeath() || mFired)
		return;

	mFired = true;

	// virtual 함수 : 부모 코드에서 불러도 실제 객체(Q·F·R 중 하나)의 OnFire가 실행된다.
	OnFire(Player);
}

void UGameplayAbility_PlayerSkill::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 정상적으로 끝났는데 발사를 안 했다 = 몽타주에 Fire 노티파이가 없거나 태그가 틀렸다.
	// IsActive() : 아직 끝나지 않았는지. (블렌드아웃·완료 델리게이트가 둘 다 와서 EndAbility가 두 번 불려도 로그는 한 번)
	if (IsActive() && !bWasCancelled && mAbilityActive && !mFired)
		UE_LOG(UGDebug, Warning, TEXT("%s : Ended without Event.Skill.Fire (check montage notify)"), *GetName());

	// 부모 : 태스크 정리, ActivationOwnedTags(State.Casting·Rooted) 제거, 어빌리티 종료 알림
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
