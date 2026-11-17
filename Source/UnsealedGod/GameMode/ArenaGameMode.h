// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../GameInfo.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaGameMode.generated.h"

// ----------------------------------------------------------------------------
// 타격감(히트스톱·카메라 흔들림)을 줄 "사건"의 종류. (10/21)
// 부르는 쪽은 "무슨 일이 일어났는지"만 알리고, 얼마나 멈추고 흔들지는 게임 모드의 표가 정한다.
// → 수치를 바꿀 때 이 파일 한 곳만 보면 된다.
// enum class : 이름이 겉으로 새지 않는 열거형. 쓸 때 EHitFeedback::PlayerHitHeavy처럼 앞에 붙여 쓴다.
// ----------------------------------------------------------------------------
enum class EHitFeedback : uint8
{
	PlayerHitLight,		// 기디언 작은 피격 (보스 1·2타, 미니언)
	PlayerHitHeavy,		// 기디언 큰 피격 (3타·돌진 마무리·도약 착지 → 넘어짐)
	StrongShotHit,		// 기디언 평타 막타(C) 적중
	MeteorExplode,		// Q 포털 운석 폭발 (적중 여부와 상관없이 흔들림)
	LeapLand,			// 보스 도약 착지 (피해도 흔들림)
	UltimateImpact,		// 보스 필살기 충격 (기둥 뒤에서도 흔들림)
	CryoseismBlast,		// (10/29) 오로라 얼음 지진 폭발 : 흔들림은 필살기 충격과 같고 소리만 얼음
	IntroSlam,			// (11/6) 시작 연출 세바로그 망치가 땅에 꽂히는 순간 (거리 감쇠 없는 흔들림)
	BossDeath			// 보스 사망 (마지막 일격)
};

// ============================================================================
// AArenaGameMode
// 아레나 레벨의 게임 규칙을 담당한다. (수업 코드의 TestGameMode와 같은 역할)
//   - 플레이어로 기디언을 쓴다.
//   - (10/19) 전투 끝 : 기디언이 죽으면 패배, 보스가 죽으면 승리.
//     레벨의 모든 캐릭터에게 virtual OnBattleEnd를 알리고, 잠시 뒤 결과 화면(WBP_Result)을 띄운다.
//   - (10/21) 타격감 : PlayHitFeedback(사건)으로 히트스톱·카메라 흔들림을 한 곳에서 처리한다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

protected:
	// 결과 화면 위젯 블루프린트. 처음 쓸 때 경로로 불러온다. (나중에 만든 에셋도 재시작 없이)
	UPROPERTY()
	TSubclassOf<class UResultWidget>	mResultWidgetClass;

	FString	mResultWidgetPath = TEXT("/Game/UnsealedGod/UI/WBP_Result.WBP_Result_C");

	// (10/21) 전투 화면(HUD). 시작할 때 만들어 붙이고, 결과 화면이 뜨면 숨긴다.
	UPROPERTY()
	TObjectPtr<class UPlayerHUDWidget>	mHUDWidget;

	FString	mHUDWidgetPath = TEXT("/Game/UnsealedGod/UI/WBP_HUD.WBP_HUD_C");

	// (11/2) 일시정지 메뉴. 처음 열 때 한 번 만들고, 닫을 때는 화면에서 떼기만 해서 다음에 다시 쓴다.
	UPROPERTY()
	TObjectPtr<class UPauseWidget>	mPauseWidget;

	FString	mPauseWidgetPath = TEXT("/Game/UnsealedGod/UI/WBP_Pause.WBP_Pause_C");

	// ---------------------------------------------------------------- 등장 연출 (10/21)
	// 짧은 연출 : 기디언(무릎 → 일어섬)과 보스가 등장 동작을 동시에 재생, 그동안 입력·AI 정지.
	// (11/5) 다시 시도·RESTART(레벨 옵션 "Retry")는 짧은 연출만. 처음 들어올 때는 아래의 시작 연출.
	FTimerHandle	mIntroTimer;

	// ---------------------------------------------------------------- 시작 연출 (11/5 피드백 : 바로 싸우지 말고 맵과 보스를 먼저 소개)
	// ① 맵 둘러보기 : 레벨에 놓은 시퀀스(태그 "Intro")를 재생. 보스는 숨겨 두고, 왼쪽 아래 맵 카드
	// ② 보스 소개   : 시퀀스가 끝난 구도에서 보스가 나타나며 등장 동작 → mIntroFreezeTime에 보스만 멈추고 보스 카드
	// ③ 전투 시작   : 카메라가 기디언 뒤로 돌아온 뒤 HUD
	// 스페이스바로 언제든 건너뛴다. 시퀀스·위젯이 없으면 짧은 연출로 대신한다.
	bool	mRetry = false;			// 레벨 옵션 "Retry" (다시 시도·RESTART)
	bool	mCinematicPlaying = false;

	FName	mIntroSequenceTag = TEXT("Intro");
	FString	mIntroWidgetPath = TEXT("/Game/UnsealedGod/UI/WBP_BossIntro.WBP_BossIntro_C");

	UPROPERTY()
	TObjectPtr<class ALevelSequenceActor>	mIntroSequence;

	UPROPERTY()
	TObjectPtr<class UBossIntroWidget>	mIntroWidget;

	UPROPERTY()
	TObjectPtr<class ABossBase>	mIntroBoss;

	// (11/5) 보스 뒤 배경 띠 : 레벨의 후처리 볼륨에 얹는 머티리얼 (보스 실루엣만 빼고 화면에 띠 그림을 그린다)
	FString	mIntroBandMaterialPath = TEXT("/Game/UnsealedGod/UI/M_PP_IntroBand.M_PP_IntroBand");

	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic>	mIntroBandMaterial;

	UPROPERTY()
	TObjectPtr<class APostProcessVolume>	mIntroPostProcess;

	float	mStageCardDelay = 0.5f;		// 둘러보기가 시작되고 맵 카드가 들어올 때까지(초)
	float	mBossCardHoldTime = 2.2f;	// 보스가 멈추고 카드가 떠 있는 시간(초). (11/5) 띠·이름·부제 시차(0.6초)만큼 1.8 → 2.2
	float	mCameraReturnTime = 1.f;	// 카메라가 기디언 뒤로 돌아오는 시간(초)

	// (11/6) 건너뛰기 : 어두워지기 → (정리) → 밝아지기, 보스는 밝아지기 시작부터 준비 시간(보스마다, ABossBase::mIntroSkipDelay) 뒤에 움직인다
	float	mSkipFadeOutTime = 0.25f;
	float	mSkipFadeInTime = 0.5f;
	bool	mSkipping = false;		// 어두워지는 중 스페이스바를 또 눌러도 한 번만
	FTimerHandle	mSkipTimer;	FTimerHandle	mBossGraceTimer;

	FTimerHandle	mStageCardTimer;
	FTimerHandle	mCinematicTimer;		// 다음 단계 예약 (한 번에 하나)

	// (11/5) 맵 둘러보기 카메라 (시퀀스가 없을 때 코드가 경로를 계산해 움직인다)
	UPROPERTY()
	TObjectPtr<class ACameraActor>	mFlyoverCamera;

	// 경로 : 시간 → 카메라 위치 / 시간 → 바라보는 점. FInterpCurveVector = 점 몇 개를 부드러운 곡선으로 잇는 엔진 자료형
	FInterpCurveVector	mFlyoverPositions;
	FInterpCurveVector	mFlyoverTargets;

	float	mFlyoverElapsed = 0.f;
	float	mFlyoverDuration = 0.f;
	bool	mFlyoverActive = false;

	// (11/5) 둘러보기 다음 카메라 단계 : 등장 동작을 따라가기(Track) → 카드 클로즈업(Card)
	// enum class : 이름이 바깥과 섞이지 않는 열거형 (EIntroCamera::Track 처럼 앞에 이름을 붙여 쓴다)
	enum class EIntroCamera : uint8 { None, Track, Card };

	EIntroCamera	mIntroCamera = EIntroCamera::None;

	// 따라가기 : 보스 위치에서 카메라까지의 거리(둘러보기 끝 구도 그대로 유지 → 보스가 걸으면 같이 옮겨 간다)
	FVector	mTrackOffset = FVector::ZeroVector;

	// 따라가기가 바라보는 점 (골반 쪽으로 천천히 옮겨 간다 → 자잘한 흔들림은 걸러진다)
	FVector	mTrackLookPoint = FVector::ZeroVector;

	// (11/5) 아직 먼저 따라갈 소켓(망치)을 쫓는 중인지. 바닥에 닿으면 false → 골반
	bool	mTrackLeading = false;

	// (11/6) 망치가 머리 위에서 한 번 보였는지(그 뒤부터 따라감) / 따라가기 시작 뒤 흐른 시간(안전장치 1.5초)
	bool	mTrackLeadArmed = false;
	float	mTrackLeadTime = 0.f;

	// (11/6) 지난 프레임의 망치 높이 (낙하 속도 = 높이 차이 ÷ 시간)
	float	mTrackPrevLeadZ = 0.f;
	bool	mTrackPrevLeadValid = false;

	// (11/6) 진짜 망치로 정한 뼈(출발할 때 가장 높던 후보) / 따라가기 시작 뒤 흐른 시간(안전장치 1.5초)
	FName	mTrackLeadBone = NAME_None;
	float	mTrackArmedTime = 0.f;

	// (11/6) 스프링이 기억하는 속도 (바라보는 점 / 카메라 자리). 속도가 이어져서 출발·전환이 부드럽다
	FVector	mTrackLookVelocity = FVector::ZeroVector;
	FVector	mTrackMoveVelocity = FVector::ZeroVector;

	// 카드 : 밀기 시작 자리·바라보던 점 → 클로즈업 자리·상체 점, 호의 중심(골반, 위에서 본 위치), 흐른 시간
	FVector	mCardStartLocation = FVector::ZeroVector;
	FVector	mCardStartLook = FVector::ZeroVector;
	FVector	mCardLocation = FVector::ZeroVector;
	FVector	mCardLookPoint = FVector::ZeroVector;
	FVector	mCardPivot = FVector::ZeroVector;
	float	mCardElapsed = 0.f;

	// (11/3) 레벨이 시작될 때 검은 화면에서 서서히 밝아지는 시간(초). 타이틀·포탈에서 검게 넘어온 화면을 이어 준다.
	float	mLevelFadeInTime = 1.f;

	// 연출이 끝난 뒤 HUD가 서서히 나타나는 시간(초)
	float	mHUDFadeInTime = 1.f;

	// 동작 길이를 못 읽었을 때(몽타주가 없을 때) 쓰는 연출 시간(초)
	float	mIntroFallbackTime = 0.5f;

	// 결과 화면이 뜨기까지 기다리는 시간(초).
	// 패배 : 기디언이 쓰러지는 동안. 승리 : 보스가 쓰러지고 사라지는 동안 (Death_front + 사라지기 2초)
	UPROPERTY(EditAnywhere, Category = "Result")
	float	mDefeatResultDelay = 1.f;

	UPROPERTY(EditAnywhere, Category = "Result")
	float	mVictoryResultDelay = 4.5f;

	// 전투가 이미 끝났는지 (승리·패배가 두 번 처리되지 않게)
	bool	mBattleEnded = false;

	// (11/4) 스테이지 클리어 글자가 사라지고 포탈까지 걸어가는 중인지. 이때는 전투가 끝났어도 일시정지를 열 수 있다
	bool	mWalkingToPortal = false;

	// 이번 결과가 승리인지 (타이머로 결과 화면을 띄울 때 사용)
	bool	mPlayerWon = false;

	FTimerHandle	mResultTimer;

	// ---------------------------------------------------------------- 스테이지 클리어 (10/26)
	// 레벨에 다음 스테이지 포탈이 놓여 있으면 승리 = "스테이지 클리어" :
	//   결과 글자만 잠깐(버튼 없음) → 사라지면 기디언 조작 복귀 + 포탈 열림.
	// 포탈이 없는 레벨(마지막 스테이지)은 지금까지처럼 결과 화면(다시 시도·게임 종료).
	UPROPERTY()
	TObjectPtr<class AStagePortal>	mStagePortal;

	// 스테이지 클리어 글자 : 다 나타난 뒤 유지 시간, 사라지는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Result")
	float	mStageClearHoldTime = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Result")
	float	mStageClearFadeOutTime = 1.f;

	FTimerHandle	mStageClearTimer;

	// (10/26) 전투가 끝나면 배경음(레벨의 앰비언트 사운드)을 이 시간(초) 동안 줄여서 끈다. (소울류 : 끝나는 순간의 정적)
	UPROPERTY(EditAnywhere, Category = "Result")
	float	mAmbienceFadeTime = 2.f;

	// ---------------------------------------------------------------- 히트스톱 (10/21)
	// 맞는 순간 게임 전체를 아주 잠깐 거의 멈춘다. (전역 시간 배율 = mHitStopDilation)
	//
	// 맞은 캐릭터만 멈추지 않는 이유 : 캐릭터 하나만 느리게 하면 몽타주는 멈추지만
	// 어빌리티의 대기 타이머(넘어짐 공중 시간, 지침 시간 등)는 그대로 흘러서 동작과 타이밍이 어긋난다.
	// 전역 시간 배율은 타이머도 같이 느리게 해서 어긋나지 않는다.
	float	mHitStopDilation = 0.05f;

	// 사건별 히트스톱 시간(실제 시간, 초). 0이면 멈추지 않는다.
	float	mHitStopPlayerHeavy = 0.10f;
	float	mHitStopStrongShot = 0.03f;
	float	mHitStopMeteor = 0.05f;
	float	mHitStopBossDeath = 0.25f;

	// 지금 히트스톱이 끝나는 실제 시각(초). 더 짧은 요청이 긴 히트스톱을 끊지 않게 비교한다.
	double	mHitStopEndRealTime = 0.0;

	FTimerHandle	mHitStopTimer;

	// ---------------------------------------------------------------- 카메라 흔들림 (10/21)
	// 흔들림 모양은 에디터에서 만든 블루프린트(부모 CameraShakeBase)가 정하고, 여기서는 배율만 정한다.
	// (11/4) 레벨 시작 때 경로로 불러 둔다 (PreloadFeedbackAssets). 에셋이 없으면 흔들림만 건너뛴다.
	UPROPERTY()
	TSubclassOf<class UCameraShakeBase>	mLightShakeClass;

	UPROPERTY()
	TSubclassOf<class UCameraShakeBase>	mHeavyShakeClass;

	FString	mLightShakePath = TEXT("/Game/UnsealedGod/Camera/CS_Light.CS_Light_C");
	FString	mHeavyShakePath = TEXT("/Game/UnsealedGod/Camera/CS_Heavy.CS_Heavy_C");

	// 사건별 흔들림 배율 (1 = 에셋에서 만든 세기 그대로)
	float	mShakePlayerLight = 0.7f;
	float	mShakePlayerHeavy = 1.f;
	float	mShakeStrongShot = 0.3f;
	float	mShakeMeteor = 0.8f;
	float	mShakeLeapLand = 0.6f;
	float	mShakeUltimate = 1.5f;
	float	mShakeBossDeath = 1.f;

	// 위치가 있는 사건(폭발·착지)의 거리 감쇠 (cm). Inner 안은 그대로, Outer 밖은 흔들리지 않는다.
	float	mMeteorShakeInner = 500.f;
	float	mMeteorShakeOuter = 2500.f;
	float	mLeapShakeInner = 400.f;
	float	mLeapShakeOuter = 2000.f;

	// ---------------------------------------------------------------- 소리 (10/26)
	// 표의 세 번째 칸 : 히트스톱·흔들림과 같은 사건에 소리도 함께 정한다. (사건 하나 = 멈춤 + 흔들림 + 소리)
	// 흔들림 에셋처럼 레벨 시작 때 경로로 불러 둔다(11/4). 아직 임포트 안 한 소리면 조용히 건너뛴다.
	// 경로 형식 : /Game/폴더/에셋이름.에셋이름  (콘텐츠 브라우저에서 우클릭 → 레퍼런스 복사의 따옴표 안쪽)
	// (10/29) AI로 만든 효과음으로 교체 (Sound/SFX)
	FString	mSoundPathPlayerHitLight = TEXT("/Game/UnsealedGod/Sound/SFX/S_SFX_Gideon_HitLight.S_SFX_Gideon_HitLight");
	FString	mSoundPathPlayerHitHeavy = TEXT("/Game/UnsealedGod/Sound/SFX/S_SFX_Gideon_HitHeavy.S_SFX_Gideon_HitHeavy");
	FString	mSoundPathLeapLand = TEXT("/Game/UnsealedGod/Sound/SFX/S_SFX_Leap_Land.S_SFX_Leap_Land");
	FString	mSoundPathUltimateImpact = TEXT("/Game/UnsealedGod/Sound/SFX/S_SFX_Ult_Impact.S_SFX_Ult_Impact");
	FString	mSoundPathCryoseismBlast = TEXT("/Game/UnsealedGod/Sound/SFX/S_SFX_Cryoseism_Blast.S_SFX_Cryoseism_Blast");

	// 불러온 소리 (경로 → 에셋). UPROPERTY()를 붙여야 가비지 컬렉션이 지우지 않는다.
	UPROPERTY()
	TMap<FString, TObjectPtr<class USoundBase>>	mLoadedSounds;

public:
	// 타격감 사건을 알린다. 히트스톱·카메라 흔들림을 표에 따라 처리한다.
	// Location : 폭발·착지처럼 위치가 있는 사건의 중심 (거리 감쇠에 사용)
	// bHit     : Q 운석처럼 "맞혔을 때만" 히트스톱이 있는 사건에서 실제로 맞혔는지
	// OverrideSound (10/29) : 기디언 피격에서 "때린 쪽"이 정한 소리. 있으면 표의 기본 소리 대신 낸다 (멈춤·흔들림은 표 그대로)
	void PlayHitFeedback(EHitFeedback Type, const FVector& Location = FVector::ZeroVector, bool bHit = true,
		class USoundBase* OverrideSound = nullptr);

protected:
	// 히트스톱 시작. Duration은 실제 시간(초).
	void StartHitStop(float Duration);

	// 히트스톱 끝 : 시간 배율을 1로 되돌린다. (타이머로 부른다)
	void EndHitStop();

	// 플레이어 카메라를 흔든다. bHeavy : 강한 흔들림 에셋을 쓸지, Scale : 세기 배율
	void PlayCameraShake(bool bHeavy, float Scale);

	// 위치 기준으로 흔든다. 플레이어가 멀수록 약해진다. (Inner 안 = Scale 그대로, Outer 밖 = 0)
	void PlayCameraShakeAt(bool bHeavy, float Scale, const FVector& Location, float Inner, float Outer);

	// (10/26) 경로의 소리를 낸다. Location이 0이면 화면 전체에 들리는 2D 소리, 아니면 그 자리에서.
	void PlayFeedbackSound(const FString& Path, const FVector& Location = FVector::ZeroVector);

	// (11/4) 경로의 소리를 찾는다. 처음 찾는 경로면 불러와서 mLoadedSounds에 기억한다. (없으면 nullptr)
	class USoundBase* FindFeedbackSound(const FString& Path);

	// (11/4) 레벨 시작 때 흔들림 에셋·타격 소리를 미리 불러 둔다. (첫 타격 순간 디스크를 읽느라 멈칫하지 않게)
	void PreloadFeedbackAssets();

public:
	// 기디언이 죽었을 때 (APlayerCharacter::Death가 부른다)
	void OnPlayerDeath(class APlayerCharacter* Player);

	// 보스가 죽었을 때 (ABossBase::Death가 부른다)
	void OnBossDeath(class ABossBase* Boss);

	// (11/2) 일시정지 : Esc 키(APlayerCharacter::PauseKey)가 부른다. 멈춰 있으면 풀고, 아니면 멈춘다.
	void TogglePause();

	// (11/5) 시작 연출 건너뛰기 : 연출 중 스페이스바(APlayerCharacter::JumpKey)가 부른다. 연출 중이 아니면 무시
	void SkipIntro();

	// 게임을 멈추고 메뉴를 띄운다. 결과 화면이 뜬 뒤(전투 끝)에는 열지 않는다.
	void PauseGame();

	// 메뉴를 닫고 게임을 재개한다. (메뉴의 RESUME 버튼과 Esc도 이 함수를 부른다)
	void ResumeGame();

protected:
	// 게임 시작 : HUD를 만든다. (이때는 플레이어 컨트롤러와 기디언이 이미 만들어져 있다)
	virtual void BeginPlay() override;

	// (11/5) 매 프레임 : 맵 둘러보기 중이면 카메라를 경로 위로 옮긴다 (그 밖에는 아무것도 안 한다)
	virtual void Tick(float DeltaSeconds) override;

	// 등장 연출 시작 (모든 BeginPlay가 끝난 다음 프레임에 부른다 → 보스 AI가 이미 돌고 있어서 멈출 수 있다)
	// (11/5) 처음 들어왔으면 시작 연출(TryStartCinematic), 다시 시도거나 시작 연출을 못 하면 짧은 연출(StartShortIntro)
	void StartIntro();

	// 짧은 연출 : 보스·기디언 등장 동작을 동시에 → 긴 쪽이 끝나면 EndIntro (10/21의 StartIntro 그대로)
	void StartShortIntro();

	// ---------------------------------------------------------------- 시작 연출 단계 (11/5, 타이머로 차례로 부른다)
	// ① 시작 : 보스 숨김·입력 잠금·위젯·시퀀스 재생. 시퀀스나 보스가 없으면 false (→ 짧은 연출)
	bool TryStartCinematic();
	void ShowStageCard();

	// (11/5) 코드 경로 둘러보기 : 경로 계산 + 카메라 만들기 / 매 프레임 카메라 옮기기
	void StartFlyoverCamera(class APlayerController* PlayerController, class APlayerCharacter* Player);
	void UpdateFlyoverCamera(float DeltaTime);

	// (11/5) 보스 뼈 위치(못 찾으면 캡슐 높이 비율로 대신) / 등장 동작 따라가기 / 카드 클로즈업 계산·매 프레임
	FVector GetIntroBoneLocation(FName BoneName, float FallbackRatio) const;
	void UpdateTrackCamera(float DeltaTime);
	FVector GetCameraLookPoint(float Distance) const;	// 지금 화면이 보스로 삼고 있는 점 (왼쪽 1/3 틀기를 되돌린 방향)
	void StartCardCamera();
	void UpdateCardCamera(float DeltaTime);

	// (11/5) 배경 띠 준비(둘러보기 시작 때 미리 불러 둔다) / 켜기(보스가 멈출 때) / 끄기(연출 끝)
	void PrepareIntroBand();
	void StartIntroBand();
	void StopIntroBand();

	// ② 둘러보기 끝 → 보스가 나타나며 등장 동작 / 멈추는 순간 보스 카드 / 카드 끝 → 보스 다시 움직임
	void OnFlyoverFinished();
	void OnBossFreeze();
	void OnBossCardShow();		// (11/5) 카메라가 거의 도착한 뒤 띠·이름·부제
	void OnBossCardEnd();

	// ③ 카메라를 BlendTime초 동안 기디언 뒤로 → 끝나면 FinishCinematic
	void ReturnCameraToPlayer(float BlendTime);

	// 시퀀스 멈춤·위젯 제거·EndIntro (정상으로 끝나도, 건너뛰어도 여기로)
	// (11/6) BossDelay > 0 : 기디언·HUD는 바로, 보스만 BossDelay초 뒤에 풀린다 (건너뛰기 준비 시간)
	void FinishCinematic(float BossDelay = 0.f);

	// (11/6) 건너뛰기 : 화면이 다 어두워진 순간 → 정리하고 밝아지기 시작
	void OnSkipFadedOut();

	// 등장 연출 끝 : 입력·AI 풀기 + HUD 서서히 나타남
	void EndIntro();

	// (11/6) 보스만 풀기 (AI·무적). EndIntro의 보스 부분이자 건너뛰기 준비 시간 끝
	void EndBossIntro();

	// 레벨의 모든 캐릭터에게 OnBattleEnd를 알리고, 결과 화면을 예약한다.
	void EndBattle(bool bPlayerWon, float ResultDelay);

	// 결과 화면을 만든다. (타이머로 부른다)
	void ShowResult();

	// (10/26) 스테이지 클리어 글자가 사라진 뒤 : 기디언 조작 복귀 + 포탈 열기 (타이머로 부른다)
	void OnStageClearEnd();

	// (10/26) 레벨의 배경음(앰비언트 사운드)을 모두 줄여서 끈다.
	void FadeOutAmbience();
};
