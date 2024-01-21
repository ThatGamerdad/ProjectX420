// Copyright 2022-2023 Horizon Games. All Rights Reserved.

#include "Widgets/KronosMatchmakingSessionWidget.h"

void UKronosMatchmakingSessionWidget::InitSessionWidget(const FKronosSearchResult& InOwningSession)
{
	OwningSession = InOwningSession;
	K2_OnSessionWidgetInitialized();
}
