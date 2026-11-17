// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_Pattern_IceCharge.h"
#include "AbilitySystemComponent.h"
#include "../../Boss/BossBase.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Sound/SoundBase.h"

UGameplayAbility_Pattern_IceCharge::UGameplayAbility_Pattern_IceCharge()
{
	// 부모(세바로그 돌진) 생성자가 먼저 실행된 뒤 여기서 오로라 값으로 덮어쓴다.
	// 사용 거리 4~16m (10/28 밸런스 : 세바로그 12m보다 멀리서도. 아레나 대부분에서 돌진이 온다). 쿨다운 태그는 부모(Cooldown.BossCharge)
	mMaxRange = 1600.f;

	// 쿨다운 4초 (10/28 밸런스 : 세바로그 돌진 5초보다 자주)
	mCoolDownTime = 4.f;

	// 질주 속도 18m/s (세바로그 20m/s. 사람 크기·첫 보스라 조금 느리게)
	mChargeSpeed = 1800.f;

	// 1.0배속. 준비 시간 = 돌진 시작 노티파이 위치(14프레임) ÷ 30fps ≈ 0.47초. 예고가 다 차는 순간 = 출발 (둘이 같아야 한다)
	mMontagePlayRate = 1.f;

	// 2페이즈 1.25배속 (10/28 밸런스) : 준비 0.47초 → 0.37초. 예고 데칼도 같은 속도로 차오른다(부모가 ToRealTime으로 계산)
	mFastPlayRate = 1.25f;
	mWindupTime = 14.f / 30.f;

	// 마무리 망치가 없으므로 목표 지점에서 멈추지 않고 "뚫고 지나간다".
	// 부모는 질주 거리 = (기디언까지 거리 - 적정 타격 거리)라서, 음수를 넣으면 그만큼 더 달린다. (3m 더)
	mStrikeDistance = -300.f;
	mMaxChargeDistance = 2000.f;	// 16m + 뚫고 지나가는 3m를 다 달릴 수 있게

	// 예고 사각형 : 반 폭 1m (몸통 판정 반지름 70 + 기디언 캡슐 34 ≈ 104), 멈춘 뒤 더 그릴 길이 1m
	mPathHalfWidth = 100.f;
	mPathExtraLength = 100.f;

	// 후딜 없음 (10/28 내 판단) : 멈추는 동작(Ability_Q_Stop)이 끝나면 바로 다음 패턴
	// (얼어붙었으면 다음 패턴이 바로 들어온다. 부모(세바로그 돌진)의 기본값은 0.8 ± 0.4초)
	mRecoveryTime = 0.f;
	mRecoveryRandom = 0.f;

	// 도착하면 Stop 섹션(Ability_Q_Stop)으로. 마무리 공격이 없으니 도착하자마자 예고를 지운다.
	mSwingSectionName = TEXT("Stop");
	mSwingHitTime = 0.f;

	static ConstructorHelpers::FObjectFinder<UAnimMontage>
		MontageAsset(TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_IceCharge.AM_Aurora_IceCharge'"));

	if (MontageAsset.Succeeded())
		mMontage = MontageAsset.Object;

	// ---------------------------------------------------------------- 이펙트 (오로라 팩 Dash 폴더)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		WindupFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Dash_WarmUp.P_Aurora_Dash_WarmUp'"));

	if (WindupFX.Succeeded())
		mWindupParticle = WindupFX.Object;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ShieldBaseFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Wall_Base.P_Aurora_Wall_Base'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ShieldTopFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Wall_Top.P_Aurora_Wall_Top'"));

	if (ShieldBaseFX.Succeeded())
		mShieldParticles.Add(ShieldBaseFX.Object);

	if (ShieldTopFX.Succeeded())
		mShieldParticles.Add(ShieldTopFX.Object);

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		BreakBaseFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_Wall_Base_Destroy.P_Aurora_Wall_Base_Destroy'"));
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		BreakTopFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Dash/FX/P_Aurora_e_Wall_Top_Destroy.P_Aurora_e_Wall_Top_Destroy'"));

	if (BreakBaseFX.Succeeded())
		mShieldBreakParticles.Add(BreakBaseFX.Object);

	if (BreakTopFX.Succeeded())
		mShieldBreakParticles.Add(BreakTopFX.Object);

	// ---------------------------------------------------------------- 방패 메시 (얼음 가시 세 조각을 같은 자리에 겹침)
	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		ShieldBottom(TEXT("/Script/Engine.StaticMesh'/Game/ParagonAurora/FX/Meshes/Aurora/SM_FrostShield_Spikey_Bottom.SM_FrostShield_Spikey_Bottom'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		ShieldMiddle(TEXT("/Script/Engine.StaticMesh'/Game/ParagonAurora/FX/Meshes/Aurora/SM_FrostShield_Spikey_Middle.SM_FrostShield_Spikey_Middle'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		ShieldTop(TEXT("/Script/Engine.StaticMesh'/Game/ParagonAurora/FX/Meshes/Aurora/SM_FrostShield_Spikey_Top.SM_FrostShield_Spikey_Top'"));

	if (ShieldBottom.Succeeded())
		mShieldMeshes.Add(ShieldBottom.Object);

	if (ShieldMiddle.Succeeded())
		mShieldMeshes.Add(ShieldMiddle.Object);

	if (ShieldTop.Succeeded())
		mShieldMeshes.Add(ShieldTop.Object);

	// ---------------------------------------------------------------- 소리 (10/29)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		WindupSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_IceCharge_Windup.S_SFX_IceCharge_Windup'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		DashSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_IceCharge_Dash.S_SFX_IceCharge_Dash'"));

	if (WindupSnd.Succeeded())	mWindupSound = WindupSnd.Object;
	if (DashSnd.Succeeded())	mDashSound = DashSnd.Object;
}

void UGameplayAbility_Pattern_IceCharge::PlayAttachedSound(USoundBase* Sound)
{
	ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	// SpawnSoundAttached : 메시에 붙여 재생 → 달리는 동안 소리가 몸을 따라온다 (한 자리에 남지 않는다)
	if (IsValid(Boss) && IsValid(Sound))
		UGameplayStatics::SpawnSoundAttached(Sound, Boss->GetMesh());
}

void UGameplayAbility_Pattern_IceCharge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 같은 어빌리티 객체를 재사용하므로 지난 돌진의 상태를 비운다.
	mActiveParticles.Empty();
	mShieldMeshComponents.Empty();
	mGuarding = false;
	mChargeCount = 0;

	// 이번에 몇 번 달릴지 : 1페이즈 1~2번 / 2페이즈 2~3번 랜덤 (10/28 내 판단). RandRange(최소, 최대) : 둘 다 포함
	const ABossBase* PhaseBoss = Cast<ABossBase>(ActorInfo->AvatarActor.Get());
	bool	bPhase2 = IsValid(PhaseBoss) && PhaseBoss->GetPhase() >= 2;

	mChargesThisTime = bPhase2 ? FMath::RandRange(mPhase2MinCharges, mPhase2MaxCharges) :
		FMath::RandRange(mPhase1MinCharges, mPhase1MaxCharges);

	// 부모 : 방향·거리 고정, 사각형 예고, 몽타주(준비 동작), 돌진 시작 노티파이 대기
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!IsActive())
		return;

	// 준비 동작 동안 기 모으기 이펙트를 몸에 붙인다.
	ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());

	if (IsValid(Boss) && IsValid(mWindupParticle))
	{
		UParticleSystemComponent* FX = UGameplayStatics::SpawnEmitterAttached(mWindupParticle, Boss->GetMesh());

		if (IsValid(FX))
			mActiveParticles.Add(FX);
	}

	// 준비 소리 = 멀리서도 "온다"를 알리는 예고
	PlayAttachedSound(mWindupSound);
}

void UGameplayAbility_Pattern_IceCharge::OnChargeStartEvent(FGameplayEventData Payload)
{
	// 기 모으기를 끄고 방패를 켠 뒤 → 부모가 질주를 시작한다.
	// (순서 주의 : 부모는 이미 가까우면 바로 StartSwing을 부른다 → 그 안에서 방패가 다시 꺼진다)
	ClearParticles();
	SetGuard(true, false);

	PlayAttachedSound(mDashSound);

	Super::OnChargeStartEvent(Payload);
}

void UGameplayAbility_Pattern_IceCharge::StartSwing()
{
	// ---------------------------------------------------------------- 2페이즈 연속 돌진 (10/28)
	// 도착했는데 이번 페이즈의 돌진 횟수가 남았으면 멈추지 않고 다시 조준해서 한 번 더 달린다.
	if (++mChargeCount < mChargesThisTime)
	{
		BeginRecharge();
		return;
	}

	// 도착 : 방패가 부서지며 꺼지고 → 부모가 Stop 섹션으로 넘긴다.
	SetGuard(false, true);

	Super::StartSwing();
}

void UGameplayAbility_Pattern_IceCharge::BeginRecharge()
{
	ABossBase* Boss = Cast<ABossBase>(GetAvatarActorFromActorInfo());
	AActor* Target = IsValid(Boss) ? Boss->GetTarget() : nullptr;

	if (!IsValid(Boss) || !IsValid(Target))
	{
		// 다시 조준할 대상이 없으면 그냥 멈춘다
		SetGuard(false, true);
		Super::StartSwing();
		return;
	}

	// 지난 돌진의 예고를 지우고, 지금 기디언 쪽으로 다시 돌아선다. (방패·Run 자세는 그대로 = 숨 고르지 않고 바로 이어짐)
	DestroyWarningDecal();
	FaceTarget();

	// 방향·거리를 "지금" 다시 고정한다. (부모 ActivateAbility와 같은 계산)
	FVector	BossLocation = Boss->GetActorLocation();
	FVector	ToTarget = Target->GetActorLocation() - BossLocation;
	ToTarget.Z = 0.0;

	mChargeDirection = ToTarget.GetSafeNormal();
	mChargeDistance = FMath::Clamp(ToTarget.Size() - mStrikeDistance, 0.f, mMaxChargeDistance);

	// 짧은 사각형 예고 (첫 돌진보다 짧게 = 두 번째는 더 빨리 반응해야 한다)
	float	TotalLength = mChargeDistance + mPathExtraLength;
	float	HalfHeight = Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Center = BossLocation + mChargeDirection * (TotalLength * 0.5f) - FVector(0.0, 0.0, HalfHeight);

	// 부모 함수 (11/4) : 돌진과 같은 선 머티리얼로 사각형 예고
	SpawnWarningBox(Center, mChargeDirection.Rotation(), TotalLength * 0.5f, mPathHalfWidth, mRechargeWindup);

	UAbilityTask_WaitDelay* WaitRecharge = UAbilityTask_WaitDelay::WaitDelay(this, mRechargeWindup);
	WaitRecharge->OnFinish.AddDynamic(this, &UGameplayAbility_Pattern_IceCharge::OnRechargeReady);
	WaitRecharge->ReadyForActivation();

	// 다시 조준할 때도 준비 소리 → 한 번 더 온다는 걸 소리로도 알린다
	PlayAttachedSound(mWindupSound);
}

void UGameplayAbility_Pattern_IceCharge::OnRechargeReady()
{
	PlayAttachedSound(mDashSound);

	// 부모의 "질주 시작"을 바로 부른다. (이 클래스의 덮어쓴 버전은 방패 이펙트를 지우므로 건너뛴다 = 방패는 켜진 그대로)
	// UGameplayAbility_Pattern_Charge:: : 부모 버전을 콕 집어 부른다. (virtual이어도 이렇게 쓰면 부모 것이 불린다)
	UGameplayAbility_Pattern_Charge::OnChargeStartEvent(FGameplayEventData());
}

void UGameplayAbility_Pattern_IceCharge::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 달리는 도중에 끊겨도(보스 사망·전투 끝) 방패 태그와 이펙트를 꼭 정리한다.
	SetGuard(false, false);
	ClearParticles();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGameplayAbility_Pattern_IceCharge::SetGuard(bool bGuard, bool bBreak)
{
	if (mGuarding == bGuard)
		return;

	mGuarding = bGuard;

	ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!IsValid(Boss) || !IsValid(ASC))
		return;

	FGameplayTag	GuardTag = FGameplayTag::RequestGameplayTag(TEXT("State.FrontGuard"));

	if (bGuard)
	{
		// AddLooseGameplayTag : 이펙트 없이 태그만 직접 붙인다. (무적 태그와 같은 방식)
		// 투사체가 맞을 때 ABossBase::TryBlockFrontHit이 이 태그와 각도를 확인한다.
		ASC->AddLooseGameplayTag(GuardTag);

		// 방패 이펙트를 몸 앞(mShieldOffset)에 붙인다. 캡슐(루트)에 붙이므로 보스가 달리면 같이 간다.
		// KeepRelativeOffset : 붙이는 대상 기준의 상대 위치로 해석한다.
		for (UParticleSystem* Particle : mShieldParticles)
		{
			if (!IsValid(Particle))
				continue;

			UParticleSystemComponent* FX = UGameplayStatics::SpawnEmitterAttached(Particle, Boss->GetRootComponent(),
				NAME_None, mShieldOffset, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset);

			if (IsValid(FX))
				mActiveParticles.Add(FX);
		}

		// 방패 메시 : 게임 중에 컴포넌트를 새로 만들어 붙인다. (수업의 "생성자 밖에서 컴포넌트 만들기")
		// NewObject(주인) → 메시 지정 → 붙이기 → RegisterComponent(월드에 등록해야 보이기 시작한다)
		for (UStaticMesh* Mesh : mShieldMeshes)
		{
			if (!IsValid(Mesh))
				continue;

			UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(Boss);

			MeshComp->SetStaticMesh(Mesh);

			// 장식 규칙 : 충돌 없음 + 그림자 끔 + 예고 데칼 안 받음 (판정은 태그·각도로 하고, 메시는 보이기만 한다)
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComp->SetCastShadow(false);
			MeshComp->bReceivesDecals = false;

			MeshComp->SetupAttachment(Boss->GetRootComponent());
			MeshComp->SetRelativeLocationAndRotation(mShieldOffset, mShieldMeshRotation);
			MeshComp->SetRelativeScale3D(FVector(mShieldMeshScale));
			MeshComp->RegisterComponent();

			mShieldMeshComponents.Add(MeshComp);
		}

		return;
	}

	ASC->RemoveLooseGameplayTag(GuardTag);

	// 방패 메시를 지운다. (부서지는 이펙트가 그 자리를 덮는다)
	for (UStaticMeshComponent* MeshComp : mShieldMeshComponents)
	{
		if (IsValid(MeshComp))
			MeshComp->DestroyComponent();
	}

	mShieldMeshComponents.Empty();

	// 방패가 있던 자리(월드 위치)에서 부서지는 이펙트
	if (bBreak)
	{
		// TransformPosition : 보스 기준 상대 위치 → 월드 위치 (크기 1.3배·회전까지 반영)
		FVector	BreakLocation = Boss->GetActorTransform().TransformPosition(mShieldOffset);

		for (UParticleSystem* Particle : mShieldBreakParticles)
		{
			if (IsValid(Particle))
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particle, BreakLocation,
					Boss->GetActorRotation(), Boss->GetActorScale3D());
		}
	}

	ClearParticles();
}

void UGameplayAbility_Pattern_IceCharge::ClearParticles()
{
	// Deactivate : 새 입자를 그만 만든다. 이미 나온 입자가 사라지면 컴포넌트가 스스로 지워진다. (SpawnEmitter의 자동 삭제)
	for (UParticleSystemComponent* FX : mActiveParticles)
	{
		if (IsValid(FX))
			FX->Deactivate();
	}

	mActiveParticles.Empty();
}
