// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerAnimInstance.h"
#include "PlayerCharacter.h"
// UKismetAnimationLibrary::CalculateDirection이 들어있다. (Build.cs에 AnimGraphRuntime 모듈 필요)
#include "KismetAnimationLibrary.h"

UPlayerAnimInstance::UPlayerAnimInstance()
{
	// 섹션 이름 기본값. 몽타주 섹션을 A, B, C로 만들면 따로 바꿀 필요가 없다.
	mAttackSectionNames.Add(TEXT("A"));
	mAttackSectionNames.Add(TEXT("B"));
	mAttackSectionNames.Add(TEXT("C"));
}

void UPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UPlayerAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	// 몽타주 재생이 끝났다면 호출될 함수를 지정한다. (수업 코드와 동일)
	OnMontageEnded.AddDynamic(this, &UPlayerAnimInstance::MontageEnd);
}

void UPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// TryGetPawnOwner : 이 애님인스턴스를 가지고 있는 스켈레탈 메시 컴포넌트의
	// Owner Pawn을 얻어온다.
	// 에디터의 애니메이션 미리보기 창에서는 Pawn이 없어서 nullptr이 나올 수 있으므로 반드시 확인한다.
	APlayerCharacter* PlayerChar =
		Cast<APlayerCharacter>(TryGetPawnOwner());

	if (IsValid(PlayerChar))
	{
		UCharacterMovementComponent* Movement =
			PlayerChar->GetCharacterMovement();

		// Velocity는 속도벡터이다. 이동속도가 있을 경우 0이 아니게 된다.
		// Length() : 벡터의 길이 = 속력(방향 없이 빠르기만).
		FVector	Velocity = Movement->Velocity;

		// 점프 중 위아래 속도까지 포함되면 "제자리 점프"도 달리는 것으로 보이므로
		// 바닥 평면(XY)의 속도만 쓴다.
		mMoveSpeed = Velocity.Size2D();

		// 속도 벡터가 캐릭터가 바라보는 방향에서 몇 도 틀어져 있는지 계산한다.
		mMoveDirection = UKismetAnimationLibrary::CalculateDirection(Velocity,
			PlayerChar->GetActorRotation());

		// 공중에 떠있는 상태인지를 체크한다.
		mAir = Movement->IsFalling();

		// 가속도가 있는지 체크한다. (이동 키를 누르고 있으면 가속도가 0보다 크다)
		float	Acceleration = Movement->GetCurrentAcceleration().Length();
		mAccelerating = Acceleration > 0.f;

		// 캐릭터의 사망 여부를 가져온다.
		// 캐릭터가 애님 인스턴스에 "죽었다"고 직접 알려주는 대신, 다른 값들처럼
		// 애님 인스턴스가 매 프레임 읽어오게 하면 두 클래스가 서로를 덜 알아도 된다.
		mDeath = PlayerChar->IsDeath();

		// 조준 각도.
		// GetBaseAimRotation : 컨트롤러가 보고 있는 방향(= 카메라 방향).
		// 몸(액터)의 회전을 빼면 "몸 기준으로 위아래 몇 도를 보는지"가 된다.
		// GetNormalized : 각도를 -180 ~ 180 범위로 맞춘다. (예 : 350도 → -10도)
		//   이걸 안 하면 아래를 볼 때 -10이 아니라 350으로 나와서 에임 오프셋이 튄다.
		FRotator	AimDelta = (PlayerChar->GetBaseAimRotation() -
			PlayerChar->GetActorRotation()).GetNormalized();

		mAimPitch = AimDelta.Pitch;
	}
}

bool UPlayerAnimInstance::PlayAttack()
{
	// 공격용 몽타주가 없을 경우 재생하지 않는다. (ABP_Gideon 클래스 디폴트에서 지정해야 한다)
	if (!IsValid(mAttackMontage) || mAttackSectionNames.IsEmpty())
	{
		UE_LOG(UGDebug, Warning, TEXT("Not Valid Attack Montage"));
		return false;
	}

	// 아직 다음 공격을 받을 수 없는 구간이다. (현재 섹션의 Combo 노티파이 전)
	if (!mAttackCombo)
		return false;

	// A부터 다시 시작하려는데 콤보 쿨타임이 안 끝났다.
	// GetWorld()->GetTimeSeconds() : 게임이 시작된 뒤 흐른 시간(초).
	if (mAttackSectionIndex == 0 && GetWorld()->GetTimeSeconds() < mNextComboTime)
		return false;

	// 몽타주를 재생하고 이번 섹션으로 이동한다. (수업 코드와 동일한 방식)
	// 이미 재생 중인 몽타주를 다시 Montage_Play하면 이전 재생은 "중간에 끊김(Interrupted)"으로 끝난다.
	Montage_Play(mAttackMontage, mAttackPlayRate);
	Montage_JumpToSection(mAttackSectionNames[mAttackSectionIndex], mAttackMontage);

	mLastAttackSection = mAttackSectionIndex;

	// [테스트용] 지금 재생한 섹션 이름을 화면에 띄운다. (Key 3 : 같은 줄을 덮어쓴다)
	if (mShowAttackDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 2.f, FColor::Yellow,
			FString::Printf(TEXT("Attack Section : %s"),
				*mAttackSectionNames[mAttackSectionIndex].ToString()));
	}

	// 다음 섹션 번호. % (나머지 연산) : 0 → 1 → 2 → 0 → ... 으로 순환한다.
	// Num 함수는 TArray의 개수를 구해주는 함수이다.
	mAttackSectionIndex = (mAttackSectionIndex + 1) % mAttackSectionNames.Num();

	// Combo 노티파이가 나오기 전까지는 다음 공격을 받지 않는다.
	mAttackCombo = false;

	return true;
}

void UPlayerAnimInstance::StopAllMontage()
{
	// 0.2초에 걸쳐 부드럽게 멈춘다. (두 번째 인자를 비우면 모든 몽타주)
	Montage_Stop(0.2f);
}

void UPlayerAnimInstance::AnimNotify_Combo()
{
	// 지금부터 다음 섹션 입력을 받는다.
	// 마우스를 누르고 있으면 다음 프레임에 바로 다음 섹션이 재생된다 → 노티파이 위치가 곧 연사 속도다.
	mAttackCombo = true;
}

void UPlayerAnimInstance::MontageEnd(UAnimMontage* Montage,
	bool Interrupted)
{
	// 공격 몽타주 재생이 끝났는지 판단한다.
	if (mAttackMontage != Montage)
		return;

	// [테스트용] 몽타주가 어떻게 끝났는지 화면에 띄운다. (Key 4)
	// mAttackSectionNames.IsValidIndex : 배열 범위 안의 번호인지 확인 (처음엔 mLastAttackSection이 -1)
	if (mShowAttackDebug && GEngine)
	{
		FString	LastName = mAttackSectionNames.IsValidIndex(mLastAttackSection) ?
			mAttackSectionNames[mLastAttackSection].ToString() : TEXT("-");

		GEngine->AddOnScreenDebugMessage(4, 2.f, Interrupted ? FColor::Silver : FColor::Orange,
			FString::Printf(TEXT("Montage End : %s (Last : %s)"),
				Interrupted ? TEXT("Interrupted") : TEXT("Finished"), *LastName));
	}

	// 끊긴 경우(Interrupted)는 두 가지가 있다.
	//   (1) 다음 섹션(A→B)을 재생하면서 이전 재생이 끊김 → 공격 몽타주는 새로 재생 중
	//   (2) 회피 같은 다른 몽타주가 공격을 끊음 → 공격 몽타주가 완전히 멈춤
	// Montage_IsPlaying : 이 몽타주의 "현재 활성 재생"이 재생 중인지 확인한다.
	//   엔진은 멈춘 재생을 활성 목록에서 빼고, 콤보로 다시 재생하면 목록이 새 재생을 가리킨다.
	//   그래서 (1)이면 true, (2)면 false가 나온다.
	if (Interrupted)
	{
		// (1) 콤보가 이어지는 중 → 아무것도 하지 않는다.
		if (Montage_IsPlaying(mAttackMontage))
			return;

		// (2) 다른 동작에 공격이 끊겼다 → 콤보를 처음(A)으로 되돌리고 바로 다시 공격할 수 있게 한다.
		//     Combo 노티파이를 못 받아서 mAttackCombo가 false로 남아 있으면 영원히 공격이 안 나가기 때문이다.
		//     회피로 끊긴 건 "C까지 다 쏜 것"이 아니므로 콤보 쿨타임은 주지 않는다.
		mAttackSectionIndex = 0;
		mAttackCombo = true;
		return;
	}

	// 여기까지 왔다 = 마지막으로 재생한 섹션이 끝까지 재생되었다 (다음 입력이 없었다).

	// 마지막 섹션(C)까지 다 쐈으면 콤보 쿨타임을 시작한다.
	// A나 B에서 손을 뗀 경우에는 쿨타임 없이 바로 다시 A부터 쏠 수 있다.
	if (mLastAttackSection == mAttackSectionNames.Num() - 1)
		mNextComboTime = GetWorld()->GetTimeSeconds() + mComboCooldown;

	// 콤보를 처음(A)으로 되돌린다.
	mAttackSectionIndex = 0;
	mAttackCombo = true;
}
