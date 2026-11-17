// Fill out your copyright notice in the Description page of Project Settings.


#include "Gideon.h"
#include "../Projectile/PortalBlastProjectile.h"
#include "../GAS/Ability/GameplayAbility_TornSpace.h"
#include "../GAS/Ability/GameplayAbility_PortalMeteor.h"
#include "../GAS/Ability/GameplayAbility_Burden.h"
#include "../GAS/Ability/GameplayAbility_MeteorShower.h"
#include "../GAS/Ability/GameplayAbility_Knockdown.h"
#include "../GAS/Ability/GameplayAbility_Bound.h"
#include "../GAS/Ability/GameplayAbility_Frozen.h"
#include "../GAS/Ability/GameplayAbility_Potion.h"

// Sets default values
AGideon::AGideon()
{
	// 자식 생성자가 실행되기 전에 부모(APlayerCharacter) 생성자가 먼저 자동으로 실행된다.
	// 그래서 여기서는 mArm, mCamera가 이미 만들어진 상태이고, 기디언 전용 설정만 하면 된다.

	// 기디언 스켈레탈 메시를 불러온다.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/ParagonGideon/Characters/Heroes/Gideon/Meshes/Gideon.Gideon'"));

	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);
	}

	// 캡슐 크기 (충돌·이동 판정용 원기둥). HalfHeight = 높이의 절반.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);

	// 메시 위치 맞추기.
	// 캡슐의 중심은 캐릭터 허리쯤이고, 메시의 기준점은 발바닥이다.
	// 그래서 메시를 캡슐 절반 높이만큼 내려야 발이 바닥에 닿는다.
	// 파라곤 메시는 앞쪽이 Y축을 보고 있어서, Yaw를 -90도 돌려 캐릭터 앞(X축)과 맞춘다.
	// (테스트해서 발이 뜨거나 묻히면 Z값을, 옆을 보고 있으면 Yaw값을 조정한다)
	GetMesh()->SetRelativeLocation(FVector(0.0, 0.0, -96.0));
	GetMesh()->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));

	// 애니메이션 블루프린트 클래스를 얻어온다.
	// FClassFinder : 에셋 "객체"가 아니라 "클래스"를 찾는다. (애님 블루프린트는 클래스로 쓴다)
	// 클래스 정보를 찾아올 때 경로의 가장 끝에 _C를 무조건 붙여야 한다.
	static ConstructorHelpers::FClassFinder<UAnimInstance>
		PlayerAnim(TEXT("/Script/Engine.AnimBlueprint'/Game/UnsealedGod/Player/ABP_Gideon.ABP_Gideon_C'"));

	if (PlayerAnim.Succeeded())
		GetMesh()->SetAnimInstanceClass(PlayerAnim.Class);

	// ---------------------------------------------------------------- 기본 공격 (포털 블라스트)
	// 발사할 투사체 클래스를 정한다. StaticClass() : 객체가 아니라 클래스 정보를 넘긴다.
	mProjectileClass = APortalBlastProjectile::StaticClass();

	// 발사할 때 손에서 번쩍이는 이펙트.
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		MuzzleFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonGideon/FX/Particles/Gideon/Abilities/Primary/FX/P_Gideon_Primary_MuzzleFlash.P_Gideon_Primary_MuzzleFlash'"));

	if (MuzzleFX.Succeeded())
		mMuzzleParticle = MuzzleFX.Object;

	// ---------------------------------------------------------------- 회피 (E 토른 스페이스)
	mDodgeAbilityClass = UGameplayAbility_TornSpace::StaticClass();

	// ---------------------------------------------------------------- 스킬 (10/16)
	// Q 포털 운석(한 방 폭발) / F 버든(지속 피해) / R 운석 비(궁극기, 공중 시전)
	mSkillQClass = UGameplayAbility_PortalMeteor::StaticClass();
	mSkillFClass = UGameplayAbility_Burden::StaticClass();
	mSkillRClass = UGameplayAbility_MeteorShower::StaticClass();

	// ---------------------------------------------------------------- 피격 (10/16)
	// 큰 공격 : Knock_Fwd(날아감) → Stun_End(무릎 → 일어섬). 작은 공격은 APlayerCharacter가 HitReact로 처리
	mKnockdownAbilityClass = UGameplayAbility_Knockdown::StaticClass();

	// 보스 속박에 묶임 (10/21) : Levitate 동작으로 공중에 묶인 채 보스의 일격을 기다린다
	mBoundAbilityClass = UGameplayAbility_Bound::StaticClass();

	// 오로라의 얼음에 갇힘 (10/28) : 그 자세 그대로 멈추고, WASD 연타 16번으로 깨고 나온다 (풀린 뒤 3초 얼음 내성)
	mFrozenAbilityClass = UGameplayAbility_Frozen::StaticClass();

	// ---------------------------------------------------------------- 회복 물약 (10/19, 1번 키)
	mPotionAbilityClass = UGameplayAbility_Potion::StaticClass();

	// ---------------------------------------------------------------- 승리 동작 (10/19, 직접 선택)
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		VictoryAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonGideon/Characters/Heroes/Gideon/Animations/Emote_Taunt_PraiseMe_T1.Emote_Taunt_PraiseMe_T1'"));

	if (VictoryAnim.Succeeded())
		mVictoryAnim = VictoryAnim.Object;
}
