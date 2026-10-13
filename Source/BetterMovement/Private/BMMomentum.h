#pragma once

#include "CoreMinimal.h"
#include "BMTypes.h"

class UFGCharacterMovementComponent;

// Adjusts a player's max speed: a slide keeps the speed it was entered with (fading by the slide curve), a jump out
// of a slide keeps its speed in the air, a negative "Speed lost while sliding" speeds the slide up, the parachute
// keeps the speed it was opened at and gains speed like a glider, and the host's speed limits cap sliding, the air
// and the parachute.
class FBMMomentum
{
public:
	// Hooks UFGCharacterMovementComponent::GetMaxSpeed, TickSlide and UpdateCharacterStateBeforeMovement. Does nothing
	// in editor builds or when already installed.
	static void InstallHook();

	// Starts adjusting Movement and sets its slide boost (the fraction of the starting slide speed gained each second
	// of sliding, 0 for none) and the speed limits it moves by.
	static void SetSettings(UFGCharacterMovementComponent* Movement, float BoostPerSecond, const FBMSpeedLimits& Limits);

	// Forgets movement components that no longer exist.
	static void Prune();

	// Puts back every movement component's boost jump limit and forgets them all.
	static void Reset();

private:
	// Returns the max speed Movement moves at: VanillaMaxSpeed, adjusted for sliding, the air or the parachute.
	static float AdjustMaxSpeed(const UFGCharacterMovementComponent* Movement, float VanillaMaxSpeed);

	// Speeds up Movement's slide by one tick of its boost.
	static void TickBoost(UFGCharacterMovementComponent* Movement, float DeltaSeconds);

	// Speeds up Movement's parachute glide for a move of DeltaSeconds starting now.
	static void TickGlide(UFGCharacterMovementComponent* Movement, float DeltaSeconds);
};
