// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosPartyPlayerStart.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "EngineUtils.h"

AKronosPartyPlayerStart::AKronosPartyPlayerStart(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CapsuleComponent->ShapeColor = FColor(255, 138, 5, 255);
	CapsuleComponent->bDrawOnlyIfSelected = true;
	CapsuleComponent->InitCapsuleSize(40.0f, 92.0f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	CapsuleComponent->bShouldCollideWhenPlacing = true;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(false);
	CapsuleComponent->Mobility = EComponentMobility::Static;
	RootComponent = CapsuleComponent;
	bCollideWhenPlacing = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

#if WITH_EDITORONLY_DATA
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));

	if (!IsRunningCommandlet())
	{
		// Structure to hold one-time initialization.
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> PlayerStartTextureObject;
			FName ID_PartyPlayerStart;
			FText NAME_PartyPlayerStart;
			FName ID_Navigation;
			FText NAME_Navigation;
			FConstructorStatics() :
				PlayerStartTextureObject(TEXT("/Engine/EditorResources/S_Player")),
				ID_PartyPlayerStart(TEXT("PartyPlayerStart")),
				NAME_PartyPlayerStart(NSLOCTEXT("SpriteCategory", "PartyPlayerStart", "Party Player Start")),
				ID_Navigation(TEXT("Navigation")),
				NAME_Navigation(NSLOCTEXT("SpriteCategory", "Navigation", "Navigation"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		if (SpriteComponent)
		{
			SpriteComponent->Sprite = ConstructorStatics.PlayerStartTextureObject.Get();
			SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
			SpriteComponent->bHiddenInGame = true;
			SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_PartyPlayerStart;
			SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_PartyPlayerStart;
			SpriteComponent->SetupAttachment(CapsuleComponent);
			SpriteComponent->SetUsingAbsoluteScale(true);
			SpriteComponent->bIsScreenSizeScaled = true;
		}

		if (ArrowComponent)
		{
			ArrowComponent->ArrowColor = FColor(200, 235, 10);
			ArrowComponent->ArrowSize = 1.0f;
			ArrowComponent->bTreatAsASprite = true;
			ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Navigation;
			ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Navigation;
			ArrowComponent->SetupAttachment(CapsuleComponent);
			ArrowComponent->bIsScreenSizeScaled = true;
		}
	}
#endif // WITH_EDITORONLY_DATA
}

AKronosPartyPlayerStart* AKronosPartyPlayerStart::FindFreePlayerStart(const UObject* WorldContextObject, bool bFindLocal)
{
	for (TActorIterator<AKronosPartyPlayerStart> It(WorldContextObject->GetWorld()); It; ++It)
	{
		if (AKronosPartyPlayerStart* PlayerStart = *It)
		{
			// Match player start type (local or remote).
			if (PlayerStart->bIsLocalPlayerStart == bFindLocal)
			{
				// Check if the player start is not taken by another player.
				if (PlayerStart->IsFree())
				{
					return PlayerStart;
				}
			}
		}
	}

	// No free player start found.
	return nullptr;
}
