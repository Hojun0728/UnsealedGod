// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../../GameInfo.h"
#include "GameplayEffect_CoolDown.generated.h"

// ============================================================================
// UGameplayEffect_CoolDown
// 스킬 쿨다운용 GameplayEffect다. (수업 코드와 동일)
// 지속시간 동안 "쿨다운 태그"(예 : Cooldown.TornSpace)를 캐릭터에게 붙여 둔다.
// 어빌리티는 발동 전에 "이 태그가 붙어 있나?"를 보고 쿨다운 중인지 판단한다.
//
// 지속시간은 스킬마다 다르므로 "GameplayEffect.CoolDown" 태그로 넘겨준다(SetByCaller).
// 붙일 태그도 스킬마다 다르므로 어빌리티가 적용할 때 DynamicGrantedTags로 넣어준다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API UGameplayEffect_CoolDown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGameplayEffect_CoolDown();
};
