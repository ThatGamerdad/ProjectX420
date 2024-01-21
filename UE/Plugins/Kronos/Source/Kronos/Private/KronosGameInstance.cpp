// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "KronosGameInstance.h"
#include "KronosOnlineSession.h"
#include "KronosConfig.h"

TSubclassOf<UOnlineSession> UKronosGameInstance::GetOnlineSessionClass()
{
	return GetDefault<UKronosConfig>()->OnlineSessionClass;
}
