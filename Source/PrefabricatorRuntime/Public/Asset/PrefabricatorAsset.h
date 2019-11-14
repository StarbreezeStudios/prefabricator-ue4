//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PrefabricatorAsset.generated.h"

USTRUCT(Blueprintable) // SBZ stephane.maruejouls - allow edition
struct PREFABRICATORRUNTIME_API FPrefabricatorPropertyAssetMapping {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FSoftObjectPath AssetReference;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FString AssetClassName;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FName AssetObjectPath;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	bool bUseQuotes = false;
};

UCLASS(Blueprintable) // SBZ stephane.maruejouls - allow edition
class PREFABRICATORRUNTIME_API UPrefabricatorProperty : public UObject {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FString PropertyName;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FString ExportedValue;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	TArray<FPrefabricatorPropertyAssetMapping> AssetSoftReferenceMappings;

	void SaveReferencedAssetValues();
	void LoadReferencedAssetValues();
};

USTRUCT(Blueprintable) // SBZ stephane.maruejouls - allow edition
struct PREFABRICATORRUNTIME_API FPrefabricatorComponentData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FTransform RelativeTransform;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FString ComponentName;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	TArray<UPrefabricatorProperty*> Properties;
};

USTRUCT(Blueprintable) // SBZ stephane.maruejouls - allow edition
struct PREFABRICATORRUNTIME_API FPrefabricatorActorData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FGuid PrefabItemID;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FTransform RelativeTransform;

	// SBZ stephane.maruejouls - allow Random placement
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	bool bRandomizeTransform = false;

	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomizeTransform"))
	FVector OffsetVariation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomizeTransform"))
	FRotator OffsetRotation = FRotator::ZeroRotator;
	// SBZ

	// SBZ stephane.maruejouls - allow Snapping when random
	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomizeTransform"))
	bool bRandomGridSnapping = false;

	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomGridSnapping"))
	float GridSnap = 10.f;

	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomizeTransform"))
	bool bRandomAngleSnapping = false;

	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomAngleSnapping"))
	float AngleSnap = 45.f;
	// SBZ

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FString ClassPath;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FSoftClassPath ClassPathRef; 

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	TArray<UPrefabricatorProperty*> Properties;

	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	TArray<FPrefabricatorComponentData> Components;

	// SBZ stephane.maruejouls - allow None
	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	float Weight = 1.f;
	// SBZ

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	FString ActorName;
#endif // WITH_EDITORONLY_DATA
};

struct FPrefabAssetSelectionConfig {
	int32 Seed = 0;
};

UCLASS(Blueprintable) // SBZ stephane.maruejouls - allow edition
class PREFABRICATORRUNTIME_API UPrefabricatorEventListener : public UObject {
	GENERATED_BODY()
public:
	/** Called when the prefab and all its child prefabs have been spawned and initialized */
	UFUNCTION(BlueprintNativeEvent, Category = "Prefabricator")
	void PostSpawn(APrefabActor* Prefab);
	virtual void PostSpawn_Implementation(APrefabActor* Prefab);
};

UCLASS(Blueprintable)
class PREFABRICATORRUNTIME_API UPrefabricatorAssetInterface : public UObject {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	TSubclassOf<UPrefabricatorEventListener> EventListener;

	UPROPERTY(EditAnywhere, Category = "Replication")
	bool bReplicates = false;

public:
	virtual class UPrefabricatorAsset* GetPrefabAsset(const FPrefabAssetSelectionConfig& InConfig) { return nullptr; }
};

enum class EPrefabricatorAssetVersion {
	InitialVersion = 0,
	AddedSoftReference,

	//----------- Versions should be placed above this line -----------------
	LastVersionPlusOne,
	LatestVersion = LastVersionPlusOne -1
};

UCLASS(Blueprintable)
class PREFABRICATORRUNTIME_API UPrefabricatorAsset : public UPrefabricatorAssetInterface {
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Prefabricator") // SBZ stephane.maruejouls - allow edition
	TArray<FPrefabricatorActorData> ActorData;

	UPROPERTY()
	TEnumAsByte<EComponentMobility::Type> PrefabMobility;

	// The ID that is regenerated on every update
	// This allows prefab actors to test against their own LastUpdateID and determine if a refresh is needed
	UPROPERTY()
	FGuid LastUpdateID;


	/** Information for thumbnail rendering */
	UPROPERTY()
	class UThumbnailInfo* ThumbnailInfo;

	UPROPERTY()
	uint32 Version;

public:
	virtual UPrefabricatorAsset* GetPrefabAsset(const FPrefabAssetSelectionConfig& InConfig) override;
};


USTRUCT(BlueprintType)
struct PREFABRICATORRUNTIME_API FPrefabricatorAssetCollectionItem {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	TSoftObjectPtr<UPrefabricatorAsset> PrefabAsset;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	float Weight = 1.0f;
};

enum class EPrefabricatorCollectionAssetVersion {
	InitialVersion = 0,

	//----------- Versions should be placed above this line -----------------
	LastVersionPlusOne,
	LatestVersion = LastVersionPlusOne - 1
};

UCLASS(Blueprintable)
class PREFABRICATORRUNTIME_API UPrefabricatorAssetCollection : public UPrefabricatorAssetInterface {
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	TArray<FPrefabricatorAssetCollectionItem> Prefabs;

	UPROPERTY()
	uint32 Version;

public:
	virtual UPrefabricatorAsset* GetPrefabAsset(const FPrefabAssetSelectionConfig& InConfig) override;
};


class PREFABRICATORRUNTIME_API FPrefabricatorAssetUtils {
public:
	static FVector FindPivot(const TArray<AActor*>& InActors);
	static EComponentMobility::Type FindMobility(const TArray<AActor*>& InActors);
	
};

