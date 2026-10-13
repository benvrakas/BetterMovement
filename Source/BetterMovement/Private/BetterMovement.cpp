#include "BetterMovement.h"

DEFINE_LOG_CATEGORY(LogBetterMovement);

void FBetterMovementModule::StartupModule()
{
	UE_LOG(LogBetterMovement, Log, TEXT("Better Movement module starting"));
}

void FBetterMovementModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FBetterMovementModule, BetterMovement)
