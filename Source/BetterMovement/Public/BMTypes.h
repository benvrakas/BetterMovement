#pragma once

#include "CoreMinimal.h"
#include "BMTypes.generated.h"

// What a player wears on their legs, for choosing which slide settings apply.
UENUM()
enum class EBMTier : uint8
{
	// No Blade Runners.
	None,
	// Vanilla Blade Runners, and Blade Runners from mods other than Mk+ Blade Runners.
	Mk1,
	// Mk+ Blade Runners tiers (any of their fall damage variants).
	Mk2,
	Mk3,
	Mk4,
	Mk5,
	Count UMETA(Hidden)
};

constexpr int32 BMTierCount = static_cast<int32>(EBMTier::Count);

// Slide settings for one tier, as the host has them.
USTRUCT()
struct FBMTierSettings
{
	GENERATED_BODY()

	// Steepest uphill slope a slide keeps going on, in degrees above horizontal.
	UPROPERTY()
	float UphillAngleDegrees = 0.f;

	// How fast a slide loses speed, in percent of vanilla; below 0, how fast it speeds up.
	UPROPERTY()
	float SpeedLossPercent = 100.f;

	bool operator==(const FBMTierSettings& Other) const
	{
		return UphillAngleDegrees == Other.UphillAngleDegrees && SpeedLossPercent == Other.SpeedLossPercent;
	}
};

// Speed limits for every player, as the host has them, in m/s.
USTRUCT()
struct FBMSpeedLimits
{
	GENERATED_BODY()

	// Fastest a slide can go.
	UPROPERTY()
	float MaxSlideSpeed = 100.f;

	// Fastest speed kept in the air after leaving the ground from a slide, and the limit on jumps out of a slide.
	UPROPERTY()
	float MaxAirSpeed = 100.f;

	// Fastest horizontal speed kept when opening the parachute.
	UPROPERTY()
	float MaxParachuteSpeed = 100.f;

	// Slowest speed at which a player runs or slides across water instead of going in; 0 turns it off.
	UPROPERTY()
	float WaterSkimSpeed = 25.f;

	// Fraction of gravity along the glide path the parachute turns into forward speed (not in m/s); 0 turns it off.
	UPROPERTY()
	float ParachuteAcceleration = 1.f;

	bool operator==(const FBMSpeedLimits& Other) const
	{
		return MaxSlideSpeed == Other.MaxSlideSpeed && MaxAirSpeed == Other.MaxAirSpeed && MaxParachuteSpeed == Other.MaxParachuteSpeed
			&& WaterSkimSpeed == Other.WaterSkimSpeed && ParachuteAcceleration == Other.ParachuteAcceleration;
	}
};

// Returns the name of Tier for logs and the Mods menu.
inline const TCHAR* BMTierName(EBMTier Tier)
{
	switch (Tier)
	{
	case EBMTier::None: return TEXT("No Blade Runners");
	case EBMTier::Mk1: return TEXT("Blade Runners");
	case EBMTier::Mk2: return TEXT("Mk.2 Blade Runners");
	case EBMTier::Mk3: return TEXT("Mk.3 Blade Runners");
	case EBMTier::Mk4: return TEXT("Mk.4 Blade Runners");
	case EBMTier::Mk5: return TEXT("Mk.5 Blade Runners");
	default: return TEXT("?");
	}
}
