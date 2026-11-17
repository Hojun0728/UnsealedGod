// Fill out your copyright notice in the Description page of Project Settings.


#include "Input.h"

UInput::UInput()
{
}

TObjectPtr<UInputAction> UInput::FindAction(
	const FString& Name) const
{
	// FindRef : 키가 있으면 값을, 없으면 nullptr을 반환한다.
	// (Find는 값의 "주소"를 반환하고, FindRef는 값 "자체"를 복사해서 반환한다.)
	return mActions.FindRef(Name);
}

UGideonInput::UGideonInput()
{
	// ConstructorHelpers::FObjectFinder<타입> : 생성자 안에서 경로로 에셋을 찾아온다.
	// static을 붙이는 이유 : 생성자가 여러 번 불려도 에셋 검색은 처음 한 번만 하게 하기 위함이다.
	// 경로 형식 : /Script/모듈.클래스'/Game/폴더/에셋이름.에셋이름'
	//   (콘텐츠 브라우저에서 에셋 우클릭 → "레퍼런스 복사"를 하면 이 형식으로 복사된다.)
	// 주의 : 에셋이 아직 없으면 Succeeded()가 false가 되고, 에디터 출력 로그에 경고가 뜬다.
	//        에셋을 만든 뒤에는 에디터를 재시작해야 CDO가 다시 만들어지면서 에셋을 찾는다.
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>
		IMC(TEXT("/Script/EnhancedInput.InputMappingContext'/Game/UnsealedGod/Input/IMC_Gideon.IMC_Gideon'"));

	if (IMC.Succeeded())
		mContext = IMC.Object;

	// 이동 (WASD). 값 형식 : Axis2D (X = 앞뒤, Y = 좌우)
	static ConstructorHelpers::FObjectFinder<UInputAction>
		MoveAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_Move.IA_Move'"));

	if (MoveAction.Succeeded())
		mActions.Add(TEXT("Move"), MoveAction.Object);

	// 시점 회전 (마우스). 값 형식 : Axis2D (X = 좌우, Y = 상하)
	static ConstructorHelpers::FObjectFinder<UInputAction>
		LookAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_Look.IA_Look'"));

	if (LookAction.Succeeded())
		mActions.Add(TEXT("Look"), LookAction.Object);

	// 점프 (스페이스바). 값 형식 : Digital(bool)
	static ConstructorHelpers::FObjectFinder<UInputAction>
		JumpAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_Jump.IA_Jump'"));

	if (JumpAction.Succeeded())
		mActions.Add(TEXT("Jump"), JumpAction.Object);

	// 회복 물약 (1번 키). 값 형식 : Digital(bool)
	// 10/19 : 보스·스킬이 다 생겨서 테스트 키(1 체력 감소·2 마나 소모)를 지우고 이 자리에 넣었다.
	static ConstructorHelpers::FObjectFinder<UInputAction>
		PotionAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_Potion.IA_Potion'"));

	if (PotionAction.Succeeded())
		mActions.Add(TEXT("Potion"), PotionAction.Object);

	// 기본 공격 (마우스 왼쪽). 값 형식 : Digital(bool)
	static ConstructorHelpers::FObjectFinder<UInputAction>
		AttackAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_Attack.IA_Attack'"));

	if (AttackAction.Succeeded())
		mActions.Add(TEXT("Attack"), AttackAction.Object);

	// 회피 - 토른 스페이스 (E). 값 형식 : Digital(bool)
	static ConstructorHelpers::FObjectFinder<UInputAction>
		DodgeAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_Dodge.IA_Dodge'"));

	if (DodgeAction.Succeeded())
		mActions.Add(TEXT("Dodge"), DodgeAction.Object);

	// 스킬 Q 포털 운석 / F 버든 / R 운석 비. 값 형식 : Digital(bool)
	static ConstructorHelpers::FObjectFinder<UInputAction>
		SkillQAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_SkillQ.IA_SkillQ'"));

	if (SkillQAction.Succeeded())
		mActions.Add(TEXT("SkillQ"), SkillQAction.Object);

	static ConstructorHelpers::FObjectFinder<UInputAction>
		SkillFAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_SkillF.IA_SkillF'"));

	if (SkillFAction.Succeeded())
		mActions.Add(TEXT("SkillF"), SkillFAction.Object);

	static ConstructorHelpers::FObjectFinder<UInputAction>
		SkillRAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_SkillR.IA_SkillR'"));

	if (SkillRAction.Succeeded())
		mActions.Add(TEXT("SkillR"), SkillRAction.Object);

	// (11/2) 일시정지 메뉴 (Esc). 값 형식 : Digital(bool)
	static ConstructorHelpers::FObjectFinder<UInputAction>
		PauseAction(TEXT("/Script/EnhancedInput.InputAction'/Game/UnsealedGod/Input/IA_Pause.IA_Pause'"));

	if (PauseAction.Succeeded())
		mActions.Add(TEXT("Pause"), PauseAction.Object);
}
