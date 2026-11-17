// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnsealedGod : ModuleRules
{
	public UnsealedGod(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			// GAS(게임플레이 어빌리티 시스템)를 쓰기 위한 모듈 3개.
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			// 애님 인스턴스에서 이동 방향을 계산하는 함수(CalculateDirection)가 들어있다.
			"AnimGraphRuntime",
			// AI : AI 컨트롤러, 비헤이비어 트리, 블랙보드
			"AIModule",
			// 내비게이션(길찾기) : AI가 내비 메시를 따라 이동할 때 필요
			"NavigationSystem",
			// UI : 위젯 블루프린트의 C++ 부모 클래스(UUserWidget), 버튼·텍스트 (결과 화면)
			"UMG",
			"Slate",
			"SlateCore",
			// (11/5) 레벨 시퀀스(시퀀서로 만든 컷신) 재생 : 시작 연출의 맵 둘러보기 카메라
			"LevelSequence",
			"MovieScene",
			// 나이아가라 이펙트를 코드에서 생성 (10/21 회복 이펙트)
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
