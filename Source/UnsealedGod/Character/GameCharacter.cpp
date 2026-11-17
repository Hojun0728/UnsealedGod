// Fill out your copyright notice in the Description page of Project Settings.


#include "GameCharacter.h"
#include "../GAS/BaseAttributeSet.h"
#include "../GAS/Effect/GameplayEffect_Damage.h"
// 근접 판정이 "플레이어만" 맞히는지 확인하는 데 쓴다. (헤더가 아니라 cpp에서 include → 순환 참조 없음)
#include "../Player/PlayerCharacter.h"
#include "DrawDebugHelpers.h"
// FOverlapResult(겹침 검사 결과) 정의 (범위 공격)
#include "Engine/OverlapResult.h"

AGameCharacter::AGameCharacter()
{
	// 지금은 매 프레임 처리할 일이 없으므로 Tick을 끈다. (자식이 필요하면 켠다)
	PrimaryActorTick.bCanEverTick = false;

	// CreateDefaultSubobject<타입>(이름) : 생성자에서 컴포넌트·서브오브젝트를 만든다.
	mASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	mAttributeSet = CreateDefaultSubobject<UBaseAttributeSet>(TEXT("AttributeSet"));

	// 생성한 AttributeSet을 ASC에 등록한다. 등록해야 GE가 이 수치를 바꿀 수 있다.
	mASC->AddAttributeSetSubobject<UBaseAttributeSet>(mAttributeSet);

	// (10/22) 예고 데칼(바닥 원·사각형)이 캐릭터 몸에 묻지 않게 한다. 벽·기둥 메시에서 끈 것과 같은 설정.
	// 공용 부모에서 끄면 보스·기디언·미니언 모두 적용된다. (예고는 "바닥에 그린 표시")
	GetMesh()->SetReceivesDecals(false);

	// (11/3) 라이팅 채널 1번도 받는다. 레벨에 "채널 1만 비추는 빛"을 두면 캐릭터만 밝아지고 배경은 그대로 어둡다.
	// (어두운 로브의 세바로그가 그늘진 돌벽에 묻혀 동작이 안 읽히던 문제) 채널 0(기본 빛)은 그대로 받는다.
	GetMesh()->LightingChannels.bChannel1 = true;
}

void AGameCharacter::BeginPlay()
{
	Super::BeginPlay();

	// ASC에 Owner(소유자)와 Avatar(실제로 월드에서 움직이는 몸)를 지정한다. 둘 다 자기 자신.
	// 반드시 Attribute를 설정하기 전에 호출해야 한다. (ASC가 자기 주인을 알아야 값을 바꿀 수 있다)
	mASC->InitAbilityActorInfo(this, this);

	// 아래 두 함수는 virtual이다. 여기(부모 코드)에서 불러도, 실제 객체가 플레이어면
	// 플레이어가 override한 버전이 실행된다. → 부모가 "순서"를 정하고, 자식이 "내용"을 채우는 구조.
	// (참고 : 생성자 안에서는 virtual 함수가 자식 버전으로 불리지 않는다. 그래서 BeginPlay에서 부른다)
	BindAttributeDelegates();
	InitAttributes();
}

void AGameCharacter::BindAttributeDelegates()
{
	// Attribute의 값이 바뀔 경우 호출될 델리게이트를 등록한다.
	// AddUObject(객체, &클래스::함수) : 이 객체의 이 함수를 콜백으로 등록한다.
	// OnHPChanged가 virtual이라서, 플레이어 객체라면 APlayerCharacter::OnHPChanged가 호출된다.
	mASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHPAttribute())
		.AddUObject(this, &AGameCharacter::OnHPChanged);
}

void AGameCharacter::InitAttributes()
{
	// 최대치를 먼저 넣어야 한다.
	// HP를 먼저 넣으면 그 순간 HPMax가 0이라서 PreAttributeChange에서 HP가 0으로 잘려버린다.
	mAttributeSet->SetHPMax(mInitHP);
	mAttributeSet->SetHP(mInitHP);
	mAttributeSet->SetAttack(mInitAttack);
}

void AGameCharacter::OnHPChanged(const FOnAttributeChangeData& Data)
{
	// 체력이 0이 되면 사망 처리한다. (Death는 virtual → 자식 버전이 호출된다)
	if (Data.NewValue <= 0.f && !mDeath)
	{
		Death();
	}
}

void AGameCharacter::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass,
	const TMap<FGameplayTag, float>& SetByCallerValues)
{
	// GE를 적용하는 3단계 (수업 코드의 ApplyCost와 같은 흐름)
	// 1. Context : "누가, 무엇으로" 적용했는지에 대한 부가 정보.
	FGameplayEffectContextHandle	Context = mASC->MakeEffectContext();
	Context.AddSourceObject(this);

	// 2. Spec : GE 클래스 + 레벨 + Context를 묶은 "적용 직전의 효과 한 건".
	//    Handle은 Spec을 안전하게 들고 다니기 위한 포장지(스마트 포인터)다.
	FGameplayEffectSpecHandle	Spec = mASC->MakeOutgoingSpec(EffectClass, 1.f, Context);

	if (!Spec.IsValid())
		return;

	// SetByCaller 값을 채워 넣는다.
	// const auto& : 타입은 컴파일러가 알아서 정하고(auto), 복사 없이(&) 읽기만(const) 한다.
	for (const auto& Pair : SetByCallerValues)
	{
		Spec.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
	}

	// 3. 자기 자신에게 적용한다. Spec.Data는 포인터이므로 *를 붙여 "가리키는 실제 객체"를 넘긴다.
	mASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

UAbilitySystemComponent* AGameCharacter::GetAbilitySystemComponent() const
{
	return mASC;
}

// 얼음 깨짐 피해 배율 (10/28 직접 결정). 이 파일 안에서만 쓰는 상수
static constexpr float	ShatterDamageScale = 1.5f;

bool AGameCharacter::ApplyDamageToTarget(AActor* Target, float Damage, const FHitResult& Hit,
	bool bUnblockable, bool bHeavy)
{
	if (!IsValid(Target) || Damage <= 0.f)
		return false;

	UAbilitySystemComponent* TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);

	if (!TargetASC)
		return false;

	// ---------------------------------------------------------------- 얼음 깨짐 (10/28)
	// 얼어 있는 대상을 때리면 얼음이 깨지며 1.5배 피해를 받고, 그 순간 풀려난다. → 다음 공격(검 2·3타)은 회피할 수 있다.
	// (얼면 3연타가 전부 확정으로 들어가던 "이중 처벌" 방지) 얼음 포박의 찌르기는 예외 = State.FrozenNoShatter
	// 피해를 주기 "전에" 풀어 준다 → 맞는 반응(움찔·큰 공격이면 넘어짐)이 평소처럼 나온다.
	if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Frozen"))) &&
		!TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.FrozenNoShatter"))))
	{
		Damage *= ShatterDamageScale;

		FGameplayEventData	ShatterEvent;
		ShatterEvent.Instigator = this;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target,
			FGameplayTag::RequestGameplayTag(TEXT("Event.Player.FrozenShatter")), ShatterEvent);
	}

	// GE 적용 3단계 (ApplyEffectToSelf와 같은 흐름, 대상만 다르다)
	FGameplayEffectContextHandle	Context = mASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddHitResult(Hit);

	FGameplayEffectSpecHandle	Spec = mASC->MakeOutgoingSpec(
		UGameplayEffect_Damage::StaticClass(), 1.f, Context);

	if (!Spec.IsValid())
		return false;

	// 음수로 넣어야 HP가 깎인다. (Additive)
	Spec.Data->SetSetByCallerMagnitude(
		FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage")), -Damage);

	// "막을 수 없는" 피해 : 이 효과에만 태그를 붙인다. 대상의 AttributeSet이 이 태그를 보면 무적을 무시한다.
	if (bUnblockable)
		Spec.Data->AddDynamicAssetTag(FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage.Unblockable")));

	// "큰 공격" 표시. 맞은 쪽 AttributeSet이 이 태그를 보고 OnDamaged에 bHeavy = true로 알려준다. (넘어짐)
	if (bHeavy)
		Spec.Data->AddDynamicAssetTag(FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.Damage.Heavy")));

	mASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

	UE_LOG(UGDebug, Warning, TEXT("%s → %s : Damage %.1f"),
		*GetName(), *Target->GetName(), Damage);

	return true;
}

bool AGameCharacter::ApplyAreaDamage(const FVector& Center, float Radius, float Damage, bool bHeavy)
{
	if (mDeath)
		return false;

	// 판정 높이 : 바닥에서 위로 2m까지. (기디언 키 약 1.9m)
	const float	Height = 200.f;

	// 1단계 : 넉넉한 구(반지름 + 높이)로 후보를 모은다. Pawn 오브젝트만, 자기 자신은 무시.
	TArray<FOverlapResult>		Overlaps;
	FCollisionObjectQueryParams	ObjectParams(ECollisionChannel::ECC_Pawn);
	FCollisionQueryParams		Params(NAME_None, false, this);

	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Radius + Height), Params);

#if ENABLE_DRAW_DEBUG
	// 판정 원기둥을 그린다. (바닥 원 = 예고 데칼·이펙트와 같아야 한다)
	if (mDrawMeleeDebug)
		DrawDebugCylinder(GetWorld(), Center, Center + FVector(0.0, 0.0, Height), Radius, 32,
			FColor::Orange, false, 1.f);
#endif

	bool	bHitAny = false;

	// 같은 액터의 컴포넌트(캡슐·메시)가 여러 번 걸릴 수 있어서 한 번만 맞게 모은다.
	TArray<AGameCharacter*>	HitCharacters;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AGameCharacter* Other = Cast<AGameCharacter>(Overlap.GetActor());

		// 다른 편만 맞힌다. (보스 → 기디언 / 기디언 → 보스·미니언, 미니언끼리·보스와 미니언은 안 맞음)
		if (!IsValid(Other) || !IsHostileTo(Other) || HitCharacters.Contains(Other))
			continue;

		// 2단계 : 정확한 판정. 수평 거리가 "원 반지름 + 상대 캡슐 반지름" 이하면 원에 몸이 걸친 것이다.
		float	OtherRadius = Other->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float	Distance2D = FVector::Dist2D(Center, Other->GetActorLocation());

		if (Distance2D > Radius + OtherRadius)
			continue;

		HitCharacters.Add(Other);

		FHitResult	Hit;
		Hit.ImpactPoint = Other->GetActorLocation();

		// GE_Damage로 주므로 회피 무적이 자동으로 적용된다.
		if (ApplyDamageToTarget(Other, Damage, Hit, false, bHeavy))
			bHitAny = true;
	}

	return bHitAny;
}

float AGameCharacter::GetAttack() const
{
	return mAttributeSet->GetAttack();
}

float AGameCharacter::GetHP() const
{
	return mAttributeSet->GetHP();
}

float AGameCharacter::GetHPMax() const
{
	return mAttributeSet->GetHPMax();
}

float AGameCharacter::GetMP() const
{
	return mAttributeSet->GetMP();
}

float AGameCharacter::GetMPMax() const
{
	return mAttributeSet->GetMPMax();
}

bool AGameCharacter::IsHostileTo(const AGameCharacter* Other) const
{
	if (!IsValid(Other) || Other->IsDeath())
		return false;

	// 두 캐릭터의 편이 다르면 적이다. (bool끼리 != : 하나만 true일 때 true)
	return IsPlayerTeam() != Other->IsPlayerTeam();
}

void AGameCharacter::Death()
{
	mDeath = true;

	// 쓰러지는 동작은 각자의 애님 블루프린트가 IsDeath()를 읽어서 보여준다. (공중이어도 바로 시작)

	// 공중에서 죽었으면 : 바로 마무리하지 않고 땅에 떨어지게 둔다. (10/15 : 공중에서 죽으면 그 자리에 떠 있었음)
	// 옆으로 날아가던 속도는 없애고, 위로 올라가던 중이었으면 위 속도도 없애서 바로 떨어지게 한다.
	if (GetCharacterMovement()->IsFalling())
	{
		FVector&	Velocity = GetCharacterMovement()->Velocity;

		// FVector& : Velocity의 "참조". 복사본이 아니라 CharacterMovement가 가진 원본을 직접 고친다.
		Velocity.X = 0.0;
		Velocity.Y = 0.0;
		Velocity.Z = FMath::Min(Velocity.Z, 0.0);

		mDeathLandingPending = true;
		return;
	}

	FinishDeathOnGround();
}

void AGameCharacter::FinishDeathOnGround()
{
	// 이동을 멈춘다.
	// MOVE_None : 이동 모드를 "없음"으로 바꿔서 중력·입력 모두 받지 않게 한다.
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

	// 캡슐 충돌을 끈다. 쓰러진 몸에 다른 캐릭터·투사체가 걸리지 않게 하기 위함이다.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGameCharacter::Landed(const FHitResult& Hit)
{
	// 부모(ACharacter)의 착지 처리를 먼저 한다.
	Super::Landed(Hit);

	if (mDeathLandingPending)
	{
		mDeathLandingPending = false;
		FinishDeathOnGround();
	}
}

AActor* AGameCharacter::GetTarget() const
{
	AAIController* AIController = Cast<AAIController>(GetController());

	if (!IsValid(AIController) || !AIController->GetBlackboardComponent())
		return nullptr;

	return Cast<AActor>(AIController->GetBlackboardComponent()->GetValueAsObject(TEXT("Target")));
}

void AGameCharacter::BeginMeleeHit()
{
	// 새 휘두르기 시작 : 맞은 대상 목록과 지난 위치를 비운다.
	mMeleeHitActors.Reset();
	mLastMeleePoints.Reset();
}

void AGameCharacter::TickMeleeHit(FName StartSocketName, FName EndSocketName, int32 PointCount,
	float Radius, float DamageScale, bool bHeavy, float FreezeTime)
{
	if (mDeath)
		return;

	// ---------------------------------------------------------------- 판정 점 위치 계산
	// 망치 머리(끝)와 손(시작)의 지금 위치. GetSocketLocation은 소켓뿐 아니라 뼈 이름(weapon_r)도 받는다.
	FVector	EndLocation = GetMesh()->GetSocketLocation(EndSocketName);
	FVector	StartLocation = EndLocation;

	// 손 소켓이 없으면 머리 한 점만 검사한다. 있으면 최소 2점(손·머리).
	int32	Count = 1;

	if (!StartSocketName.IsNone())
	{
		StartLocation = GetMesh()->GetSocketLocation(StartSocketName);
		Count = FMath::Max(PointCount, 2);
	}

	// 첫 프레임 : 지난 위치가 없으므로 "지금 위치에서 제자리 검사"를 한다.
	bool	bFirstFrame = mLastMeleePoints.Num() != Count;

	if (bFirstFrame)
		mLastMeleePoints.SetNum(Count);

	// Pawn 오브젝트만 찾는다. (플레이어 캡슐) 자기 자신은 무시.
	FCollisionObjectQueryParams	ObjectParams(ECollisionChannel::ECC_Pawn);
	FCollisionQueryParams		Params(NAME_None, false, this);

	// 모든 점에서 걸린 것을 한 목록에 모은다.
	TArray<FHitResult>	AllHits;

	for (int32 i = 0; i < Count; ++i)
	{
		// Alpha : 손(0) → 머리(1) 사이의 비율. 4점이면 0, 1/3, 2/3, 1.
		// Lerp(A, B, Alpha) : A와 B 사이에서 Alpha 비율만큼 간 위치. (선형 보간)
		float	Alpha = Count == 1 ? 1.f : (float)i / (float)(Count - 1);
		FVector	Point = FMath::Lerp(StartLocation, EndLocation, Alpha);

		// 지난 프레임의 같은 점 → 지금 점으로 쓸어서 검사한다.
		FVector	From = bFirstFrame ? Point : mLastMeleePoints[i];

		mLastMeleePoints[i] = Point;

		// ---------------------------------------------------------------- 호(弧)로 나눠 쓸기 (10/28)
		// 빠른 가로 베기는 한 프레임에 칼끝이 몸 둘레를 크게 돈다. From → Point를 "직선"으로 쓸면
		// 실제 칼이 지나간 바깥 호가 아니라 몸 가까이 지나가는 현(직선)만 검사해서, 앞에 선 기디언이 안 맞았다. (오로라 1타)
		// → 공격자 몸 중심을 기준으로 "수평 각도·거리·높이"를 나눠서 보간한다 = 호를 따라 여러 조각으로 쓴다.
		FVector	Center = GetActorLocation();
		FVector	FromLocal = From - Center;
		FVector	ToLocal = Point - Center;

		// Atan2(y, x) : 수평 방향의 각도(라디안). FindDeltaAngleRadians : 두 각도의 차이를 -π ~ π로 (짧은 쪽으로 돈다)
		float	FromAngle = FMath::Atan2(FromLocal.Y, FromLocal.X);
		float	DeltaAngle = FMath::FindDeltaAngleRadians(FromAngle, FMath::Atan2(ToLocal.Y, ToLocal.X));
		float	FromRadius = FVector2D(FromLocal).Size();
		float	ToRadius = FVector2D(ToLocal).Size();

		// 10°마다 한 조각 (최대 12조각). 거의 안 돌았으면(찌르기·내려치기) 1조각 = 지금까지와 같은 직선 쓸기
		int32	Steps = FMath::Clamp(FMath::CeilToInt(FMath::Abs(DeltaAngle) / FMath::DegreesToRadians(10.f)), 1, 12);

		FVector	SegmentStart = From;
		bool	bHit = false;

		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			float	T = (float)Step / (float)Steps;
			float	Angle = FromAngle + DeltaAngle * T;
			float	Radius2D = FMath::Lerp(FromRadius, ToRadius, T);

			// 마지막 조각은 정확히 지금 점에서 끝나게 한다. (계산 오차 방지)
			FVector	SegmentEnd = Step == Steps ? Point :
				Center + FVector(FMath::Cos(Angle) * Radius2D, FMath::Sin(Angle) * Radius2D,
					FMath::Lerp(FromLocal.Z, ToLocal.Z, T));

			// SweepMulti : 구를 SegmentStart → SegmentEnd로 쓸면서 겹치는 것을 "전부" 찾는다.
			TArray<FHitResult>	Hits;

			if (GetWorld()->SweepMultiByObjectType(Hits, SegmentStart, SegmentEnd, FQuat::Identity,
				ObjectParams, FCollisionShape::MakeSphere(Radius), Params))
				bHit = true;

			AllHits.Append(Hits);

#if ENABLE_DRAW_DEBUG
			// 호를 따라 나눈 조각 끝마다 그린다. (빠른 휘두르기도 칼이 지나간 길이 보인다)
			if (mDrawMeleeDebug)
				DrawDebugSphere(GetWorld(), SegmentEnd, Radius, 12, bHit ? FColor::Red : FColor::Green, false, 0.1f);
#endif

			SegmentStart = SegmentEnd;
		}
	}

	for (const FHitResult& Hit : AllHits)
	{
		// 플레이어만 맞힌다. (나중에 소환될 미니언 같은 같은 편은 제외)
		APlayerCharacter* Player = Cast<APlayerCharacter>(Hit.GetActor());

		if (!IsValid(Player))
			continue;

		// 이번 휘두르기에서 이미 맞았으면 넘어간다. (여러 점이 동시에 맞혀도 피해는 한 번)
		if (mMeleeHitActors.Contains(Player))
			continue;

		mMeleeHitActors.Add(Player);

		// 피해 = 공격력 × 배율. GE_Damage로 주므로 회피 무적이 자동으로 적용된다.
		ApplyDamageToTarget(Player, mAttributeSet->GetAttack() * DamageScale, Hit, false, bHeavy);

		// 얼리는 공격 (10/28) : 회피 무적이 아니면 얼음 이벤트. (피해는 AttributeSet이 무적을 막지만, 이벤트는 여기서 막아야 한다)
		if (FreezeTime > 0.f)
		{
			UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();

			if (PlayerASC && !PlayerASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"))))
			{
				FGameplayEventData	FreezeEvent;
				FreezeEvent.Instigator = this;
				FreezeEvent.EventMagnitude = FreezeTime;

				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Player,
					FGameplayTag::RequestGameplayTag(TEXT("Event.Player.Frozen")), FreezeEvent);
			}
		}
	}
}

void AGameCharacter::EndMeleeHit()
{
	mMeleeHitActors.Reset();
	mLastMeleePoints.Reset();
}

