//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"

class PREFABRICATOREDITOR_API FPrefabActorCustomization : public IDetailCustomization {
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

	static TSharedRef<IDetailCustomization> MakeInstance();

	static FReply HandleSaveToAsset(IDetailLayoutBuilder* DetailBuilder);
	static FReply HandleSaveToNewAsset(IDetailLayoutBuilder* DetailBuilder);
	static FReply HandleLoadFromAsset(IDetailLayoutBuilder* DetailBuilder);
	static FReply RandomizePrefabCollection(IDetailLayoutBuilder* DetailBuilder);
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
};

class PREFABRICATOREDITOR_API FPrefabRandomizerCustomization : public IDetailCustomization {
public:
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface

	static TSharedRef<IDetailCustomization> MakeInstance();

	static FReply HandleRandomize(IDetailLayoutBuilder* DetailBuilder);
};

