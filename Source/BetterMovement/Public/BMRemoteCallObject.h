#pragma once

#include "CoreMinimal.h"
#include "BMTypes.h"
#include "FGRemoteCallObject.h"
#include "BMRemoteCallObject.generated.h"

// Carries settings a dedicated server admin changed in their Mods menu to the server, which applies and saves them.
UCLASS(NotBlueprintable)
class BETTERMOVEMENT_API UBMRemoteCallObject : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	// Applies Changes to the server's configuration if the sending player is a server admin.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetSettings(const TArray<FBMSettingChange>& Changes);

	// Shows Text in the local player's chat.
	UFUNCTION(Client, Reliable)
	void Client_ShowMessage(const FString& Text);

	// Returns the local player's instance, or nullptr (logged) if the server doesn't run the mod.
	static UBMRemoteCallObject* GetLocal(const UObject* WorldContext);

	// Returns whether the local player is logged in as an admin of the server they're connected to.
	static bool IsLocalAdmin(const UObject* WorldContext);

	// Shows Text in the local player's chat as a Better Movement message.
	static void ShowInChat(const UObject* WorldContext, const FString& Text);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// Returns the name of the player who owns this object, for messages and the log.
	FString GetPlayerName() const;

	UPROPERTY(Replicated, Meta = (NoAutoJson))
	bool mForceNetField_UBMRemoteCallObject = false;
};
