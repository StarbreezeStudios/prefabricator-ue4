//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "PrefabricatorRuntimeModule.h"

#include "Utils/PrefabricatorService.h"
#include "Prefab/PrefabTools.h"	// SBZ stephane.maruejouls - remove prefab at runtime

class FPrefabricatorRuntime : public IPrefabricatorRuntime
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
// SBZ stephane.maruejouls - remove prefab at runtime
#if WITH_EDITOR
	FDelegateHandle PreSaveHandle;
#endif
// SBZ
};

IMPLEMENT_MODULE(FPrefabricatorRuntime, PrefabricatorRuntime)

void FPrefabricatorRuntime::StartupModule()
{
	// Set the runtime prefabricator service
	// Set this only if it is null (the editor might have set the editor service, and if so, we won't override it)
	if (!FPrefabricatorService::Get().IsValid()) {
		FPrefabricatorService::Set(MakeShareable(new FPrefabricatorRuntimeService));
	}
// SBZ stephane.maruejouls - remove prefab at runtime
#if WITH_EDITOR
	PreSaveHandle = FEditorDelegates::PreSaveWorld.AddStatic(FPrefabTools::CleanupBeforeSave);
#endif
// SBZ
}


void FPrefabricatorRuntime::ShutdownModule()
{
	// Clear the service object
	FPrefabricatorService::Set(nullptr);
// SBZ stephane.maruejouls - remove prefab at runtime
#if WITH_EDITOR
	FEditorDelegates::PreSaveWorld.Remove(PreSaveHandle);
#endif
// SBZ

}

