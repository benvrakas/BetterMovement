#pragma once

#include "CoreMinimal.h"
#include "Configuration/ModConfiguration.h"
#include "BMTypes.h"
#include "BMConfig.generated.h"

// Better Movement settings, editable in-game and in Satisfactory Mod Manager: speed limits for everyone, then one
// section per tier.
UCLASS()
class BETTERMOVEMENT_API UBMConfig : public UModConfiguration
{
	GENERATED_BODY()

public:
	UBMConfig();

	// Setting keys inside each tier's section.
	static const FString UphillAngleKey;
	static const FString SpeedLossKey;

	// Key of the speed limits section, and the setting keys inside it.
	static const FString SpeedLimitsKey;
	static const FString MaxSlideSpeedKey;
	static const FString MaxAirSpeedKey;
	static const FString MaxParachuteSpeedKey;
	static const FString WaterSkimSpeedKey;
	static const FString ParachuteAccelerationKey;

	// Lowest and highest accepted values.
	static constexpr float MinUphillAngle = 0.f;
	static constexpr float MaxUphillAngle = 89.f;
	static constexpr float MinSpeedLoss = -500.f;
	static constexpr float MaxSpeedLoss = 1000.f;
	static constexpr float MinSpeedLimit = 1.f;
	static constexpr float MaxSpeedLimit = 300.f;
	static constexpr float MinWaterSkimSpeed = 0.f;
	static constexpr float MinParachuteAcceleration = 0.f;
	static constexpr float MaxParachuteAcceleration = 2.f;

	// Returns the key of Tier's section.
	static FString GetTierSectionKey(EBMTier Tier);

	// Rebuilds the default configuration with SML's Blueprint property classes, which carry the editor widgets
	// SML's Mods menu shows. Call before SML registers the configuration.
	static void UseSMLEditorClasses();

	// Fills OutSettings with every tier's settings, clamped to the accepted range. Returns false, leaving OutSettings
	// empty, if the configuration isn't available.
	static bool ReadTierSettings(const UObject* WorldContext, TArray<FBMTierSettings>& OutSettings);

	// Fills OutLimits with the speed limits, clamped to the accepted range. Returns false, leaving OutLimits at its
	// defaults, if the configuration isn't available.
	static bool ReadSpeedLimits(const UObject* WorldContext, FBMSpeedLimits& OutLimits);
};
