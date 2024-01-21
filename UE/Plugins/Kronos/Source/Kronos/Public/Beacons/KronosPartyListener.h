// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHost.h"
#include "KronosPartyListener.generated.h"

/**
 * Beacon that is listening for incoming client connections.
 * This is basically a *dummy* class. Its only purpose is to allow us to override config params.
 */
UCLASS()
class KRONOS_API AKronosPartyListener : public AOnlineBeaconHost
{
	GENERATED_BODY()
};
