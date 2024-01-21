// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "KronosGameInstance.generated.h"

/**
 * KronosGameInstance is basically a *dummy* class. Its only purpose is to override the game instance's online session class, so that the corrent one will be spawned.
 *
 * To minimize dependencies, some functions that are better suited for the GameInstance class (e.g. traveling to session) are implemented in KronosOnlineSession instead.
 * The reason behind this is that other plugins (or your project itself) usually have a custom GameInstance class already, and merging these classes may not be an easy task.
 * Kronos was designed to be a standalone plugin, and this way it doesn't have to depend on the GameInstance class.
 * OnlineSession classes are part of the GameInstance anyways, so it doesn't really make a difference.
 *
 * If you have a custom GameInstance class and you want to make it *compatible* with Kronos, all you have to do is override the GetOnlineSessionClass() function.
 * Just copy & paste the code below into your custom GameInstance class (don't forget the .cpp part of the function as well).
 *
 * @see UKronosOnlineSession
 */
UCLASS()
class KRONOS_API UKronosGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	//~ Begin UGameInstance Interface
	virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;
	//~ End UGameInstance Interface
};
