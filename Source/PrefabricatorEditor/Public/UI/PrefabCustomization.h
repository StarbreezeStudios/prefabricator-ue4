//$ Copyright 2015-20, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "SharedPointer.h"			 // SBZ stephane.maruejouls - PD3-480 - Collection dropdown
#include "Widgets/Input/SComboBox.h" // SBZ stephane.maruejouls - PD3-480 - Collection dropdown

class UPrefabricatorAsset;			// SBZ stephane.maruejouls - PD3-480 - Collection dropdown
class APrefabActor;					// SBZ stephane.maruejouls - PD3-480 - Collection dropdown

class PREFABRICATOREDITOR_API FPrefabActorCustomization : public IDetailCustomization {
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

	static TSharedRef<IDetailCustomization> MakeInstance();

	static FReply HandleSaveToAsset(IDetailLayoutBuilder* DetailBuilder);
	static FReply HandleSaveToNewAsset(IDetailLayoutBuilder* DetailBuilder);
	static FReply HandleLoadFromAsset(IDetailLayoutBuilder* DetailBuilder);
	FReply RandomizePrefabCollection(IDetailLayoutBuilder* DetailBuilder);					// SBZ stephane.maruejouls - PD3-480 - Collection dropdown
	static FReply UnlinkPrefab(IDetailLayoutBuilder* DetailBuilder, bool bRecursive);
	static FReply UpdateThumbFromViewport(IDetailLayoutBuilder* DetailBuilder);

	// SBZ stephane.maruejouls - Instancing
	static FReply MakeInstances(IDetailLayoutBuilder* DetailBuilder, bool bHierarchical);
	static FReply MakeStaticMeshes(IDetailLayoutBuilder* DetailBuilder);
	// SBZ
	// SBZ stephane.maruejouls - Merge Meshes
	bool ContainsOnlyStatics(IDetailLayoutBuilder* DetailBuilder, bool bOnlyStatics) const;
	bool ContainsInstances(IDetailLayoutBuilder* DetailBuilder) const;
	static FReply MergeMeshes(IDetailLayoutBuilder* DetailBuilder);
	// SBZ
	// SBZ stephane.maruejouls - Update merged from prefab
	bool ContainsMerged(IDetailLayoutBuilder* DetailBuilder) const;
	static FReply UpdateMergedPrefabs(IDetailLayoutBuilder* DetailBuilder);
	// SBZ
	
	// SBZ stephane.maruejouls - PD3-480 - Collection dropdown
private:
	void UpdateSelectedAsset(TArray<APrefabActor*> PrefabActors);
	UPrefabricatorAsset* GetSelectedAssetPrefab() const { return SelectedAssetPrefab; }
	UPrefabricatorAsset* SelectedAssetPrefab = nullptr;
	FText GetSelectedPrefabName() const;
	TSharedPtr< FAssetThumbnail > CurrentPrefabThumbnail;
	TSharedPtr<SComboBox<UObject*>> CollectionComboBox;
	// SBZ
};

class PREFABRICATOREDITOR_API FPrefabRandomizerCustomization : public IDetailCustomization {
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

	static TSharedRef<IDetailCustomization> MakeInstance();

	static FReply HandleRandomize(IDetailLayoutBuilder* DetailBuilder);
};

class PREFABRICATOREDITOR_API FPrefabricatorAssetCustomization : public IDetailCustomization {
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

	static TSharedRef<IDetailCustomization> MakeInstance();
};

class PREFABRICATOREDITOR_API FPrefabDebugCustomization : public IDetailCustomization {
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

	static TSharedRef<IDetailCustomization> MakeInstance();

	static FReply SaveDebugData(IDetailLayoutBuilder* DetailBuilder);
	static FReply LoadDebugData(IDetailLayoutBuilder* DetailBuilder);
};

