// Fill out your copyright notice in the Description page of Project Settings.


#include "Minion.h"
#include "../AI/EnemyController.h"
#include "Sound/SoundBase.h"

AMinion::AMinion()
{
	// ---------------------------------------------------------------- 수치
	// HP 60 : 기디언 기본 공격(10, 막타 15)으로 5~6방. 공격력 5 : 한 대에 5.
	mInitHP = 60.f;
	mInitAttack = 5.f;

	// ---------------------------------------------------------------- AI
	// 보스와 같은 컨트롤러(BT 실행 + Target = 플레이어). 소환되면 자동으로 빙의한다.
	AIControllerClass = AEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ---------------------------------------------------------------- 이동
	// 기디언(600)보다 느리다 → 도망치며 쏠 수 있다. 이동 방향으로 몸을 돌린다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = 450.f;

	// ---------------------------------------------------------------- 몸
	// 미니언 키 약 1.6m. 캡슐 반높이 80, 반지름 35.
	GetCapsuleComponent()->SetCapsuleHalfHeight(80.f);
	GetCapsuleComponent()->SetCapsuleRadius(35.f);

	// 조준선(Visibility)에 걸리도록 (보스와 같은 이유 : 기본 Pawn 프로필은 Visibility를 무시한다)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility,
		ECollisionResponse::ECR_Block);

	// 카메라는 무시 (보스와 같은 이유 : 몸이 카메라 앞에 와도 시점이 확 당겨지지 않게)
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/ParagonMinions/Characters/Minions/Dusk_Minions/Meshes/Minion_Lane_Melee_Dusk.Minion_Lane_Melee_Dusk'"));

	if (MeshAsset.Succeeded())
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);

	// 메시를 캡슐 바닥에 맞추고, 파라곤 메시 방향(Y축 정면)을 X축 정면으로 돌린다. (기디언·세바로그와 같은 이유)
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -80.0), FRotator(0.0, -90.0, 0.0));

	// 애님 블루프린트 (부모 클래스 = UBossAnimInstance 재사용 : 이동 속도·사망만 쓰는 공용 애님 인스턴스)
	static ConstructorHelpers::FClassFinder<UAnimInstance>
		AnimClass(TEXT("/Script/Engine.AnimBlueprint'/Game/UnsealedGod/Minion/ABP_Minion.ABP_Minion_C'"));

	if (AnimClass.Succeeded())
		GetMesh()->SetAnimInstanceClass(AnimClass.Class);

	// ---------------------------------------------------------------- 사망 동작 (Death_A~E)
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathA(TEXT("/Script/Engine.AnimSequence'/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Death_A.Death_A'"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathB(TEXT("/Script/Engine.AnimSequence'/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Death_B.Death_B'"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathC(TEXT("/Script/Engine.AnimSequence'/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Death_C.Death_C'"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathD(TEXT("/Script/Engine.AnimSequence'/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Death_D.Death_D'"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathE(TEXT("/Script/Engine.AnimSequence'/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/Death_E.Death_E'"));

	if (DeathA.Succeeded()) mDeathAnims.Add(DeathA.Object);
	if (DeathB.Succeeded()) mDeathAnims.Add(DeathB.Object);
	if (DeathC.Succeeded()) mDeathAnims.Add(DeathC.Object);
	if (DeathD.Succeeded()) mDeathAnims.Add(DeathD.Object);
	if (DeathE.Succeeded()) mDeathAnims.Add(DeathE.Object);

	// ---------------------------------------------------------------- 이펙트
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		SpawnFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonMinions/FX/Particles/Minions/Shared/P_MinionSpawn.P_MinionSpawn'"));

	if (SpawnFX.Succeeded())
		mSpawnParticle = SpawnFX.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		DeathFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonMinions/FX/Particles/Minions/Minion_melee/FX/Death/P_MeleeMinion_Chunks_Ranged.P_MeleeMinion_Chunks_Ranged'"));

	if (DeathFX.Succeeded())
		mDeathParticle = DeathFX.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		DeathSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Minion_Death.S_SFX_Minion_Death'"));

	if (DeathSnd.Succeeded())
		mDeathSound = DeathSnd.Object;

	// ---------------------------------------------------------------- BT·공격 몽타주 (11/4 : 실행 중 → 생성자에서 미리)
	// 생성자에서 넣으면 AI 컨트롤러가 빙의하며 GetBehaviorTree를 부르기 전에 이미 들어 있다.
	// (그래서 10/15에 만든 PostInitializeComponents 덮어쓰기도 필요 없어져서 지웠다)
	static ConstructorHelpers::FObjectFinder<UBehaviorTree>
		BTAsset(TEXT("/Script/AIModule.BehaviorTree'/Game/UnsealedGod/Minion/BT_Minion.BT_Minion'"));

	if (BTAsset.Succeeded())
		mBehaviorTree = BTAsset.Object;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		AttackAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Minion/AM_Minion_Attack.AM_Minion_Attack'"));

	if (AttackAsset.Succeeded())
		mAttackMontage = AttackAsset.Object;
}

void AMinion::BeginPlay()
{
	// 부모 : GAS 초기화 (HP 60, 공격력 5)
	Super::BeginPlay();

	// 등장 이펙트 (발밑)
	if (IsValid(mSpawnParticle))
	{
		FVector	Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mSpawnParticle, Feet);
	}
}

bool AMinion::Attack(AActor* Target)
{
	if (mDeath || mAttacking || !IsValid(mAttackMontage))
		return false;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
		return false;

	// 타겟을 바라본다. (좌우만)
	if (IsValid(Target))
	{
		FVector	ToTarget = Target->GetActorLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.0, ToTarget.Rotation().Yaw, 0.0));
	}

	// Montage_Play → 재생 길이(0이면 실패). 섹션 중 하나로 바로 점프한다.
	if (AnimInstance->Montage_Play(mAttackMontage) <= 0.f)
		return false;

	if (!mAttackSections.IsEmpty())
	{
		// RandRange(최소, 최대) : 그 사이의 랜덤 정수 (양 끝 포함)
		FName	Section = mAttackSections[FMath::RandRange(0, mAttackSections.Num() - 1)];
		AnimInstance->Montage_JumpToSection(Section, mAttackMontage);
	}

	// 몽타주가 끝나면 알려 달라고 등록한다. (끝나야 다음 행동을 고른다)
	// FOnMontageEnded : 이 몽타주 한 번에만 쓰는 델리게이트(함수 연결)
	FOnMontageEnded	EndDelegate;
	EndDelegate.BindUObject(this, &AMinion::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, mAttackMontage);

	mAttacking = true;

	return true;
}

void AMinion::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	mAttacking = false;
}

void AMinion::Death()
{
	// 부모 : 이동 정지·충돌 끄기 (공중이면 착지 후)
	Super::Death();

	mAttacking = false;

	// AI를 멈춘다. (보스와 같은 이유 : 죽은 뒤에도 쫓아가거나 공격하려 하지 않게)
	AAIController* AIController = Cast<AAIController>(GetController());

	if (IsValid(AIController))
	{
		AIController->StopMovement();

		if (IsValid(AIController->GetBrainComponent()))
			AIController->GetBrainComponent()->StopLogic(TEXT("Death"));
	}

	// 사망 동작 하나를 랜덤으로. 시퀀스를 몽타주 에셋 없이 바로 재생한다.
	// PlaySlotAnimationAsDynamicMontage(애니메이션, 슬롯, 블렌드 인, 블렌드 아웃)
	//   블렌드 아웃 0 : 끝난 자세 그대로 → 그 순간 조각 이펙트와 함께 사라진다.
	float	Length = 0.5f;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (IsValid(AnimInstance) && !mDeathAnims.IsEmpty())
	{
		UAnimSequence* DeathAnim = mDeathAnims[FMath::RandRange(0, mDeathAnims.Num() - 1)];

		AnimInstance->PlaySlotAnimationAsDynamicMontage(DeathAnim, TEXT("DefaultSlot"), 0.1f, 0.f);
		Length = DeathAnim->GetPlayLength();
	}

	// 동작이 끝날 즈음 사라진다.
	GetWorldTimerManager().SetTimer(mDeathTimer, this, &AMinion::OnDeathAnimFinished,
		FMath::Max(Length - 0.05f, 0.05f), false);
}

void AMinion::OnDeathAnimFinished()
{
	// 조각 이펙트 (몸 중심)
	if (IsValid(mDeathParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mDeathParticle, GetActorLocation());

	if (IsValid(mDeathSound))
		UGameplayStatics::PlaySoundAtLocation(this, mDeathSound, GetActorLocation());

	// 사망 디졸브는 11/3 연출 작업에서 (지금은 바로 사라짐)
	Destroy();
}

void AMinion::OnBattleEnd(bool bPlayerWon)
{
	// 부모 : "전투 끝" 표시
	Super::OnBattleEnd(bPlayerWon);

	if (mDeath)
		return;

	// 승리 : 보스가 쓰러지면 남은 미니언도 부서지며 사라진다. (원래 사망 처리 그대로)
	if (bPlayerWon)
	{
		Death();
		return;
	}

	// 패배 : 비헤이비어 트리를 멈추고 그 자리에 선다. 하던 공격도 멈춘다.
	// (10/19 : 둘러싸고 찌르는 연출을 만들었지만 판정 디버그까지 겹쳐 정신없어서 "가만히 서 있기"로 바꿨다)
	AAIController* AIController = Cast<AAIController>(GetController());

	if (IsValid(AIController))
	{
		AIController->StopMovement();

		if (IsValid(AIController->GetBrainComponent()))
			AIController->GetBrainComponent()->StopLogic(TEXT("BattleEnd"));
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (IsValid(AnimInstance))
		AnimInstance->Montage_Stop(0.2f);

	mAttacking = false;
}
