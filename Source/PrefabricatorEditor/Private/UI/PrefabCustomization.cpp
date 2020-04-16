//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "UI/PrefabCustomization.h"

#include "Asset/PrefabricatorAsset.h"
#include "Asset/PrefabricatorAssetUserData.h"  // SBZ stephane.maruejouls - Instancing
#include "Instancing/PrefabInstancingActor.h"  // SBZ stephane.maruejouls - Instancing
#include "Prefab/PrefabActor.h"
#include "Prefab/PrefabComponent.h"
#include "Prefab/PrefabTools.h"
#include "Prefab/Random/PrefabRandomizerActor.h"
#include "PrefabricatorEditorModule.h"
#include "PrefabricatorSettings.h"
#include "Utils/PrefabEditorTools.h"
#include "Utils/PrefabricatorService.h" // SBZ stephane.maruejouls - undo revamp

#include "ContentBrowserModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorViewportClient.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

// SBZ stephane.maruejouls - Instancing
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
// SBZ

// SBZ stephane.maruejouls - Merge Meshes
#include "AssetRegistryModule.h"
#include "Components/ShapeComponent.h"
#include "Dialogs/Dialogs.h"
#include "MeshMergeModule.h"
#include "Misc/ScopedSlowTask.h"
// SBZ

// SBZ stephane.maruejouls - save to disk
#include "FileHelpers.h"
// SBZ

#define LOCTEXT_NAMESPACE "PrefabActorCustomization" 

namespace {
	template<typename T>
	TArray<T*> GetDetailObject(IDetailLayoutBuilder* DetailBuilder) {
		TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
		DetailBuilder->GetObjectsBeingCustomized(CustomizedObjects);
		TArray<T*> Result;
		for (TWeakObjectPtr<UObject> CustomizedObject : CustomizedObjects) {
			T* Obj = Cast<T>(CustomizedObject.Get());
			if (Obj) {
				Result.Add(Obj);
			}
		}
		return Result;
	}
}

///////////////////////////////// FPrefabActorCustomization /////////////////////////////////

void FPrefabActorCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(&DetailBuilder);
	TArray<UObject*> PrefabComponents;
	bool bIsCollection = false;
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor->PrefabComponent) {
			PrefabComponents.Add(PrefabActor->PrefabComponent);

			UPrefabricatorAssetInterface* Asset = PrefabActor->PrefabComponent->PrefabAssetInterface.LoadSynchronous();
			if (Asset && Asset->IsA<UPrefabricatorAssetCollection>()) {
				bIsCollection = true;
				break;
			}
		}

	}
	FPrefabDetailsExtend& ExtenderDelegate = IPrefabricatorEditorModule::Get().GetPrefabActorDetailsExtender();

	
	if(ExtenderDelegate.IsBound())
	{
		ExtenderDelegate.Execute(DetailBuilder);
	}
	

	if (!bIsCollection) {
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Prefab Asset Actions", FText::GetEmpty(), ECategoryPriority::Important);
		Category.AddExternalObjectProperty(PrefabComponents, GET_MEMBER_NAME_CHECKED(UPrefabComponent, PrefabAssetInterface));

		Category.AddCustomRow(LOCTEXT("PrefabCommand_Filter", "save load prefab asset"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_SaveToAsset", "Save Prefab to Asset"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::HandleSaveToAsset, &DetailBuilder))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_SaveToNewAsset", "Save Prefab to New Asset"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::HandleSaveToNewAsset, &DetailBuilder))
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_LoadFromAsset", "Load Prefab from Asset"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::HandleLoadFromAsset, &DetailBuilder))
			]

		];

		Category.AddCustomRow(LOCTEXT("PrefabCommandRandomize_Filter", "randomize prefab collection asset"))
			.WholeRowContent()
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_RandomizeCollection", "Randomize"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::RandomizePrefabCollection, &DetailBuilder))
			];


		Category.AddCustomRow(LOCTEXT("PrefabCommandUnlink_Filter", "unlink prefab"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_Unlink", "Unlink"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::UnlinkPrefab, &DetailBuilder, false))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_Unlink_Recurse", "Unlink Recursive"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::UnlinkPrefab, &DetailBuilder, true))
			]
		];

		// SBZ stephane.maruejouls - Instancing
		Category.AddCustomRow(LOCTEXT("PrefabCommandInstance_Filter", "instancing"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_Instancing", "Instanced"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::MakeInstances, &DetailBuilder, false))
				.ToolTipText(LOCTEXT("PrefabCommand_InstancingTP", "Will create InstancedStaticMesh from children\nThis will only work if there is only StaticMeshActors"))
				.IsEnabled(this, &FPrefabActorCustomization::ContainsOnlyStatics, &DetailBuilder, false)
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_Hierachical", "Hierarchical"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::MakeInstances, &DetailBuilder, true))
				.ToolTipText(LOCTEXT("PrefabCommand_HierachicalTP", "Will create HierachicalInstancedStaticMesh from children\nThis will only work if there is only StaticMeshActors"))
				.IsEnabled(this, &FPrefabActorCustomization::ContainsOnlyStatics, &DetailBuilder, false)
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_StaticMesh", "StaticMesh"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::MakeStaticMeshes, &DetailBuilder))
				.ToolTipText(LOCTEXT("PrefabCommand_StaticMeshTP","Will create StaticMeshActor from any (Hierarchical)InstanciedStaticMesh children\nThis will only work if there are some InstancedActors"))
				.IsEnabled(this, &FPrefabActorCustomization::ContainsInstances, &DetailBuilder)
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_MergeMesh", "MergeMesh"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::MergeMeshes, &DetailBuilder))
				.ToolTipText(LOCTEXT("PrefabCommand_MergeMeshTP", "Will merge all children StaticMeshActor into one StaticMeshActor\nThis will only work if there is only Static StaticMeshActors"))
				.IsEnabled(this, &FPrefabActorCustomization::ContainsOnlyStatics, &DetailBuilder, true)
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_UpdateMergedMesh", "UpdateMerged"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::UpdateMergedPrefabs, &DetailBuilder))
				.ToolTipText(LOCTEXT("PrefabCommand_UpdateMergedMeshTP", "Will reload the prefab and remerge it"))
				.IsEnabled(this, &FPrefabActorCustomization::ContainsMerged, &DetailBuilder)
			]
		];
		// SBZ

		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(APrefabActor, Seed));
	}
	else {
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Prefab Collection Actions", FText::GetEmpty(), ECategoryPriority::Important);
		Category.AddExternalObjectProperty(PrefabComponents, GET_MEMBER_NAME_CHECKED(UPrefabComponent, PrefabAssetInterface));

		Category.AddProperty(GET_MEMBER_NAME_CHECKED(APrefabActor, Seed));
			
		Category.AddCustomRow(LOCTEXT("PrefabCollectionCommandRandomize_Filter", "randomize prefab collection asset"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("PrefabCollectionCommand_RandomizeCollection", "Randomize"))
			.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::RandomizePrefabCollection, &DetailBuilder))
		];

		Category.AddCustomRow(LOCTEXT("PrefabCollectionCommand_Filter", "load prefab collection asset"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("PrefabCollectionCommand_RecreateCollection", "Reload Prefab"))
			.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::HandleLoadFromAsset, &DetailBuilder))
		];

		Category.AddCustomRow(LOCTEXT("PrefabCommandUnlink_Filter", "unlink prefab"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_Unlink", "Unlink"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::UnlinkPrefab, &DetailBuilder, false))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_Unlink_Recurse", "Unlink Recursive"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::UnlinkPrefab, &DetailBuilder, true))
			]
		];
	}


	const UPrefabricatorSettings* PS = GetDefault<UPrefabricatorSettings>();
	if (!PS->bShowAssetThumbnails)
	{
		// Add an option to save the viewport image as a thumbnail for the asset
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Prefab Collection Actions", FText::GetEmpty(), ECategoryPriority::Important);
		Category.AddCustomRow(LOCTEXT("PrefabThumb_Filter", "thumbnail thumb"))
			.WholeRowContent()
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabThumbCommand_SaveThumbnail", "Update Thumbnail"))
			.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::UpdateThumbFromViewport, &DetailBuilder))
			];
	}
}

TSharedRef<IDetailCustomization> FPrefabActorCustomization::MakeInstance()
{
	return MakeShareable(new FPrefabActorCustomization);
}

FReply FPrefabActorCustomization::HandleSaveToAsset(IDetailLayoutBuilder* DetailBuilder)
{
	TArray<UPackage*> PackagesToSave; // SBZ stephane.maruejouls - save to disk
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor) {
			PrefabActor->SavePrefab();

			UPrefabricatorAsset* PrefabAsset = Cast<UPrefabricatorAsset>(PrefabActor->PrefabComponent->PrefabAssetInterface.LoadSynchronous());
			if (PrefabAsset) {
				PackagesToSave.Add(PrefabAsset->GetOutermost());	// SBZ stephane.maruejouls - save to disk
				const UPrefabricatorSettings* PS = GetDefault<UPrefabricatorSettings>();
				if(PS->bAllowDynamicUpdate)
				{
					// Refresh all the existing prefabs in the level
					FPrefabEditorTools::ReloadPrefabsInLevel(PrefabActor->GetWorld(), PrefabAsset);
				}
			}
		}
	}
	// SBZ stephane.maruejouls - save to disk
	if (PackagesToSave.Num() > 0)
	{
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirtyOnAssetSave*/true, /*bPromptToSave=*/ false);
	}
	// SBZ
	return FReply::Handled();
}

FReply FPrefabActorCustomization::HandleSaveToNewAsset(IDetailLayoutBuilder* DetailBuilder)
{
	TArray<UPackage*> PackagesToSave; // SBZ stephane.maruejouls - save to disk
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_SaveNewAsset", "Save to new asset")); 	// SBZ stephane.maruejouls - undo revamp
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor) {
			
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);

			if(Children.Num() > 0)
			{
				FPrefabTools::UnlinkAndDestroyPrefabActor(PrefabActor, "/", false);
				FPrefabTools::CreatePrefabFromActors(Children);
				// SBZ stephane.maruejouls - save to disk
				for (AActor* Child : Children)
				{
					if (Child->GetRootComponent())
					{
						UPrefabricatorAssetUserData* ChildUserData = Child->GetRootComponent()->GetAssetUserData<UPrefabricatorAssetUserData>();
						if (ChildUserData)
						{
							TWeakObjectPtr<APrefabActor> NewPrefabActor = ChildUserData->PrefabActor;
							if (NewPrefabActor.IsValid())
							{
								UPrefabricatorAsset* PrefabAsset = Cast<UPrefabricatorAsset>(NewPrefabActor->PrefabComponent->PrefabAssetInterface.LoadSynchronous());
								if (PrefabAsset)
								{
									PackagesToSave.Add(PrefabAsset->GetOutermost());
									break;
								}
							}
						}
					}					
				}
				// SBZ
			}
		}
	}
	// SBZ stephane.maruejouls - save to disk
	if (PackagesToSave.Num() > 0)
	{
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirtyOnAssetSave*/true, /*bPromptToSave=*/ false);
	}
	// SBZ
	return FReply::Handled();
}

FReply FPrefabActorCustomization::HandleLoadFromAsset(IDetailLayoutBuilder* DetailBuilder)
{
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_LoadFromAsset", "Load from asset")); 	// SBZ stephane.maruejouls - undo revamp
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor) {
			PrefabActor->LoadPrefab();
		}
	}
	return FReply::Handled();
}

FReply FPrefabActorCustomization::RandomizePrefabCollection(IDetailLayoutBuilder* DetailBuilder)
{
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_Randomize", "Randomize")); 	// SBZ stephane.maruejouls - undo revamp
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor) {
			FRandomStream Random;
			Random.Initialize(FMath::Rand());
			PrefabActor->RandomizeSeed(Random);

			FPrefabLoadSettings LoadSettings;
			LoadSettings.bRandomizeNestedSeed = true;
			LoadSettings.Random = &Random;
			FPrefabTools::LoadStateFromPrefabAsset(PrefabActor, LoadSettings);
		}
	}
	return FReply::Handled();
}

FReply FPrefabActorCustomization::UnlinkPrefab(IDetailLayoutBuilder* DetailBuilder, bool bRecursive)
{
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_Unlink", "Unlink")); 	// SBZ stephane.maruejouls - undo revamp
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor) {
			AActor* Parent = PrefabActor->GetAttachParentActor();
			if (Parent)
			{
				FPrefabTools::UnlinkAndDestroyPrefabActor(PrefabActor, "/", bRecursive);
			}
			else
			{
				FString FolderPath = PrefabActor->GetFolderPath().ToString() + "/" + PrefabActor->GetName();
				FPrefabTools::UnlinkAndDestroyPrefabActor(PrefabActor, FolderPath, bRecursive);
			}
		}
	}
	return FReply::Handled();
}

FReply FPrefabActorCustomization::UpdateThumbFromViewport(IDetailLayoutBuilder* DetailBuilder)
{
	if (GEditor) {
		TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
		for (APrefabActor* PrefabActor : PrefabActors) {
			if (PrefabActor) {
				UPrefabricatorAsset* Asset = PrefabActor->GetPrefabAsset();
				IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
				TArray<FAssetData> AssetList;
				AssetList.Add(FAssetData(Asset));
				ContentBrowser.CaptureThumbnailFromViewport(GEditor->GetActiveViewport(), AssetList);
			}
		}
	}
	return FReply::Handled();
}

// SBZ stephane.maruejouls - Instancing
static void ConvertToInstances(APrefabActor* PrefabActor, bool bHierarchical)
{
	TArray<AActor*> Children;
	PrefabActor->GetAttachedActors(Children);
	TMultiMap<UStaticMesh*, APrefabInstancingActor*> InstancedActorMap;

	if (Children.Num() > 0)
	{
		UWorld* World = PrefabActor->GetWorld();
		for (AActor* Actor : Children)
		{
			if (APrefabInstancingActor* Instance = Cast<APrefabInstancingActor>(Actor))
			{
				InstancedActorMap.Add(Instance->GetStaticMesh(), Instance);
			}
		}
		for (AActor* Actor : Children)
		{
			if (AStaticMeshActor* ChildSM = Cast<AStaticMeshActor>(Actor))
			{
				UStaticMesh* SM = ChildSM->GetStaticMeshComponent()->GetStaticMesh();
				TArray<UMaterialInterface*> Materials = ChildSM->GetStaticMeshComponent()->GetMaterials();
				if (::IsValid(SM))
				{
					TArray<APrefabInstancingActor*> FoundInstances;
					InstancedActorMap.MultiFind(SM, FoundInstances);
					APrefabInstancingActor* Instance = nullptr;
					if (FoundInstances.Num() != 0)
					{
						for (const auto& FoundInstance : FoundInstances)
						{
							TArray<UMaterialInterface*> Mats = FoundInstance->GetMaterials();
							if (Mats == Materials)
							{
								Instance = FoundInstance;
								break;
							}
						}
					}

					if (Instance == nullptr)
					{
						FTransform InstanceTransform = PrefabActor->GetActorTransform();
						InstanceTransform.SetScale3D(FVector::OneVector);
						InstanceTransform.SetRotation(FQuat::Identity);
						Instance = World->SpawnActorDeferred<APrefabInstancingActor>(APrefabInstancingActor::StaticClass(), InstanceTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
						Instance->SetupInstances(bHierarchical, SM, ChildSM->GetStaticMeshComponent()->Mobility);
						size_t index = 0;
						for (UMaterialInterface* Material : Materials)
						{
							Instance->SetMaterial(index++, Material);
						}
						UGameplayStatics::FinishSpawningActor(Instance, InstanceTransform);
						Instance->AttachToActor(PrefabActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));
						InstancedActorMap.Add(SM, Instance);

						UPrefabricatorAssetUserData* ChildUserData = Instance->GetRootComponent()->GetAssetUserData<UPrefabricatorAssetUserData>();
						FGuid ItemID;
						if (ChildUserData && ChildUserData->PrefabActor == PrefabActor)
						{
							ItemID = ChildUserData->ItemID;
						}
						else
						{
							ItemID = FGuid::NewGuid();
						}
						FPrefabTools::AssignAssetUserData(Instance, ItemID, PrefabActor);

					}
					FTransform InstanceLocal = ChildSM->GetActorTransform();
					InstanceLocal.SetLocation(InstanceLocal.GetLocation() - Instance->GetActorLocation());
					Instance->AddInstance(InstanceLocal);
					Instance->AddReferenceToActor(ChildSM);
				}
			}
		}
		for (auto& TupleInstance : InstancedActorMap)
		{
			auto Instance = TupleInstance.Value;
			Instance->Cleanup(2);
		}
	}
}
FReply FPrefabActorCustomization::MakeInstances(IDetailLayoutBuilder* DetailBuilder, bool bHierarchical)
{
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_MakeInstanced", "Make Instanced Meshes"));

	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) 
	{
		if (PrefabActor) 
		{
			ConvertToInstances(PrefabActor, bHierarchical);
		}
	}
	return FReply::Handled();
}

static void ConvertToStaticMeshes(APrefabActor* PrefabActor)
{
	UWorld* World = PrefabActor->GetWorld();
	TArray<AActor*> Children;
	PrefabActor->GetAttachedActors(Children);
	for (AActor* Child : Children)
	{
		if (APrefabInstancingActor* Instance = Cast<APrefabInstancingActor>(Child))
		{
			UInstancedStaticMeshComponent* InstancedSM = Instance->GetInstancedStaticMeshComponent();
			for (int32 idx = 0; idx < InstancedSM->GetInstanceCount(); ++idx)
			{
				FTransform Transform;
				InstancedSM->GetInstanceTransform(idx, Transform, true);
				AStaticMeshActor* SMActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform);
				SMActor->GetStaticMeshComponent()->SetStaticMesh(InstancedSM->GetStaticMesh());
				auto Materials = InstancedSM->GetMaterials();
				int MaterialIdx = 0;
				for (UMaterialInterface* Material : Materials)
				{
					SMActor->GetStaticMeshComponent()->SetMaterial(MaterialIdx++, Material);
				}
				SMActor->AttachToActor(PrefabActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));
				SMActor->SetMobility(InstancedSM->Mobility);

				UPrefabricatorAssetUserData* ChildUserData = SMActor->GetRootComponent()->GetAssetUserData<UPrefabricatorAssetUserData>();
				FGuid ItemID;
				if (ChildUserData && ChildUserData->PrefabActor == PrefabActor)
				{
					ItemID = ChildUserData->ItemID;
				}
				else
				{
					ItemID = FGuid::NewGuid();
				}
				FPrefabTools::AssignAssetUserData(SMActor, ItemID, PrefabActor);
			}
			Child->Destroy();
		}
	}
}

FReply FPrefabActorCustomization::MakeStaticMeshes(IDetailLayoutBuilder* DetailBuilder)
{
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_MakeStatic", "Make Static Meshes"));

	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors)
	{
		if (PrefabActor)
		{
			ConvertToStaticMeshes(PrefabActor);
		}
	}

	return FReply::Handled();
}
// SBZ

// SBZ stephane.maruejouls - merge meshes
bool FPrefabActorCustomization::ContainsInstances(IDetailLayoutBuilder* DetailBuilder) const
{
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors)
	{
		if (PrefabActor)
		{
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);
			for (AActor* Child : Children)
			{
				if (Cast< APrefabInstancingActor>(Child))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool FPrefabActorCustomization::ContainsOnlyStatics(IDetailLayoutBuilder* DetailBuilder, bool bOnlyStatics) const
{
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors)
	{
		if (PrefabActor)
		{
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);
			if (Children.Num() < 2)
			{
				return false;
			}

			for (AActor* Child : Children)
			{
				if (Child && Child->IsPendingKill())
				{
					continue;
				}
				TArray<AActor*> GrandChildren;
				Child->GetAttachedActors(GrandChildren);
				if (GrandChildren.Num() > 0)
				{
					return 0;
				}

				AStaticMeshActor* ChildSM = Cast<AStaticMeshActor>(Child);
				if (ChildSM)
				{
					const TSet<UActorComponent*> Components = ChildSM->GetComponents();
					if (Components.Num() != 1)
					{
						return false;
					}
					if (!Cast<UStaticMeshComponent>(*Components.begin()))
					{
						return false;
					}
					if (bOnlyStatics)
					{
						if (Cast<UStaticMeshComponent>(*Components.begin())->Mobility != EComponentMobility::Static)
						{
							return false;
						}
					}
				}
				else
				{
					return false;
				}
			}
		}
	}
			
	return true;
}

static bool UpdateComponents(AActor* Actor, TArray<UPrimitiveComponent*>& SelectedComponents)
{
	TArray<UPrimitiveComponent*> PrimComponents;
	Actor->GetComponents<UPrimitiveComponent>(PrimComponents);
	bool bActorIncluded = false;
	for (UPrimitiveComponent* PrimComponent : PrimComponents)
	{
		bool bInclude = false; // Should put into UI list
		bool bShouldIncorporate = false; // Should default to part of merged mesh
		bool bIsMesh = false;
		if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimComponent))
		{
			bShouldIncorporate = (StaticMeshComponent->GetStaticMesh() != nullptr);
			bInclude = true;
			bIsMesh = true;
		}

		if (bInclude && bShouldIncorporate)
		{
			bActorIncluded = true;
			SelectedComponents.Add(PrimComponent);
		}
	}
	return bActorIncluded;
}

static void GetAttachedActorsRecursive(AActor* Actor, TArray<AActor*>& OutActors)
{
	TArray<AActor*> Children;
	Actor->GetAttachedActors(Children);
	for (AActor* Child : Children)
	{
		if (Child)
		{
			OutActors.Add(Child);
			GetAttachedActorsRecursive(Child, OutActors);
		}
	}
}

static void MergePrefab( APrefabActor* PrefabActor, FString SaveObjectPath, bool bUpdate)
{
	const IMeshMergeUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshMergeModule>("MeshMergeUtilities").GetUtilities();
	IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	const FString PackagePathName = PrefabActor->GetPrefabAsset()->GetPathName();
	const FString PackagePath = FPackageName::GetLongPackagePath(PackagePathName);
	const FString InAssetName = "SM_Merged_"+PrefabActor->GetPrefabAsset()->GetName();
	const FString AssetPath = PackagePath / InAssetName;
	
	
	FString PackageName, AssetName;
	AssetTools.CreateUniqueAssetName(*AssetPath, TEXT(""), PackageName, AssetName);

	const FString DefaultPath = FPackageName::GetLongPackagePath(PackageName);
	const FString DefaultName = FPackageName::GetShortName(PackageName);

	// Initialize SaveAssetDialog config
	FSaveAssetDialogConfig SaveAssetDialogConfig;
	SaveAssetDialogConfig.DialogTitleOverride = LOCTEXT("Prefab_CreateMergedActorTitle", "Create Merged Actor");
	SaveAssetDialogConfig.DefaultPath = DefaultPath;
	SaveAssetDialogConfig.DefaultAssetName = DefaultName;
	SaveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;

	if (SaveObjectPath.IsEmpty())
	{
		SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(SaveAssetDialogConfig);
	}
	if (!SaveObjectPath.IsEmpty())
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FAssetData AssetData = AssetRegistry.Get().GetAssetByObjectPath(*SaveObjectPath);
		bool bReuse = false;
		if (!bUpdate && AssetData.IsValid())
		{
			bReuse = OpenMsgDlgInt(EAppMsgType::YesNo, EAppReturnType::No, LOCTEXT("Prefab_CreateMergedActor_Replace", "Reuse the existing asset (Yes)\nor\nOverwrite the existing asset (No)"), LOCTEXT("Prefab_Existing", "Existing asset")) == EAppReturnType::Yes;
		}
		PackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);

		TArray<UPrimitiveComponent*> SelectedComponents;
		TArray<AActor*> Actors;
		GetAttachedActorsRecursive(PrefabActor, Actors);
		TArray<AActor*> MergedActors;
		for (AActor* Actor : Actors)
		{
			if (UpdateComponents(Actor, SelectedComponents))
			{
				MergedActors.Add(Actor);
			}
		}

		FVector MergedActorLocation;
		TArray<UObject*> AssetsToSync;
		if (!bReuse)
		{
			// Merge...
			{
				FScopedSlowTask SlowTask(0, LOCTEXT("MergingActorsSlowTask", "Merging prefab ..."));
				SlowTask.MakeDialog();

				if (SelectedComponents.Num())
				{
					UWorld* World = PrefabActor->GetWorld();
					checkf(World != nullptr, TEXT("Invalid World retrieved from Mesh components"));
					const float ScreenAreaSize = TNumericLimits<float>::Max();
					FMeshMergingSettings Settings;
					Settings.bMergePhysicsData = true;
					Settings.LODSelectionType = EMeshLODSelectionType::AllLODs;
					MeshUtilities.MergeComponentsToStaticMesh(SelectedComponents, World, Settings, nullptr, nullptr, PackageName, AssetsToSync, MergedActorLocation, ScreenAreaSize, true);
				}
			}
		}
		else
		{
			if (SelectedComponents.Num())
			{
				MergedActorLocation = SelectedComponents[0]->GetComponentLocation();
				AssetsToSync.Add(AssetData.GetAsset());
			}
		}

		if (AssetsToSync.Num())
		{
			int32 AssetCount = AssetsToSync.Num();
			for (int32 AssetIndex = 0; AssetIndex < AssetCount; AssetIndex++)
			{
				AssetRegistry.AssetCreated(AssetsToSync[AssetIndex]);
				GEditor->BroadcastObjectReimported(AssetsToSync[AssetIndex]);
			}

			//Also notify the content browser that the new assets exists
			ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync, true);

			// Place new mesh in the world
			{
				UStaticMesh* MergedMesh = nullptr;
				if (AssetsToSync.FindItemByClass(&MergedMesh))
				{
					const FScopedTransaction Transaction(LOCTEXT("PlaceMergedActor", "Place Merged Actor"));
					PrefabActor->GetLevel()->Modify();

					UWorld* World = PrefabActor->GetLevel()->OwningWorld;
					FActorSpawnParameters Params;
					Params.OverrideLevel = PrefabActor->GetLevel();
					FRotator MergedActorRotation(ForceInit);

					AStaticMeshActor* MergedActor = World->SpawnActor<AStaticMeshActor>(MergedActorLocation, MergedActorRotation, Params);
					MergedActor->GetStaticMeshComponent()->SetStaticMesh(MergedMesh);
					MergedActor->SetActorLabel(AssetsToSync[0]->GetName());
					World->UpdateCullDistanceVolumes(MergedActor, MergedActor->GetStaticMeshComponent());

					MergedActor->AttachToActor(PrefabActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));
					MergedActor->SetMobility(EComponentMobility::Static);

					UPrefabricatorAssetUserData* ChildUserData = MergedActor->GetRootComponent()->GetAssetUserData<UPrefabricatorAssetUserData>();
					FGuid ItemID;
					if (ChildUserData && ChildUserData->PrefabActor == PrefabActor)
					{
						ItemID = ChildUserData->ItemID;
					}
					else
					{
						ItemID = FGuid::NewGuid();
					}
					FPrefabTools::AssignAssetUserData(MergedActor, ItemID, PrefabActor);


					// Remove source actors
					for (AActor* Actor : MergedActors)
					{
						Actor->Destroy();
					}
				}
			}
		}
	}
}

FReply FPrefabActorCustomization::MergeMeshes(IDetailLayoutBuilder* DetailBuilder)
{
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_MergeMeshes", "Merge Meshes"));

	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors)
	{
		if (PrefabActor)
		{
			MergePrefab(PrefabActor, TEXT(""), false );
		}
	}
	return FReply::Handled();
}
// SBZ


// SBZ stephane.maruejouls - Update merged from prefab
bool FPrefabActorCustomization::ContainsMerged(IDetailLayoutBuilder* DetailBuilder) const
{
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors)
	{
		if (PrefabActor)
		{
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);
			if (Children.Num() == 1)
			{
				if (Cast<AStaticMeshActor>(Children[0]) != nullptr)
				{
					return true;
				}
			}
			if (Children.Num() >=  1)
			{
				bool bHierachical = false;
				bool bInstance = false;
				for (AActor* Child : Children)
				{
					if (APrefabInstancingActor* Instance = Cast<APrefabInstancingActor>(Child))
					{
						bInstance = bInstance || !Instance->IsHierarchical();
						bHierachical = bHierachical  || Instance->IsHierarchical();
					}
				}
				return bInstance || bHierachical;
			}
		}
	}
	return false;
}

FReply FPrefabActorCustomization::UpdateMergedPrefabs(IDetailLayoutBuilder* DetailBuilder)
{
	FPrefabricatorScopedTransaction UndoTransaction(LOCTEXT("Prefab_UpdateMerge", "Update Merged Prefab"));
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors)
	{
		if (PrefabActor)
		{
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);
			bool bMerged = false;
			bool bHierachical = false;
			bool bInstance = false;
			if (Children.Num() == 1)
			{
				if (Cast<AStaticMeshActor>(Children[0]) != nullptr)
				{
					bMerged = true;
				}
			}
			if (!bMerged && Children.Num() >= 1)
			{
				bHierachical = true;
				bInstance = true;
				for (AActor* Child : Children)
				{
					if (APrefabInstancingActor* Instance = Cast<APrefabInstancingActor>(Child))
					{
						bInstance = bInstance || !Instance->IsHierarchical();
						bHierachical = bHierachical || Instance->IsHierarchical();
					}
				}
			}
			if (bMerged || bHierachical || bInstance)
			{
				if (bMerged)
				{
					AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Children[0]);
					UStaticMesh* SM = SMActor->GetStaticMeshComponent()->GetStaticMesh();
					if (SM)
					{
						FString Path = SM->GetPathName();
						PrefabActor->LoadPrefab();
						MergePrefab(PrefabActor,Path, true);
					}
				}
				else if (bHierachical)
				{
					PrefabActor->LoadPrefab();
					ConvertToInstances(PrefabActor, true);
				}
				else if (bInstance)
				{
					PrefabActor->LoadPrefab();
					ConvertToInstances(PrefabActor, false);
				}
			}
		}
	}
	return FReply::Handled();
}
// SBZ

///////////////////////////////// FPrefabRandomizerCustomization /////////////////////////////////

void FPrefabRandomizerCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Prefab Randomizer", FText::GetEmpty(), ECategoryPriority::Important);
	Category.AddCustomRow(LOCTEXT("PrefabRandomizerCommand_Filter", "randomize prefab collection asset"))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text(LOCTEXT("PrefabRandomizerCommand_Randomize", "Randomize"))
			.OnClicked(FOnClicked::CreateStatic(&FPrefabRandomizerCustomization::HandleRandomize, &DetailBuilder))
		];
}

TSharedRef<IDetailCustomization> FPrefabRandomizerCustomization::MakeInstance()
{
	return MakeShareable(new FPrefabRandomizerCustomization);
}

FReply FPrefabRandomizerCustomization::HandleRandomize(IDetailLayoutBuilder* DetailBuilder)
{
	TArray<APrefabRandomizer*> PrefabRandomizers = GetDetailObject<APrefabRandomizer>(DetailBuilder);
	for (APrefabRandomizer* PrefabRandomizer : PrefabRandomizers) {
		if (PrefabRandomizer) {
			if (PrefabRandomizer->MaxBuildTimePerFrame > 0) {
				// This requires async build. We need to switch the editor to realtime mode
				FPrefabEditorTools::SwitchLevelViewportToRealtimeMode();
			}

			PrefabRandomizer->Randomize(FMath::Rand());
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE 

