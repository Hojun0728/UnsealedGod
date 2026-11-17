// Fill out your copyright notice in the Description page of Project Settings.


#include "BossBase.h"
#include "UltimateLight.h"
#include "BossAnimInstance.h"
#include "../AI/EnemyController.h"
#include "../GAS/BaseAttributeSet.h"
#include "../GAS/Ability/GameplayAbility_BossPattern.h"
#include "../Player/PlayerCharacter.h"
#include "../GameMode/ArenaGameMode.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"

ABossBase::ABossBase()
{
	// ---------------------------------------------------------------- AI
	// AIControllerClass : 이 Pawn에 자동으로 빙의할 AI 컨트롤러 클래스. (수업 코드와 동일)
	// AutoPossessAI : 레벨에 놓여 있든, 게임 중 생성되든 AI 컨트롤러가 자동으로 빙의한다.
	AIControllerClass = AEnemyController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ---------------------------------------------------------------- 충돌
	// 언리얼 기본 Pawn 프로필은 Visibility 채널을 "무시"한다.
	// 그런데 기디언의 조준은 Visibility로 선을 쏘고(GetAimTarget), 세바로그 팩에는 물리 에셋도 없다.
	// 그대로 두면 조준선이 보스 몸을 통과해 뒤쪽 벽을 조준점으로 잡아서, 보스 가장자리를 쏘면 빗나간다.
	// → 보스 캡슐만 Visibility를 "막도록" 바꾼다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility,
		ECollisionResponse::ECR_Block);

	// 기본 Pawn 프로필은 Camera 채널을 "막는다". 기디언 카메라의 스프링 암은 Camera 채널로 선을 쏴서
	// 막히면 카메라를 앞으로 당기므로, 보스 몸이 카메라와 기디언 사이에 들어오면 시점이 확 가까워졌다. (10/14)
	// → 보스 캡슐·메시는 Camera를 "무시"한다. 벽·기둥(WorldStatic)은 그대로 카메라를 막는다.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera,
		ECollisionResponse::ECR_Ignore);

	// ---------------------------------------------------------------- 이동
	// 보스는 플레이어와 달리 컨트롤러(마우스) 방향이 아니라 "이동하는 방향"으로 몸을 돌린다.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 몸을 돌리는 속도(초당 각도). 너무 빠르면 순간 회전처럼 보이고, 너무 느리면 둔해 보인다.
	GetCharacterMovement()->RotationRate = FRotator(0.0, 360.0, 0.0);

	// 걷는 속도. 기디언(앞 600 / 옆 550 / 뒤 500)보다 느리거나 같다.
	// 크게 거리를 좁히는 건 도약·돌진 패턴이 한다. 페이즈마다 조금씩 빨라진다(mPhaseWalkSpeeds).
	GetCharacterMovement()->MaxWalkSpeed = 500.f;	// 1페이즈 값. 실제 값은 BeginPlay·EnterPhase에서 mPhaseWalkSpeeds로 정한다

	// ---------------------------------------------------------------- Tick (10/19)
	// 점진 성장·사망 후 사라지기에만 Tick을 쓴다. 쓸 수는 있게 하되(bCanEverTick) 처음엔 꺼 둔다.
	// 필요할 때 SetActorTickEnabled(true)로 켜고, 끝나면 다시 끈다. → 평소엔 매 프레임 비용이 없다.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// 사망·패배·순간이동 연출 에셋(동작·이펙트)은 보스마다 달라서 자식 생성자가 채운다. (10/26 ASevarog로 이동)
}

void ABossBase::BeginPlay()
{
	// 부모 : GAS 초기화 (주인 지정 → HP 감지 등록 → 초기값)
	Super::BeginPlay();

	// 시작 크기 적용. (기본 크기 × 1.0)
	SetBossScale(1.f);

	// 걷는 속도 = 1페이즈 값 (레벨 디테일에서 mPhaseWalkSpeeds를 바꿨으면 그 값이 적용되도록 시작할 때 넣는다)
	if (!mPhaseWalkSpeeds.IsEmpty())
		GetCharacterMovement()->MaxWalkSpeed = mPhaseWalkSpeeds[0];

	mAnimInst = Cast<UBossAnimInstance>(GetMesh()->GetAnimInstance());

	// 머티리얼 슬롯마다 다이내믹 머티리얼 인스턴스를 만들어 둔다. (수업 AMonsterBaseGAS와 같은 방식)
	// CreateDynamicMaterialInstance(슬롯 번호) : 그 슬롯의 머티리얼을 부모로 하는 인스턴스를 만들어 슬롯에 바로 적용한다.
	// (10/15 : SetScalarParameterValueOnMaterials는 오래된 파라곤 머티리얼에서 적용되지 않아 직접 만든다)
	for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
	{
		UMaterialInstanceDynamic* Material = GetMesh()->CreateDynamicMaterialInstance(i);

		if (IsValid(Material))
			mMaterials.Add(Material);
	}

	// 기둥(액터 태그 "Pillar")을 찾아 둔다. 점프 중에 통과하는 데 쓴다.
	// GetAllActorsWithTag : 레벨에서 그 태그가 붙은 액터를 모두 찾는다. (매번 찾으면 느리므로 시작할 때 한 번)
	TArray<AActor*>	Pillars;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Pillar"), Pillars);

	for (AActor* Pillar : Pillars)
		mPillars.Add(Pillar);

	UE_LOG(UGDebug, Warning, TEXT("Boss : found %d pillars"), mPillars.Num());

	// 패턴 어빌리티를 전부 ASC에 등록한다. 등록해야 발동할 수 있다.
	// for (자료형 변수 : 배열) : 배열의 원소를 하나씩 꺼내 반복한다.
	for (const TSubclassOf<UGameplayAbility_BossPattern>& PatternClass : mPatternClasses)
	{
		if (PatternClass)
			mASC->GiveAbility(FGameplayAbilitySpec(PatternClass, 1));
	}

	// 첫 지침 기준 (4~6 랜덤)
	mOverloadThreshold = FMath::RandRange(mOverloadMin, FMath::Max(mOverloadMin, mOverloadMax));

	// [테스트용] 시작 페이즈가 1이 아니면 그 페이즈부터 시작한다. (10/19)
	// 다음 프레임에 하는 이유 : 기디언(플레이어)이 이 시점에 아직 생성되지 않았을 수 있어서, 물약 지급이 빠질 수 있다.
	// SetTimerForNextTick : 이번 프레임이 끝나고 다음 프레임 시작에 한 번 부른다.
	if (mDebugStartPhase > 1)
		GetWorldTimerManager().SetTimerForNextTick(this, &ABossBase::ApplyDebugStartPhase);
}

void ABossBase::ApplyDebugStartPhase()
{
	if (mDeath)
		return;

	// 페이즈 2 ~ 4. 필살기 기준 칸 수 + 1을 넘지 않게
	int32	StartPhase = FMath::Clamp(mDebugStartPhase, 2, mUltimateHPRatios.Num() + 1);

	// 1. 이미 지난 필살기는 건너뛴다. (HP를 깎기 "전에" 해야 HP 변화 감지가 필살기를 예약하지 않는다)
	//    3페이즈 시작 = 75%·50% 칸을 지남 → 다음 확인 칸은 25%
	mNextUltimateIndex = StartPhase - 1;
	mPendingUltimatePhase = 0;

	// 2. 페이즈 진입 (태그·공격력·속도·발광·미니언 소환 대기·물약 +1)
	EnterPhase(StartPhase);

	// 크기는 서서히 커지는 대신 바로 목표 크기로
	mGrowing = false;
	SetBossScale(mScaleTo);

	// 3. HP = 이 페이즈가 시작되는 비율 (3페이즈 = 50%). 칸 번호 = 페이즈 - 2
	float	StartRatio = mUltimateHPRatios[StartPhase - 2];
	mAttributeSet->SetHP(mAttributeSet->GetHPMax() * StartRatio);

	// 4. 물약 : 1페이즈부터 안 썼을 때와 같은 개수 (시작 1 + 지나온 페이즈마다 1).
	//    EnterPhase가 이미 1개를 줬으므로 나머지(StartPhase - 2개)만 더 준다.
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
		Player->AddPotion(StartPhase - 2);

	UE_LOG(UGDebug, Warning, TEXT("Boss : DEBUG start phase %d (HP %.0f%%)"), StartPhase, StartRatio * 100.f);

	// 화면 글자는 디버그 표시가 켜져 있을 때만 (10/21). 출력 로그에는 항상 남는다.
	if (GEngine && ShouldDrawDebug())
	{
		GEngine->AddOnScreenDebugMessage(7, 5.f, FColor::Yellow,
			FString::Printf(TEXT("DEBUG : Start Phase %d"), StartPhase));
	}
}

TSubclassOf<UGameplayAbility_BossPattern> ABossBase::SelectPattern(AActor* Target) const
{
	if (mDeath || !IsValid(Target))
		return nullptr;

	// 후보 목록과 각각의 가중치.
	TArray<TSubclassOf<UGameplayAbility_BossPattern>>	Candidates;
	TArray<float>										Weights;

	for (const TSubclassOf<UGameplayAbility_BossPattern>& PatternClass : mPatternClasses)
	{
		if (!PatternClass)
			continue;

		// GetDefaultObject : 이 클래스의 기본 객체(CDO). 패턴의 설정값(거리·가중치)을 읽는 용도.
		const UGameplayAbility_BossPattern* PatternCDO =
			PatternClass->GetDefaultObject<UGameplayAbility_BossPattern>();

		// 1. 거리 조건 (패턴이 스스로 판단)
		if (!PatternCDO->CanUsePattern(this, Target))
			continue;

		// 2. GAS 조건 : 쿨다운, 비용, 필요한 태그(페이즈), 막는 태그(그로기)
		// FindAbilitySpecFromClass : ASC에 등록된 이 어빌리티의 정보(Spec)를 찾는다.
		FGameplayAbilitySpec* Spec = mASC->FindAbilitySpecFromClass(PatternClass);

		if (!Spec || !PatternCDO->CanActivateAbility(Spec->Handle, mASC->AbilityActorInfo.Get()))
			continue;

		Candidates.Add(PatternClass);
		Weights.Add(PatternCDO->GetWeight());
	}

	if (Candidates.IsEmpty())
		return nullptr;

	// 우선순위 : 후보 중 가장 높은 우선순위만 남긴다. (필살기 10 > 일반 패턴 0)
	int32	MaxPriority = INT32_MIN;

	for (const TSubclassOf<UGameplayAbility_BossPattern>& PatternClass : Candidates)
	{
		MaxPriority = FMath::Max(MaxPriority,
			PatternClass->GetDefaultObject<UGameplayAbility_BossPattern>()->GetPriority());
	}

	// 뒤에서부터 지운다 : 앞에서부터 지우면 지운 뒤 칸이 당겨져서 다음 칸을 건너뛴다.
	for (int32 i = Candidates.Num() - 1; i >= 0; --i)
	{
		if (Candidates[i]->GetDefaultObject<UGameplayAbility_BossPattern>()->GetPriority() < MaxPriority)
		{
			Candidates.RemoveAt(i);
			Weights.RemoveAt(i);
		}
	}

	// 같은 패턴 연속 금지 : 다른 후보가 있으면 바로 전에 쓴 패턴은 뺀다.
	if (Candidates.Num() > 1)
	{
		int32	LastIndex = Candidates.Find(mLastPattern);

		// INDEX_NONE(-1) : 찾지 못했다는 뜻
		if (LastIndex != INDEX_NONE)
		{
			Candidates.RemoveAt(LastIndex);
			Weights.RemoveAt(LastIndex);
		}
	}

	// 가중치 랜덤 : 가중치를 다 더한 범위에서 무작위 값을 뽑고, 그 값이 속한 칸의 패턴을 고른다.
	//   예) 가중치 [1, 3] → 0~4 중 무작위 → 0~1이면 첫 번째(25%), 1~4면 두 번째(75%)
	float	TotalWeight = 0.f;

	for (float Weight : Weights)
		TotalWeight += Weight;

	float	Pick = FMath::FRandRange(0.f, TotalWeight);

	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		Pick -= Weights[i];

		if (Pick <= 0.f)
			return Candidates[i];
	}

	// 부동소수점 오차로 반복문을 빠져나온 경우 마지막 후보.
	return Candidates.Last();
}

bool ABossBase::ActivatePattern(TSubclassOf<UGameplayAbility_BossPattern> PatternClass)
{
	if (!PatternClass || mDeath)
		return false;

	// 새 패턴 시작 : 지난 후딜 기록을 지운다.
	mPatternRecoveryEndTime = 0.f;

	if (!mASC->TryActivateAbilityByClass(PatternClass))
		return false;

	mCurrentPattern = PatternClass;
	mLastPattern = PatternClass;

	// 과부하 (10/19) : 3페이즈부터 일반 패턴(우선순위 0 : 3연타·도약·돌진)을 쓸 때마다 +1.
	// 필살기·소환·지침 자체는 세지 않는다. 기준에 닿으면 지침 대기 → 다음 선택에서 지침 패턴(우선순위 8)이 뽑힌다.
	if (mPhase >= mOverloadPhase && !mExhaustPending &&
		PatternClass->GetDefaultObject<UGameplayAbility_BossPattern>()->GetPriority() == 0)
	{
		++mOverloadCount;

		if (mOverloadCount >= mOverloadThreshold)
			mExhaustPending = true;
	}

	return true;
}

void ABossBase::ConsumeExhaust()
{
	mExhaustPending = false;
	mOverloadCount = 0;

	// 다음 기준을 다시 뽑는다. (4~6) 매번 같은 횟수면 너무 기계적이라 조금씩 다르게.
	mOverloadThreshold = FMath::RandRange(mOverloadMin, FMath::Max(mOverloadMin, mOverloadMax));
}

bool ABossBase::IsPatternActive() const
{
	if (!mCurrentPattern)
		return false;

	FGameplayAbilitySpec* Spec = mASC->FindAbilitySpecFromClass(mCurrentPattern);

	// IsActive : 이 어빌리티가 지금 실행 중인지 (EndAbility가 불리면 false)
	return Spec && Spec->IsActive();
}

void ABossBase::StartPatternRecovery(float Seconds)
{
	// GetTimeSeconds : 게임이 시작된 뒤 흐른 시간(일시정지·슬로모션 반영). "지금 + 후딜" = 끝나는 시각
	mPatternRecoveryEndTime = GetWorld()->GetTimeSeconds() + FMath::Max(Seconds, 0.f);
}

bool ABossBase::IsInPatternRecovery() const
{
	return GetWorld()->GetTimeSeconds() < mPatternRecoveryEndTime;
}

void ABossBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 메시 높이 = 캡슐 바닥(-반높이) + 띄우는 높이.
	// GetUnscaledCapsuleHalfHeight : 크기 배율을 적용하기 "전"의 반높이. 메시 위치(상대 위치)도
	// 크기 배율 적용 전 값이라 단위를 맞춘 것이다. (최종 화면에서는 둘 다 크기 배율만큼 커진다)
	// X·Y는 0 그대로, 회전(-90도)은 생성자에서 정한 값 그대로 둔다.
	float	MeshZ = -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + mHoverHeight;

	GetMesh()->SetRelativeLocation(FVector(0.0, 0.0, MeshZ));
}

void ABossBase::SetBossScale(float Multiplier)
{
	// 액터 크기를 바꾸면 캡슐·메시가 같은 비율로 커진다. (캡슐이 커지니 판정도 같이 커진다)
	// FVector(값) : X·Y·Z를 모두 같은 값으로 채운 벡터 (균등 스케일)
	// 크기를 바꾸기 전·후의 캡슐 반높이 차이만큼 위로 올린다.
	// 액터는 캡슐 "가운데"를 기준으로 커지므로, 그냥 키우면 캡슐 아래쪽이 바닥에 파묻힌다.
	float	OldHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	mScaleMultiplier = Multiplier;

	SetActorScale3D(FVector(mBaseScale * Multiplier));

	float	NewHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	AddActorWorldOffset(FVector(0.0, 0.0, NewHalfHeight - OldHalfHeight));
}

void ABossBase::OnHPChanged(const FOnAttributeChangeData& Data)
{
	// (10/21) 보스 HP·페이즈 표시는 HUD가 매 프레임 읽어 간다. (예전 화면 6번 줄 디버그 글자 삭제)

	// 필살기 기준 확인 : 체력 비율이 다음 기준 이하로 내려갔으면 "필살기 대기".
	// while : 한 번에 여러 기준을 넘어도(76% → 49%) 기준 칸은 모두 지나가고, 대기는 하나만 남긴다.
	//         대신 들어갈 페이즈는 가장 높은 것으로 (75%·50%를 한 번에 넘으면 3페이즈 → 페이즈를 건너뛰지 않게)
	float	HPMax = mAttributeSet->GetHPMax();

	if (!mDeath && Data.NewValue > 0.f && HPMax > 0.f)
	{
		float	Ratio = Data.NewValue / HPMax;

		while (mUltimateHPRatios.IsValidIndex(mNextUltimateIndex) &&
			Ratio <= mUltimateHPRatios[mNextUltimateIndex])
		{
			// 칸 번호 0(75%) → 2페이즈, 1(50%) → 3페이즈, 2(25%) → 4페이즈
			mPendingUltimatePhase = mNextUltimateIndex + 2;

			UE_LOG(UGDebug, Warning, TEXT("Ultimate pending : HP %.0f%% → Phase %d"),
				mUltimateHPRatios[mNextUltimateIndex] * 100.f, mPendingUltimatePhase);

			++mNextUltimateIndex;
		}
	}

	// 부모 : 체력이 0이 되면 Death()
	Super::OnHPChanged(Data);
}

void ABossBase::EnterPhase(int32 NewPhase)
{
	// 낮아지는 페이즈, 같은 페이즈, 죽은 뒤는 무시
	if (NewPhase <= mPhase || mDeath)
		return;

	// "느슨한(Loose) 태그" : GameplayEffect 없이 코드에서 직접 붙였다 떼는 태그.
	// 지나가는 페이즈의 태그를 모두 붙인다. (3페이즈면 Boss.Phase.2와 Boss.Phase.3)
	// → 패턴의 ActivationRequiredTags에 Boss.Phase.2를 넣으면 "2페이즈 이상"에서만 쓸 수 있다.
	for (int32 Phase = mPhase + 1; Phase <= NewPhase; ++Phase)
	{
		FString	TagName = FString::Printf(TEXT("Boss.Phase.%d"), Phase);

		mASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName(*TagName)));
	}

	mPhase = NewPhase;

	// 페이즈별 크기와 공격력. 칸 번호 = 페이즈 - 1 (없으면 마지막 칸 값)
	int32	Index = mPhase - 1;

	// 크기는 바로 바꾸지 않고 목표만 정한 뒤 Tick이 mScaleGrowTime초 동안 서서히 키운다. (10/19)
	if (!mPhaseScaleMultipliers.IsEmpty())
	{
		mScaleFrom = mScaleMultiplier;
		mScaleTo = mPhaseScaleMultipliers[FMath::Min(Index, mPhaseScaleMultipliers.Num() - 1)];
		mScaleElapsed = 0.f;
		mGrowing = true;

		SetActorTickEnabled(true);
	}

	if (!mPhaseAttackMultipliers.IsEmpty())
	{
		float	Multiplier = mPhaseAttackMultipliers[FMath::Min(Index, mPhaseAttackMultipliers.Num() - 1)];

		// 공격력은 기본값(mInitAttack) × 배율로 다시 정한다. (곱하기를 누적하지 않게)
		mAttributeSet->SetAttack(mInitAttack * Multiplier);
	}

	// 갑옷 발광 : BeginPlay에서 만든 다이내믹 머티리얼들에 HammerUp을 설정한다.
	// (파라미터가 없는 머티리얼(망치·몸통 등)은 설정해도 아무 일도 없다 → 갑옷 머티리얼만 반응)
	if (!mPhaseHammerGlow.IsEmpty())
	{
		float	Glow = mPhaseHammerGlow[FMath::Min(Index, mPhaseHammerGlow.Num() - 1)];

		SetMaterialsScalar(TEXT("HammerUp"), Glow);

		// 확인용 : 첫 번째 머티리얼에서 다시 읽어 본 값 (갑옷이 첫 슬롯이 아니면 0일 수 있다)
		float	ReadBack = -1.f;

		if (!mMaterials.IsEmpty() && IsValid(mMaterials[0]))
			mMaterials[0]->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("HammerUp")), ReadBack);

		UE_LOG(UGDebug, Warning, TEXT("Boss : HammerUp %.2f on %d materials (slot0 read %.2f)"),
			Glow, mMaterials.Num(), ReadBack);
	}

	// 어깨 장식(Stage_1~4)은 애님 인스턴스가 페이즈를 읽어서 ABP에서 바꾼다.

	// 페이즈별 걷는 속도 (500 / 510 / 530 / 550)
	if (!mPhaseWalkSpeeds.IsEmpty())
		GetCharacterMovement()->MaxWalkSpeed = mPhaseWalkSpeeds[FMath::Min(Index, mPhaseWalkSpeeds.Num() - 1)];

	// 회복 물약 +1 : 페이즈마다 하나씩 (10/19 밸런스). 안 쓴 건 이어진다.
	if (APlayerCharacter* Player = Cast<APlayerCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)))
		Player->AddPotion(1);

	// 미니언 소환 대기 : 이번 페이즈의 소환 수, 페이즈 시작 시각 (mSummonDelay초 뒤 소환 패턴이 쓴다)
	if (mPhaseSummonCounts.IsValidIndex(Index))
		mPendingSummonCount = mPhaseSummonCounts[Index];

	mPhaseStartTime = GetWorld()->GetTimeSeconds();

	// 페이즈 전환 소리 (10/26) : 2페이즈부터. (1페이즈는 시작이라 등장 연출이 대신한다)
	// 대사 칸 번호 = 페이즈 - 2 (2페이즈 = 0번 칸). 칸이 모자라면 마지막 칸을 쓴다.
	if (mPhase >= 2)
	{
		if (!mPhaseVoices.IsEmpty())
		{
			USoundBase* Voice = mPhaseVoices[FMath::Min(mPhase - 2, mPhaseVoices.Num() - 1)];

			if (IsValid(Voice))
				UGameplayStatics::SpawnSoundAttached(Voice, GetMesh());
		}

		if (IsValid(mPhaseStinger))
			UGameplayStatics::PlaySound2D(this, mPhaseStinger);
	}

	UE_LOG(UGDebug, Warning, TEXT("Boss : Phase %d (Scale x%.2f, Attack %.0f)"),
		mPhase, GetActorScale3D().X, mAttributeSet->GetAttack());

	// 자식 보스만의 페이즈 처리 (10/28 : 오로라 = 얼음 여제 모습)
	OnPhaseEntered(mPhase);
}

void ABossBase::SetIgnorePillars(bool bIgnore)
{
	// IgnoreActorWhenMoving : 캡슐이 움직일 때(스윕) 이 액터와 부딪히지 않게 한다. 바닥 찾기에도 적용된다.
	for (const TWeakObjectPtr<AActor>& Pillar : mPillars)
	{
		if (Pillar.IsValid())
			GetCapsuleComponent()->IgnoreActorWhenMoving(Pillar.Get(), bIgnore);
	}
}

void ABossBase::TeleportToGround(const FVector& Location)
{
	// 위에서 아래로 선을 쏴서 바닥 높이를 찾는다. 기둥과 보스 자신은 무시. (기둥 위가 아니라 바닥에 서게)
	FVector	Start(Location.X, Location.Y, GetActorLocation().Z + 2000.0);
	FVector	End(Location.X, Location.Y, GetActorLocation().Z - 5000.0);

	FHitResult				Hit;
	FCollisionQueryParams	Params(NAME_None, false, this);

	for (const TWeakObjectPtr<AActor>& Pillar : mPillars)
	{
		if (Pillar.IsValid())
			Params.AddIgnoredActor(Pillar.Get());
	}

	if (!GetWorld()->LineTraceSingleByObjectType(Hit, Start, End,
		FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic), Params))
		return;

	// (11/6) 그 자리가 기둥·벽 안이면 아레나 중앙 쪽으로 당긴 빈 바닥으로 (망령·속박이 기둥에 붙은 기디언 옆에 나타날 때).
	// 기둥 안에 나타나면 충돌이 몸을 밀어내면서 기둥 위로 올라가 버렸다
	FVector	Ground = FindOpenGround(Hit.ImpactPoint, GetArenaCenter());

	// 바닥 + 캡슐 반높이 = 캡슐 중심 위치
	float	HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// TeleportTo : 순간이동. 속도·물리 상태를 안전하게 정리해 준다.
	TeleportTo(Ground + FVector(0.0, 0.0, HalfHeight + 2.0), GetActorRotation());

	UE_LOG(UGDebug, Warning, TEXT("Boss : TeleportToGround (%s)"), *Location.ToString());
}

FVector ABossBase::FindOpenGround(const FVector& Desired, const FVector& Toward) const
{
	// Desired에서 Toward 쪽으로 25cm씩 옮겨 가며, 보스 캡슐이 기둥·벽(WorldStatic)과 겹치지 않는 첫 바닥 지점을 찾는다.
	// 바닥 높이는 기둥을 무시하고 아래로 선을 쏴서 (기둥 윗면이 아니라 진짜 바닥)
	float	HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float	Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();

	FCollisionQueryParams	FloorParams(NAME_None, false, this);

	for (const TWeakObjectPtr<AActor>& Pillar : mPillars)
	{
		if (Pillar.IsValid())
			FloorParams.AddIgnoredActor(Pillar.Get());
	}

	// 겹침 검사는 기둥도 포함 (자기 자신만 무시)
	FCollisionQueryParams		OverlapParams(NAME_None, false, this);
	FCollisionObjectQueryParams	StaticObjects(ECollisionChannel::ECC_WorldStatic);
	FCollisionShape				Capsule = FCollisionShape::MakeCapsule(Radius, HalfHeight);

	FVector	Step = Toward - Desired;
	Step.Z = 0.0;

	float	Distance = Step.Size();
	Step = Distance > 1.f ? Step / Distance : FVector::ZeroVector;

	const float	StepLength = 25.f;
	int32		StepCount = FMath::CeilToInt(Distance / StepLength);

	for (int32 i = 0; i <= StepCount; ++i)
	{
		FVector	Point = Desired + Step * FMath::Min(i * StepLength, Distance);

		FHitResult	FloorHit;
		FVector		Start(Point.X, Point.Y, GetActorLocation().Z + 2000.0);
		FVector		End(Point.X, Point.Y, GetActorLocation().Z - 5000.0);

		if (!GetWorld()->LineTraceSingleByObjectType(FloorHit, Start, End, StaticObjects, FloorParams))
			continue;

		// OverlapAnyTestByObjectType : 그 자리에 이 모양을 놓으면 무언가와 겹치는지만 빠르게 확인 (바닥에서 2cm 띄워서 바닥과는 안 겹치게)
		FVector	CapsuleCenter = FloorHit.ImpactPoint + FVector(0.0, 0.0, HalfHeight + 2.0);

		if (!GetWorld()->OverlapAnyTestByObjectType(CapsuleCenter, FQuat::Identity, StaticObjects, Capsule, OverlapParams))
		{
			if (i > 0)
				UE_LOG(UGDebug, Warning, TEXT("Boss : landing spot moved off pillar/wall (%.0f cm)"), i * StepLength);

			return FloorHit.ImpactPoint;
		}
	}

	// 끝까지 못 찾으면 원래 자리 (Toward가 빈 곳이면 마지막 칸에서 찾았을 것)
	return FVector(Desired.X, Desired.Y, Desired.Z);
}

bool ABossBase::CanSummonMinions() const
{
	return mPendingSummonCount > 0 && !mDeath &&
		GetWorld()->GetTimeSeconds() - mPhaseStartTime >= mSummonDelay;
}

int32 ABossBase::ConsumePendingSummon()
{
	int32	Count = mPendingSummonCount;

	mPendingSummonCount = 0;

	return Count;
}

int32 ABossBase::ConsumePendingUltimate()
{
	int32	Phase = mPendingUltimatePhase;

	mPendingUltimatePhase = 0;

	return Phase;
}

void ABossBase::Death()
{
	// 부모 : mDeath = true, 이동 정지, 캡슐 충돌 끄기
	Super::Death();

	UE_LOG(UGDebug, Warning, TEXT("Boss Death : %s"), *GetName());

	// 필살기 기 모으는 중에 죽었으면 고조 소리도 끊는다. (10/26)
	StopUltimateChargeSound();

	// 쓰러지는 비명 (10/29)
	if (IsValid(mDeathVoice))
		UGameplayStatics::SpawnSoundAttached(mDeathVoice, GetMesh());

	// 패턴 몽타주를 쓰는 중에 죽었으면 멈춘다. (쓰러지는 동작은 ABP가 Death_front로)
	if (IsValid(mAnimInst))
		mAnimInst->Montage_Stop(0.2f);

	// AI를 멈춘다. 멈추지 않으면 죽은 뒤에도 비헤이비어 트리가 계속 돌며 플레이어를 쫓으려 한다.
	// BrainComponent : AI 컨트롤러 안에서 비헤이비어 트리를 실행하는 컴포넌트.
	AAIController* AIController = Cast<AAIController>(GetController());

	if (IsValid(AIController))
	{
		AIController->StopMovement();

		if (IsValid(AIController->GetBrainComponent()))
			AIController->GetBrainComponent()->StopLogic(TEXT("Death"));
	}

	// ---------------------------------------------------------------- 사라지기 예약 (10/19)
	// 쓰러지는 동작(Death_front)이 다 끝날 때쯤 사라지기 시작한다. 애니메이션 길이를 읽어서 맞춘다.
	float	FallTime = IsValid(mDeathAnim) ? mDeathAnim->GetPlayLength() : 2.f;

	GetWorldTimerManager().SetTimer(mDeathFadeTimer, this, &ABossBase::StartDeathFade,
		FMath::Max(FallTime, 0.1f), false);

	// 게임 모드에 알린다 → 모든 캐릭터에게 "승리" + 결과 화면 예약 (10/19)
	// GetAuthGameMode<T>() : 지금 레벨의 게임 모드를 T로 형변환해서 돌려준다. (다른 게임 모드면 nullptr)
	AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();

	if (IsValid(GameMode))
		GameMode->OnBossDeath(this);
}

void ABossBase::OnBattleEnd(bool bPlayerWon)
{
	// 부모 : "전투 끝" 표시
	Super::OnBattleEnd(bPlayerWon);

	// 승리(보스 사망)면 이미 Death에서 다 처리했다.
	if (mDeath || bPlayerWon)
		return;

	// ---------------------------------------------------------------- 패배 : 하던 패턴 멈추기
	// CancelAllAbilities : 실행 중인 패턴 어빌리티를 모두 취소한다. (몽타주·예고 데칼·돌진 힘이 정리된다)
	mASC->CancelAllAbilities();

	// 필살기 기 모으는 중에 기디언이 죽었으면 고조 소리도 끊는다. (10/26)
	StopUltimateChargeSound();

	AAIController* AIController = Cast<AAIController>(GetController());

	if (IsValid(AIController))
	{
		AIController->StopMovement();

		if (IsValid(AIController->GetBrainComponent()))
			AIController->GetBrainComponent()->StopLogic(TEXT("BattleEnd"));
	}

	// ---------------------------------------------------------------- 쓰러진 기디언 쪽으로 돌아서기
	APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);

	if (IsValid(Player))
	{
		FVector	ToPlayer = Player->GetActorLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.0, ToPlayer.Rotation().Yaw, 0.0));
	}

	// ---------------------------------------------------------------- 영혼 뽑기
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	// 영혼 뽑기가 없는 보스(오로라)는 바로 승리 동작
	if (!AnimInstance || !IsValid(mSoulSiphonAnim))
	{
		PlayVictoryEmote();
		return;
	}

	AnimInstance->PlaySlotAnimationAsDynamicMontage(mSoulSiphonAnim, TEXT("DefaultSlot"), 0.25f, 0.2f,
		mSoulSiphonPlayRate);

	// 실제 재생 시간 = 길이 ÷ (재생 속도 × 에셋 속도 스케일)
	float	SiphonTime = mSoulSiphonAnim->GetPlayLength() /
		FMath::Max(mSoulSiphonPlayRate * mSoulSiphonAnim->RateScale, 0.01f);

	// 뽑히는 동안 기디언 몸에 영혼 소용돌이. 다 뽑히면 이펙트를 끈다.
	if (IsValid(Player) && IsValid(mSoulSiphonParticle))
	{
		UParticleSystemComponent* SoulFX = UGameplayStatics::SpawnEmitterAttached(mSoulSiphonParticle,
			Player->GetRootComponent());

		if (IsValid(SoulFX))
		{
			// TWeakObjectPtr : 이펙트가 먼저 사라졌으면 람다 안에서 무효가 되어 안전하다.
			TWeakObjectPtr<UParticleSystemComponent>	WeakFX = SoulFX;
			FTimerHandle	FXTimer;

			// 람다를 타이머에 넣으면 "몇 초 뒤 이 코드 실행"을 함수를 따로 만들지 않고 쓸 수 있다.
			GetWorldTimerManager().SetTimer(FXTimer, [WeakFX]()
			{
				if (WeakFX.IsValid())
					WeakFX->DeactivateSystem();
			}, FMath::Max(SiphonTime, 0.1f), false);
		}
	}

	// 다 뽑으면 승리 동작(박수)
	GetWorldTimerManager().SetTimer(mVictoryTimer, this, &ABossBase::PlayVictoryEmote,
		FMath::Max(SiphonTime - 0.2f, 0.1f), false);
}

void ABossBase::PlayVictoryEmote()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!AnimInstance || !IsValid(mVictoryEmoteAnim))
		return;

	// 인자 : 애니메이션, 슬롯, 블렌드 인, 블렌드 아웃, 재생 속도, 반복 횟수
	AnimInstance->PlaySlotAnimationAsDynamicMontage(mVictoryEmoteAnim, TEXT("DefaultSlot"), 0.2f, 0.3f,
		1.f, FMath::Max(mVictoryEmoteLoopCount, 1));

	// 승리 대사 (10/29, 오로라 = 비웃음). 즉석 몽타주라 노티파이를 못 붙여 코드로
	if (IsValid(mVictoryVoice))
		UGameplayStatics::SpawnSoundAttached(mVictoryVoice, GetMesh());
}

void ABossBase::StartDeathFade()
{
	// 순간이동 중(숨김)에 죽었으면 먼저 보이게 한다. (버든 지속 피해 등으로 숨은 동안 죽을 수 있다)
	SetActorHiddenInGame(false);

	StartFade(0.f, mDeathFadeEnd, mDeathFadeTime, true);

	// 몸 가운데(액터 위치)에서 영혼 불씨 이펙트. 보스 크기만큼 키운다.
	if (IsValid(mDeathFadeParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mDeathFadeParticle,
			GetActorLocation(), GetActorRotation(), GetActorScale3D());
	}
}

void ABossBase::StartFade(float From, float To, float Time, bool bHideAtEnd)
{
	mFadeFrom = From;
	mFadeTo = To;
	mFadeTime = Time;
	mFadeElapsed = 0.f;
	mFadeHideAtEnd = bHideAtEnd;
	mFading = true;

	// 시작 값을 바로 넣는다. (첫 Tick 전에 한 프레임 원래 모습이 보이지 않게)
	SetMaterialsScalar(TEXT("FadeOut"), From);

	SetActorTickEnabled(true);
}

// (10/21) 사라짐·나타남 그림자 이펙트. P_ShadowTrails는 원래 몸에 붙어 계속 흐르는 "반복(루프)" 이펙트라
// 그냥 터뜨리면 영원히 남는다. → 잠깐 뒤 Deactivate(새 입자 그만 만들기)를 불러, 남은 연기가 흩어지면 스스로 지워지게 한다.
// static 함수 : 이 cpp 파일 안에서만 쓰는 도우미 (헤더를 고치지 않아 라이브 코딩으로 적용 가능)
static void SpawnVanishParticle(ABossBase* Boss, UParticleSystem* Particle)
{
	if (!IsValid(Boss) || !IsValid(Particle))
		return;

	// SpawnEmitterAtLocation은 기본으로 "끝나면 스스로 삭제(bAutoDestroy)" 설정이다. 반복 이펙트는 끝나지 않을 뿐.
	UParticleSystemComponent* Component = UGameplayStatics::SpawnEmitterAtLocation(Boss->GetWorld(), Particle,
		Boss->GetActorLocation(), Boss->GetActorRotation(), Boss->GetActorScale3D());

	if (!IsValid(Component))
		return;

	// CreateWeakLambda(객체, 람다) : 그 객체가 아직 살아 있을 때만 람다를 실행하는 타이머 함수.
	// 0.6초 뒤 Deactivate → 이미 나온 연기는 자연스럽게 사라지고, 다 사라지면 컴포넌트가 지워진다.
	FTimerHandle	Handle;

	Boss->GetWorldTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateWeakLambda(Component, [Component]()
		{
			Component->Deactivate();
		}),
		0.6f, false);
}

void ABossBase::StartVanish(float Time)
{
	if (mDeath)
		return;

	mVanished = true;

	// 검게 → 깎여 사라짐 → 숨김. 사망 연출과 같은 머티리얼 효과를 짧게 쓴다.
	StartFade(0.f, mVanishFadeEnd, Time, true);

	// 사라진 동안은 맞지도, 길을 막지도 않는다.
	// SetActorEnableCollision : 이 액터의 모든 충돌을 끄고 켠다. (캡슐·메시 전부)
	SetActorEnableCollision(false);

	// 사라지는 순간 그림자 잔상 이펙트 (세바로그 팩)
	SpawnVanishParticle(this, mVanishParticle);
}

void ABossBase::StartAppear(float Time)
{
	if (mDeath || !mVanished)
		return;

	mVanished = false;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// 깎인 상태 → 원래 모습. 숨기지 않는다.
	StartFade(mVanishFadeEnd, 0.f, Time, false);

	SpawnVanishParticle(this, mVanishParticle);
}

AUltimateLight* ABossBase::SpawnUltimateLight(float HeightOffset, float ChargeTime)
{
	// 엄폐 판정 라인 트레이스가 시작되는 점과 같은 위치 → 그림자 = 판정
	FVector	Location = GetActorLocation() + FVector(0.0, 0.0, HeightOffset);

	AUltimateLight* Light = GetWorld()->SpawnActor<AUltimateLight>(Location, FRotator::ZeroRotator);

	if (IsValid(Light))
	{
		Light->Setup(mUltLightColor, mUltLightRadius, mUltChargeIntensity, mUltFlashIntensity, mUltFadeTime);
		Light->StartCharge(ChargeTime);
	}

	return Light;
}

void ABossBase::PlayUltimateChargeEffect()
{
	if (IsValid(mUltChargeParticle))
		UGameplayStatics::SpawnEmitterAttached(mUltChargeParticle, GetMesh());

	// SpawnSoundAttached : 컴포넌트(메시)에 붙여 소리를 낸다. 반환값 = 재생 중인 오디오 컴포넌트 → 기억했다가 충격 때 끊는다.
	if (IsValid(mUltChargeSound))
	{
		StopUltimateChargeSound();

		mUltChargeAudio = UGameplayStatics::SpawnSoundAttached(mUltChargeSound, GetMesh());
	}
}

void ABossBase::StopUltimateChargeSound()
{
	if (!IsValid(mUltChargeAudio))
		return;

	// FadeOut(시간, 목표 볼륨) : 시간 동안 볼륨을 줄인 뒤 멈춘다. 뚝 끊으면 "틱" 소리가 날 수 있어서 짧게 줄인다.
	mUltChargeAudio->FadeOut(mUltChargeFadeTime, 0.f);
	mUltChargeAudio = nullptr;
}

void ABossBase::PlayUltimateImpactEffect()
{
	// 고조가 끝나는 순간 (10/26). 충격 이펙트가 없어도 소리는 끊어야 하므로 먼저.
	StopUltimateChargeSound();

	if (!IsValid(mUltImpactParticle))
		return;

	float	HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector	Ground = GetActorLocation() - FVector(0.0, 0.0, HalfHeight);

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mUltImpactParticle, Ground, GetActorRotation(),
		FVector(mUltImpactParticleScale));
}

void ABossBase::HoldForIntro(bool bHide)
{
	if (mDeath || mIntroPlaying)
		return;

	mIntroPlaying = true;

	// ---------------------------------------------------------------- AI 멈춤
	// PauseLogic(이유) : 비헤이비어 트리를 멈춘 자리에서 잠시 세운다. (ResumeLogic으로 이어서 실행)
	// 게임 모드가 "모든 BeginPlay가 끝난 다음 프레임"에 부르므로 이때는 이미 BT가 돌고 있다.
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();

		if (IsValid(AIController->GetBrainComponent()))
			AIController->GetBrainComponent()->PauseLogic(TEXT("Intro"));
	}

	// ---------------------------------------------------------------- 무적 (연출 중에 몰래 때려 HP를 깎지 못하게)
	// AttributeSet이 State.Invincible이면 피해를 막는다. (기디언 회피와 같은 태그)
	mASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));

	// (11/5) 숨김 : SetActorHiddenInGame(true) = 액터의 모든 부품을 그리지 않는다. 다시 false면 원래대로 보인다
	if (bHide)
	{
		SetActorHiddenInGame(true);

		// (11/6) 숨어 있어도 뼈 계산은 계속 : 캐릭터 메시 기본값(AlwaysTickPose)은 "안 보이면 뼈 갱신을 건너뜀"이라,
		// 나타나는 순간 숨어 있던 때의 묵은 자세가 한 프레임 그려지고 망치 소켓 높이도 틀리게 읽혔다 (오로라 여제 메시와 같은 설정)
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

float ABossBase::StartIntro()
{
	if (mDeath)
		return 0.f;

	// 아직 준비 전이면(다시 시도 = 짧은 연출) 여기서 멈춤·무적. 이미 준비했으면 건너뛴다.
	HoldForIntro(false);

	// 숨어 있었는지 (맵 둘러보기). IsHidden : SetActorHiddenInGame(true) 상태인지
	bool	bWasHidden = IsHidden();

	// (11/6) 숨어 있었으면 바로 보이지 않고 mIntroRevealDelay초 뒤에 보인다 (RevealForIntro).
	// 동작을 시작한 그 프레임에는 아직 애니메이션이 몸에 적용되기 전이라, 숨어 있던 자세·자리가 한 프레임 그려졌다
	// (녹화 : 오로라가 화면 왼쪽 아래에 머리만 1프레임 나왔다 사라짐). 몇 프레임 숨긴 채 동작을 먼저 돌린다.
	if (bWasHidden)
		GetWorldTimerManager().SetTimer(mIntroRevealTimer, this, &ABossBase::RevealForIntro, mIntroRevealDelay, false);
	else
		SetActorHiddenInGame(false);

	// ---------------------------------------------------------------- 동작
	// 경로가 비어 있으면(등장 동작이 없는 보스) 불러오지 않는다.
	if (!IsValid(mIntroMontage) && !mIntroMontagePath.IsEmpty())
		mIntroMontage = LoadObject<UAnimMontage>(nullptr, *mIntroMontagePath);

	if (!IsValid(mIntroMontage) || !IsValid(mAnimInst))
	{
		UE_LOG(UGDebug, Warning, TEXT("Boss : intro montage not found (%s)"), *mIntroMontagePath);
		return 0.f;
	}

	// (11/6) 숨어 있다가 나타나면 블렌드 인 0초 : 몽타주는 기본으로 0.25초 동안 "직전 자세 → 첫 자세"를 섞는데,
	// 직전 자세(숨어 있던 대기 자세 = 망치를 쥔 손)와 첫 자세(하늘의 망치)를 섞으면 망치가 손에서 하늘로 휘돌며 날아갔다.
	// 보이지 않던 몸이라 섞을 필요가 없다 → 첫 프레임부터 몽타주 자세 그대로.
	// 다시 시도(이미 보이는 상태)는 기본 블렌드 그대로 (갑자기 자세가 바뀌는 게 더 튄다)
	// FAlphaBlendArgs(시간) : 섞는 시간(초)과 곡선을 담는 구조체
	// (11/6) 등장 이펙트 : 동작과 같은 순간에 메시 발밑에 (처음 들어올 때 · 다시 시도 모두)
	// 메시의 월드 변환(위치·회전·크기)을 그대로 쓴다 → 파라곤 원작이 메시 기준으로 만든 이펙트라 방향·크기가 동작과 맞는다.
	// FTransform(보정 회전, 보정 위치) * 메시 변환 : 보정을 메시 기준으로 먼저 적용한 뒤 월드로 옮긴다
	// (11/10) "쾅" 예약 : 등장 동작이 mIntroImpactTime초에 닿는 순간 소리·흔들림 (몽타주 재생 속도 1 기준)
	if (mIntroImpactTime >= 0.f)
		GetWorldTimerManager().SetTimer(mIntroImpactTimer, this, &ABossBase::OnIntroImpact, FMath::Max(mIntroImpactTime, 0.01f), false);

	if (IsValid(mIntroSpawnParticle))
	{
		FTransform	MeshTransform = GetMesh()->GetComponentTransform();

		// (11/6) 높이는 보스 위치가 아니라 "실제 바닥"으로 : 다시 시도(짧은 연출)는 레벨이 열리자마자 시작해서
		// 보스가 아직 공중에 떠 있다 (크기를 키울 때 위로 올려 둔 뒤 중력으로 내려앉기 전) → 계단이 허공 높이에 생겼다.
		// LineTraceSingleByChannel(결과, 시작, 끝, 채널, 설정) : 보스 위치에서 아래로 선을 쏴서 처음 닿는 바닥 (자기 자신은 무시)
		FHitResult				FloorHit;
		FCollisionQueryParams	TraceParams(SCENE_QUERY_STAT(IntroSpawnFloor), false, this);
		FVector					TraceStart = GetActorLocation();

		if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceStart - FVector(0.0, 0.0, 3000.0),
			ECollisionChannel::ECC_WorldStatic, TraceParams))
		{
			FVector	Location = MeshTransform.GetLocation();

			Location.Z = FloorHit.ImpactPoint.Z;
			MeshTransform.SetLocation(Location);
		}

		FTransform	Offset(mIntroSpawnRotation, mIntroSpawnOffset);
		FTransform	SpawnTransform = Offset * MeshTransform;

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mIntroSpawnParticle, SpawnTransform);
	}

	if (bWasHidden)
		return mAnimInst->Montage_PlayWithBlendIn(mIntroMontage, FAlphaBlendArgs(0.f));

	return mAnimInst->Montage_Play(mIntroMontage);
}

void ABossBase::RevealForIntro()
{
	SetActorHiddenInGame(false);
}

void ABossBase::OnIntroImpact()
{
	if (mDeath)
		return;

	// 2D 소리 : 거리와 상관없이 크게 (다시 시도 때 기디언이 멀리 있어도 들리게. 필살기 충격음과 같은 방식)
	if (IsValid(mIntroImpactSound))
		UGameplayStatics::PlaySound2D(this, mIntroImpactSound);

	// 화면 흔들림 (거리 감쇠 없음). 게임 모드의 타격감 표 한 곳에서
	if (AArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>())
		GameMode->PlayHitFeedback(EHitFeedback::IntroSlam);
}

void ABossBase::EndIntro()
{
	if (!mIntroPlaying)
		return;

	// (11/6) 나타나기 전에 끝나도(건너뛰기) 예약을 지우고 아래에서 바로 보이게
	GetWorldTimerManager().ClearTimer(mIntroRevealTimer);

	mIntroPlaying = false;

	mASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible")));

	// (11/5) 안전장치 : 건너뛰기로 끝나도 보이고 움직이게
	SetActorHiddenInGame(false);
	SetIntroFrozen(false);

	if (mDeath)
		return;

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (IsValid(AIController->GetBrainComponent()))
			AIController->GetBrainComponent()->ResumeLogic(TEXT("Intro"));
	}
}

void ABossBase::SetIntroFrozen(bool bFrozen)
{
	// CustomTimeDilation : 이 액터(와 부품)만의 시간 배율. 0이면 애니메이션·이동이 그 자세로 멈춘다.
	// 월드 전체(SetGlobalTimeDilation)가 아니라 보스만 멈추므로 카드 애니메이션·카메라는 그대로 흐른다.
	CustomTimeDilation = bFrozen ? 0.f : 1.f;
}

void ABossBase::SetIntroMaskEnabled(bool bEnabled)
{
	// 보스의 모든 메시(오로라는 얼음 여제 메시까지)에 켠다.
	// GetComponents<T>(배열) : 이 액터가 가진 T 종류의 부품을 전부 배열에 담아 준다.
	TArray<UPrimitiveComponent*>	Components;
	GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* Component : Components)
	{
		// 캡슐(충돌용)은 화면에 안 그려지니 건너뛴다. 메시만
		if (Component->IsA<UMeshComponent>())
			Component->SetRenderCustomDepth(bEnabled);
	}
}

void ABossBase::StopIntroMontage()
{
	// (11/10) 건너뛰면 아직 안 난 "쾅"도 취소
	GetWorldTimerManager().ClearTimer(mIntroImpactTimer);

	if (IsValid(mAnimInst) && IsValid(mIntroMontage))
		mAnimInst->Montage_Stop(0.2f, mIntroMontage);
}

bool ABossBase::TryBlockFrontHit(const FVector& HitPoint)
{
	if (mDeath || !IsValid(mASC) ||
		!mASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.FrontGuard"))))
		return false;

	// 몸 중심 → 맞은 자리 방향과 몸 정면의 각도를 수평으로만 비교한다. (높이 차이는 무시)
	// 내적(Dot) : 두 방향이 같으면 1, 직각이면 0, 반대면 -1. 각도 θ 안쪽이면 내적 ≥ cos θ
	FVector	ToHit = HitPoint - GetActorLocation();
	ToHit.Z = 0.0;

	FVector	Forward = GetActorForwardVector();
	Forward.Z = 0.0;

	float	Dot = FVector::DotProduct(ToHit.GetSafeNormal(), Forward.GetSafeNormal());

	if (Dot < FMath::Cos(FMath::DegreesToRadians(mFrontGuardHalfAngle)))
		return false;

	if (IsValid(mFrontGuardBlockParticle))
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mFrontGuardBlockParticle, HitPoint);

	if (IsValid(mFrontGuardBlockSound))
		UGameplayStatics::PlaySoundAtLocation(this, mFrontGuardBlockSound, HitPoint);

	return true;
}

void ABossBase::SetMaterialsScalar(FName ParameterName, float Value)
{
	// 파라미터가 없는 머티리얼은 설정해도 아무 일도 없다. (HammerUp은 갑옷만, FadeOut은 5개 모두 반응)
	for (UMaterialInstanceDynamic* Material : mMaterials)
	{
		if (IsValid(Material))
			Material->SetScalarParameterValue(ParameterName, Value);
	}
}

void ABossBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ---------------------------------------------------------------- 점진 성장
	if (mGrowing)
	{
		mScaleElapsed += DeltaTime;

		// Alpha : 0(시작) → 1(끝). 0으로 나누지 않게 시간이 0이면 바로 1.
		float	Alpha = mScaleGrowTime > 0.f ? FMath::Clamp(mScaleElapsed / mScaleGrowTime, 0.f, 1.f) : 1.f;

		// Ease Out : 처음엔 빠르게 커지다가 끝에서 부드럽게 멈춘다. 1 - (1 - a)²
		float	Eased = 1.f - FMath::Square(1.f - Alpha);

		// SetBossScale이 커진 만큼 위로 올려 주므로, 매 프레임 불러도 발이 바닥에 묻히지 않는다.
		SetBossScale(FMath::Lerp(mScaleFrom, mScaleTo, Eased));

		if (Alpha >= 1.f)
			mGrowing = false;
	}

	// ---------------------------------------------------------------- 페이드 (사망 사라지기 · 순간이동 사라짐·나타남)
	if (mFading)
	{
		mFadeElapsed += DeltaTime;

		float	Alpha = mFadeTime > 0.f ? FMath::Clamp(mFadeElapsed / mFadeTime, 0.f, 1.f) : 1.f;

		SetMaterialsScalar(TEXT("FadeOut"), FMath::Lerp(mFadeFrom, mFadeTo, Alpha));

		// 다 끝나면 (필요하면) 숨긴다. (FadeOut 끝 값에서 조금 남아 있어도 확실히 사라지게)
		if (Alpha >= 1.f)
		{
			mFading = false;

			if (mFadeHideAtEnd)
				SetActorHiddenInGame(true);
		}
	}

	// 할 일이 없으면 Tick을 다시 끈다.
	if (!mGrowing && !mFading)
		SetActorTickEnabled(false);
}
