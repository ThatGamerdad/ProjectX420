// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

/**
 * Custom gameplay debugger category for Kronos.
 */
class FKronosGameplayDebuggerCategory : public FGameplayDebuggerCategory
{
public:

    /** Default constructor. */
    FKronosGameplayDebuggerCategory();

    /** Draw debug data to the gameplay debugger canvas. */
    virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

    /** Draw a single debug section to the debugger canvas. */
    void DrawSection(FGameplayDebuggerCanvasContext& CanvasContext, const FString& SectionData, float& CategoryWidth, float& CategoryHeight);

    /** Instance the category. */
    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
};

#endif // WITH_GAMEPLAY_DEBUGGER
