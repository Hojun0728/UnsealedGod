// Fill out your copyright notice in the Description page of Project Settings.


#include "IceDecoy.h"
#include "../GAS/BaseAttributeSet.h"
#include "Sound/SoundBase.h"

AIceDecoy::AIceDecoy()
{
	// 매 프레임 기디언과의 거리를 본다 (폭발 예고 판단)
	PrimaryActorTick.bCanEverTick = true;

	// 체력 30 = 평타 3발 (10/28 직접 결정). 공격력은 보스가 생성 직후 넘겨준다(SetAttackFrom).
	mInitHP = 30.f;

	// ---------------------------------------------------------------- AI
	// 비헤이비어 트리 없이 "기디언에게 걸어가기"만 하므로 기본 AI 컨트롤러로 충분하다. (미니언은 BT가 있어 AEnemyController)
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0, 360.0, 0.0);
	GetCharacterMovement()->MaxWalkSpeed = mWalkSpeed;

	// ---------------------------------------------------------------- 외형 (오로라와 같은 메시·캡슐·애님 블루프린트)
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora.Aurora'"));

	if (MeshAsset.Succeeded())
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);

	GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);

	// 파라곤 메시 방향 보정 + 발을 캡슐 바닥에 (보스는 OnConstruction이 하지만 분신은 여기서 고정)
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -96.0), FRotator(0.0, -90.0, 0.0));

	// 기디언의 조준선(Visibility)이 몸에 걸리게, 카메라는 통과하게 (보스·미니언과 같은 설정)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	static ConstructorHelpers::FClassFinder<UAnimInstance>
		AnimClass(TEXT("/Script/Engine.AnimBlueprint'/Game/UnsealedGod/Boss/Aurora/ABP_Aurora.ABP_Aurora_C'"));

	if (AnimClass.Succeeded())
		GetMesh()->SetAnimInstanceClass(AnimClass.Class);

	// 얼음 머티리얼은 생성한 보스가 넘겨준다(SetIceMaterial). 오로라 디테일에서 바꿀 수 있게. (10/28 : 팩의 분신 머티리얼은 검게 나와서 교체)

	// ---------------------------------------------------------------- 이펙트·소리
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		SpawnFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Leap/FX/P_Aurora_Decoy_Spawn.P_Aurora_Decoy_Spawn'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		DamagedFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Leap/FX/P_Aurora_Decoy_Damaged.P_Aurora_Decoy_Damaged'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		FuseFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Leap/FX/P_Aurora_Decoy_Frost.P_Aurora_Decoy_Frost'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ExplodeFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Leap/FX/P_Aurora_Decoy_Explode_Fragments.P_Aurora_Decoy_Explode_Fragments'"));

	if (SpawnFX.Succeeded())	mSpawnParticle = SpawnFX.Object;
	if (DamagedFX.Succeeded())	mDamagedParticle = DamagedFX.Object;
	if (FuseFX.Succeeded())		mFuseParticle = FuseFX.Object;
	if (ExplodeFX.Succeeded())	mExplodeParticle = ExplodeFX.Object;

	// 소리 (10/29 : 마법 효과음 팩 → AI 효과음. 예고·폭발·부서짐을 소리로 구분)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		FuseSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Decoy_Fuse.S_SFX_Decoy_Fuse'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		ExplodeSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Decoy_Explode.S_SFX_Decoy_Explode'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		BreakSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_Break.S_SFX_Ice_Break'"));

	if (FuseSnd.Succeeded())	mFuseSound = FuseSnd.Object;
	if (ExplodeSnd.Succeeded())	mExplodeSound = ExplodeSnd.Object;
	if (BreakSnd.Succeeded())	mBreakSound = BreakSnd.Object;
}

void AIceDecoy::BeginPlay()
{
	// 부모 : GAS 초기화 (체력 30)
	Super::BeginPlay();

	// 보스(오로라)와 같은 크기
	SetActorScale3D(FVector(1.3));

	GetCharacterMovement()->MaxWalkSpeed = mWalkSpeed;

	if (IsValid(mSpawnParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mSpawnParticle, GetActorLocation(), GetActorRotation(),
			GetActorScale3D());

	ChasePlayer();
}

void AIceDecoy::SetIceMaterial(UMaterialInterface* Material)
{
	mIceMaterial = Material;

	if (!IsValid(mIceMaterial))
		return;

	// 모든 머티리얼 슬롯을 같은 얼음으로 → 통째로 얼음 조각처럼 보인다
	for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
		GetMesh()->SetMaterial(i, mIceMaterial);
}

void AIceDecoy::SetAttackFrom(float BossAttack)
{
	mInitAttack = BossAttack;

	if (IsValid(mAttributeSet))
		mAttributeSet->SetAttack(BossAttack);
}

void AIceDecoy::ChasePlayer()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	// MoveToActor(목표, 도착 거리) : 내비 메시로 길을 찾아 목표를 따라간다. 목표가 움직이면 길을 다시 찾는다.
	if (IsValid(AIController) && IsValid(Player))
		AIController->MoveToActor(Player, mTriggerDistance * 0.5f);
}

void AIceDecoy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (mDeath || mFusing || mBattleEnded)
		return;

	mLifeElapsed += DeltaTime;

	// 수명이 다하면 그 자리에서 폭발 예고
	if (mLifeElapsed >= mLifeTime)
	{
		StartFuse();
		return;
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (!IsValid(Player))
		return;

	// 기디언이 가까우면 폭발 예고 (수평 거리)
	FVector	ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.0;

	if (ToPlayer.Size() <= mTriggerDistance)
	{
		StartFuse();
		return;
	}

	// 0.5초마다 길찾기를 다시 요청한다. (처음 요청이 실패했거나, 기디언이 회피로 멀리 순간이동한 경우 대비)
	mRepathElapsed += DeltaTime;

	if (mRepathElapsed >= 0.5f)
	{
		mRepathElapsed = 0.f;
		ChasePlayer();
	}
}

void AIceDecoy::StartFuse()
{
	if (mFusing || mDeath)
		return;

	mFusing = true;

	// 멈춰 선다
	if (AAIController* AIController = Cast<AAIController>(GetController()))
		AIController->StopMovement();

	GetCharacterMovement()->StopMovementImmediately();

	// 빛나며 금이 가는 예고 (몸에 붙임)
	if (IsValid(mFuseParticle))
		UGameplayStatics::SpawnEmitterAttached(mFuseParticle, GetMesh());

	// 0.6초 동안 차오르는 "쩌저적" = 피할 신호. 이 소리가 끝나는 순간 폭발 소리가 이어진다
	if (IsValid(mFuseSound))
		UGameplayStatics::PlaySoundAtLocation(this, mFuseSound, GetActorLocation());

	GetWorldTimerManager().SetTimer(mFuseTimer, this, &AIceDecoy::Explode, FMath::Max(mFuseTime, 0.01f), false);
}

void AIceDecoy::Explode()
{
	if (mDeath)
		return;

	// ---------------------------------------------------------------- 판정 : 반지름 안 + 회피 무적 아님
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (IsValid(Player) && !mBattleEnded)
	{
		FVector	ToPlayer = Player->GetActorLocation() - GetActorLocation();
		ToPlayer.Z = 0.0;

		UAbilitySystemComponent* PlayerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);

		if (ToPlayer.Size() <= mExplodeRadius && PlayerASC &&
			!PlayerASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
		{
			FHitResult	Hit;
			Hit.ImpactPoint = Player->GetActorLocation();
			Hit.Location = Hit.ImpactPoint;

			ApplyDamageToTarget(Player, mAttributeSet->GetAttack() * mExplodeDamageScale, Hit, false, false);

			// 얼어붙음 (서리 고리·얼음 지진과 같은 이벤트·같은 탈출 시스템)
			FGameplayEventData	FreezeEvent;
			FreezeEvent.Instigator = this;
			FreezeEvent.EventMagnitude = mFreezeTime;

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Player,
				FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Frozen")), FreezeEvent);
		}
	}

	if (IsValid(mExplodeSound))
		UGameplayStatics::PlaySoundAtLocation(this, mExplodeSound, GetActorLocation());

	Shatter();
}

void AIceDecoy::OnDamaged(float Damage, AActor* Attacker, bool bHeavy)
{
	Super::OnDamaged(Damage, Attacker, bHeavy);

	if (!mDeath && IsValid(mDamagedParticle))
		UGameplayStatics::SpawnEmitterAttached(mDamagedParticle, GetMesh());
}

void AIceDecoy::Death()
{
	// 체력 0 = 기디언이 부쉈다 → 터지지 않고 파편만. (부모 : 이동 정지·충돌 끄기)
	Super::Death();

	GetWorldTimerManager().ClearTimer(mFuseTimer);

	if (AAIController* AIController = Cast<AAIController>(GetController()))
		AIController->StopMovement();

	// 부쉈을 때는 폭발보다 가벼운 "쨍그랑" (터지지 않았다는 걸 소리로도)
	if (!IsHidden() && IsValid(mBreakSound))
		UGameplayStatics::PlaySoundAtLocation(this, mBreakSound, GetActorLocation());

	Shatter();
}

void AIceDecoy::OnBattleEnd(bool bPlayerWon)
{
	Super::OnBattleEnd(bPlayerWon);

	// 전투가 끝나면(승패 모두) 그 자리에서 부서진다. 패배 연출 중에 걸어다니지 않게.
	GetWorldTimerManager().ClearTimer(mFuseTimer);
	Shatter();
}

void AIceDecoy::Shatter()
{
	if (IsHidden())
		return;

	if (IsValid(mExplodeParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mExplodeParticle, GetActorLocation(), GetActorRotation(),
			GetActorScale3D());

	// 숨기고 충돌을 끈 뒤 잠깐 뒤 지운다. (같은 프레임에 지우면 아직 남은 타이머·이벤트가 지워진 액터를 건드릴 수 있어서)
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetLifeSpan(0.5f);
}
