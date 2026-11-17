// Fill out your copyright notice in the Description page of Project Settings.


#include "Aurora.h"
#include "Sound/SoundBase.h"
#include "../GAS/Ability/GameplayAbility_Pattern_SwordCombo.h"
#include "../GAS/Ability/GameplayAbility_Pattern_IceCharge.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Hoarfrost.h"
#include "../GAS/Ability/GameplayAbility_Pattern_Cryoseism.h"
#include "../GAS/Ability/GameplayAbility_Pattern_IceSculpture.h"
#include "../GAS/Ability/GameplayAbility_Pattern_IceDecoy.h"
#include "../GAS/Ability/GameplayAbility_Pattern_IceGrab.h"
#include "../GAS/Ability/GameplayAbility_Pattern_AuroraExhausted.h"

AAurora::AAurora()
{
	// ---------------------------------------------------------------- 수치 (10/26 직접 결정)
	// 체력 2000 (10/28 내 판단 : 3000 → 2000). 세바로그(4000)보다 적다.
	// 물약이 2개뿐(시작 1 + 2페이즈 1)이라 세바로그(4개)와 같은 비율로 맞춤. 패턴이 적고 싸움이 짧은 첫 보스
	mInitHP = 2000.f;

	// 공격력 12 (10/28 밸런스 : 10은 너무 쉬웠음, 체감 난이도 2 → 목표 7~8). 패턴 피해 = 공격력 × 패턴별 배율
	mInitAttack = 12.f;

	// 기본 크기 1.3배 : 사람 크기 캐릭터라 보스 덩치를 낸다. 페이즈 성장은 없다(아래 mPhaseScaleMultipliers).
	mBaseScale = 1.3f;

	mHoverHeight = 0.f;

	mBossName = FText::FromString(TEXT("봉인의 수호자, 오로라"));

	// (11/5) 시작 연출 카드 : 맵 이름(왼쪽 아래) · 보스 소개(영어 이름 + 한글 칭호) · 카드를 띄울 순간 (LevelStart 123프레임)
	mStageName = FText::FromString(TEXT("봉인된 얼음 궁전"));
	mIntroName = FText::FromString(TEXT("AURORA"));
	mIntroTitle = FText::FromString(TEXT("봉인의 수호자"));
	mIntroFreezeTime = 4.13f;
	mIntroSkipDelay = 1.f;		// (11/6) 건너뛰기 준비 시간 : 1.5초는 길게 느껴짐 (세바로그는 기본 1.5)
	mIntroTintColor = FLinearColor(0.25f, 0.85f, 1.f, 0.6f);	// 청록 (얼음)

	// (11/10) 검에 맞았을 때 기디언 쪽 피격 소리 : 날카롭게 베는 칼 소리 (AI 생성 → 앞 "슥"을 잘라 맞는 순간에 바로 터지게)
	// 비어 있으면 공용 기본 피격음이 나는데, 둔탁해서 "빈 플라스틱 봉"처럼 들렸다. 작은 피격 = 1·2타, 큰 피격 = 3타(넘어짐)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		HitLightSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Aurora_HitLight.S_SFX_Aurora_HitLight'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		HitHeavySnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Aurora_HitHeavy.S_SFX_Aurora_HitHeavy'"));

	if (HitLightSnd.Succeeded())	mHitSoundOnTargetLight = HitLightSnd.Object;
	if (HitHeavySnd.Succeeded())	mHitSoundOnTargetHeavy = HitHeavySnd.Object;

	// (11/5) 시작 연출 그림 (AI 이미지 → 배경 투명 처리, UI/Intro). 레벨에 놓은 보스의 디테일에서 바꿀 수 있다
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		StageIconTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_Icon_IcePalace.T_UI_Icon_IcePalace'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		NameTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_BossName_Aurora.T_UI_BossName_Aurora'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		TitleTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_BossTitle_Aurora.T_UI_BossTitle_Aurora'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		BandTex(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Intro/T_UI_BossBand_Aurora.T_UI_BossBand_Aurora'"));

	if (StageIconTex.Succeeded())	mStageIcon = StageIconTex.Object;
	if (NameTex.Succeeded())		mIntroNameImage = NameTex.Object;
	if (TitleTex.Succeeded())		mIntroTitleImage = TitleTex.Object;
	if (BandTex.Succeeded())		mIntroBandTexture = BandTex.Object;

	// 체력바 틀 (얼음 결정). 세바로그 틀과 크기·레일 위치를 같게 만들어서 그림만 바꿔 끼우면 된다.
	static ConstructorHelpers::FObjectFinder<UTexture2D>
		BarFrame(TEXT("/Script/Engine.Texture2D'/Game/UnsealedGod/UI/Frames/T_UI_Boss2BarFrame.T_UI_Boss2BarFrame'"));

	if (BarFrame.Succeeded())
		mHPBarFrame = BarFrame.Object;

	// ---------------------------------------------------------------- 메시
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		MeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/ParagonAurora/Characters/Heroes/Aurora/Meshes/Aurora.Aurora'"));

	if (MeshAsset.Succeeded())
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);

	// 캡슐 : 기디언과 같은 사람 크기 (크기 배율 1.0 기준. 실제로는 × 1.3 = 반 높이 125, 반지름 44)
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.f);
	GetCapsuleComponent()->SetCapsuleRadius(34.f);

	// 파라곤 메시 방향(Y축 정면)을 X축 정면으로 돌린다. (기디언·세바로그와 같은 이유)
	GetMesh()->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));

	// ---------------------------------------------------------------- 2페이즈 얼음 여제 메시 (10/28)
	// 원래 메시에 붙인다 → 위치·회전·크기를 그대로 따라간다. 처음엔 숨김.
	mEmpressMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EmpressMesh"));
	mEmpressMesh->SetupAttachment(GetMesh());
	// (11/3) 원래 메시처럼 캐릭터 전용 빛(라이팅 채널 1)도 받는다. (AGameCharacter 생성자 참고)
	mEmpressMesh->LightingChannels.bChannel1 = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh>
		EmpressAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/ParagonAurora/Characters/Heroes/Aurora/Skins/GlacialEmpress/Meshes/Aurora_GlacialEmpress.Aurora_GlacialEmpress'"));

	if (EmpressAsset.Succeeded())
		mEmpressMesh->SetSkeletalMeshAsset(EmpressAsset.Object);

	// 보이기만 하는 메시 : 충돌 없음, 예고 데칼 안 받음 (원래 메시와 같은 규칙)
	mEmpressMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mEmpressMesh->SetReceivesDecals(false);
	mEmpressMesh->SetVisibility(false);

	// ---------------------------------------------------------------- 애님 블루프린트
	// 에디터에서 Content/UnsealedGod/Boss/Aurora/ABP_Aurora로 만든다. (오로라 에셋은 Boss/Aurora 하위 폴더, 보스 공용 BT·블랙보드·데칼은 Boss) (부모 클래스 BossAnimInstance, 스켈레톤 Aurora_Skeleton)
	static ConstructorHelpers::FClassFinder<UAnimInstance>
		BossAnim(TEXT("/Script/Engine.AnimBlueprint'/Game/UnsealedGod/Boss/Aurora/ABP_Aurora.ABP_Aurora_C'"));

	if (BossAnim.Succeeded())
		GetMesh()->SetAnimInstanceClass(BossAnim.Class);

	// ---------------------------------------------------------------- 비헤이비어 트리
	// 세바로그와 같은 BT를 쓴다. BT는 "패턴 실행 / 쫓아가기" 두 칸뿐이라 보스가 바뀌어도 그대로다.
	// 어떤 패턴을 쓰는지는 BT가 아니라 아래 패턴 목록(mPatternClasses)이 정한다.
	static ConstructorHelpers::FObjectFinder<UBehaviorTree>
		BTAsset(TEXT("/Script/AIModule.BehaviorTree'/Game/UnsealedGod/Boss/BT_Sevarog.BT_Sevarog'"));

	if (BTAsset.Succeeded())
		mBehaviorTree = BTAsset.Object;

	// ---------------------------------------------------------------- 패턴 목록
	// 패턴 6개 : 검 연타 · 얼음 돌진 · 서리 고리 · 얼음 지진(50% 전환) · 얼음 분신 · 얼음 포박(잡기)
	mPatternClasses.Add(UGameplayAbility_Pattern_SwordCombo::StaticClass());	// 10/28 : 검 3연타
	mPatternClasses.Add(UGameplayAbility_Pattern_IceCharge::StaticClass());	// 10/28 : 얼음 돌진 (정면 방패)
	mPatternClasses.Add(UGameplayAbility_Pattern_Hoarfrost::StaticClass());	// 10/28 : 서리 고리 (얼어붙음, 1페이즈부터)
	mPatternClasses.Add(UGameplayAbility_Pattern_Cryoseism::StaticClass());	// 10/28 : HP 50% 얼음 지진 (페이즈 전환 + 얼음 여제 변신)
	// (10/28 뺌) 얼음 조각 후속타 : 다른 패턴에 얼면 멀리서 미끄러져 와 찌르는 게 어색하고, 오히려 얼음의 위험을 줄였다(내 판단).
	// → 찌르기는 얼음 포박 전용. 얼어 있는 동안은 평소 패턴이 그대로 들어온다(얼음은 무적이 아님). 클래스는 얼음 포박의 부모로 남긴다.
	mPatternClasses.Add(UGameplayAbility_Pattern_IceDecoy::StaticClass());	// 10/28 : 얼음 분신 (뒤로 공중제비 + 걸어오는 얼음 폭탄)
	mPatternClasses.Add(UGameplayAbility_Pattern_IceGrab::StaticClass());	// 10/28 : 얼음 포박 (세바로그 속박과 같은 잡기 문법)
	mPatternClasses.Add(UGameplayAbility_Pattern_AuroraExhausted::StaticClass());	// 11/4 : 얼음 지진을 피하면 지침 (지친 동작 + 헐떡임 4초)

	// ---------------------------------------------------------------- 페이즈 (2페이즈)
	// 필살기(얼음 지진)는 HP 50%에서 한 번 → 끝나면 2페이즈. (세바로그는 75·50·25% 세 번 → 4페이즈)
	mUltimateHPRatios = { 0.5f };

	// 페이즈마다 커지기 : 없음 (성장은 세바로그만의 개성으로 남긴다)
	mPhaseScaleMultipliers = { 1.f, 1.f };

	// 공격력 : 2페이즈 1.3배 (10/28 밸런스 : 1.2 → 1.3)
	mPhaseAttackMultipliers = { 1.f, 1.3f };

	// 걷는 속도 : 기디언 앞 600에 거의 붙는 580 / 2페이즈 600 (10/28 밸런스 : 500은 쉽게 따돌려짐)
	mPhaseWalkSpeeds = { 580.f, 600.f };

	// 미니언 소환 없음
	mPhaseSummonCounts = { 0, 0 };

	// 과부하(지침) 없음 : 페이즈가 2까지라 99는 절대 닿지 않는다. (오로라 팩에는 Stun_End도 없다)
	mOverloadPhase = 99;

	// 빠른 페이즈 = 2페이즈 (10/28 밸런스) : 얼음 여제가 되면 패턴마다 정한 "빠른 값"(재생 속도·후딜)을 쓴다. 세바로그 3페이즈 2배속과 같은 기능
	mFastPhase = 2;

	// ---------------------------------------------------------------- 등장
	// 원작 LevelStart_Montage를 복제해 슬롯만 DefaultSlot으로 바꿔 만든다. (세바로그와 같은 방법)
	mIntroMontagePath = TEXT("/Script/Engine.AnimMontage'/Game/UnsealedGod/Boss/Aurora/AM_Aurora_LevelStart.AM_Aurora_LevelStart'");

	// (11/6) 등장 이펙트 : 원작 얼음 계단 (허공에서 걸어 내려오는 동작이 계단을 밟고 내려오는 것처럼)
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		IntroSpawnFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Aurora/P_Aurora_LevelStart_Stairs.P_Aurora_LevelStart_Stairs'"));

	if (IntroSpawnFX.Succeeded())
		mIntroSpawnParticle = IntroSpawnFX.Object;

	// ---------------------------------------------------------------- 사망
	// 쓰러지는 동작(ABP가 재생)의 길이 → 다 쓰러지면 얼어붙기 시작
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		DeathAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Death.Death'"));

	if (DeathAnim.Succeeded())
		mDeathAnim = DeathAnim.Object;

	// 얼어붙기 : 원작 궁극기에 맞은 적이 얼음이 되는 이펙트
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		FreezeFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Ultimate/FX/P_Aurora_Ultimate_Frozen.P_Aurora_Ultimate_Frozen'"));

	if (FreezeFX.Succeeded())
		mFreezeParticle = FreezeFX.Object;

	// 부서지기 : 원작 얼음 분신이 터질 때의 파편
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		ShatterFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Abilities/Leap/FX/P_Aurora_Decoy_Explode_Fragments.P_Aurora_Decoy_Explode_Fragments'"));

	if (ShatterFX.Succeeded())
		mShatterParticle = ShatterFX.Object;

	// ---------------------------------------------------------------- 패배 연출 : 영혼 뽑기 없이 바로 비웃음 1번
	// mSoulSiphonAnim을 비워 두면 부모(OnBattleEnd)가 바로 승리 동작으로 간다.
	static ConstructorHelpers::FObjectFinder<UAnimSequence>
		LaughAnim(TEXT("/Script/Engine.AnimSequence'/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Emote_Laugh_T1.Emote_Laugh_T1'"));

	if (LaughAnim.Succeeded())
		mVictoryEmoteAnim = LaughAnim.Object;

	mVictoryEmoteLoopCount = 1;

	// ---------------------------------------------------------------- 얼음 분신 머티리얼 (10/28)
	static ConstructorHelpers::FObjectFinder<UMaterialInterface>
		DecoyMat(TEXT("/Script/Engine.MaterialInstanceConstant'/Game/ParagonAurora/FX/Materials/Aurora/M_Aurora_Chunk_FresFade_Inst.M_Aurora_Chunk_FresFade_Inst'"));

	if (DecoyMat.Succeeded())
		mDecoyMaterial = DecoyMat.Object;

	// ---------------------------------------------------------------- 정면 방패 (얼음 돌진) : 막을 때 얼음 조각이 튄다
	static ConstructorHelpers::FObjectFinder<UParticleSystem>
		BlockFX(TEXT("/Script/Engine.ParticleSystem'/Game/ParagonAurora/FX/Particles/Aurora/P_Aurora_Sculpture_Chip.P_Aurora_Sculpture_Chip'"));

	if (BlockFX.Succeeded())
		mFrontGuardBlockParticle = BlockFX.Object;

	// ---------------------------------------------------------------- 소리 (10/29, AI 효과음)
	// 페이즈 전환 공용 소리(브람)는 쓰지 않는다 : 얼음 지진 폭발 + 변신 소리가 이미 같은 순간에 난다
	mPhaseStinger = nullptr;

	static ConstructorHelpers::FObjectFinder<USoundBase>
		TransformSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Aurora_Transform.S_SFX_Aurora_Transform'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		DeathFreezeSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Ice_Freeze.S_SFX_Ice_Freeze'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		DeathShatterSnd(TEXT("/Script/Engine.SoundWave'/Game/UnsealedGod/Sound/SFX/S_SFX_Decoy_Explode.S_SFX_Decoy_Explode'"));

	// ---------------------------------------------------------------- 보이스 (10/29, 오로라 팩. 몽타주 기합은 노티파이로)
	// 2페이즈 대사 : 칸 0 = 2페이즈 (세바로그 페이즈 대사와 같은 칸, 부모 EnterPhase가 튼다)
	static ConstructorHelpers::FObjectFinder<USoundBase>
		PhaseVoice2(TEXT("/Script/Engine.SoundCue'/Game/ParagonAurora/Audio/Cues/Aurora_Status_Buffed.Aurora_Status_Buffed'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		DeathVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonAurora/Audio/Cues/Aurora_Effort_Death.Aurora_Effort_Death'"));
	static ConstructorHelpers::FObjectFinder<USoundBase>
		LaughVoice(TEXT("/Script/Engine.SoundCue'/Game/ParagonAurora/Audio/Cues/Aurora_Effort_Laugh.Aurora_Effort_Laugh'"));

	mPhaseVoices.Add(PhaseVoice2.Succeeded() ? PhaseVoice2.Object : nullptr);

	if (DeathVoice.Succeeded())		mDeathVoice = DeathVoice.Object;
	if (LaughVoice.Succeeded())		mVictoryVoice = LaughVoice.Object;	// 기디언이 졌을 때 비웃음 동작과 함께

	if (TransformSnd.Succeeded())		mTransformSound = TransformSnd.Object;
	if (DeathFreezeSnd.Succeeded())		mDeathFreezeSound = DeathFreezeSnd.Object;
	if (DeathShatterSnd.Succeeded())	mShatterSound = DeathShatterSnd.Object;	// 분신 폭발과 같은 "얼음 조각상이 터지는" 소리
}

void AAurora::BeginPlay()
{
	Super::BeginPlay();

	// SetLeaderPoseComponent : 이 메시는 스스로 애니메이션을 돌리지 않고, 리더(원래 메시)의 뼈 자세를 그대로 쓴다.
	// 같은 스켈레톤(Aurora_Skeleton)이라 가능하다. 애님 블루프린트도 하나만 돈다.
	if (IsValid(mEmpressMesh))
		mEmpressMesh->SetLeaderPoseComponent(GetMesh());
}

void AAurora::RevealEmpress()
{
	if (mEmpressRevealed || !IsValid(mEmpressMesh))
		return;

	mEmpressRevealed = true;

	// 원래 메시를 숨겨도 애니메이션·뼈 계산은 계속하게 한다.
	// (기본값은 "안 보이면 뼈 갱신을 건너뜀"이라, 그대로 숨기면 칼 소켓(판정)과 여제 메시의 자세가 멈춘다)
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	GetMesh()->SetVisibility(false);
	mEmpressMesh->SetVisibility(true);

	// 몸에 붙여 튼다 (얼음 지진 폭발 직후 공중에서 내려오는 동안 따라온다)
	if (IsValid(mTransformSound))
		UGameplayStatics::SpawnSoundAttached(mTransformSound, GetMesh());

	UE_LOG(UGDebug, Warning, TEXT("Aurora : Glacial Empress"));
}

void AAurora::OnPhaseEntered(int32 NewPhase)
{
	Super::OnPhaseEntered(NewPhase);

	if (NewPhase >= 2)
		RevealEmpress();
}

void AAurora::StartDeathFade()
{
	// Super::StartDeathFade()를 부르지 않는다 → 세바로그식 "검게 깎여 사라지기"는 하지 않는다.

	// 숨어 있던 중에 죽었을 경우를 대비해 보이게 한다. (부모와 같은 안전장치)
	SetActorHiddenInGame(false);

	// 쓰러진 자세 그대로 멈춘다. bPauseAnims : 이 메시의 애니메이션 진행을 멈춘다. (얼어서 굳은 느낌)
	GetMesh()->bPauseAnims = true;

	// 몸에 얼음을 붙인다. 메시에 붙이므로 보스 크기(1.3배)를 그대로 따라간다.
	if (IsValid(mFreezeParticle))
		UGameplayStatics::SpawnEmitterAttached(mFreezeParticle, GetMesh());

	if (IsValid(mDeathFreezeSound))
		UGameplayStatics::PlaySoundAtLocation(this, mDeathFreezeSound, GetActorLocation());

	GetWorldTimerManager().SetTimer(mShatterTimer, this, &AAurora::Shatter,
		FMath::Max(mShatterDelay, 0.01f), false);
}

void AAurora::Shatter()
{
	// 얼음 파편. 보스 크기만큼 키운다.
	if (IsValid(mShatterParticle))
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), mShatterParticle,
			GetActorLocation(), GetActorRotation(), GetActorScale3D());
	}

	if (IsValid(mShatterSound))
		UGameplayStatics::PlaySoundAtLocation(this, mShatterSound, GetActorLocation());

	// 몸과 몸에 붙은 얼음 이펙트를 함께 숨긴다. (액터를 숨기면 붙어 있는 컴포넌트도 모두 숨겨진다)
	SetActorHiddenInGame(true);
}
