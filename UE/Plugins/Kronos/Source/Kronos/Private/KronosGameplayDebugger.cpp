// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosGameplayDebugger.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "KronosMatchmakingManager.h"
#include "KronosMatchmakingPolicy.h"
#include "KronosOnlineSession.h"
#include "CanvasItem.h"

FKronosGameplayDebuggerCategory::FKronosGameplayDebuggerCategory()
{
    bShowOnlyWithDebugActor = false;
}

void FKronosGameplayDebuggerCategory::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
    if (!IsValid(OwnerPC))
    {
        CanvasContext.Print(TEXT("{red}Can't debug without a valid debug PlayerController."));
        return;
    }

    const float CategoryOriginX = CanvasContext.CursorX;
    const float CategoryOriginY = CanvasContext.CursorY;
    float CategoryWidth = 0.0f;
    float CategoryHight = 0.0f;

    UKronosOnlineSession* OnlineSession = UKronosOnlineSession::Get(OwnerPC);
    UKronosMatchmakingManager* MatchmakingManager = UKronosMatchmakingManager::Get(OwnerPC);
    UKronosMatchmakingPolicy* MatchmakingPolicy = MatchmakingManager->GetMatchmakingPolicy();

    FString MatchmakingDebugData = IsValid(MatchmakingPolicy) ? MatchmakingPolicy->GetDebugString() : TEXT("{grey}Matchmaking hasn't been started yet.");
    DrawSection(CanvasContext, MatchmakingDebugData, CategoryWidth, CategoryHight);

    if (!IsValid(OnlineSession))
    {
        DrawSection(CanvasContext, TEXT("{red}OnlineSession is invalid!"), CategoryWidth, CategoryHight);
        return;
    }

    FString GameSessionDebugData = OnlineSession->GetSessionDebugString(NAME_GameSession);
    if (!GameSessionDebugData.IsEmpty())
    {
        DrawSection(CanvasContext, GameSessionDebugData, CategoryWidth, CategoryHight);
    }
    
    FString PartySessionDebugData = OnlineSession->GetSessionDebugString(NAME_PartySession);
    if (!PartySessionDebugData.IsEmpty())
    {
        DrawSection(CanvasContext, PartySessionDebugData, CategoryWidth, CategoryHight);
    }

    if (GameSessionDebugData.IsEmpty() && PartySessionDebugData.IsEmpty())
    {
        DrawSection(CanvasContext, TEXT("{grey}No session data was found."), CategoryWidth, CategoryHight);
    }

    CanvasContext.CursorX = CategoryOriginX;
    CanvasContext.CursorY = CategoryOriginY + CategoryHight;
}

void FKronosGameplayDebuggerCategory::DrawSection(FGameplayDebuggerCanvasContext& CanvasContext, const FString& SectionData, float& CategoryWidth, float& CategoryHeight)
{
    const FVector2D SectionOrigin = FVector2D(CanvasContext.CursorX, CanvasContext.CursorY);
    const FVector2D SectionPadding = FVector2D(10.0f, 5.0f);
    const FVector2D SectionSpacing = FVector2D(3.0f, 0.0f);
    const float TextSpacing = 2.0f;

    float SectionWidth = 0.0f;
    float SectionHeight = 0.0f;

    TArray<FString> ParsedSectionData;
    SectionData.ParseIntoArray(ParsedSectionData, TEXT("\n"));

    for (auto It = ParsedSectionData.CreateIterator(); It; ++It)
    {
        FString& DataEntry = *It;
		DataEntry = DataEntry.Replace(TEXT("\t"), TEXT("    "));

		float SizeX;
		float SizeY;
		CanvasContext.MeasureString(DataEntry, SizeX, SizeY);

		SectionWidth = FMath::Max(SectionWidth, SizeX);
		SectionHeight += SizeY;

        if (It.GetIndex() < ParsedSectionData.Num() - 1)
        {
            SectionHeight += TextSpacing;
        }
    }

    SectionWidth += SectionPadding.X * 2;
    SectionHeight += SectionPadding.Y * 2;

    FCanvasTileItem Background(FVector2D::ZeroVector, FVector2D(SectionWidth, SectionHeight), FLinearColor(0.0f, 0.0f, 0.0f, 0.5f));
    Background.BlendMode = SE_BLEND_Translucent;
    CanvasContext.DrawItem(Background, SectionOrigin.X, SectionOrigin.Y);

    CanvasContext.CursorX += SectionPadding.X;
    CanvasContext.CursorY += SectionPadding.Y;

    for (const FString& DataEntry : ParsedSectionData)
    {
        CanvasContext.Print(DataEntry);
        CanvasContext.CursorX = SectionOrigin.X + SectionPadding.X;
        CanvasContext.CursorY += TextSpacing;
    }

    CanvasContext.CursorX = SectionOrigin.X + SectionWidth + SectionSpacing.X;
    CanvasContext.CursorY = SectionOrigin.Y;

    CategoryWidth += SectionWidth;
    CategoryHeight += SectionHeight;
}

TSharedRef<FGameplayDebuggerCategory> FKronosGameplayDebuggerCategory::MakeInstance()
{
    return MakeShareable(new FKronosGameplayDebuggerCategory());
}

#endif // WITH_GAMEPLAY_DEBUGGER
