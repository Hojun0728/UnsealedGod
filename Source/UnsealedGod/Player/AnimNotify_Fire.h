// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_Fire.generated.h"

// ============================================================================
// UAnimNotify_Fire
// 공격 몽타주의 "손에서 장풍이 나가는 프레임"에 놓는 노티파이다.
// (수업 코드의 UAnimNotify_WukongAttack과 같은 구조 : 노티파이 → 캐릭터 함수 호출)
//
// 수업 코드와 다른 점 : 노티파이가 "어느 손(소켓)에서, 몇 배의 피해로" 쏠지를 직접 가지고 있다.
// 몽타주 에디터에서 노티파이를 클릭하면 디테일 패널에서 두 값을 바꿀 수 있다.
//   A 섹션 : Muzzle_01, 1.0  /  B 섹션 : Muzzle_02, 1.0  /  C 섹션 : Muzzle_03, 1.5
// 그래서 C++ 코드에 "A는 오른손" 같은 규칙을 적지 않아도 되고, 몽타주만 보고 수정할 수 있다.
//
// meta = (DisplayName) : 몽타주 에디터의 "노티파이 추가" 목록에 보일 이름.
// ============================================================================
UCLASS(meta = (DisplayName = "Fire Projectile"))
class UNSEALEDGOD_API UAnimNotify_Fire : public UAnimNotify
{
	GENERATED_BODY()

protected:
	// 투사체가 나갈 스켈레톤 소켓 이름.
	UPROPERTY(EditAnywhere, Category = "Fire")
	FName	mSocketName = TEXT("Muzzle_01");

	// 피해 배율. 캐릭터의 Attack 값에 곱해진다. (3타째는 1.5)
	UPROPERTY(EditAnywhere, Category = "Fire")
	float	mDamageScale = 1.f;

	// 투사체 크기 배율. 투사체의 기본 크기에 곱해진다. (3타째는 1.5 → 1.2 × 1.5 = 1.8배)
	UPROPERTY(EditAnywhere, Category = "Fire")
	float	mProjectileScale = 1.f;

	// (10/21) 강타 : 맞히면 짧은 히트스톱 + 약한 카메라 흔들림. 막타(C)만 체크한다.
	// 보스 판정 노티파이(Melee Hit Window)의 Heavy 체크박스와 같은 방식. 이름 끝에 " Strong"이 붙는다.
	UPROPERTY(EditAnywhere, Category = "Fire", meta = (DisplayName = "Strong"))
	bool	mStrongHit = false;

public:
	// 애니메이션이 이 노티파이 위치를 지나갈 때 엔진이 호출한다.
	// MeshComp : 이 애니메이션을 재생 중인 스켈레탈 메시 컴포넌트의 주소
	virtual void Notify(USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	// 몽타주 타임라인에 표시될 이름. "Fire Muzzle_01 x1.0"처럼 보이게 해서 구분하기 쉽게 한다.
	virtual FString GetNotifyName_Implementation() const override;
};
