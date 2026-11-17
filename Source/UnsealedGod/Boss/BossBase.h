// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Character/GameCharacter.h"
#include "BossBase.generated.h"

// ============================================================================
// FIntroFlyoverSettings (11/5)
// 시작 연출의 "맵 둘러보기" 카메라 경로 값. 레벨에 태그 "Intro" 시퀀스가 없을 때 게임 모드가 이 값으로 경로를 계산한다.
//   0초 : 아레나 중심에서 StartDistance 떨어진 곳, StartHeight 높이 (전체를 내려다봄)
//   → OrbitEndTime초 : 중심을 축으로 OrbitDegrees만큼 돌며 OrbitEndDistance · OrbitEndHeight까지 내려옴
//   → Duration초 : 보스 앞 (키 × FinalDistanceRatio), 발바닥 + (키 × FinalHeightRatio)에서 올려다봄. 보스가 화면 왼쪽 1/3에 오도록 카메라를 오른쪽으로 튼다
// USTRUCT : 언리얼이 아는 구조체 (디테일 창에 묶음으로 보이고 저장된다). 아레나마다 기둥 위치가 달라서 보스(레벨)마다 조정한다
// ============================================================================
USTRUCT(BlueprintType)
struct FIntroFlyoverSettings
{
	GENERATED_BODY()

	// 전체 시간(초)
	UPROPERTY(EditAnywhere)
	float	Duration = 7.f;

	// 시작 : 아레나 중심에서의 거리·높이(cm)
	UPROPERTY(EditAnywhere)
	float	StartDistance = 1800.f;

	UPROPERTY(EditAnywhere)
	float	StartHeight = 1200.f;

	// 중심을 축으로 도는 각도(도). 음수면 반대 방향으로 돈다
	UPROPERTY(EditAnywhere)
	float	OrbitDegrees = 120.f;

	// 돌기가 끝나는 시각(초)과 그때의 거리·높이(cm)
	UPROPERTY(EditAnywhere)
	float	OrbitEndTime = 4.f;

	UPROPERTY(EditAnywhere)
	float	OrbitEndDistance = 1000.f;

	UPROPERTY(EditAnywhere)
	float	OrbitEndHeight = 400.f;

	// (11/5 녹화 확인 뒤) 마지막 구도는 "보스 키(캡슐 높이 × 크기 배율)의 몇 배"로 정한다.
	// cm로 정하면 오로라(키 약 250)와 세바로그(키 약 360)에 같은 값을 쓸 수 없어서.
	// 이름을 바꾼 이유 : 예전 이름(FinalDistance 700 등)이 레벨에 저장돼 있어도 비율로 잘못 읽히지 않게.
	// 거리 : 보스에서 기디언 쪽으로 키 × 몇 배 앞 (1.7 → 보스가 화면 높이의 약 3/4)
	UPROPERTY(EditAnywhere)
	float	FinalDistanceRatio = 1.7f;

	// 카메라 높이 : 보스 발바닥에서 키 × 몇 배 위 (0.4 = 허리 아래 → 살짝 올려다봄)
	UPROPERTY(EditAnywhere)
	float	FinalHeightRatio = 0.4f;

	// 바라보는 높이 : 보스 발바닥에서 키 × 몇 배 위 (카메라 높이보다 높으면 올려다본다)
	UPROPERTY(EditAnywhere)
	float	FinalLookRatio = 0.55f;

	// ---------------------------------------------------------------- (11/5) 보스 카드 클로즈업 (보더랜드식 상체 구도)
	// 보스가 멈추는 순간의 실제 머리·골반 뼈 위치로 구도를 계산한다 → 등장 동작 중 자세가 어떻든 맞는다
	// 머리~골반 길이가 화면 높이의 몇 배를 채우는지 (0.85 = 거의 꽉)
	UPROPERTY(EditAnywhere)
	float	CardFill = 0.85f;

	// 보스 정면에서 몇 도 돌아간 곳에서 찍는지 (+ = 보스의 오른쪽 어깨 쪽 → 몸이 화면 가운데를 향한다)
	UPROPERTY(EditAnywhere)
	float	CardOrbitDegrees = 20.f;

	// 밀고 들어가는 시간(초). 천천히 출발 → 천천히 도착 (보스를 중심으로 호를 그리며)
	UPROPERTY(EditAnywhere)
	float	CardPushTime = 0.7f;

	// 보스가 멈추고 카드(띠 → 이름 → 부제)가 나오기까지(초). 카메라가 거의 도착할 때 나오게
	UPROPERTY(EditAnywhere)
	float	CardDelay = 0.4f;

	// 등장 동작 따라가기 : 바라보는 점·카메라 자리가 따라가는 속도 (낮을수록 늦고 무겁게, 자잘한 흔들림은 무시)
	UPROPERTY(EditAnywhere)
	float	TrackLookSpeed = 2.5f;

	UPROPERTY(EditAnywhere)
	float	TrackMoveSpeed = 1.5f;

	// 카드가 떠 있는 동안 천천히 더 다가가는 비율 (0.03 = 거리의 3%)
	UPROPERTY(EditAnywhere)
	float	CardDrift = 0.03f;

	// 구도 계산에 쓰는 뼈 이름 (파라곤 스켈레톤 기준). 못 찾으면 캡슐 높이로 대신 계산한다
	UPROPERTY(EditAnywhere)
	FName	HeadBone = TEXT("head");

	UPROPERTY(EditAnywhere)
	FName	PelvisBone = TEXT("pelvis");

	// ---------------------------------------------------------------- (11/5) 등장 동작의 첫 순간 (세바로그 : 하늘에서 떨어지는 망치)
	// 둘러보기 마지막 구도에서 몇 도 위를 올려다보며 도착하는지 (웅크린 첫 자세를 화면 밖으로). 0 = 보스를 그대로 봄
	UPROPERTY(EditAnywhere)
	float	FinalLookUpDegrees = 0.f;

	// 등장 동작이 시작되면 골반보다 먼저 따라갈 소켓·뼈 후보 (예 : 망치). 비어 있으면 바로 골반.
	// (11/6) 하나가 아니라 여러 개 : 매 프레임 그중 가장 높은 것의 높이를 따른다 (어느 뼈가 떨어지는 망치인지 몰라도 되게)
	UPROPERTY(EditAnywhere)
	TArray<FName>	LeadSockets;

	// 그 소켓을 따라가는 속도 (골반 따라가기 TrackLookSpeed보다 빠르게 = 떨어지는 망치를 쫓는다)
	UPROPERTY(EditAnywhere)
	float	LeadFollowSpeed = 12.f;

	// 소켓이 발바닥 + 키 × 이 비율 아래로 내려오면 "바닥에 닿았다" → 골반 따라가기로 넘어간다
	UPROPERTY(EditAnywhere)
	float	LeadLandRatio = 0.4f;

	// ---------------------------------------------------------------- (11/5) 석상 방식 (비출 대상이 있으면 돌기 대신)
	// 레벨에 놓인 보스에서 목록으로 고른다 (예 : 세바로그 → MonoStatue). 비어 있으면 돌기 방식
	// 앞(아레나 쪽) 낮게 얼굴 올려다보기 → 옆으로 미끄러짐 → 옆 → 등 뒤 어깨 위에서 아레나 → 보스 앞
	UPROPERTY(EditAnywhere)
	TObjectPtr<AActor>	FocusActor;

	// 석상 앞 거리 (석상 키 × 몇 배)
	UPROPERTY(EditAnywhere)
	float	FocusFrontRatio = 1.f;

	// 처음 옆으로 미끄러지는 폭 (석상 키 × 몇 배, 왼쪽 → 오른쪽)
	UPROPERTY(EditAnywhere)
	float	FocusSlideRatio = 0.25f;

	// 등 뒤 어깨 위 높이 (머리 끝 + 석상 키 × 몇 배)
	UPROPERTY(EditAnywhere)
	float	FocusOverHeightRatio = 0.5f;

	// 카메라 가로 시야각(도). 마지막 구도에서 보스를 왼쪽 1/3에 두는 각도 계산에도 쓴다
	UPROPERTY(EditAnywhere)
	float	FieldOfView = 70.f;
};

// ============================================================================
// ABossBase
// 모든 보스의 공통 부모 클래스다. (세바로그·오로라. 10/26 : 보스 2를 붙이며 세바로그 전용 에셋 경로를 자식으로 옮김)
// 보스마다 다른 것(메시·동작·이펙트·이름·수치)은 자식 생성자가 채우고, 여기는 칸과 공통 흐름만 둔다.
// GAS·체력·사망은 부모 AGameCharacter가 처리하고, 여기서는 보스만의 것을 더한다.
//   - 크기 배율 : 2페이즈에서 미니언을 흡수할 때마다 커진다 (SetBossScale)
//   - 보스 HP 화면 표시 (체력바 UI는 11/2)
//   - 조준선(Visibility)이 몸에 걸리도록 충돌 설정
// 앞으로 추가할 것 : 패턴 목록(10/19~), 페이즈(10/27), 흡수 단계(10/28)
// ============================================================================
UCLASS()
class UNSEALEDGOD_API ABossBase : public AGameCharacter
{
	GENERATED_BODY()

public:
	ABossBase();

protected:
	// 기본 크기 배율. 자식 클래스가 생성자에서 정한다. (세바로그 = 1.2)
	// 레벨에 놓은 크기가 아니라 이 값으로 크기를 정한다 → 흡수 성장 때 같은 함수로 키울 수 있다.
	UPROPERTY(EditAnywhere, Category = "Boss")
	float	mBaseScale = 1.f;

	// 메시를 바닥에서 띄우는 높이(cm, 크기 배율 1.0 기준). 세바로그는 사신이라 떠 있다.
	// 캡슐(충돌·이동)은 바닥에 그대로 두고, 보이는 메시만 올린다 → 길찾기·판정에는 영향 없음.
	// 메시 위치는 액터 크기를 같이 받으므로, 보스가 커지면 떠 있는 높이도 비례해서 높아진다.
	// 레벨에 놓은 보스의 디테일에서 바꾸면 뷰포트에 바로 반영된다(OnConstruction).
	UPROPERTY(EditAnywhere, Category = "Boss")
	float	mHoverHeight = 0.f;

	// (10/26) 화면 아래 보스 체력바 위에 뜨는 이름. 자식 생성자가 정한다. (전에는 HUD 코드에 세바로그 이름이 고정돼 있었음)
	UPROPERTY(EditAnywhere, Category = "Boss")
	FText	mBossName;

	// ---------------------------------------------------------------- 시작 연출 카드 (11/5)
	// 맵 둘러보기 때 왼쪽 아래 맵 이름 카드 (마리오카트 코스 소개). 보스 하나 = 아레나 하나라 보스가 자기 무대를 안다.
	// 자식 생성자가 기본값을 정하고, 레벨에 놓은 보스의 디테일에서 바꿀 수 있다. 아이콘이 없으면 아이콘 칸만 숨는다.
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	FText	mStageName;

	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	TObjectPtr<UTexture2D>	mStageIcon;

	// 보스 소개 카드 (보더랜드) : 영어 이름 크게 + 한글 칭호 작게
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	FText	mIntroName;

	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	FText	mIntroTitle;

	// (11/5) 보더랜드식 이름 로고 (AI 이미지, 배경 투명). 있으면 글자(mIntroName) 대신 이 그림을 띄운다.
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	TObjectPtr<UTexture2D>	mIntroNameImage;

	// (11/5) 부제 그림 (AI 이미지, 배경 투명). 있으면 글자(mIntroTitle) 대신 이 그림을 띄운다.
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	TObjectPtr<UTexture2D>	mIntroTitleImage;

	// (11/5) 보스 "뒤"에 깔리는 배경 띠 (16:9, 배경 투명). 후처리 머티리얼이 보스 실루엣만 빼고 화면에 그린다 (카타가와 볼 방식)
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	TObjectPtr<UTexture2D>	mIntroBandTexture;

	// (11/5) 멈춘 순간 화면 가장자리에 번지는 보스 색 (보더랜드의 색 덧칠). 세바로그 초록 / 오로라 청록
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	FLinearColor	mIntroTintColor = FLinearColor(0.2f, 1.f, 0.3f, 0.6f);

	// 등장 동작의 몇 초에 보스를 멈추고 카드를 띄울지 (몽타주 시간, 재생 속도 1). 내가 몽타주에서 고른 순간
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	float	mIntroFreezeTime = 4.f;

	// (11/6) 시작 연출을 건너뛰면 화면이 밝아지기 시작한 뒤 몇 초 동안 보스가 가만히 있는지 (기디언이 자리 잡을 시간).
	// 보스마다 다르다 : 느리고 무거운 세바로그는 1.5초가 자연스럽고, 오로라는 길게 느껴졌다
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	float	mIntroSkipDelay = 1.5f;

	// (11/6) 등장 이펙트 (원작 LevelStart 이펙트 : 오로라 얼음 계단 / 세바로그 그림자 소용돌이). 등장 동작을 시작할 때 발밑에.
	// 몸에 붙이지 않고 그 자리에 놓는다 (오로라가 계단을 걸어 내려가도 계단은 제자리에 있어야 해서).
	// 방향·크기는 메시를 따른다 (원작 동작이 메시 기준으로 만들어져 있어서). 어긋나면 위치·회전 보정으로 맞춘다
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	TObjectPtr<UParticleSystem>	mIntroSpawnParticle;

	// 보정 : 위치(cm, 메시 기준 앞·옆·위) / 회전(도)
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	FVector	mIntroSpawnOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	FRotator	mIntroSpawnRotation = FRotator::ZeroRotator;

	// (11/10) 등장 동작의 "쾅" 순간 (세바로그 : 하늘의 망치가 땅에 꽂힘) : 소리 + 화면 흔들림. 시각은 몽타주 기준(초), 0보다 작으면 없음
	// 시각으로 정해서 시작 연출·다시 시도 모두 같은 순간에 난다
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	TObjectPtr<USoundBase>	mIntroImpactSound;

	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	float	mIntroImpactTime = -1.f;

	FTimerHandle	mIntroImpactTimer;

	void OnIntroImpact();

	// (11/6) 숨어 있다가 등장 동작을 시작한 뒤 몇 초 뒤에 보일지. 첫 프레임의 묵은 자세가 그려지지 않게 (3프레임 정도)
	float	mIntroRevealDelay = 0.05f;
	FTimerHandle	mIntroRevealTimer;

	void RevealForIntro();
	// (11/5) 맵 둘러보기 카메라 경로 (레벨에 "Intro" 시퀀스가 없을 때)
	UPROPERTY(EditAnywhere, Category = "Boss|Intro")
	FIntroFlyoverSettings	mIntroFlyover;

	// (10/26) 보스 체력바 틀 그림 (1910×280). 보스마다 다르다 : 세바로그 = 초록 뿔, 오로라 = 얼음 결정.
	// 체력 채움 색은 모든 보스가 같은 빨강이라 WBP_HUD에 둔다. (직접 결정 : 소울류처럼 "빨강 = 보스 체력" 통일)
	UPROPERTY(EditAnywhere, Category = "Boss")
	TObjectPtr<UTexture2D>	mHPBarFrame;

	// 이 보스의 애님 인스턴스. (사망 시 몽타주 정지 등에 사용)
	UPROPERTY()
	TObjectPtr<class UBossAnimInstance>	mAnimInst;

	// 이 보스가 사용할 비헤이비어 트리. 자식 클래스가 생성자에서 정한다. (세바로그 = BT_Sevarog)
	// 보스 컨트롤러가 빙의할 때 꺼내서 실행한다.
	UPROPERTY(EditAnywhere, Category = "Boss")
	TObjectPtr<UBehaviorTree>	mBehaviorTree;

	// ---------------------------------------------------------------- 패턴
	// 이 보스가 쓸 수 있는 패턴 어빌리티 목록. 자식 클래스가 생성자에서 채운다.
	// 게임 시작 시 전부 ASC에 등록(GiveAbility)하고, BTTask_RunPattern이 이 중에서 골라 실행한다.
	// 패턴을 추가할 때는 클래스를 만들어 이 목록에 넣기만 하면 된다. (BT는 그대로)
	UPROPERTY(EditAnywhere, Category = "Boss")
	TArray<TSubclassOf<class UGameplayAbility_BossPattern>>	mPatternClasses;

	// 지금 실행 중인 패턴, 바로 전에 쓴 패턴 (같은 패턴 연속 금지에 사용)
	TSubclassOf<class UGameplayAbility_BossPattern>	mCurrentPattern;
	TSubclassOf<class UGameplayAbility_BossPattern>	mLastPattern;

	// 후딜이 끝나는 시각(게임 시간, 초). 패턴이 끝날 때 패턴이 정해 준다.
	float	mPatternRecoveryEndTime = 0.f;

	// ---------------------------------------------------------------- 필살기 (기둥 엄폐 궁극기)
	// 체력 비율이 이 값들 이하로 내려갈 때마다 필살기를 한 번씩 쓴다. (75% · 50% · 25%, 큰 값부터)
	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	TArray<float>	mUltimateHPRatios = { 0.75f, 0.5f, 0.25f };

	// 아레나 중앙 (필살기를 쓰러 뛰어가는 곳). 레벨에 놓은 보스의 디테일에서 바꿀 수 있다.
	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	FVector	mArenaCenter = FVector::ZeroVector;

	// 다음에 확인할 비율의 칸 번호
	int32	mNextUltimateIndex = 0;

	// 쓰기를 기다리는 필살기가 끝나면 들어갈 페이즈. 0이면 기다리는 필살기 없음.
	// 기준 칸 번호 + 2 = 페이즈 (75% → 2, 50% → 3, 25% → 4). 한 번에 여러 기준을 넘으면 가장 높은 페이즈.
	int32	mPendingUltimatePhase = 0;

	// ---------------------------------------------------------------- 페이즈 (1 ~ 4)
	// 필살기 3번이 페이즈 경계. 페이즈가 오를 때마다 커지고 강해진다. (10/15 : 흡수 단계 대신 페이즈로)
	// 페이즈 N에 들어가면 ASC에 Boss.Phase.2 ~ Boss.Phase.N 태그가 모두 붙는다. → "N페이즈 이상" 조건으로 쓴다.
	int32	mPhase = 1;

	// ---------------------------------------------------------------- 과부하·지침 (10/19)
	// 이 페이즈부터 일반 패턴(우선순위 0)을 쓸 때마다 과부하 +1. 기준(4~6 랜덤)에 닿으면 "지침 대기" → 지침 패턴
	UPROPERTY(EditAnywhere, Category = "Boss|Exhaust")
	int32	mOverloadPhase = 3;

	UPROPERTY(EditAnywhere, Category = "Boss|Exhaust")
	int32	mOverloadMin = 4;

	UPROPERTY(EditAnywhere, Category = "Boss|Exhaust")
	int32	mOverloadMax = 6;

	// 지금 쌓인 과부하, 이번 기준, 지침 대기 중인지
	int32	mOverloadCount = 0;
	int32	mOverloadThreshold = 5;
	bool	mExhaustPending = false;

	// [테스트용] 이 페이즈부터 시작한다 (10/19). 1 = 원래 게임. 3이면 HP 50%·3페이즈 상태·물약 3개로 시작.
	// 레벨에 놓은 보스의 디테일에서 바꾼다. 다시 시도해도(레벨 다시 열기) 같은 값으로 시작한다. 테스트 후 1로 되돌릴 것.
	UPROPERTY(EditAnywhere, Category = "Boss|Debug", meta = (ClampMin = "1", ClampMax = "4"))
	int32	mDebugStartPhase = 1;

	// 페이즈별 크기 배율 (기본 크기 1.2 × 배율 → 1.2 / 1.3 / 1.4 / 1.5). 칸 번호 = 페이즈 - 1
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TArray<float>	mPhaseScaleMultipliers = { 1.f, 1.0833f, 1.1667f, 1.25f };

	// 페이즈별 공격력 배율 (기본 공격력 × 배율)
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TArray<float>	mPhaseAttackMultipliers = { 1.f, 1.2f, 1.4f, 1.6f };

	// 이 페이즈부터 패턴이 빨라진다 (패턴의 mFast 값 사용). 50% 필살기 = Victory = 3페이즈 진입
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	int32	mFastPhase = 3;

	// 페이즈별 걷는 속도 (cm/초). 기디언 앞 600 / 옆 550 / 뒤 500 기준 (10/15 테스트값)
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TArray<float>	mPhaseWalkSpeeds = { 500.f, 510.f, 530.f, 550.f };

	// 페이즈별 미니언 소환 수 (페이즈에 들어가면 한 번 소환). 1페이즈 0, 2페이즈 3, 3페이즈 5, 4페이즈 7
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TArray<int32>	mPhaseSummonCounts = { 0, 3, 5, 7 };

	// 페이즈가 시작되고 몇 초 뒤에 소환할지. (필살기 직후 바로 미니언까지 쏟아지면 숨 돌릴 틈이 없어서)
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	float	mSummonDelay = 5.f;

	// 소환을 기다리는 미니언 수(0이면 없음), 이번 페이즈가 시작된 시각(게임 시간)
	int32	mPendingSummonCount = 0;
	float	mPhaseStartTime = 0.f;

	// 페이즈별 망치 발광 (머티리얼 파라미터 HammerUp, M_Sevarog_ArmourRuins 안의 MF_SevarogArmorStack)
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TArray<float>	mPhaseHammerGlow = { 0.f, 0.33f, 0.66f, 1.f };

	// 메시의 머티리얼 슬롯마다 만든 다이내믹 머티리얼 인스턴스 (수업 AMonsterBaseGAS의 mMaterials와 같은 방식)
	// 페이즈 발광(HammerUp), 나중에 사망 디졸브(11/3)에 쓴다.
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>>	mMaterials;

	// ---------------------------------------------------------------- 점진 성장 (10/19)
	// 페이즈가 오를 때 크기를 한 번에 바꾸지 않고 이 시간(초) 동안 서서히 키운다. (순간적으로 확 커지면 어색해서)
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	float	mScaleGrowTime = 1.5f;

	// 지금 크기 배율, 성장 시작·목표 배율, 지난 시간, 성장 중인지
	float	mScaleMultiplier = 1.f;
	float	mScaleFrom = 1.f;
	float	mScaleTo = 1.f;
	float	mScaleElapsed = 0.f;
	bool	mGrowing = false;

	// ---------------------------------------------------------------- 필살기 빛·충격파 (10/22)
	// 보스 가슴에서 붉은 빛이 차오르고(기둥 뒤에 그림자 = 안전지대), 충격 순간 번쩍 + 충격파 이펙트.
	// 레벨의 보스 디테일 → Boss|Ultimate에서 숫자로 조정한다. (밝기 단위 : 칸델라)
	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	FLinearColor	mUltLightColor = FLinearColor(1.f, 0.12f, 0.05f);

	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	float	mUltLightRadius = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	float	mUltChargeIntensity = 10000.f;	// (10/22 : 50000 → 10000, 예고 원이 묻히지 않게 은은하게)

	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	float	mUltFlashIntensity = 600000.f;	// (10/22 : 터질 때만 확)

	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	float	mUltFadeTime = 0.4f;

	// 기 모으기(바닥 원이 차오르기 시작) 순간 보스에게 붙는 이펙트. 비워 두면 없음. (직접 선택 : P_UltActivate)
	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	TObjectPtr<UParticleSystem>	mUltChargeParticle;

	// 충격 순간 보스 발밑에 터뜨릴 이펙트. 비워 두면 터뜨리지 않는다. (팩의 Ultimate 폴더에서 골라 넣는다)
	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	TObjectPtr<UParticleSystem>	mUltImpactParticle;

	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	float	mUltImpactParticleScale = 1.f;

	// (10/26) 기 모으기 소리. 몸에 붙여 재생하고, 충격 순간 mUltChargeFadeTime초 동안 줄이며 끊는다.
	// → 소리가 차오르는 시간보다 길어도 항상 "충격 = 고조 끝"이 맞는다. (충격 "쾅"은 게임 모드 표의 UltimateImpact)
	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	TObjectPtr<class USoundBase>	mUltChargeSound;

	UPROPERTY(EditAnywhere, Category = "Boss|Ultimate")
	float	mUltChargeFadeTime = 0.15f;

	// 재생 중인 기 모으기 소리. (끊을 때 쓴다)
	// UAudioComponent : 소리 하나를 재생하는 컴포넌트. 볼륨을 줄이거나 멈출 수 있다.
	UPROPERTY()
	TObjectPtr<class UAudioComponent>	mUltChargeAudio;

	// ---------------------------------------------------------------- 페이즈 전환 소리 (10/26)
	// 대사 : 칸 0 = 2페이즈, 1 = 3페이즈, 2 = 4페이즈 (Sevarog_Ability_Passive_Level_02~04). 몸에 붙여 재생.
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TArray<TObjectPtr<class USoundBase>>	mPhaseVoices;

	// 효과음(브람) : 모든 페이즈 전환에 공통. 화면 전체에 들리게 2D.
	UPROPERTY(EditAnywhere, Category = "Boss|Phase")
	TObjectPtr<class USoundBase>	mPhaseStinger;

	// ---------------------------------------------------------------- 정면 방패 (10/28, 오로라 얼음 돌진)
	// State.FrontGuard 태그가 붙어 있는 동안, 몸 정면 ±mFrontGuardHalfAngle도 안에 맞은 투사체는 피해 없이 막힌다.
	// 태그는 패턴(얼음 돌진)이 질주하는 동안만 붙인다. 위에서 떨어지는 운석·바닥 장판은 투사체 정면 판정이 아니라 안 막힌다.
	UPROPERTY(EditAnywhere, Category = "Boss|FrontGuard")
	float	mFrontGuardHalfAngle = 60.f;

	// 막았을 때 맞은 자리에서 튀는 이펙트·소리. 비워 두면 없음.
	UPROPERTY(EditAnywhere, Category = "Boss|FrontGuard")
	TObjectPtr<UParticleSystem>	mFrontGuardBlockParticle;

	UPROPERTY(EditAnywhere, Category = "Boss|FrontGuard")
	TObjectPtr<class USoundBase>	mFrontGuardBlockSound;

	// ---------------------------------------------------------------- 사망 후 사라지기 (10/19)
	// 머티리얼의 MF_DeathFade 파라미터 "FadeOut" : 0 원래 → 0.5 완전히 검게 → 그 뒤 노이즈 무늬로 깎여 사라짐.
	// 쓰러지는 동작(Death_front)이 끝나면 mDeathFadeTime초 동안 0 → mDeathFadeEnd로 올리고, 끝나면 숨긴다.
	UPROPERTY(EditAnywhere, Category = "Boss|Death")
	float	mDeathFadeTime = 2.f;

	// FadeOut의 끝 값. 1.0에서도 다 안 사라져서 조정 가능하게 둔다. (마지막엔 어차피 숨긴다)
	UPROPERTY(EditAnywhere, Category = "Boss|Death")
	float	mDeathFadeEnd = 1.5f;

	// 사라지기 시작할 때 터지는 이펙트 (영혼 불씨)
	UPROPERTY(EditAnywhere, Category = "Boss|Death")
	TObjectPtr<UParticleSystem>	mDeathFadeParticle;

	// 쓰러지는 동작. 길이를 읽어서 언제 사라지기 시작할지 정한다. (ABP가 재생하는 것과 같은 애니메이션)
	UPROPERTY()
	TObjectPtr<UAnimSequence>	mDeathAnim;

	// (10/21) 페이드 일반화 : 사망 사라지기와 순간이동(속박·망령)의 사라짐·나타남이 같은 Tick 코드를 쓴다.
	// FadeOut 값을 From → To로 Time초 동안 바꾸고, bHideAtEnd면 끝에서 숨긴다.
	float	mFadeFrom = 0.f;
	float	mFadeTo = 0.f;
	float	mFadeTime = 0.f;
	float	mFadeElapsed = 0.f;
	bool	mFadeHideAtEnd = false;
	bool	mFading = false;

	// 순간이동으로 사라질 때 FadeOut 끝 값 (사망과 같은 1.5 : 검게 → 깎여 사라짐)
	float	mVanishFadeEnd = 1.5f;

	// ---------------------------------------------------------------- 등장 연출 (10/21)
	// 등장 동작 (원작 LevelStart_Montage를 복제해 슬롯만 DefaultSlot으로). 경로는 자식 생성자가 정한다.
	// 몽타주가 없으면 등장 동작 없이 게임 모드의 대체 시간만 기다린다.
	UPROPERTY()
	TObjectPtr<UAnimMontage>	mIntroMontage;

	FString	mIntroMontagePath;

	bool	mIntroPlaying = false;

	// 순간이동으로 사라져 있는지 (숨김 + 충돌 꺼짐)
	bool	mVanished = false;

	// 사라질 때·나타날 때 터지는 그림자 잔상 이펙트
	UPROPERTY(EditAnywhere, Category = "Boss|Teleport")
	TObjectPtr<UParticleSystem>	mVanishParticle;
	FTimerHandle	mDeathFadeTimer;

	// ---------------------------------------------------------------- 패배 연출 (10/19)
	// 기디언이 죽으면 : (영혼 뽑기) → 승리 동작. 동작은 자식 생성자가 정한다.
	//   세바로그 = 영혼 뽑기(Soul_Siphon) → 박수(Emote_Clap_T1) / 오로라 = 영혼 뽑기 없이 바로 비웃음(Emote_Laugh_T1)
	// 영혼 뽑기 칸이 비어 있으면 바로 승리 동작으로 간다. (10/26 : 박수 → "승리 동작"으로 이름 일반화)
	UPROPERTY()
	TObjectPtr<UAnimSequence>	mSoulSiphonAnim;

	UPROPERTY()
	TObjectPtr<UAnimSequence>	mVictoryEmoteAnim;

	// (10/29) 보이스 : 승리 동작을 시작할 때 / 쓰러질 때. 자식 생성자가 정한다 (비워 두면 없음)
	UPROPERTY()
	TObjectPtr<class USoundBase>	mVictoryVoice;

	UPROPERTY()
	TObjectPtr<class USoundBase>	mDeathVoice;

	// 영혼을 뽑히는 동안 쓰러진 기디언 몸에 붙는 이펙트
	UPROPERTY(EditAnywhere, Category = "Boss|Victory")
	TObjectPtr<UParticleSystem>	mSoulSiphonParticle;

	// 영혼 뽑기 재생 속도. 원래 속도는 너무 짧아 보기 어려워서 느리게 (10/19 내 판단 : 0.5 = 2배 길게)
	UPROPERTY(EditAnywhere, Category = "Boss|Victory")
	float	mSoulSiphonPlayRate = 0.5f;

	// 승리 동작 반복 횟수 (세바로그 박수 3번)
	UPROPERTY(EditAnywhere, Category = "Boss|Victory")
	int32	mVictoryEmoteLoopCount = 3;

	FTimerHandle	mVictoryTimer;

	// ---------------------------------------------------------------- 기둥
	// 레벨에서 액터 태그 "Pillar"가 붙은 기둥 판정(BO_Pillar)들. 점프하는 동안 통과하려고 시작할 때 찾아 둔다.
	TArray<TWeakObjectPtr<AActor>>	mPillars;

	// (근접 판정·디버그 그리기·GetTarget은 미니언도 쓰므로 공통 부모 AGameCharacter로 옮겼다. 10/15)

public:
	// 이 보스의 비헤이비어 트리 (AGameCharacter의 virtual 함수를 덮어씀 → AEnemyController가 실행)
	virtual UBehaviorTree* GetBehaviorTree() const override
	{
		return mBehaviorTree;
	}

	// 지금 쓸 수 있는 패턴 중 하나를 가중치 랜덤으로 고른다. 없으면 nullptr.
	TSubclassOf<class UGameplayAbility_BossPattern> SelectPattern(AActor* Target)	const;

	// 고른 패턴을 실행한다. 실행됐으면 true.
	bool ActivatePattern(TSubclassOf<class UGameplayAbility_BossPattern> PatternClass);

	// 실행한 패턴이 아직 진행 중인지. (BTTask_RunPattern이 매 프레임 확인)
	bool IsPatternActive()	const;

	// 패턴이 끝난 뒤 쉬는 시간(후딜). 패턴 어빌리티가 끝날 때 부른다.
	void StartPatternRecovery(float Seconds);

	// 아직 후딜 중인지. (BTTask_RunPattern은 패턴 + 후딜이 모두 끝나야 완료)
	bool IsInPatternRecovery()	const;

	// 필살기 : 쓰기를 기다리는 게 있는지 / 꺼내 쓰기 (돌려주는 값 = 필살기가 끝나면 들어갈 페이즈)
	bool HasPendingUltimate()	const
	{
		return mPendingUltimatePhase > 0;
	}

	int32 ConsumePendingUltimate();

	// 페이즈 올리기. 필살기가 끝날 때 부른다. 크기·공격력·태그를 바꾼다.
	// 앞으로 여기에 더할 것 : 외형 변화(Stage 모프·망치 발광), 미니언 소환 대기
	void EnterPhase(int32 NewPhase);

	int32 GetPhase()	const
	{
		return mPhase;
	}

	// 미니언 소환 : 지금 소환할 때인지 (기다리는 수가 있고 + 페이즈 시작 후 mSummonDelay초 지남)
	bool CanSummonMinions()	const;

	// 소환할 수를 꺼내 쓴다. (돌려준 뒤 0으로)
	int32 ConsumePendingSummon();

	// 지침 : 대기 중인지 (지침 패턴의 사용 조건) / 꺼내 쓰기 (과부하 초기화 + 다음 기준 다시 뽑기)
	bool IsExhaustPending()	const
	{
		return mExhaustPending;
	}

	void ConsumeExhaust();

	// (11/4) 과부하 횟수와 상관없이 지침 대기를 바로 세운다. → 다음 선택에서 지침 패턴(우선순위 8)이 나온다.
	// 오로라 : 필살기(얼음 지진)를 피하면 지친다. (세바로그는 지금처럼 과부하로만)
	void RequestExhaust()
	{
		mExhaustPending = true;
	}

	// 지금 빠른 페이즈(3페이즈 이상)인지. 패턴이 재생 속도·후딜을 정할 때 본다.
	bool IsFastPhase()	const
	{
		return mPhase >= mFastPhase;
	}

	const FVector& GetArenaCenter()	const
	{
		return mArenaCenter;
	}

	// HUD가 보스 이름을 읽어 간다. (10/26)
	const FText& GetBossName()	const
	{
		return mBossName;
	}

	UTexture2D* GetHPBarFrame()	const
	{
		return mHPBarFrame;
	}

	// (10/28) 투사체가 HitPoint에 맞았을 때 정면 방패로 막는지 확인한다. 막으면 막는 이펙트·소리를 내고 true.
	// 투사체(AProjectileBase)가 피해를 주기 전에 부른다.
	bool TryBlockFrontHit(const FVector& HitPoint);

	// 점프하는 동안 기둥을 통과(true) / 원래대로(false). (10/15 : 점프 경로의 기둥 위에 착지하던 버그)
	// 바닥은 그대로 막으므로 정상 착지한다.
	void SetIgnorePillars(bool bIgnore);

	// (11/6) Desired 자리가 기둥·벽 안이면 Toward 쪽으로 25cm씩 당겨 보스가 설 수 있는 빈 바닥(발 높이)을 돌려준다.
	// 도약 착지·순간이동(망령·속박·필살기 안전장치)이 기둥 위에 올라가지 않게
	FVector FindOpenGround(const FVector& Desired, const FVector& Toward) const;

	// (10/21) 순간이동 패턴(속박·망령)용. Time초 동안 검게 깎여 사라진 뒤 숨김 + 충돌 끔 / 다시 보이고 충돌 켬 + Time초 동안 원래 모습으로.
	void StartVanish(float Time);
	void StartAppear(float Time);

	// (10/22) 필살기 빛을 보스 가슴 높이(HeightOffset)에 만들어 차오르기 시작한다. 반환값 = 만든 빛 (충격 때 Flash)
	class AUltimateLight* SpawnUltimateLight(float HeightOffset, float ChargeTime);

	// (10/22) 충격 순간 이펙트 (발밑)
	void PlayUltimateImpactEffect();

	// (10/22) 기 모으기 시작 이펙트 (몸에 붙임) + (10/26) 기 모으기 소리
	void PlayUltimateChargeEffect();

	// (10/26) 기 모으기 소리를 줄이며 끊는다. 충격 순간 / 보스 사망 / 전투 끝에서 부른다.
	void StopUltimateChargeSound();

	// (10/21) 등장 연출 : 동작 재생 + AI 멈춤 + 무적. 반환값 = 동작 길이(초) / 끝 : AI 다시 시작 + 무적 해제
	// (11/5) 처음 들어올 때는 HoldForIntro(true)로 먼저 멈추고 숨겨 둔 뒤(맵 둘러보기), 나중에 StartIntro로 나타나며 동작
	float StartIntro();
	void EndIntro();

	// (11/5) 등장 준비 : AI 멈춤 + 무적. bHide면 숨긴다 (맵 둘러보기 동안 빈 아레나를 보여 주려고)
	void HoldForIntro(bool bHide);

	// (11/5) 보스 소개 카드 동안 보스만 멈춘다 (정지 화면). CustomTimeDilation : 이 액터만의 시간 배율 (0 = 멈춤)
	void SetIntroFrozen(bool bFrozen);

	// (11/5) 건너뛰기 : 등장 동작을 바로 끝낸다
	void StopIntroMontage();

	// (11/5) 커스텀 뎁스 : 보스만 따로 그리는 깊이 정보를 켠다/끈다. 배경 띠 후처리가 이것으로 보스 실루엣을 찾아 그 뒤에만 띠를 그린다
	void SetIntroMaskEnabled(bool bEnabled);

	const FText& GetStageName()	const
	{
		return mStageName;
	}

	UTexture2D* GetStageIcon()	const
	{
		return mStageIcon;
	}

	const FText& GetIntroName()	const
	{
		return mIntroName;
	}

	const FText& GetIntroTitle()	const
	{
		return mIntroTitle;
	}

	float GetIntroSkipDelay()	const
	{
		return mIntroSkipDelay;
	}

	float GetIntroFreezeTime()	const
	{
		return mIntroFreezeTime;
	}

	UTexture2D* GetIntroNameImage()	const
	{
		return mIntroNameImage;
	}

	UTexture2D* GetIntroTitleImage()	const
	{
		return mIntroTitleImage;
	}

	UTexture2D* GetIntroBandTexture()	const
	{
		return mIntroBandTexture;
	}

	const FLinearColor& GetIntroTintColor()	const
	{
		return mIntroTintColor;
	}

	const FIntroFlyoverSettings& GetIntroFlyover()	const
	{
		return mIntroFlyover;
	}

	bool IsVanished()	const
	{
		return mVanished;
	}

	// 수평 위치 Location으로 순간이동하고 그 아래 바닥에 선다. (필살기 안전장치 : 항상 중앙에서 시전)
	void TeleportToGround(const FVector& Location);

protected:
	virtual void BeginPlay() override;

	// 액터가 "만들어지거나 에디터에서 값이 바뀔 때마다" 호출되는 AActor의 virtual 함수.
	// 게임 실행 전에도(에디터에서 배치·수정할 때) 불리므로, 띄우는 높이를 뷰포트에서 바로 확인할 수 있다.
	virtual void OnConstruction(const FTransform& Transform) override;

	// 화면에 보스 HP를 표시하고, 부모의 처리(0이면 사망)를 이어서 한다.
	virtual void OnHPChanged(const FOnAttributeChangeData& Data) override;

	// 성장·사라지기가 진행 중일 때만 켜지는 Tick. (평소엔 꺼져 있다)
	virtual void Tick(float DeltaTime) override;

	// (10/28) 페이즈가 오른 직후 부른다. 기본은 아무것도 안 한다. 자식이 덮어써 자기만의 변화를 더한다.
	// (세바로그의 성장·발광은 모든 보스 공통 값이라 EnterPhase 안에 있고, 오로라의 메시 교체처럼 보스 전용인 것만 여기)
	virtual void OnPhaseEntered(int32 NewPhase) {}

	// 쓰러진 뒤 사라지기 시작 (Death에서 타이머로 부른다)
	// virtual (10/26) : 보스마다 사라지는 방식이 다르다. 세바로그 = 이 함수(검게 깎여 사라짐),
	// 오로라 = override해서 얼어붙었다가 얼음 파편으로 부서짐. 부르는 쪽(Death)은 어떤 보스인지 몰라도 된다.
	virtual void StartDeathFade();

	// FadeOut을 From → To로 Time초 동안 (Tick이 진행). bHideAtEnd : 끝나면 숨길지
	void StartFade(float From, float To, float Time, bool bHideAtEnd);

	// [테스트용] mDebugStartPhase 페이즈로 바로 시작 (BeginPlay 다음 프레임)
	void ApplyDebugStartPhase();

	// 머티리얼 슬롯 전부에 같은 스칼라 파라미터 값을 넣는다. (HammerUp·FadeOut)
	void SetMaterialsScalar(FName ParameterName, float Value);

public:
	// 보스 크기를 정한다. 최종 크기 = 기본 크기 × Multiplier.
	// 시작할 때 1.0으로 한 번 부르고, 페이즈가 오를 때는 Tick이 매 프레임 조금씩 부른다(점진 성장).
	void SetBossScale(float Multiplier);

	// 사망 처리. 부모(이동 정지·충돌 끄기)에 보스 전용 처리를 더한다. (승리 연출은 11/3)
	virtual void Death() override;

	// 전투 끝 (AGameCharacter의 virtual 덮어쓰기). 패배(기디언 사망)면 패턴을 멈추고 영혼 뽑기 → 박수.
	virtual void OnBattleEnd(bool bPlayerWon) override;

protected:
	// 영혼 뽑기가 끝나면(또는 뽑기가 없으면 바로) 승리 동작
	void PlayVictoryEmote();
};
