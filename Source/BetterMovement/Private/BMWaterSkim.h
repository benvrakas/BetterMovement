#pragma once

#include "CoreMinimal.h"

class AActor;
class UCharacterMovementComponent;

// Lets a player moving at least the host's water skim speed run and slide across water: before each move, invisible
// floor tiles are placed at the water surface around them, and removed once they're slower.
class FBMWaterSkim
{
public:
	// Hooks UCharacterMovementComponent::UpdateCharacterStateBeforeMovement. Does nothing in editor builds or when
	// already installed.
	static void InstallHook();

	// Starts handling Movement: it skims at MinSpeed m/s or faster (0 turns skimming off), on tiles owned by TileOwner.
	static void SetSettings(UCharacterMovementComponent* Movement, float MinSpeed, AActor* TileOwner);

	// Forgets movement components that no longer exist and removes their tiles.
	static void Prune();

	// Removes every tile and forgets every movement component.
	static void Reset();

private:
	// Places or removes Movement's tiles for a move of DeltaSeconds starting now.
	static void BeforeMove(UCharacterMovementComponent* Movement, float DeltaSeconds);
};
