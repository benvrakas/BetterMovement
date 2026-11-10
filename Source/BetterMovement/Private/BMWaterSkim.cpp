#include "BMWaterSkim.h"
#include "BetterMovement.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "FGCharacterMovementComponent.h"
#include "FGCharacterPlayer.h"
#include "GameFramework/Character.h"
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
	// Seconds the server keeps trusting a remote player's position after they were last near water.
	constexpr double TrustLingerSeconds = 2.0;
	// Seconds after a player stops skimming during which corrections from the server are still logged.
	constexpr double CorrectionLogSeconds = 2.0;
	// Shortest time between two logged corrections for one player.
	constexpr double CorrectionLogInterval = 1.0;

	struct FSkimState
	{
		// Slowest horizontal speed that skims, in cm/s; 0 when skimming is off.
		float MinSpeed = 0.f;
		TWeakObjectPtr<AActor> TileOwner;
		TMap<FIntPoint, TWeakObjectPtr<UBoxComponent>> Tiles;
		bool bSkimming = false;
		// World time this player last had tiles under or ahead of them.
		double LastSkimTime = -1.0e9;

		// Server only: the remote player's moves are taken as they are sent, without error checks or corrections.
		bool bTrusting = false;
		// The movement component's own values of the two trust flags, put back when trust ends.
		bool bSavedIgnoreChecks = false;
		bool bSavedAcceptAuthoritative = false;
		double LastNearWaterTime = -1.0e9;
		// Server only: forced position updates are being skipped and no move has come in since the first skip.
		bool bSkippingForcedUpdates = false;
		float LongestSkippedGap = 0.f;

		// Client only: when a server correction was last logged, and how many were not logged since then.
		double LastCorrectionLogTime = -1.0e9;
		int32 UnloggedCorrections = 0;
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

	// Returns whether Movement belongs to a remote player's character on the server.
	bool IsRemoteOnServer(const UCharacterMovementComponent& Movement)
	{
		const ACharacter* Character = Movement.GetCharacterOwner();
		return Character && Character->GetLocalRole() == ROLE_Authority && !Character->IsLocallyControlled();
	}

	// Makes the server take Movement's position and movement mode from its player's moves without correcting them.
	void StartTrust(UCharacterMovementComponent* Movement, FSkimState& State)
	{
		State.bSavedIgnoreChecks = Movement->bIgnoreClientMovementErrorChecksAndCorrection;
		State.bSavedAcceptAuthoritative = Movement->bServerAcceptClientAuthoritativePosition;
		Movement->bIgnoreClientMovementErrorChecksAndCorrection = true;
		Movement->bServerAcceptClientAuthoritativePosition = true;
		State.bTrusting = true;
		UE_LOG(LogBetterMovement, Log, TEXT("%s: near water, server now trusts their position"), *DescribeSkimmer(Movement));
	}

	// Puts back Movement's own trust flags, leaving any that something else turned off while trusting.
	void EndTrust(UCharacterMovementComponent* Movement, FSkimState& State, const TCHAR* Reason)
	{
		if (!State.bTrusting)
		{
			return;
		}
		State.bTrusting = false;
		State.bSkippingForcedUpdates = false;
		if (Movement)
		{
			if (Movement->bIgnoreClientMovementErrorChecksAndCorrection)
			{
				Movement->bIgnoreClientMovementErrorChecksAndCorrection = State.bSavedIgnoreChecks;
			}
			if (Movement->bServerAcceptClientAuthoritativePosition)
			{
				Movement->bServerAcceptClientAuthoritativePosition = State.bSavedAcceptAuthoritative;
			}
			UE_LOG(LogBetterMovement, Log, TEXT("%s: server checks their position again (%s)"), *DescribeSkimmer(Movement), Reason);
		}
	}

	// Places or removes Movement's tiles for a move of DeltaSeconds starting now. Returns whether any are wanted.
	bool UpdateTiles(UCharacterMovementComponent* Movement, FSkimState& State, float DeltaSeconds)
	{
		if (!ShouldSkim(*Movement, State))
		{
			if (State.Tiles.Num() > 0)
			{
				DestroyTiles(State, Movement->GetOwner());
			}
			if (State.bSkimming)
			{
				State.bSkimming = false;
				UE_LOG(LogBetterMovement, Log, TEXT("%s: stopped skimming at %.0f cm/s"), *DescribeSkimmer(Movement), Movement->Velocity.Size2D());
			}
			return false;
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
				TWeakObjectPtr<UBoxComponent>& Tile = State.Tiles.FindOrAdd(Cell);
				if (!Tile.IsValid())
				{
					Tile = CreateTile(State.TileOwner.Get(), Character, Cell, SurfaceZ);
					++Created;
				}
			}
		}
		for (auto It = State.Tiles.CreateIterator(); It; ++It)
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
		if (bOverWater)
		{
			State.LastSkimTime = World->GetTimeSeconds();
		}
		if (bOverWater != State.bSkimming)
		{
			State.bSkimming = bOverWater;
			UE_LOG(LogBetterMovement, Log, TEXT("%s: %s skimming at %.0f cm/s (%d tiles)"), *DescribeSkimmer(Movement),
				bOverWater ? TEXT("started") : TEXT("stopped"), Movement->Velocity.Size2D(), State.Tiles.Num());
		}
		if (Created > 0)
		{
			UE_LOG(LogBetterMovement, VeryVerbose, TEXT("%s: created %d water tiles (%d total)"), *DescribeSkimmer(Movement), Created, State.Tiles.Num());
		}
		return bOverWater;
	}

	// On the server, starts or ends trust in a remote player's moves. bOverWater is whether this move placed tiles.
	void UpdateTrust(UCharacterMovementComponent* Movement, FSkimState& State, bool bOverWater)
	{
		if (!IsRemoteOnServer(*Movement))
		{
			EndTrust(Movement, State, TEXT("no longer a remote player"));
			return;
		}
		if (State.MinSpeed <= 0.f || !Movement->UpdatedComponent)
		{
			EndTrust(Movement, State, TEXT("water skimming off"));
			return;
		}

		UWorld* World = Movement->GetWorld();
		const double Now = World->GetTimeSeconds();
		if (State.bSkippingForcedUpdates)
		{
			State.bSkippingForcedUpdates = false;
			UE_LOG(LogBetterMovement, Log, TEXT("%s: moves arrived again after a %.2f s gap"), *DescribeSkimmer(Movement), State.LongestSkippedGap);
		}

		// Speed is left out on purpose: a server running this player's moves slower than the client must not stop
		// trusting them, or it would correct them into the water.
		bool bNearWater = bOverWater || Movement->IsInWater();
		if (!bNearWater)
		{
			const FVector Location = Movement->UpdatedComponent->GetComponentLocation();
			const float FeetZ = Location.Z - Movement->GetCharacterOwner()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const FIntPoint Cell(FMath::FloorToInt(Location.X / CellSize), FMath::FloorToInt(Location.Y / CellSize));
			float SurfaceZ = 0.f;
			bNearWater = FindSurface(World, Cell, FeetZ, SurfaceZ);
		}

		if (bNearWater)
		{
			State.LastNearWaterTime = Now;
			if (!State.bTrusting)
			{
				StartTrust(Movement, State);
			}
		}
		else if (State.bTrusting && Now - State.LastNearWaterTime > TrustLingerSeconds)
		{
			EndTrust(Movement, State, TEXT("away from water"));
		}
	}

	// Returns whether the server should skip simulating Movement's player itself after DeltaTime without their moves.
	bool ShouldSkipForcedUpdate(UCharacterMovementComponent* Movement, float DeltaTime)
	{
		FSkimState* State = SkimStates.Find(Movement);
		if (!State || !State->bTrusting)
		{
			return false;
		}
		if (!State->bSkippingForcedUpdates)
		{
			State->bSkippingForcedUpdates = true;
			State->LongestSkippedGap = 0.f;
			UE_LOG(LogBetterMovement, Log, TEXT("%s: no moves for %.2f s; holding them in place instead of simulating them near water"), *DescribeSkimmer(Movement), DeltaTime);
		}
		State->LongestSkippedGap = FMath::Max(State->LongestSkippedGap, DeltaTime);
		return true;
	}

	// Logs a server correction of the local player that arrives while they skim or shortly after.
	void LogCorrection(UCharacterMovementComponent* Movement, const FVector& NewLocation, const FVector& NewVelocity, bool bBaseRelativePosition, uint8 ServerMovementMode)
	{
		FSkimState* State = SkimStates.Find(Movement);
		if (!State || State->MinSpeed <= 0.f || !Movement->UpdatedComponent)
		{
			return;
		}
		const double Now = Movement->GetWorld()->GetTimeSeconds();
		if (!State->bSkimming && Now - State->LastSkimTime > CorrectionLogSeconds)
		{
			return;
		}
		if (Now - State->LastCorrectionLogTime < CorrectionLogInterval)
		{
			++State->UnloggedCorrections;
			return;
		}

		TEnumAsByte<EMovementMode> Mode;
		TEnumAsByte<EMovementMode> GroundMode;
		uint8 CustomMode = 0;
		Movement->UnpackNetworkMovementMode(ServerMovementMode, Mode, CustomMode, GroundMode);
		const FString ModeName = Mode == MOVE_Custom
			? FString::Printf(TEXT("Custom %d"), CustomMode)
			: StaticEnum<EMovementMode>()->GetNameStringByValue(Mode.GetValue());
		const FString Error = bBaseRelativePosition
			? FString(TEXT("relative to a moving base"))
			: FString::Printf(TEXT("%.0f cm off, %.0f cm vertically"), FVector::Dist(NewLocation, Movement->UpdatedComponent->GetComponentLocation()),
				NewLocation.Z - Movement->UpdatedComponent->GetComponentLocation().Z);
		UE_LOG(LogBetterMovement, Log, TEXT("%s: server corrected position %s skimming: server mode %s, %s, speed server %.0f / client %.0f cm/s (%d more since last log)"),
			*DescribeSkimmer(Movement), State->bSkimming ? TEXT("while") : TEXT("just after"), *ModeName, *Error,
			NewVelocity.Size2D(), Movement->Velocity.Size2D(), State->UnloggedCorrections);
		State->LastCorrectionLogTime = Now;
		State->UnloggedCorrections = 0;
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
	SUBSCRIBE_UOBJECT_METHOD(UFGCharacterMovementComponent, ForcePositionUpdate, [](auto& Scope, UCharacterMovementComponent* Self, float DeltaTime)
	{
		if (ShouldSkipForcedUpdate(Self, DeltaTime))
		{
			Scope.Override(false);
		}
	});
	UE_LOG(LogBetterMovement, Log, TEXT("Hook installed: UCharacterMovementComponent::ForcePositionUpdate"));
	if (!IsRunningDedicatedServer())
	{
		SUBSCRIBE_UOBJECT_METHOD(UFGCharacterMovementComponent, ClientAdjustPosition_Implementation, [](auto& Scope, UCharacterMovementComponent* Self, float TimeStamp, FVector NewLocation,
			FVector NewVelocity, UPrimitiveComponent* NewBase, FName NewBaseBoneName, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode, TOptional<FRotator> OptionalRotation)
		{
			LogCorrection(Self, NewLocation, NewVelocity, bBaseRelativePosition, ServerMovementMode);
		});
		UE_LOG(LogBetterMovement, Log, TEXT("Hook installed: UCharacterMovementComponent::ClientAdjustPosition_Implementation"));
	}
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
		UCharacterMovementComponent* Movement = Pair.Key.Get();
		DestroyTiles(Pair.Value, Movement ? Movement->GetOwner() : nullptr);
		EndTrust(Movement, Pair.Value, TEXT("mod settings reset"));
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
	const bool bOverWater = UpdateTiles(Movement, *State, DeltaSeconds);
	UpdateTrust(Movement, *State, bOverWater);
}
