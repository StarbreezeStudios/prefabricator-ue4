//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Utils/PrefabEditorTools.h"

#include "Asset/PrefabricatorAsset.h"		   // SBZ stephane.maruejouls - save to disk
#include "Prefab/PrefabActor.h"
#include "Prefab/PrefabTools.h"				   // SBZ stephane.maruejouls - save to disk
#include "Prefab/PrefabComponent.h"

#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "FileHelpers.h"					   // SBZ stephane.maruejouls - save to disk
#include "Framework/Notifications/NotificationManager.h"
#include "ScopedSlowTask.h"					   // SBZ stephane.maruejouls - save to disk

// SBZ stephane.maruejouls - save to disk
#define LOCTEXT_NAMESPACE "PrefabActorCustomization" 
// SBZ

void FPrefabEditorTools::ReloadPrefabsInLevel(UWorld* World, UPrefabricatorAsset* InAsset)
{
	for (TActorIterator<APrefabActor> It(World); It; ++It) {
		APrefabActor* PrefabActor = *It;
		if (PrefabActor && PrefabActor->PrefabComponent) {
			bool bShouldRefresh = true;
			// The provided asset can be null, in which case we refresh everything, else we search if it matches the particular prefab asset
			if (InAsset) {
				UPrefabricatorAsset* ActorAsset = PrefabActor->GetPrefabAsset();
				bShouldRefresh = (InAsset == ActorAsset);
			}
			if (bShouldRefresh) {
				if (PrefabActor->IsPrefabOutdated()) {
					PrefabActor->LoadPrefab();
				}
			}
		}
	}
}

void FPrefabEditorTools::ShowNotification(FText Text, SNotificationItem::ECompletionState State /*= SNotificationItem::CS_Fail*/)
{
	FNotificationInfo Info(Text);
	Info.bFireAndForget = true;
	Info.FadeOutDuration = 1.0f;
	Info.ExpireDuration = 2.0f;

	TWeakPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
	if (NotificationPtr.IsValid())
	{
		NotificationPtr.Pin()->SetCompletionState(State);
	}
}

void FPrefabEditorTools::SwitchLevelViewportToRealtimeMode()
{
	FEditorViewportClient* Client = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
	if (Client) {
		bool bRealtime = Client->IsRealtime();
		if (!bRealtime) {
			ShowNotification(NSLOCTEXT("Prefabricator", "PrefabRealtimeMode", "Switched viewport to Realtime mode"), SNotificationItem::CS_None);
			Client->SetRealtime(true);
		}
	}
	else {
		ShowNotification(NSLOCTEXT("Prefabricator", "PRefabClientNotFound", "Warning: Cannot find active viewport"));
	}
}

// SBZ stephane.maruejouls - save to disk
void FPrefabEditorTools::CreatePrefab()
{
	TArray<UPackage*> PackagesToSave;
	FPrefabTools::CreatePrefab();

	TArray<AActor*> SelectedActors;
	FPrefabTools::GetSelectedActors(SelectedActors);
	FScopedSlowTask SlowTask(SelectedActors.Num(), LOCTEXT("FPrefabEditorTools_CreatePrefab", "Creating Prefab"));
	SlowTask.MakeDialog(false);
	for (AActor* Actor : SelectedActors)
	{
		if (APrefabActor* PrefabActor = Cast<APrefabActor>(Actor))
		{
			SlowTask.EnterProgressFrame(1.f);
			UPrefabricatorAsset* PrefabAsset = Cast<UPrefabricatorAsset>(PrefabActor->PrefabComponent->PrefabAssetInterface.LoadSynchronous());
			if (PrefabAsset)
			{
				PackagesToSave.Add(PrefabAsset->GetOutermost());
			}
		}		
	}
	if (PackagesToSave.Num() > 0)
	{
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirtyOnAssetSave*/true, /*bPromptToSave=*/ false);
	}
}
// SBZ