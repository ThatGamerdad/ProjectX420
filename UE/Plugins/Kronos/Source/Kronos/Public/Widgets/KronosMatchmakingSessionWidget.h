// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KronosTypes.h"
#include "KronosMatchmakingSessionWidget.generated.h"

/**
 * Widget for a session search result.
 */
UCLASS()
class KRONOS_API UKronosMatchmakingSessionWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	/** The owning search result this widget represents. */
	FKronosSearchResult OwningSession;

public:

	/** Initializes the session widget. */
	UFUNCTION(BlueprintCallable, Category = "Default")
	virtual void InitSessionWidget(const FKronosSearchResult& InOwningSession);

	/** Get the owning search result this widget represents. */
	UFUNCTION(BlueprintPure, Category = "Default")
	virtual FKronosSearchResult& GetOwningSession() { return OwningSession; }

protected:

	/**
	 * Event when the session widget is initialized. Called after the native OnInitialized() event.
	 * At this point the owning session is set.
	 */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Events", DisplayName = "On Session Widget Initialized")
	void K2_OnSessionWidgetInitialized();
};
