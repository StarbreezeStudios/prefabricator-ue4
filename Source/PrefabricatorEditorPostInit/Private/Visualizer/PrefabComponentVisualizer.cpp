//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Visualizers/PrefabComponentVisualizer.h"

#include "Prefab/PrefabActor.h"
#include "Prefab/PrefabComponent.h"
#include "Prefab/PrefabTools.h"
#include "Asset/PrefabricatorAsset.h"
#include "Asset/PrefabricatorAssetUserData.h"

#include "GameFramework/Actor.h"
#include "SceneManagement.h"

void FPrefabComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UPrefabComponent* PrefabComponent = Cast<const UPrefabComponent>(Component);
	if (!PrefabComponent) return;
	
	AActor* Parent = PrefabComponent->GetOwner();
	if (!Parent) return;

	APrefabActor* PrefabActor = Cast<APrefabActor>(Parent);
	if (!PrefabActor) return;

	FBox Bounds = FPrefabTools::GetPrefabBounds(Parent);
	Bounds = Bounds.ExpandBy(2);

	const float Thickness = 0;
	const FMatrix LocalToWorld = FMatrix::Identity;
	DrawWireBox(PDI, LocalToWorld, Bounds, FLinearColor::Green, SDPG_Foreground, Thickness);

	UPrefabricatorAssetInterface* Asset = PrefabComponent->PrefabAssetInterface.LoadSynchronous();
	if (Asset && Asset->IsA<UPrefabricatorAsset>())
	{
		TArray<AActor*> ExistingActorPool;
		FPrefabTools::GetActorChildren(PrefabActor, ExistingActorPool);
		TMap<FGuid, AActor*> ActorByItemID;

		for (AActor* ExistingActor : ExistingActorPool) {
			if (ExistingActor && ExistingActor->GetRootComponent()) {
				UPrefabricatorAssetUserData* PrefabUserData = ExistingActor->GetRootComponent()->GetAssetUserData<UPrefabricatorAssetUserData>();
				if (PrefabUserData && PrefabUserData->PrefabActor == PrefabActor) {
					ActorByItemID.Add(PrefabUserData->ItemID, ExistingActor);
				}
			}
		}

		UPrefabricatorAsset* PrefabAsset = CastChecked<UPrefabricatorAsset>(Asset);
		FVector PrefabActorLocation = Component->GetOwner()->GetActorLocation();
		for (const FPrefabricatorActorData& ActorData : PrefabAsset->ActorData)
		{
			FVector Location = PrefabActorLocation + ActorData.RelativeTransform.GetTranslation();
			// Is there an child with that ID ?
			if (ActorByItemID.Contains(ActorData.PrefabItemID))
			{
				Location = (PrefabActor->SpawnedInfos[ActorData.PrefabItemID].RelativeTransform * PrefabActor->GetActorTransform()).TransformPosition(FVector::ZeroVector);
				// Did user moved the actor ?
				if (PrefabActor->SpawnedInfos[ActorData.PrefabItemID].OriginalTransform.GetLocation() != ActorByItemID[ActorData.PrefabItemID]->GetActorLocation())
				{
					Location = ActorByItemID[ActorData.PrefabItemID]->GetActorLocation();
				}
			}
			
			DrawWireStar(PDI, Location, 10.f, FLinearColor::Green, SDPG_Foreground);
			FPrefabricatorRandomData RandomizationSettings(PrefabActor->ActorRandomDataOverrides.Contains(ActorData.PrefabItemID) ? PrefabActor->ActorRandomDataOverrides[ActorData.PrefabItemID] : ActorData.RandomizationSettings);
			if (RandomizationSettings.bRandomizeTransform)
			{
				FVector OffsetVariation = RandomizationSettings.Offset.GetLocation();
				FRotator OffsetRotation = RandomizationSettings.Offset.Rotator();
				Bounds = FBox(Location + OffsetVariation, Location - OffsetVariation);
				DrawWireBox(PDI, LocalToWorld, Bounds, FLinearColor::Gray, SDPG_Foreground, Thickness);
				if (!FMath::IsNearlyZero(OffsetRotation.Yaw))
				{
					DrawArc(PDI, Location, FVector::ForwardVector, FVector::RightVector, -OffsetRotation.Yaw, OffsetRotation.Yaw, 50, 20, FLinearColor::Gray, SDPG_Foreground);
				}
				if (!FMath::IsNearlyZero(OffsetRotation.Pitch))
				{
					DrawArc(PDI, Location, FVector::UpVector, FVector::ForwardVector, -OffsetRotation.Pitch, OffsetRotation.Pitch, 50, 20, FLinearColor::Gray, SDPG_Foreground);
				}
				if (!FMath::IsNearlyZero(OffsetRotation.Roll))
				{
					DrawArc(PDI, Location, FVector::UpVector, FVector::RightVector, -OffsetRotation.Roll, OffsetRotation.Roll, 50, 20, FLinearColor::Gray, SDPG_Foreground);
				}
			}
		}
	}
}

