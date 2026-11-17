// Fill out your copyright notice in the Description page of Project Settings.


#include "Sevarog.h"
#include "Sound/SoundBase.h"
#include "../GAS/Ability/GameplayAbility_Pattern_HammerCombo.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Leap.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Charge.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Ultimate.h"
#include "../GAS/Ability/GameplayAbility_Pattern_SummonMinions.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Exhausted.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Bind.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Phantom.h"

ASevarog::ASevarog()
{
	// ---------------------------------------------------------------- 수치
	// 체력 3500 (10/19 : 계산상 6000 → 테스트에서 딜이 빡빡해 4000. 11/3 : 오로라를 깨고 넘어오는 2스테이지라 3500)
	// 테스트할 때는 레벨에 놓은 세바로그의 디테일 → Stat → M Init HP를 낮춰서 빠르게 확인할 수 있다.
	mInitHP = 3500.f;

	// 공격력. 패턴 피해 = 공격력 × 패턴별 배율 (예 : 3연타 1·2타 1.5배 = 15, 3타 2.5배 = 25)
	mInitAttack = 10.f;

	// 기본 크기 1.2배. 흡수 4단계에서 1.5배까지 커질 예정. (10/12 결정)
	mBaseScale = 1.2f;

	// 메시를 바닥에서 띄우는 높이. 처음에는 사신 느낌으로 20cm(실제 24cm)였지만,
	// 3연타 2타(비스듬히 내려치기)가 기디언 머리 위로 지나가서 0으로 확정. (10/14)
	// 0이어도 망토 끝이 바닥에 끌리지 않고, 3타가 바닥에 파묻혀 보이지 않는 것을 확인했다.
	// 레벨에 놓은 세바로그의 디테일 → Boss → M Hover Height로 눈으로 보며 조정할 수 있다.
	mHoverHeight = 0.f;

	// ---------------------------------------------------------------- 메시
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog.Sevarog'"));

	if (MeshAsset.Succeeded())
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);

	// 캡슐 크기 (크기 배율 1.0 기준. 실제로는 × 1.2)
	// 세바로그는 기디언보다 약 1.6배 크고 몸통이 넓다. 테스트해서 몸에 맞게 조정한다.
	GetCapsuleComponent()->SetCapsuleHalfHeight(150.f);
	GetCapsuleComponent()->SetCapsuleRadius(70.f);

	// 파라곤 메시 방향(Y축 정면)을 X축 정면으로 돌린다. (기디언과 같은 이유)
	// 메시 높이(Z)는 ABossBase::OnConstruction이 "캡슐 바닥 + 띄우는 높이"로 정한다.
	GetMesh()->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));

	// ---------------------------------------------------------------- 애님 블루프린트
	// 클래스 정보를 찾아올 때 경로의 가장 끝에 _C를 무조건 붙여야 한다.
	static ConstructorHelpers::FClassFinder<UAnimInstance>
		BossAnim(TEXT("/Script/Engine.AnimBlueprint'/Game/UnsealedGod/Boss/ABP_Sevarog.ABP_Sevarog_C'"));

	if (BossAnim.Succeeded())
		GetMesh()->SetAnimInstanceClass(BossAnim.Class);

	// ---------------------------------------------------------------- 비헤이비어 트리
	// 에디터에서 Content/UnsealedGod/Boss/BT_Sevarog로 만든다. (블랙보드는 BB_Boss)
	static ConstructorHelpers::FObjectFinder<UBehaviorTree>
		BTAsset(TEXT("/Script/AIModule.BehaviorTree'/Game/UnsealedGod/Boss/BT_Sevarog.BT_Sevarog'"));

	if (BTAsset.Succeeded())
		mBehaviorTree = BTAsset.Object;

	// ---------------------------------------------------------------- 패턴 목록
	// 패턴을 추가할 때는 여기에 한 줄씩 더한다. (도약 10/22, 돌진 10/23, 궁극기 10/26 …)
	mPatternClasses.Add(UGameplayAbility_Pattern_HammerCombo::StaticClass());
	mPatternClasses.Add(UGameplayAbility_Pattern_Leap::StaticClass());
	mPatternClasses.Add(UGameplayAbility_Pattern_Charge::StaticClass());
	mPatternClasses.Add(UGameplayAbility_Pattern_Ultimate::StaticClass());
	mPatternClasses.Add(UGameplayAbility_Pattern_SummonMinions::StaticClass());
	mPatternClasses.Add(UGameplayAbility_Pattern_Exhausted::StaticClass());	// 10/19 : 3페이즈부터 과부하 → 지침
	mPatternClasses.Add(UGameplayAbility_Pattern_Bind::StaticClass());		// 10/21 : 2페이즈부터 속박 (파훼법은 E)
	mPatternClasses.Add(UGameplayAbility_Pattern_Phantom::StaticClass());	// 10/21 : 3페이즈부터 망령 순간이동

	// ---------------------------------------------------------------- 페이즈 전환 대사 (10/26)
	// 팩 원래 용도 = "영혼을 모아 단계가 오를 때" 대사 → 페이즈마다 커지는 우리 연출과 같은 뜻. 칸 0·1·2 = 2·3·4페이즈
	// 반복문 안의 static은 한 번만 만들어지므로 쓸 수 없어서, 세 줄을 따로 쓴다.
	static ConstructorHelpers::FObjectFinder<USoundBase>
		PhaseVoice2(TEXT("/Script/Engine.SoundCue'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Ability_Passive_Level_02.Sevarog_Ability_Passive_Level_02'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		PhaseVoice3(TEXT("/Script/Engine.SoundCue'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Ability_Passive_Level_03.Sevarog_Ability_Passive_Level_03'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		PhaseVoice4(TEXT("/Script/Engine.SoundCue'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Sounds/SoundCues/Sevarog_Ability_Passive_Level_04.Sevarog_Ability_Passive_Level_04'"));

	mPhaseVoices.Add(PhaseVoice2.Succeeded() ? PhaseVoice2.Object : nullptr);
	mPhaseVoices.Add(PhaseVoice3.Succeeded() ? PhaseVoice3.Object : nullptr);
	mPhaseVoices.Add(PhaseVoice4.Succeeded() ? PhaseVoice4.Object : nullptr);

	// ---------------------------------------------------------------- 효과음 (10/29, AI 효과음. 전에는 레벨 디테일에서 골랐음)
	// 필살기 기 모으기 : 사신이 영혼을 모으는 위험한 소리. 충격 "쾅"은 게임 모드 표의 UltimateImpact
	static ConstructorHelpers::FObjectFinder<USoundBase>
		UltChargeSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ult_Charge.S_SFX_Ult_Charge'"));
	// 페이즈 전환 브람 (대사와 함께, 화면 전체에)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		StingerSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Phase_Stinger.S_SFX_Phase_Stinger'"));

	// 망치에 맞았을 때 기디언 쪽 피격 소리 : 무거운 돌덩이로 퍽 치는 소리를 5반음 낮춤 (오로라 검과 같은 소리면 망치가 가벼워 보였음)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		HitLightSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Sevarog_HitLight.S_SFX_Sevarog_HitLight'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		HitHeavySnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Sevarog_HitHeavy.S_SFX_Sevarog_HitHeavy'"));

	if (UltChargeSnd.Succeeded())	mUltChargeSound = UltChargeSnd.Object;
	if (StingerSnd.Succeeded())		mPhaseStinger = StingerSnd.Object;
	if (HitLightSnd.Succeeded())	mHitSoundOnTargetLight = HitLightSnd.Object;
	if (HitHeavySnd.Succeeded())	mHitSoundOnTargetHeavy = HitHeavySnd.Object;

	// ---------------------------------------------------------------- 이름·등장 (10/26 : HUD·부모에서 옮겨 옴)
	// TEXT("...") 앞의 FText::FromString : 화면에 보일 글자(FText)로 바꾼다.
	mBossName = FText::FromString(TEXT("돌아온 신, 세바로그"));

	// (11/5) 시작 연출 카드 : 맵 이름(왼쪽 아래) · 보스 소개(영어 이름 + 한글 칭호) · 카드를 띄울 순간 (LevelStart 129프레임)
	mStageName = FText::FromString(TEXT("무너진 아슈르 신전"));
	mIntroName = FText::FromString(TEXT("SEVAROG"));
	mIntroTitle = FText::FromString(TEXT("돌아온 신"));
	mIntroFreezeTime = 4.31f;
	mIntroTintColor = FLinearColor(0.2f, 1.f, 0.3f, 0.6f);	// 초록 (영혼불)

	// (11/5) 둘러보기 : 석상 방식 (Focus Actor는 레벨에 놓은 세바로그에서 MonoStatue로 고른다). 석상 구간이 있어 7 → 8초
	mIntroFlyover.Duration = 8.f;

	// (11/5) 두건·어깨 가시가 머리 뼈보다 높이 솟아 있어서 클로즈업을 조금 멀리 (0.85 → 0.65)
	mIntroFlyover.CardFill = 0.65f;

	// (11/5) 등장 동작 : 하늘을 올려다보며 도착 → 떨어지는 망치(HammerCenter 소켓)를 빠르게 따라 바닥으로 → 골반
	// (첫 자세인 웅크린 모습이 망치보다 먼저 보이지 않게)
	mIntroFlyover.FinalLookUpDegrees = 35.f;
	// (11/6) 망치 관련 소켓·뼈 후보 (스켈레톤에서 확인한 이름). 매 프레임 가장 높은 것 = 하늘에서 떨어지는 망치
	mIntroFlyover.LeadSockets = { TEXT("HammerCenter"), TEXT("weapon_r"), TEXT("root_weapon_r") };

	// (11/5) 시작 연출 그림 (AI 이미지 → 배경 투명 처리, UI/Intro). 레벨에 놓은 보스의 디테일에서 바꿀 수 있다
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		StageIconTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_Icon_Arena.T_UI_Icon_Arena'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		NameTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_BossName_Sevarog.T_UI_BossName_Sevarog'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		TitleTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_BossTitle_Sevarog.T_UI_BossTitle_Sevarog'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		BandTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_BossBand_Sevarog.T_UI_BossBand_Sevarog'"));

	if (StageIconTex.Succeeded())	mStageIcon = StageIconTex.Object;
	if (NameTex.Succeeded())		mIntroNameImage = NameTex.Object;
	if (TitleTex.Succeeded())		mIntroTitleImage = TitleTex.Object;
	if (BandTex.Succeeded())		mIntroBandTexture = BandTex.Object;

	// 체력바 틀 (초록 뿔). WBP_HUD에 처음 넣어 둔 그림과 같다.
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		BarFrame(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Frames/T_UI_BossBarFrame.T_UI_BossBarFrame'"));

	if (BarFrame.Succeeded())
		mHPBarFrame = BarFrame.Object;

	// 망치가 떨어지고 땅에서 솟아오르는 동작 (원작 LevelStart_Montage를 복제해 슬롯만 DefaultSlot으로)
	mIntroMontagePath = TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/AM_Sevarog_LevelStart.AM_Sevarog_LevelStart'");

	// (11/6) 등장 이펙트 : 원작 그림자 소용돌이 + 영혼불 불씨 (땅에서 솟아오를 때)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		IntroSpawnFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonSevarog/FX/Particles/P_Sevarog_LevelStart.P_Sevarog_LevelStart'"));

	if (IntroSpawnFX.Succeeded())
		mIntroSpawnParticle = IntroSpawnFX.Object;

	// (11/10) 하늘에서 떨어진 망치가 땅에 꽂히는 순간 "쿵" (AI 생성, 묵직한 충격 + 우주 같은 긴 잔향)
	// AM_Sevarog_LevelStart 12프레임(30fps) = 0.41초
	static ConstructorHelpers::FObjectFinder<USoundBase>
		IntroSlamSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Sevarog_IntroSlam.S_SFX_Sevarog_IntroSlam'"));

	if (IntroSlamSnd.Succeeded())
		mIntroImpactSound = IntroSlamSnd.Object;

	mIntroImpactTime = 0.41f;

	// ---------------------------------------------------------------- 사망 연출 (10/26 : 부모 ABossBase에서 옮겨 옴)
	// 쓰러지는 동작은 ABP가 재생한다. 여기서는 길이만 읽어 "언제 사라지기 시작할지"를 정한다.
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		DeathAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Death_front.Death_front'"));

	if (DeathAnim.Succeeded())
		mDeathAnim = DeathAnim.Object;

	// 사라지기 시작할 때 영혼 불씨
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		DeathFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonSevarog/FX/Particles/Abilities/SoulStackPassive/FX/P_SoulStageEmbersBurst.P_SoulStageEmbersBurst'"));

	if (DeathFX.Succeeded())
		mDeathFadeParticle = DeathFX.Object;

	// 순간이동(속박·망령) 사라짐·나타남 잔상 (10/21)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		VanishFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonSevarog/FX/Particles/Abilities/SoulStackPassive/FX/P_ShadowTrails.P_ShadowTrails'"));

	if (VanishFX.Succeeded())
		mVanishParticle = VanishFX.Object;

	// ---------------------------------------------------------------- 패배 연출 (10/19) : 영혼 뽑기 → 박수 3번
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		SiphonAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Soul_Siphon.Soul_Siphon'"));

	if (SiphonAnim.Succeeded())
		mSoulSiphonAnim = SiphonAnim.Object;

	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		ClapAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonSevarog/Characters/Heroes/Sevarog/Animations/Emote_Clap_T1.Emote_Clap_T1'"));

	if (ClapAnim.Succeeded())
		mVictoryEmoteAnim = ClapAnim.Object;

	mVictoryEmoteLoopCount = 3;

	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		SiphonFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonSevarog/FX/Particles/Abilities/SoulSiphon/FX/P_SoulSwirlsBody.P_SoulSwirlsBody'"));

	if (SiphonFX.Succeeded())
		mSoulSiphonParticle = SiphonFX.Object;
}
