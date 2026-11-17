// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PlayerCharacter.h"
#include "Gideon.generated.h"

// ============================================================================
// AGideon
// 주인공 기디언. APlayerCharacter를 상속받는다. (수업 코드의 WukongGAS와 같은 역할)
// 생성자에서 기디언의 메시와 애님 블루프린트를 불러온다.
//
// 앞으로 추가할 것 (날짜별)
//   10/14~15 : 기본 공격 (포털 블라스트, A→B→C 순환)
//   10/16    : E 토른 스페이스 (순간이동 회피)
//   10/16    : Q 포털 운석, F 버든, R 운석 비 (궁극기)  (원래 10/30 Q 코즈믹 리프트·R 블랙홀에서 변경)
// 이 기능들은 부모(APlayerCharacter)에 virtual 함수로 만들고, 여기서 override로 내용을 채운다.
// ============================================================================
UCLASS()
class UNSEALEDGOD_API AGideon : public APlayerCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AGideon();
};
