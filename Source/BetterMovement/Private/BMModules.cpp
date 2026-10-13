#include "BMModules.h"
#include "BMConfig.h"
#include "BMMomentum.h"
#include "BMWaterSkim.h"
#include "BMMovementSubsystem.h"

UBMGameInstanceModule::UBMGameInstanceModule()
{
	bRootModule = true;
	ModConfigurations.Add(UBMConfig::StaticClass());
}

void UBMGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	// SML registers the configuration during initialization; its editor classes must be in place first.
	if (Phase == ELifecyclePhase::INITIALIZATION)
	{
		UBMConfig::UseSMLEditorClasses();
		FBMMomentum::InstallHook();
		FBMWaterSkim::InstallHook();
	}
	Super::DispatchLifecycleEvent(Phase);
}

UBMGameWorldModule::UBMGameWorldModule()
{
	bRootModule = true;
	ModSubsystems.Add(ABMMovementSubsystem::StaticClass());
}
