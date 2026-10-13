#include "BMMomentum.h"
#include "BetterMovement.h"
#include "Curves/CurveFloat.h"
#include "FGCharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Patching/NativeHookManager.h"

namespace
{
	constexpr float CentimetresPerMetre = 100.f;

	struct FMomentumState
	{
		float BoostPerSecond = 0.f;
		// Speed limits in cm/s.
		float MaxSlideSpeed = 0.f;
		float MaxAirSpeed = 0.f;
		float MaxParachuteSpeed = 0.f;
		// Fraction of gravity along the glide path the parachute turns into horizontal speed.
		float GlideAcceleration = 0.f;
		float VanillaBoostJumpLimit = 0.f;
		bool bWasSliding = false;
		bool bAirborneFromSlide = false;
		// Horizontal speed when the current slide started, and the slide curve's value at that moment.
		float EntrySpeed = 0.f;
		float EntryCurveValue = 0.f;
		// Speed the boost builds from, and seconds of the current slide it has been building.
		float BoostBaseSpeed = 0.f;
		float BoostSeconds = 0.f;
		// Horizontal speed when the player left the ground from a slide.
		float TakeoffSpeed = 0.f;
		bool bParachuting = false;
		// Horizontal speed when the current parachute opened.
		float ParachuteEntrySpeed = 0.f;

		// Speed the boost has reached so far in the current slide.
		float GetBoostedSpeed() const
		{
			return FMath::Min(BoostBaseSpeed * (1.f + BoostPerSecond * BoostSeconds), MaxSlideSpeed);
		}
	};

	TMap<TWeakObjectPtr<UFGCharacterMovementComponent>, FMomentumState> States;
	bool bHookInstalled = false;

	// Returns the name of the player Movement belongs to, or its owner's object name if it has no player state.
	FString DescribeOwner(const UFGCharacterMovementComponent* Movement)
	{
		const APawn* Pawn = Cast<APawn>(Movement->GetOwner());
		const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
		return PlayerState ? PlayerState->GetPlayerName() : GetNameSafe(Movement->GetOwner());
	}

	FMomentumState* FindState(const UFGCharacterMovementComponent* Movement)
	{
		return States.Find(TWeakObjectPtr<UFGCharacterMovementComponent>(const_cast<UFGCharacterMovementComponent*>(Movement)));
	}
}

void FBMMomentum::InstallHook()
{
#if WITH_EDITOR
	UE_LOG(LogBetterMovement, Log, TEXT("Editor build: movement hooks not installed"));
#else
	if (bHookInstalled)
	{
		return;
	}
	bHookInstalled = true;
	SUBSCRIBE_UOBJECT_METHOD(UFGCharacterMovementComponent, GetMaxSpeed, [](auto& Scope, const UFGCharacterMovementComponent* Self)
	{
		const float VanillaMaxSpeed = Scope(Self);
		const float Adjusted = AdjustMaxSpeed(Self, VanillaMaxSpeed);
		if (Adjusted != VanillaMaxSpeed)
		{
			Scope.Override(Adjusted);
		}
	});
	UE_LOG(LogBetterMovement, Log, TEXT("Hook installed: UFGCharacterMovementComponent::GetMaxSpeed"));
	SUBSCRIBE_METHOD_AFTER(UFGCharacterMovementComponent::TickSlide, [](UFGCharacterMovementComponent* Self, const float DeltaSeconds)
	{
		TickBoost(Self, DeltaSeconds);
	});
	UE_LOG(LogBetterMovement, Log, TEXT("Hook installed: UFGCharacterMovementComponent::TickSlide"));
	SUBSCRIBE_UOBJECT_METHOD(UFGCharacterMovementComponent, UpdateCharacterStateBeforeMovement, [](auto& Scope, UCharacterMovementComponent* Self, float DeltaSeconds)
	{
		if (UFGCharacterMovementComponent* Movement = Cast<UFGCharacterMovementComponent>(Self))
		{
			TickGlide(Movement, DeltaSeconds);
		}
	});
	UE_LOG(LogBetterMovement, Log, TEXT("Hook installed: UCharacterMovementComponent::UpdateCharacterStateBeforeMovement (glide)"));
#endif
}

void FBMMomentum::SetSettings(UFGCharacterMovementComponent* Movement, float BoostPerSecond, const FBMSpeedLimits& Limits)
{
	FMomentumState* State = States.Find(Movement);
	if (!State)
	{
		State = &States.Add(Movement);
		State->VanillaBoostJumpLimit = Movement->mBoostJumpVelocityLimit;
	}
	State->BoostPerSecond = FMath::Max(BoostPerSecond, 0.f);
	State->MaxSlideSpeed = Limits.MaxSlideSpeed * CentimetresPerMetre;
	State->MaxAirSpeed = Limits.MaxAirSpeed * CentimetresPerMetre;
	State->MaxParachuteSpeed = Limits.MaxParachuteSpeed * CentimetresPerMetre;
	if (State->GlideAcceleration != Limits.ParachuteAcceleration)
	{
		UE_LOG(LogBetterMovement, Log, TEXT("%s: parachute acceleration %.2f"), *DescribeOwner(Movement), Limits.ParachuteAcceleration);
		State->GlideAcceleration = Limits.ParachuteAcceleration;
	}

	// The boost jump limit caps a jump out of a slide, so it follows the air speed limit.
	if (Movement->mBoostJumpVelocityLimit != State->MaxAirSpeed)
	{
		UE_LOG(LogBetterMovement, Log, TEXT("%s: boost jump limit %.0f -> %.0f cm/s (vanilla %.0f)"),
			*DescribeOwner(Movement), Movement->mBoostJumpVelocityLimit, State->MaxAirSpeed, State->VanillaBoostJumpLimit);
		Movement->mBoostJumpVelocityLimit = State->MaxAirSpeed;
	}
}

void FBMMomentum::Prune()
{
	for (auto It = States.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void FBMMomentum::Reset()
{
	for (const TPair<TWeakObjectPtr<UFGCharacterMovementComponent>, FMomentumState>& Pair : States)
	{
		if (UFGCharacterMovementComponent* Movement = Pair.Key.Get())
		{
			Movement->mBoostJumpVelocityLimit = Pair.Value.VanillaBoostJumpLimit;
		}
	}
	States.Reset();
}

float FBMMomentum::AdjustMaxSpeed(const UFGCharacterMovementComponent* Movement, float VanillaMaxSpeed)
{
	FMomentumState* State = FindState(Movement);
	if (!State)
	{
		return VanillaMaxSpeed;
	}

	// The parachute keeps the horizontal speed it was opened at, never above the limit and never below vanilla's
	// strafe speed (unless the limit is lower).
	if (Movement->IsParachuting())
	{
		State->bWasSliding = false;
		State->bAirborneFromSlide = false;
		if (!State->bParachuting)
		{
			State->bParachuting = true;
			State->ParachuteEntrySpeed = Movement->Velocity.Size2D();
			UE_LOG(LogBetterMovement, Verbose, TEXT("Parachute opened at %.0f cm/s (vanilla max %.0f, limit %.0f)"),
				State->ParachuteEntrySpeed, VanillaMaxSpeed, State->MaxParachuteSpeed);
		}
		const float KeptSpeed = FMath::Min(static_cast<float>(Movement->Velocity.Size2D()), State->ParachuteEntrySpeed);
		return FMath::Min(FMath::Max(VanillaMaxSpeed, KeptSpeed), State->MaxParachuteSpeed);
	}
	State->bParachuting = false;

	// The slide and air adjustments only stop the game pulling the player down; they never raise them above their
	// current speed.
	if (!Movement->IsSliding())
	{
		if (Movement->IsFalling() && (State->bWasSliding || State->bAirborneFromSlide))
		{
			if (!State->bAirborneFromSlide)
			{
				State->bAirborneFromSlide = true;
				State->TakeoffSpeed = Movement->Velocity.Size2D();
				UE_LOG(LogBetterMovement, Verbose, TEXT("Left the ground from a slide at %.0f cm/s"), State->TakeoffSpeed);
			}
			State->bWasSliding = false;
			const float KeptSpeed = FMath::Min3(static_cast<float>(Movement->Velocity.Size2D()), State->TakeoffSpeed, State->MaxAirSpeed);
			return FMath::Max(VanillaMaxSpeed, KeptSpeed);
		}
		State->bWasSliding = false;
		State->bAirborneFromSlide = false;
		return VanillaMaxSpeed;
	}

	const UCurveFloat* SlideCurve = Movement->mSlideCurve;
	const float CurveValue = SlideCurve ? SlideCurve->GetFloatValue(Movement->mSlideTime) : 1.f;
	if (!State->bWasSliding)
	{
		State->bWasSliding = true;
		State->bAirborneFromSlide = false;
		State->EntrySpeed = Movement->Velocity.Size2D();
		State->EntryCurveValue = CurveValue;
		State->BoostBaseSpeed = FMath::Max(VanillaMaxSpeed, State->EntrySpeed);
		State->BoostSeconds = 0.f;
		UE_LOG(LogBetterMovement, Verbose, TEXT("Slide started at %.0f cm/s (vanilla slide speed %.0f, boost %.0f%%/s, limit %.0f)"),
			State->EntrySpeed, VanillaMaxSpeed, State->BoostPerSecond * 100.f, State->MaxSlideSpeed);
	}

	const float CurrentSpeed = Movement->Velocity.Size();
	float Result = VanillaMaxSpeed;
	if (State->EntryCurveValue > KINDA_SMALL_NUMBER)
	{
		const float FadedEntrySpeed = State->EntrySpeed * CurveValue / State->EntryCurveValue;
		Result = FMath::Max(Result, FMath::Min(CurrentSpeed, FadedEntrySpeed));
	}
	if (State->BoostPerSecond > 0.f)
	{
		Result = FMath::Max(Result, FMath::Min(CurrentSpeed, State->GetBoostedSpeed()));
	}
	return FMath::Min(Result, State->MaxSlideSpeed);
}

void FBMMomentum::TickBoost(UFGCharacterMovementComponent* Movement, float DeltaSeconds)
{
	FMomentumState* State = States.Find(Movement);
	if (!State || State->BoostPerSecond <= 0.f || !State->bWasSliding || !Movement->IsSliding() || DeltaSeconds <= 0.f)
	{
		return;
	}

	State->BoostSeconds += DeltaSeconds;
	// Adds at most one tick's worth of boost, so running into something still slows the slide down.
	const float Speed = Movement->Velocity.Size();
	const float Target = FMath::Min(State->GetBoostedSpeed(), Speed + State->BoostBaseSpeed * State->BoostPerSecond * DeltaSeconds);
	if (Speed > KINDA_SMALL_NUMBER && Target > Speed)
	{
		Movement->Velocity *= Target / Speed;
	}
}

void FBMMomentum::TickGlide(UFGCharacterMovementComponent* Movement, float DeltaSeconds)
{
	FMomentumState* State = States.Find(Movement);
	if (!State || State->GlideAcceleration <= 0.f || !Movement->IsParachuting() || DeltaSeconds <= 0.f)
	{
		return;
	}

	// Only speeds up while the player holds a direction and Shift; without Shift the kept speed holds, and letting go
	// of the direction lets the parachute slow down as vanilla.
	if (!Movement->GetIsSprinting())
	{
		return;
	}
	const FVector Input = Movement->GetCurrentAcceleration();
	const FVector2D InputDirection = FVector2D(Input.X, Input.Y).GetSafeNormal();
	const FVector2D Horizontal(Movement->Velocity.X, Movement->Velocity.Y);
	const float Speed = Horizontal.Size();
	if (InputDirection.IsZero() || Speed >= State->MaxParachuteSpeed)
	{
		return;
	}

	// Gravity along the glide path: g x sin(glide angle), with the angle from the current fall and horizontal speed.
	const float FallSpeed = FMath::Max(static_cast<float>(-Movement->Velocity.Z), 0.f);
	const float PathSpeed = FMath::Sqrt(Speed * Speed + FallSpeed * FallSpeed);
	if (PathSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	const float Gain = State->GlideAcceleration * FMath::Abs(Movement->GetGravityZ()) * (FallSpeed / PathSpeed) * DeltaSeconds;
	const float NewSpeed = FMath::Min(Speed + Gain, State->MaxParachuteSpeed);
	const FVector2D Direction = Speed > 100.f ? Horizontal / Speed : InputDirection;
	const FVector2D NewHorizontal = Horizontal + Direction * (NewSpeed - Speed);
	Movement->Velocity.X = NewHorizontal.X;
	Movement->Velocity.Y = NewHorizontal.Y;

	// The gained speed becomes the speed the parachute keeps, so GetMaxSpeed doesn't brake it back down.
	if (State->bParachuting)
	{
		State->ParachuteEntrySpeed = FMath::Max(State->ParachuteEntrySpeed, NewSpeed);
	}
}
