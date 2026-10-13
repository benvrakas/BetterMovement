#pragma once

#include "CoreMinimal.h"
#include "Module/GameInstanceModule.h"
#include "Module/GameWorldModule.h"
#include "BMModules.generated.h"

// Better Movement's root game instance module; registers its configuration with SML.
UCLASS()
class BETTERMOVEMENT_API UBMGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	UBMGameInstanceModule();

	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};

// Better Movement's root game world module; registers its subsystem with SML.
UCLASS()
class BETTERMOVEMENT_API UBMGameWorldModule : public UGameWorldModule
{
	GENERATED_BODY()

public:
	UBMGameWorldModule();
};
