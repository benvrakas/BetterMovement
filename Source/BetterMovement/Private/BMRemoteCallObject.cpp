#include "BMRemoteCallObject.h"
#include "BetterMovement.h"
#include "BMConfig.h"
#include "BMMovementSubsystem.h"
#include "FGChatManager.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

namespace
{
	// Most settings one message may change; the configuration has fewer than this.
	constexpr int32 MaxChanges = 64;
	constexpr int32 MaxPathLength = 64;

	const FLinearColor MessageColor(0.4f, 0.8f, 1.f);

	FChatMessageStruct MakeMessage(const UWorld* World, const FString& Text)
	{
		FChatMessageStruct Message;
		Message.MessageText = FText::FromString(Text);
		Message.MessageType = EFGChatMessageType::CMT_CustomMessage;
		Message.MessageSender = FText::FromString(TEXT("Better Movement"));
		Message.MessageSenderColor = MessageColor;
		const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
		Message.ServerTimeStamp = GameState ? GameState->GetServerWorldTimeSeconds() : 0.f;
		return Message;
	}
}

void UBMRemoteCallObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBMRemoteCallObject, mForceNetField_UBMRemoteCallObject);
}

UBMRemoteCallObject* UBMRemoteCallObject::GetLocal(const UObject* WorldContext)
{
	AFGPlayerController* Controller = WorldContext ? Cast<AFGPlayerController>(UGameplayStatics::GetPlayerController(WorldContext, 0)) : nullptr;
	if (!Controller)
	{
		UE_LOG(LogBetterMovement, Warning, TEXT("No local player controller to send settings through"));
		return nullptr;
	}
	UBMRemoteCallObject* RCO = Controller->GetRemoteCallObjectOfClass<UBMRemoteCallObject>();
	if (!RCO)
	{
		UE_LOG(LogBetterMovement, Warning, TEXT("Remote call object not registered on the local player (is the mod installed on the server?)"));
	}
	return RCO;
}

bool UBMRemoteCallObject::IsLocalAdmin(const UObject* WorldContext)
{
	const APlayerController* Controller = WorldContext ? UGameplayStatics::GetPlayerController(WorldContext, 0) : nullptr;
	const AFGPlayerState* State = Controller ? Cast<AFGPlayerState>(Controller->PlayerState) : nullptr;
	return State && State->IsServerAdmin();
}

void UBMRemoteCallObject::ShowInChat(const UObject* WorldContext, const FString& Text)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (AFGChatManager* ChatManager = World ? AFGChatManager::Get(World) : nullptr)
	{
		ChatManager->AddChatMessageToReceived(MakeMessage(World, Text));
	}
}

FString UBMRemoteCallObject::GetPlayerName() const
{
	const AFGPlayerController* Controller = GetOwnerPlayerController();
	const APlayerState* State = Controller ? Controller->GetPlayerState<APlayerState>() : nullptr;
	return State ? State->GetPlayerName() : GetNameSafe(Controller);
}

void UBMRemoteCallObject::Client_ShowMessage_Implementation(const FString& Text)
{
	ShowInChat(this, Text);
}

bool UBMRemoteCallObject::Server_SetSettings_Validate(const TArray<FBMSettingChange>& Changes)
{
	return Changes.Num() <= MaxChanges
		&& !Changes.ContainsByPredicate([](const FBMSettingChange& Change) { return Change.Path.Len() > MaxPathLength; });
}

void UBMRemoteCallObject::Server_SetSettings_Implementation(const TArray<FBMSettingChange>& Changes)
{
	const AFGPlayerController* Controller = GetOwnerPlayerController();
	const AFGPlayerState* State = Controller ? Cast<AFGPlayerState>(Controller->PlayerState) : nullptr;
	if (!State || !State->IsServerAdmin())
	{
		UE_LOG(LogBetterMovement, Log, TEXT("Ignored %d setting changes from %s: not a server admin"), Changes.Num(), *GetPlayerName());
		Client_ShowMessage(TEXT("Only server admins can change this server's settings. Your change still applies in games you host."));
		return;
	}

	TArray<FString> Applied;
	for (const FBMSettingChange& Change : Changes)
	{
		FString Description;
		if (UBMConfig::SetValue(this, Change.Path, Change.Value, Description))
		{
			UE_LOG(LogBetterMovement, Log, TEXT("Admin %s set %s to %g (%s)"), *GetPlayerName(), *Change.Path, Change.Value, *Description);
			Applied.Add(Description);
		}
		else
		{
			UE_LOG(LogBetterMovement, Warning, TEXT("Admin %s sent unknown setting '%s'"), *GetPlayerName(), *Change.Path);
		}
	}
	if (Applied.Num() == 0)
	{
		return;
	}

	if (ABMMovementSubsystem* Subsystem = ABMMovementSubsystem::Get(this))
	{
		Subsystem->ApplyConfigNow();
	}
	const FString Text = FString::Printf(TEXT("%s changed the server's settings. %s"), *GetPlayerName(), *FString::Join(Applied, TEXT("; ")));
	if (AFGChatManager* ChatManager = AFGChatManager::Get(GetWorld()))
	{
		ChatManager->BroadcastChatMessage(MakeMessage(GetWorld(), Text));
	}
}
