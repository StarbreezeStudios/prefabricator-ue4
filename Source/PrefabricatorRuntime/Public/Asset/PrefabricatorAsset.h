//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PrefabricatorAsset.generated.h"

USTRUCT(Blueprintable)
struct PREFABRICATORRUNTIME_API FPrefabricatorPropertyAssetMapping {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FSoftObjectPath AssetReference;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FString AssetClassName;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FName AssetObjectPath;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	bool bUseQuotes = false;
};

UCLASS(Blueprintable)
class PREFABRICATORRUNTIME_API UPrefabricatorProperty : public UObject {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FString PropertyName;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FString ExportedValue;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	TArray<FPrefabricatorPropertyAssetMapping> AssetSoftReferenceMappings;

	void SaveReferencedAssetValues();
	void LoadReferencedAssetValues();
};

USTRUCT(Blueprintable)
struct PREFABRICATORRUNTIME_API FPrefabricatorComponentData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FTransform RelativeTransform;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FString ComponentName;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	TArray<UPrefabricatorProperty*> Properties;
};

USTRUCT(Blueprintable)
struct PREFABRICATORRUNTIME_API FPrefabricatorActorData {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FGuid PrefabItemID;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FTransform RelativeTransform;

	// SBZ stephane.maruejouls - allow Random placement
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	bool bRandomizeTransform = false;

	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomizeTransform"))
	FVector OffsetVariation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Prefabricator", meta = (EditCondition = "bRandomizeTransform"))
	FRotator OffsetRotation = FRotator::ZeroRotator;
	// SBZ

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FString ClassPath;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FSoftClassPath ClassPathRef;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	TArray<UPrefabricatorProperty*> Properties;

	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	TArray<FPrefabricatorComponentData> Components;

	// SBZ stephane.maruejouls - allow None
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	float Weight = 1.f;
	// SBZ

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
	FString ActorName;
#endif // WITH_EDITORONLY_DATA
};

struct FPrefabAssetSelectionConfig {
	int32 Seed = 0;
};

UCLASS(Blueprintable)
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
	UPROPERTY(EditAnywhere, Category = "Prefabricator")
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

