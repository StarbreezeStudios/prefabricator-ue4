//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "UI/PrefabCustomization.h"

#include "Asset/PrefabricatorAsset.h"
#include "Prefab/PrefabActor.h"
#include "Prefab/PrefabComponent.h"
#include "Prefab/PrefabTools.h"
#include "Instancing/PrefabInstancingActor.h"
#include "Prefab/Random/PrefabRandomizerActor.h"
#include "PrefabricatorEditorModule.h"
#include "PrefabricatorSettings.h"
#include "Utils/PrefabEditorTools.h"

#include "ContentBrowserModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorViewportClient.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"

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
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_Hierachical", "Hierarchical"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::MakeInstances, &DetailBuilder, true))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			//.Padding(4.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PrefabCommand_StaticMesh", "StaticMesh"))
				.OnClicked(FOnClicked::CreateStatic(&FPrefabActorCustomization::MakeStaticMeshes, &DetailBuilder))
			]
		];

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
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor) {
			PrefabActor->SavePrefab();

			UPrefabricatorAsset* PrefabAsset = Cast<UPrefabricatorAsset>(PrefabActor->PrefabComponent->PrefabAssetInterface.LoadSynchronous());
			if (PrefabAsset) {
				const UPrefabricatorSettings* PS = GetDefault<UPrefabricatorSettings>();
				if(PS->bAllowDynamicUpdate)
				{
					// Refresh all the existing prefabs in the level
					FPrefabEditorTools::ReloadPrefabsInLevel(PrefabActor->GetWorld(), PrefabAsset);
				}
			}
		}
	}
	return FReply::Handled();
}

FReply FPrefabActorCustomization::HandleSaveToNewAsset(IDetailLayoutBuilder* DetailBuilder)
{
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) {
		if (PrefabActor) {
			
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);

			if(Children.Num() > 0)
			{
				FPrefabTools::UnlinkAndDestroyPrefabActor(PrefabActor, "/", false);
				FPrefabTools::CreatePrefabFromActors(Children);
			}
		}
	}
	return FReply::Handled();
}

FReply FPrefabActorCustomization::HandleLoadFromAsset(IDetailLayoutBuilder* DetailBuilder)
{
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


FReply FPrefabActorCustomization::MakeInstances(IDetailLayoutBuilder* DetailBuilder, bool bHierarchical)
{
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors) 
	{
		if (PrefabActor) 
		{
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);

			TMap<UStaticMesh*, APrefabInstancingActor*> InstancedActorMap;

			if (Children.Num() > 0)
			{
				UWorld* World = PrefabActor->GetWorld();
				for (AActor* Actor : Children)
				{
					if (APrefabInstancingActor* Instance = Cast<APrefabInstancingActor>(Actor))
					{
						if (bHierarchical  && !Instance->bISM && ::IsValid(Instance->InstancedStaticMesh))
						{
							InstancedActorMap.Add(Instance->InstancedStaticMesh->GetStaticMesh(), Instance);
						}
						if (!bHierarchical  && Instance->bISM && ::IsValid(Instance->HierarchicalInstancedStaticMesh))
						{
							InstancedActorMap.Add(Instance->HierarchicalInstancedStaticMesh->GetStaticMesh(), Instance);
						}
					}
				}
				for (AActor* Actor : Children)
				{
					if (AStaticMeshActor* ChildSM = Cast<AStaticMeshActor>(Actor))
					{
						UStaticMesh* SM = ChildSM->GetStaticMeshComponent()->GetStaticMesh();
						if (::IsValid(SM))
						{
							auto FoundInstance = InstancedActorMap.Find(SM);
							APrefabInstancingActor* Instance = nullptr;
							if (!FoundInstance)
							{
								FTransform InstanceTransform = PrefabActor->GetActorTransform();
								InstanceTransform.SetScale3D(FVector::OneVector);
								InstanceTransform.SetRotation(FQuat::Identity);
								Instance = World->SpawnActorDeferred<APrefabInstancingActor>(APrefabInstancingActor::StaticClass(), InstanceTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
								if (!bHierarchical)
								{
									Instance->bISM = true;
									Instance->InstancedStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
									Instance->InstancedStaticMesh->SetStaticMesh(SM);
								}
								else
								{
									Instance->bISM = false;
									Instance->HierarchicalInstancedStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
									Instance->HierarchicalInstancedStaticMesh->SetStaticMesh(SM);
								}
								size_t index = 0;
								for (UMaterialInterface* Material : ChildSM->GetStaticMeshComponent()->GetMaterials())
								{
									if (bHierarchical)
									{
										Instance->HierarchicalInstancedStaticMesh->SetMaterial(index++, Material);
									}
									else
									{
										Instance->InstancedStaticMesh->SetMaterial(index++, Material);
									}
								}
								UGameplayStatics::FinishSpawningActor(Instance, InstanceTransform);
								Instance->AttachToActor(PrefabActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));
								InstancedActorMap.Add(SM, Instance);
							}
							else
							{
								Instance = *FoundInstance;
							}
							FTransform InstanceLocal = ChildSM->GetActorTransform();
							InstanceLocal.SetLocation(InstanceLocal.GetLocation() - Instance->GetActorLocation());
							if( bHierarchical )
							{
								Instance->HierarchicalInstancedStaticMesh->AddInstance(InstanceLocal);
							}
							else
							{
								Instance->InstancedStaticMesh->AddInstance(InstanceLocal);
							}
							Instance->AddReferenceToActor(ChildSM);
						}
					}
				}
				for(TTuple<UStaticMesh*, APrefabInstancingActor*> TupleInstance : InstancedActorMap)
				{
					auto Instance = TupleInstance.Value;
					Instance->Cleanup(bHierarchical ? Instance->HierarchicalInstancedStaticMesh->GetInstanceCount() < 2 : Instance->InstancedStaticMesh->GetInstanceCount() < 2);
				}
			}
		}
	}
	return FReply::Handled();
}

FReply FPrefabActorCustomization::MakeHierarchicalInstances(IDetailLayoutBuilder* DetailBuilder)
{
	return FReply::Handled();
}

FReply FPrefabActorCustomization::MakeStaticMeshes(IDetailLayoutBuilder* DetailBuilder)
{
	TArray<APrefabActor*> PrefabActors = GetDetailObject<APrefabActor>(DetailBuilder);
	for (APrefabActor* PrefabActor : PrefabActors)
	{
		if (PrefabActor)
		{
			UWorld* World = PrefabActor->GetWorld();
			TArray<AActor*> Children;
			PrefabActor->GetAttachedActors(Children);
			for (AActor* Child : Children)
			{
				if (APrefabInstancingActor* Instance = Cast<APrefabInstancingActor>(Child))
				{
					UInstancedStaticMeshComponent* InstancedSM = Instance->bISM ? Instance->InstancedStaticMesh : Instance->HierarchicalInstancedStaticMesh;
					for (int32 idx = 0; idx < InstancedSM->GetInstanceCount(); ++idx)
					{
						FTransform Transform;
						InstancedSM->GetInstanceTransform(idx,Transform, true);
						AStaticMeshActor* SMActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Transform);
						SMActor->GetStaticMeshComponent()->SetStaticMesh(InstancedSM->GetStaticMesh());
						auto Materials = InstancedSM->GetMaterials();
						int MaterialIdx = 0;
						for (UMaterialInterface* Material : Materials)
						{
							SMActor->GetStaticMeshComponent()->SetMaterial(MaterialIdx++, Material);
						}
						SMActor->AttachToActor(PrefabActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, true));
					}
				}
				Child->Destroy();
			}
		}
	}
	return FReply::Handled();
}


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

