#include "BMWaterSkim.h"
#include "BetterMovement.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "FGCharacterMovementComponent.h"
#include "FGCharacterPlayer.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerState.h"
#include "Patching/NativeHookManager.h"

namespace
{
	constexpr float SkimCentimetresPerMetre = 100.f;

	// Side of a grid cell, in cm. Each cell with water under its centre gets one square tile.
	constexpr float CellSize = 1000.f;
	// How far tiles reach past the tile edges, so neighbouring tiles overlap instead of leaving seams.
	constexpr float TileOverlap = 10.f;
	constexpr float TileHalfHeight = 50.f;
	// Height of a tile's top above the water surface, keeping the player's capsule out of the water volume.
	constexpr float TileLift = 5.f;
	// Distance around the player's path covered with tiles.
	constexpr float CoverMargin = 1000.f;
	// Seconds of travel ahead of the player covered with tiles, so a player whose moves reach the server late still
	// has floor under them there.
	constexpr float LookAheadSeconds = 1.f;
	// Longest distance ahead covered with tiles, in cm.
	constexpr float MaxLookAhead = 15000.f;
	// Water surfaces count from this far above the player's feet down to SurfaceSearchDepth below them.
	constexpr float SurfaceSearchUp = 50.f;
	constexpr float SurfaceSearchDepth = 2000.f;
	// Downward speed, in cm/s, a faster fall is slowed to in the move that reaches the water, so landing on it does
	// no fall damage.
	constexpr float LandingFallSpeed = 600.f;

	struct FSkimState
	{
		// Slowest horizontal speed that skims, in cm/s; 0 when skimming is off.
		float MinSpeed = 0.f;
		TWeakObjectPtr<AActor> TileOwner;
		TMap<FIntPoint, TWeakObjectPtr<UBoxComponent>> Tiles;
		bool bSkimming = false;
	};

	TMap<TWeakObjectPtr<UCharacterMovementComponent>, FSkimState> SkimStates;

	// Water surface heights under each cell's centre, for the world and water volume count they were found with.
	TMap<FIntPoint, TArray<float>> SurfaceCache;
	TWeakObjectPtr<UWorld> SurfaceCacheWorld;
	int32 SurfaceCacheVolumeCount = -1;

	bool bSkimHookInstalled = false;
	bool bLoggedBoundsFallback = false;

	// Returns the name of the player Movement belongs to, or its owner's object name if it has no player state.
	FString DescribeSkimmer(const UCharacterMovementComponent* Movement)
	{
		const APawn* Pawn = Cast<APawn>(Movement->GetOwner());
		const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
		return PlayerState ? PlayerState->GetPlayerName() : GetNameSafe(Movement->GetOwner());
	}

	FVector2D GetCellCentre(const FIntPoint& Cell)
	{
		return FVector2D((Cell.X + 0.5f) * CellSize, (Cell.Y + 0.5f) * CellSize);
	}

	// Returns the heights of every water surface under Cell's centre.
	const TArray<float>& GetSurfaces(UWorld* World, const FIntPoint& Cell)
	{
		// Water volumes stream in and out with the world, so a changed count makes the cached heights stale.
		const int32 VolumeCount = World->GetNonDefaultPhysicsVolumeCount();
		if (SurfaceCacheWorld.Get() != World || SurfaceCacheVolumeCount != VolumeCount)
		{
			SurfaceCache.Reset();
			SurfaceCacheWorld = World;
			SurfaceCacheVolumeCount = VolumeCount;
		}
		if (const TArray<float>* Cached = SurfaceCache.Find(Cell))
		{
			return *Cached;
		}

		TArray<float>& Surfaces = SurfaceCache.Add(Cell);
		const FVector2D Centre = GetCellCentre(Cell);
		for (auto It = World->GetNonDefaultPhysicsVolumeIterator(); It; ++It)
		{
			APhysicsVolume* Volume = It->Get();
			UBrushComponent* Brush = Volume ? Volume->GetBrushComponent() : nullptr;
			if (!Brush || !Volume->bWaterVolume)
			{
				continue;
			}
			const FBox Box = Brush->Bounds.GetBox();
			if (Centre.X < Box.Min.X || Centre.X > Box.Max.X || Centre.Y < Box.Min.Y || Centre.Y > Box.Max.Y)
			{
				continue;
			}

			FHitResult Hit;
			const FVector Start(Centre.X, Centre.Y, Box.Max.Z + 10.f);
			const FVector End(Centre.X, Centre.Y, Box.Min.Z - 10.f);
			if (Brush->LineTraceComponent(Hit, Start, End, FCollisionQueryParams(SCENE_QUERY_STAT(BMWaterSurface), false)))
			{
				Surfaces.Add(Hit.ImpactPoint.Z);
			}
			else if (Volume->EncompassesPoint(FVector(Centre.X, Centre.Y, Box.Max.Z - 1.f)))
			{
				if (!bLoggedBoundsFallback)
				{
					bLoggedBoundsFallback = true;
					UE_LOG(LogBetterMovement, Log, TEXT("Water skim: trace missed %s; using the top of its bounds as the surface"), *GetNameSafe(Volume));
				}
				Surfaces.Add(Box.Max.Z);
			}
		}
		return Surfaces;
	}

	// Finds the highest water surface under Cell's centre near FeetZ. Returns false if there is none.
	bool FindSurface(UWorld* World, const FIntPoint& Cell, float FeetZ, float& OutZ)
	{
		bool bFound = false;
		for (const float Z : GetSurfaces(World, Cell))
		{
			if (Z >= FeetZ - SurfaceSearchDepth && Z <= FeetZ + SurfaceSearchUp && (!bFound || Z > OutZ))
			{
				OutZ = Z;
				bFound = true;
			}
		}
		return bFound;
	}

	// Makes every player other than Owner ignore Tile when moving, or stop ignoring it.
	void SetIgnoredByOthers(UWorld* World, UPrimitiveComponent* Tile, const AActor* Owner, bool bIgnore)
	{
		for (TActorIterator<AFGCharacterPlayer> It(World); It; ++It)
		{
			if (*It != Owner)
			{
				if (UCapsuleComponent* Capsule = It->GetCapsuleComponent())
				{
					Capsule->IgnoreComponentWhenMoving(Tile, bIgnore);
				}
			}
		}
	}

	// Creates a tile covering Cell with its top just above SurfaceZ, solid only to Character's capsule.
	UBoxComponent* CreateTile(AActor* TileOwner, const ACharacter* Character, const FIntPoint& Cell, float SurfaceZ)
	{
		const FVector2D Centre = GetCellCentre(Cell);
		UBoxComponent* Tile = NewObject<UBoxComponent>(TileOwner, NAME_None, RF_Transient);
		// Static so the character doesn't treat the tile as a moving base and send positions relative to it.
		Tile->SetMobility(EComponentMobility::Static);
		Tile->SetBoxExtent(FVector(CellSize / 2.f + TileOverlap, CellSize / 2.f + TileOverlap, TileHalfHeight), false);
		Tile->SetWorldLocation(FVector(Centre.X, Centre.Y, SurfaceZ + TileLift - TileHalfHeight));
		Tile->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Tile->SetCollisionObjectType(ECC_WorldStatic);
		Tile->SetCollisionResponseToAllChannels(ECR_Ignore);
		Tile->SetCollisionResponseToChannel(Character->GetCapsuleComponent()->GetCollisionObjectType(), ECR_Block);
		Tile->SetGenerateOverlapEvents(false);
		Tile->SetCanEverAffectNavigation(false);
		Tile->CanCharacterStepUpOn = ECB_Yes;
		Tile->SetHiddenInGame(true);
		Tile->RegisterComponent();
		SetIgnoredByOthers(TileOwner->GetWorld(), Tile, Character, true);
		return Tile;
	}

	void DestroyTile(UBoxComponent* Tile, const AActor* Character)
	{
		if (Tile)
		{
			SetIgnoredByOthers(Tile->GetWorld(), Tile, Character, false);
			Tile->DestroyComponent();
		}
	}

	void DestroyTiles(FSkimState& State, const AActor* Character)
	{
		for (const TPair<FIntPoint, TWeakObjectPtr<UBoxComponent>>& Pair : State.Tiles)
		{
			DestroyTile(Pair.Value.Get(), Character);
		}
		State.Tiles.Reset();
	}

	// Returns whether Movement is fast enough and in a state to skim.
	bool ShouldSkim(const UCharacterMovementComponent& Movement, const FSkimState& State)
	{
		if (State.MinSpeed <= 0.f || !State.TileOwner.IsValid() || !Movement.UpdatedComponent || !Movement.GetCharacterOwner())
		{
			return false;
		}
		return (Movement.IsMovingOnGround() || Movement.IsFalling()) && Movement.Velocity.Size2D() >= State.MinSpeed;
	}
}

void FBMWaterSkim::InstallHook()
{
#if WITH_EDITOR
	UE_LOG(LogBetterMovement, Log, TEXT("Editor build: water skim hook not installed"));
#else
	if (bSkimHookInstalled)
	{
		return;
	}
	bSkimHookInstalled = true;
	SUBSCRIBE_UOBJECT_METHOD(UFGCharacterMovementComponent, UpdateCharacterStateBeforeMovement, [](auto& Scope, UCharacterMovementComponent* Self, float DeltaSeconds)
	{
		BeforeMove(Self, DeltaSeconds);
	});
	UE_LOG(LogBetterMovement, Log, TEXT("Hook installed: UCharacterMovementComponent::UpdateCharacterStateBeforeMovement"));
#endif
}

void FBMWaterSkim::SetSettings(UCharacterMovementComponent* Movement, float MinSpeed, AActor* TileOwner)
{
	FSkimState& State = SkimStates.FindOrAdd(Movement);
	const float MinSpeedCm = FMath::Max(MinSpeed, 0.f) * SkimCentimetresPerMetre;
	if (State.MinSpeed != MinSpeedCm)
	{
		UE_LOG(LogBetterMovement, Log, TEXT("%s: water skim speed %.0f cm/s%s"), *DescribeSkimmer(Movement), MinSpeedCm, MinSpeedCm > 0.f ? TEXT("") : TEXT(" (off)"));
		State.MinSpeed = MinSpeedCm;
	}
	State.TileOwner = TileOwner;
}

void FBMWaterSkim::Prune()
{
	for (auto It = SkimStates.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			DestroyTiles(It.Value(), nullptr);
			It.RemoveCurrent();
		}
	}
}

void FBMWaterSkim::Reset()
{
	for (TPair<TWeakObjectPtr<UCharacterMovementComponent>, FSkimState>& Pair : SkimStates)
	{
		const UCharacterMovementComponent* Movement = Pair.Key.Get();
		DestroyTiles(Pair.Value, Movement ? Movement->GetOwner() : nullptr);
	}
	SkimStates.Reset();
	SurfaceCache.Reset();
	SurfaceCacheWorld.Reset();
	SurfaceCacheVolumeCount = -1;
}

void FBMWaterSkim::BeforeMove(UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	FSkimState* State = SkimStates.Find(Movement);
	if (!State)
	{
		return;
	}

	if (!ShouldSkim(*Movement, *State))
	{
		if (State->Tiles.Num() > 0)
		{
			DestroyTiles(*State, Movement->GetOwner());
		}
		if (State->bSkimming)
		{
			State->bSkimming = false;
			UE_LOG(LogBetterMovement, Log, TEXT("%s: stopped skimming at %.0f cm/s"), *DescribeSkimmer(Movement), Movement->Velocity.Size2D());
		}
		return;
	}

	ACharacter* Character = Movement->GetCharacterOwner();
	UWorld* World = Movement->GetWorld();
	const FVector Location = Movement->UpdatedComponent->GetComponentLocation();
	const float FeetZ = Location.Z - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FIntPoint Centre(FMath::FloorToInt(Location.X / CellSize), FMath::FloorToInt(Location.Y / CellSize));

	// Covers the path from the player to where they'll be after LookAheadSeconds (or this move, if longer), plus
	// CoverMargin on every side. A cell counts if any part of it is within CoverMargin of the path.
	const FVector2D PathStart(Location.X, Location.Y);
	FVector2D Ahead = FVector2D(Movement->Velocity.X, Movement->Velocity.Y) * FMath::Max(DeltaSeconds, LookAheadSeconds);
	if (Ahead.Size() > MaxLookAhead)
	{
		Ahead = Ahead.GetSafeNormal() * MaxLookAhead;
	}
	const FVector2D PathEnd = PathStart + Ahead;
	const float CellReach = CoverMargin + CellSize * UE_HALF_SQRT_2;
	const FIntPoint MinCell(FMath::FloorToInt((FMath::Min(PathStart.X, PathEnd.X) - CoverMargin) / CellSize),
		FMath::FloorToInt((FMath::Min(PathStart.Y, PathEnd.Y) - CoverMargin) / CellSize));
	const FIntPoint MaxCell(FMath::FloorToInt((FMath::Max(PathStart.X, PathEnd.X) + CoverMargin) / CellSize),
		FMath::FloorToInt((FMath::Max(PathStart.Y, PathEnd.Y) + CoverMargin) / CellSize));

	TSet<FIntPoint> Wanted;
	int32 Created = 0;
	bool bHasCentreSurface = false;
	float CentreSurfaceZ = 0.f;
	for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
	{
		for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
		{
			const FIntPoint Cell(X, Y);
			const FVector2D CellCentre = GetCellCentre(Cell);
			if (Cell != Centre && FVector2D::Distance(CellCentre, FMath::ClosestPointOnSegment2D(CellCentre, PathStart, PathEnd)) > CellReach)
			{
				continue;
			}
			float SurfaceZ = 0.f;
			if (!FindSurface(World, Cell, FeetZ, SurfaceZ))
			{
				continue;
			}
			Wanted.Add(Cell);
			if (Cell == Centre)
			{
				bHasCentreSurface = true;
				CentreSurfaceZ = SurfaceZ;
			}
			TWeakObjectPtr<UBoxComponent>& Tile = State->Tiles.FindOrAdd(Cell);
			if (!Tile.IsValid())
			{
				Tile = CreateTile(State->TileOwner.Get(), Character, Cell, SurfaceZ);
				++Created;
			}
		}
	}
	for (auto It = State->Tiles.CreateIterator(); It; ++It)
	{
		if (!Wanted.Contains(It.Key()))
		{
			DestroyTile(It.Value().Get(), Character);
			It.RemoveCurrent();
		}
	}

	const float FallSpeed = -Movement->Velocity.Z;
	if (bHasCentreSurface && Movement->IsFalling() && FallSpeed > LandingFallSpeed
		&& FeetZ - (CentreSurfaceZ + TileLift) <= FallSpeed * FMath::Max(DeltaSeconds, 0.f))
	{
		UE_LOG(LogBetterMovement, Verbose, TEXT("%s: landing on water, fall slowed from %.0f to %.0f cm/s"), *DescribeSkimmer(Movement), FallSpeed, LandingFallSpeed);
		Movement->Velocity.Z = -LandingFallSpeed;
	}

	const bool bOverWater = Wanted.Num() > 0;
	if (bOverWater != State->bSkimming)
	{
		State->bSkimming = bOverWater;
		UE_LOG(LogBetterMovement, Log, TEXT("%s: %s skimming at %.0f cm/s (%d tiles)"), *DescribeSkimmer(Movement),
			bOverWater ? TEXT("started") : TEXT("stopped"), Movement->Velocity.Size2D(), State->Tiles.Num());
	}
	if (Created > 0)
	{
		UE_LOG(LogBetterMovement, VeryVerbose, TEXT("%s: created %d water tiles (%d total)"), *DescribeSkimmer(Movement), Created, State->Tiles.Num());
	}
}
