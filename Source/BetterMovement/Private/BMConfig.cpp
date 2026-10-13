#include "BMConfig.h"
#include "BetterMovement.h"
#include "Configuration/ConfigManager.h"
#include "Configuration/Properties/ConfigPropertyFloat.h"
#include "Configuration/Properties/ConfigPropertySection.h"
#include "Configuration/Properties/WidgetExtension/CP_Float.h"
#include "Configuration/Properties/WidgetExtension/CP_Section.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "BetterMovement"

const FString UBMConfig::UphillAngleKey = TEXT("UphillAngle");
const FString UBMConfig::SpeedLossKey = TEXT("SpeedLoss");
const FString UBMConfig::SpeedLimitsKey = TEXT("SpeedLimits");
const FString UBMConfig::MaxSlideSpeedKey = TEXT("MaxSlideSpeed");
const FString UBMConfig::MaxAirSpeedKey = TEXT("MaxAirSpeed");
const FString UBMConfig::MaxParachuteSpeedKey = TEXT("MaxParachuteSpeed");
const FString UBMConfig::WaterSkimSpeedKey = TEXT("WaterSkimSpeed");
const FString UBMConfig::ParachuteAccelerationKey = TEXT("ParachuteAcceleration");

namespace
{
	FConfigId MakeConfigId()
	{
		FConfigId Id;
		Id.ModReference = TEXT("BetterMovement");
		return Id;
	}

	struct FTierDefaults
	{
		float UphillAngle;
		// Shown as the section header, followed by Hint in brackets.
		FText Name;
		FText Hint;
		FText Tooltip;
	};

	// Default uphill angle and Mods menu text of each tier, indexed by EBMTier.
	TArray<FTierDefaults> GetTierDefaults()
	{
		return {
			{ 7.4f, LOCTEXT("TierNone", "No Blade Runners"), LOCTEXT("TierNoneHint", "vanilla limit 7.4°"), LOCTEXT("TierNoneTip", "When you aren't wearing Blade Runners.") },
			{ 15.f, LOCTEXT("TierMk1", "Blade Runners"), LOCTEXT("TierMk1Hint", "also other mods' Blade Runners"), LOCTEXT("TierMk1Tip", "Vanilla Blade Runners. Blade Runners added by other mods (not Mk+ Blade Runners) use these settings too.") },
			{ 20.f, LOCTEXT("TierMk2", "Mk.2 Blade Runners"), LOCTEXT("TierMkPlusHint", "only with Mk+ Blade Runners"), LOCTEXT("TierMk2Tip", "Only used if you have the optional Mk+ Blade Runners mod. Covers every Mk.2 variant.") },
			{ 27.f, LOCTEXT("TierMk3", "Mk.3 Blade Runners"), LOCTEXT("TierMkPlusHint", "only with Mk+ Blade Runners"), LOCTEXT("TierMk3Tip", "Only used if you have the optional Mk+ Blade Runners mod. Covers every Mk.3 variant.") },
			{ 35.f, LOCTEXT("TierMk4", "Mk.4 Blade Runners"), LOCTEXT("TierMkPlusHint", "only with Mk+ Blade Runners"), LOCTEXT("TierMk4Tip", "Only used if you have the optional Mk+ Blade Runners mod.") },
			{ 40.f, LOCTEXT("TierMk5", "Mk.5 Blade Runners"), LOCTEXT("TierMkPlusHint", "only with Mk+ Blade Runners"), LOCTEXT("TierMk5Tip", "Only used if you have the optional Mk+ Blade Runners mod.") },
		};
	}

	// Returns the header name of the section stored under SectionKey, or empty text.
	FText GetSectionName(const FString& SectionKey)
	{
		if (SectionKey == UBMConfig::SpeedLimitsKey)
		{
			return LOCTEXT("SpeedLimits", "Speed limits");
		}
		const TArray<FTierDefaults> Defaults = GetTierDefaults();
		for (int32 Index = 0; Index < BMTierCount; ++Index)
		{
			if (UBMConfig::GetTierSectionKey(static_cast<EBMTier>(Index)) == SectionKey)
			{
				return Defaults[Index].Name;
			}
		}
		return FText::GetEmpty();
	}

	// Returns the registered live configuration's root section, or null if it isn't registered yet.
	const UConfigPropertySection* FindRoot(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UConfigManager* Manager = GameInstance ? GameInstance->GetSubsystem<UConfigManager>() : nullptr;
		return Manager ? Manager->GetConfigurationRootSection(MakeConfigId()) : nullptr;
	}

	// Returns the float setting Key inside Section, or null.
	const UConfigPropertyFloat* FindFloat(const UConfigPropertySection* Section, const FString& Key)
	{
		const TObjectPtr<UConfigProperty>* Found = Section ? Section->SectionProperties.Find(Key) : nullptr;
		return Found ? Cast<UConfigPropertyFloat>(Found->Get()) : nullptr;
	}
}

FString UBMConfig::GetTierSectionKey(EBMTier Tier)
{
	switch (Tier)
	{
	case EBMTier::None: return TEXT("NoBladeRunners");
	case EBMTier::Mk1: return TEXT("Mk1");
	case EBMTier::Mk2: return TEXT("Mk2");
	case EBMTier::Mk3: return TEXT("Mk3");
	case EBMTier::Mk4: return TEXT("Mk4");
	case EBMTier::Mk5: return TEXT("Mk5");
	default: return TEXT("Unknown");
	}
}

UBMConfig::UBMConfig()
{
	ConfigId = MakeConfigId();
	DisplayName = LOCTEXT("ConfigName", "Better Movement");
	Description = LOCTEXT("ConfigDescription", "Speed limits for sliding, the air and the parachute, water skimming, and how you slide with each Blade Runners tier.");

	RootSection = CreateDefaultSubobject<UConfigPropertySection>(TEXT("RootSection"));

	UConfigPropertySection* Limits = CreateDefaultSubobject<UConfigPropertySection>(TEXT("SpeedLimitsSection"));
	Limits->DisplayName = LOCTEXT("SpeedLimitsHint", "everyone, in m/s");
	Limits->Tooltip = LOCTEXT("SpeedLimitsTip", "Top speeds for every player, whatever they're wearing. In multiplayer, the host's settings apply to everyone.");
	auto AddLimit = [this, Limits](const FString& Key, const TCHAR* ObjectName, const FText& Name, const FText& Tooltip, float Default)
	{
		UConfigPropertyFloat* Limit = CreateDefaultSubobject<UConfigPropertyFloat>(ObjectName);
		Limit->DisplayName = Name;
		Limit->Tooltip = Tooltip;
		Limit->DefaultValue = Default;
		Limit->Value = Default;
		Limits->SectionProperties.Add(Key, Limit);
	};
	const FBMSpeedLimits DefaultLimits;
	AddLimit(MaxSlideSpeedKey, TEXT("MaxSlideSpeed"), LOCTEXT("MaxSlideSpeed", "Max slide speed"),
		LOCTEXT("MaxSlideSpeedTip", "Fastest a slide can go, in m/s (1 to 300), including speed kept from bhopping and speed gained from a negative Speed lost while sliding. A normal slide starts at about 10."), DefaultLimits.MaxSlideSpeed);
	AddLimit(MaxAirSpeedKey, TEXT("MaxAirSpeed"), LOCTEXT("MaxAirSpeed", "Max air speed"),
		LOCTEXT("MaxAirSpeedTip", "Fastest speed you keep in the air after jumping out of a slide, in m/s (1 to 300). Also the limit on jumps out of a slide, which vanilla sets to 33."), DefaultLimits.MaxAirSpeed);
	AddLimit(MaxParachuteSpeedKey, TEXT("MaxParachuteSpeed"), LOCTEXT("MaxParachuteSpeed", "Max parachute speed"),
		LOCTEXT("MaxParachuteSpeedTip", "Fastest speed you keep when you open the parachute while moving fast, in m/s (1 to 300). The parachute's own speed (10, or 15 holding Shift) is unchanged; set this below 15 to slow it down."), DefaultLimits.MaxParachuteSpeed);
	AddLimit(WaterSkimSpeedKey, TEXT("WaterSkimSpeed"), LOCTEXT("WaterSkimSpeed", "Water skim speed"),
		LOCTEXT("WaterSkimSpeedTip", "Moving at least this fast, in m/s (0 to 300), you slide or run across water instead of going in; slow down and you drop in. 0 turns it off. Vanilla has no water skimming."), DefaultLimits.WaterSkimSpeed);
	AddLimit(ParachuteAccelerationKey, TEXT("ParachuteAcceleration"), LOCTEXT("ParachuteAcceleration", "Parachute acceleration"),
		LOCTEXT("ParachuteAccelerationTip", "How strongly the parachute turns falling into forward speed while you hold a direction and Shift, like a glider (0 to 2). Without Shift you keep your speed. It's the share of gravity along your glide path you gain: 1 (the default) is all of it, 0.5 half, 0 turns it off (vanilla). You speed up fastest from slow, and less as you get faster, up to the max parachute speed. Let go of the movement keys to slow down."), DefaultLimits.ParachuteAcceleration);
	RootSection->SectionProperties.Add(SpeedLimitsKey, Limits);

	const TArray<FTierDefaults> Defaults = GetTierDefaults();
	for (int32 Index = 0; Index < BMTierCount; ++Index)
	{
		const FString SectionKey = GetTierSectionKey(static_cast<EBMTier>(Index));
		const FTierDefaults& Tier = Defaults[Index];

		UConfigPropertySection* Section = CreateDefaultSubobject<UConfigPropertySection>(*(SectionKey + TEXT("Section")));
		Section->DisplayName = Tier.Hint;
		Section->Tooltip = Tier.Tooltip;

		UConfigPropertyFloat* UphillAngle = CreateDefaultSubobject<UConfigPropertyFloat>(*(SectionKey + TEXT("UphillAngle")));
		UphillAngle->DisplayName = LOCTEXT("UphillAngle", "Max uphill angle (degrees)");
		UphillAngle->Tooltip = LOCTEXT("UphillAngleTip", "The steepest slope you can keep sliding up, in degrees above flat (0 to 89). Vanilla is about 7.4. Ramps on an 8 m foundation: 1 m is 7.1, 2 m is 14.0, 4 m is 26.6. In multiplayer, the host's settings apply to everyone.");
		UphillAngle->DefaultValue = Tier.UphillAngle;
		UphillAngle->Value = Tier.UphillAngle;
		Section->SectionProperties.Add(UphillAngleKey, UphillAngle);

		UConfigPropertyFloat* SpeedLoss = CreateDefaultSubobject<UConfigPropertyFloat>(*(SectionKey + TEXT("SpeedLoss")));
		SpeedLoss->DisplayName = LOCTEXT("SpeedLoss", "Speed lost while sliding (%)");
		SpeedLoss->Tooltip = LOCTEXT("SpeedLossTip", "How quickly a slide loses speed, on flat ground and uphill, in percent of normal. A slide starts from the speed you go into it with, for example from bhopping. 100 is vanilla, 50 keeps your speed twice as long, 0 never slows down. Below 0 the slide speeds up instead: -50 adds half your starting slide speed every second, up to the max slide speed. Steep downhill slopes still speed you up as usual. In multiplayer, the host's settings apply to everyone.");
		SpeedLoss->DefaultValue = 100.f;
		SpeedLoss->Value = 100.f;
		Section->SectionProperties.Add(SpeedLossKey, SpeedLoss);


		RootSection->SectionProperties.Add(SectionKey, Section);
	}
}

namespace
{
	const TCHAR* SMLPropertyPath = TEXT("/SML/Interface/UI/Menu/Mods/ConfigProperties/");

	// Loads SML's Blueprint subclass of a config property class, e.g. BP_ConfigPropertyFloat for UConfigPropertyFloat.
	UClass* LoadEditorClass(const UClass* NativeClass)
	{
		const FString Name = TEXT("BP_") + NativeClass->GetName();
		const FString Path = FString::Printf(TEXT("%s%s.%s_C"), SMLPropertyPath, *Name, *Name);
		UClass* Loaded = LoadClass<UConfigProperty>(nullptr, *Path);
		if (!Loaded || !Loaded->IsChildOf(NativeClass))
		{
			UE_LOG(LogBetterMovement, Warning, TEXT("Config: SML editor class %s not found; that setting won't be editable in the Mods menu"), *Path);
			return nullptr;
		}
		return Loaded;
	}

	// Creates an instance of EditorClass under Outer with every property value of Source.
	UConfigProperty* CloneAs(const UConfigProperty* Source, UClass* EditorClass, UObject* Outer)
	{
		UConfigProperty* Clone = NewObject<UConfigProperty>(Outer, EditorClass, NAME_None, RF_Public);
		for (TFieldIterator<FProperty> It(Source->GetClass()); It; ++It)
		{
			It->CopyCompleteValue_InContainer(Clone, Source);
		}
		return Clone;
	}

	// Sets the accepted range of Property, a number setting stored under Key, replacing SML's 0 to 1 default.
	void ApplyNumberRange(const FString& Key, UConfigProperty* Property)
	{
		UCP_Float* Float = Cast<UCP_Float>(Property);
		if (!Float)
		{
			return;
		}
		if (Key == UBMConfig::UphillAngleKey)
		{
			Float->MinValue = UBMConfig::MinUphillAngle;
			Float->MaxValue = UBMConfig::MaxUphillAngle;
		}
		else if (Key == UBMConfig::SpeedLossKey)
		{
			Float->MinValue = UBMConfig::MinSpeedLoss;
			Float->MaxValue = UBMConfig::MaxSpeedLoss;
		}
		else if (Key == UBMConfig::MaxSlideSpeedKey || Key == UBMConfig::MaxAirSpeedKey || Key == UBMConfig::MaxParachuteSpeedKey)
		{
			Float->MinValue = UBMConfig::MinSpeedLimit;
			Float->MaxValue = UBMConfig::MaxSpeedLimit;
		}
		else if (Key == UBMConfig::WaterSkimSpeedKey)
		{
			Float->MinValue = UBMConfig::MinWaterSkimSpeed;
			Float->MaxValue = UBMConfig::MaxSpeedLimit;
		}
		else if (Key == UBMConfig::ParachuteAccelerationKey)
		{
			Float->MinValue = UBMConfig::MinParachuteAcceleration;
			Float->MaxValue = UBMConfig::MaxParachuteAcceleration;
		}
		else
		{
			UE_LOG(LogBetterMovement, Warning, TEXT("Config: no range for number setting '%s'; the Mods menu only accepts 0 to 1"), *Key);
		}
	}

	// Converts OldSection and everything under it to SML's Blueprint editor classes, laid out as a vertical list.
	// Nested sections get a collapsible header showing their name and display name.
	UConfigPropertySection* ConvertSectionRecursive(const UConfigPropertySection* OldSection, const FString& SectionKey, UClass* SectionEditorClass, UObject* Outer, bool bIsRoot, int32& OutConverted, int32& OutTotal)
	{
		UConfigPropertySection* NewSection = Cast<UConfigPropertySection>(CloneAs(OldSection, SectionEditorClass, Outer));
		NewSection->SectionProperties.Reset();
		if (UCP_Section* EditorSection = Cast<UCP_Section>(NewSection))
		{
			EditorSection->WidgetType = ECP_SectionWidgetType::CPS_Vertical;
			// SML shows the header as "HeaderText (DisplayName)" and shows nothing when HeaderText is empty.
			EditorSection->HasHeader = !bIsRoot;
			EditorSection->HeaderText = bIsRoot ? FText::GetEmpty() : GetSectionName(SectionKey);
		}
		for (const TPair<FString, TObjectPtr<UConfigProperty>>& Pair : OldSection->SectionProperties)
		{
			UConfigProperty* Old = Pair.Value;
			if (!Old)
			{
				continue;
			}
			++OutTotal;
			if (const UConfigPropertySection* OldChildSection = Cast<UConfigPropertySection>(Old))
			{
				NewSection->SectionProperties.Add(Pair.Key, ConvertSectionRecursive(OldChildSection, Pair.Key, SectionEditorClass, NewSection, false, OutConverted, OutTotal));
				++OutConverted;
				continue;
			}
			if (UClass* EditorClass = LoadEditorClass(Old->GetClass()))
			{
				UConfigProperty* Clone = CloneAs(Old, EditorClass, NewSection);
				ApplyNumberRange(Pair.Key, Clone);
				NewSection->SectionProperties.Add(Pair.Key, Clone);
				++OutConverted;
				continue;
			}
			NewSection->SectionProperties.Add(Pair.Key, Old);
		}
		return NewSection;
	}
}

void UBMConfig::UseSMLEditorClasses()
{
	UBMConfig* Defaults = GetMutableDefault<UBMConfig>();
	UConfigPropertySection* OldRoot = Defaults->RootSection;
	UClass* SectionClass = OldRoot ? LoadEditorClass(UConfigPropertySection::StaticClass()) : nullptr;
	if (!SectionClass || OldRoot->GetClass() == SectionClass)
	{
		return;
	}

	int32 Converted = 0, Total = 0;
	Defaults->RootSection = ConvertSectionRecursive(OldRoot, FString(), SectionClass, Defaults, true, Converted, Total);
	UE_LOG(LogBetterMovement, Log, TEXT("Config: %d of %d settings/sections use SML's editor widgets"), Converted, Total);
}

bool UBMConfig::ReadTierSettings(const UObject* WorldContext, TArray<FBMTierSettings>& OutSettings)
{
	OutSettings.Reset();
	const UConfigPropertySection* Root = FindRoot(WorldContext);
	if (!Root)
	{
		UE_LOG(LogBetterMovement, Verbose, TEXT("Config: not registered yet"));
		return false;
	}

	const TArray<FTierDefaults> Defaults = GetTierDefaults();
	OutSettings.SetNum(BMTierCount);
	for (int32 Index = 0; Index < BMTierCount; ++Index)
	{
		const EBMTier Tier = static_cast<EBMTier>(Index);
		const TObjectPtr<UConfigProperty>* SectionProperty = Root->SectionProperties.Find(GetTierSectionKey(Tier));
		const UConfigPropertySection* Section = SectionProperty ? Cast<UConfigPropertySection>(SectionProperty->Get()) : nullptr;
		const UConfigPropertyFloat* UphillAngle = FindFloat(Section, UphillAngleKey);
		const UConfigPropertyFloat* SpeedLoss = FindFloat(Section, SpeedLossKey);
		if (!UphillAngle || !SpeedLoss)
		{
			UE_LOG(LogBetterMovement, Warning, TEXT("Config: settings for %s missing (section %d, angle %d, speed loss %d); using defaults"),
				BMTierName(Tier), Section != nullptr, UphillAngle != nullptr, SpeedLoss != nullptr);
		}

		FBMTierSettings& Settings = OutSettings[Index];
		Settings.UphillAngleDegrees = FMath::Clamp(UphillAngle ? UphillAngle->Value : Defaults[Index].UphillAngle, MinUphillAngle, MaxUphillAngle);
		Settings.SpeedLossPercent = FMath::Clamp(SpeedLoss ? SpeedLoss->Value : 100.f, MinSpeedLoss, MaxSpeedLoss);
	}
	return true;
}

bool UBMConfig::ReadSpeedLimits(const UObject* WorldContext, FBMSpeedLimits& OutLimits)
{
	OutLimits = FBMSpeedLimits();
	const UConfigPropertySection* Root = FindRoot(WorldContext);
	if (!Root)
	{
		return false;
	}
	const TObjectPtr<UConfigProperty>* SectionProperty = Root->SectionProperties.Find(SpeedLimitsKey);
	const UConfigPropertySection* Section = SectionProperty ? Cast<UConfigPropertySection>(SectionProperty->Get()) : nullptr;
	auto ReadLimit = [Section](const FString& Key, float& InOutValue, float MinValue = MinSpeedLimit, float MaxValue = MaxSpeedLimit)
	{
		if (const UConfigPropertyFloat* Limit = FindFloat(Section, Key))
		{
			InOutValue = FMath::Clamp(Limit->Value, MinValue, MaxValue);
		}
		else
		{
			UE_LOG(LogBetterMovement, Warning, TEXT("Config: speed limit '%s' missing (section %d); using %.0f m/s"), *Key, Section != nullptr, InOutValue);
		}
	};
	ReadLimit(MaxSlideSpeedKey, OutLimits.MaxSlideSpeed);
	ReadLimit(MaxAirSpeedKey, OutLimits.MaxAirSpeed);
	ReadLimit(MaxParachuteSpeedKey, OutLimits.MaxParachuteSpeed);
	ReadLimit(WaterSkimSpeedKey, OutLimits.WaterSkimSpeed, MinWaterSkimSpeed);
	ReadLimit(ParachuteAccelerationKey, OutLimits.ParachuteAcceleration, MinParachuteAcceleration, MaxParachuteAcceleration);
	return true;
}

#undef LOCTEXT_NAMESPACE
