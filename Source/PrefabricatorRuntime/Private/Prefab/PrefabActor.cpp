//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Prefab/PrefabActor.h"

#include "Asset/PrefabricatorAsset.h"
#include "Asset/PrefabricatorAssetUserData.h"
#include "Prefab/PrefabComponent.h"
#include "Prefab/PrefabTools.h"

#include "Components/BillboardComponent.h"
#include "Engine/PointLight.h"

DEFINE_LOG_CATEGORY_STATIC(LogPrefabActor, Log, All);


APrefabActor::APrefabActor(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	PrefabComponent = ObjectInitializer.CreateDefaultSubobject<UPrefabComponent>(this, "PrefabComponent");
	RootComponent = PrefabComponent;
}

namespace {
	void DestroyAttachedActorsRecursive(AActor* ActorToDestroy, TSet<AActor*>& Visited) {
		if (!ActorToDestroy || !ActorToDestroy->GetRootComponent()) return;

		if (Visited.Contains(ActorToDestroy)) return;
		Visited.Add(ActorToDestroy);

		UPrefabricatorAssetUserData* PrefabUserData = ActorToDestroy->GetRootComponent()->GetAssetUserData<UPrefabricatorAssetUserData>();
		if (!PrefabUserData) return;

		UWorld* World = ActorToDestroy->GetWorld();
		if (!World) return;

		TArray<AActor*> AttachedActors;
		ActorToDestroy->GetAttachedActors(AttachedActors);
		for (AActor* AttachedActor : AttachedActors) {
			DestroyAttachedActorsRecursive(AttachedActor, Visited);
		}
		ActorToDestroy->Destroy();
	}
}

void APrefabActor::Destroyed()
{
	Super::Destroyed();

	// Destroy all attached actors
	{
		TSet<AActor*> Visited;
		TArray<AActor*> AttachedActors;
		GetAttachedActors(AttachedActors);
		for (AActor* AttachedActor : AttachedActors) {
			DestroyAttachedActorsRecursive(AttachedActor, Visited);
		}
	}
}

void APrefabActor::PostLoad()
{
	Super::PostLoad();

}

void APrefabActor::PostActorCreated()
{
	Super::PostActorCreated();
}

void APrefabActor::RandomizeSeed()
{
	Seed = FMath::Rand();
}

void APrefabActor::OnSeedChanged()
{
	FRandomStream Random;
	Random.Initialize(Seed);

	FPrefabLoadSettings LoadSettings;
	LoadSettings.Random = &Random;
	FPrefabTools::RandomizeState(this, LoadSettings);
}

#if WITH_EDITOR
void APrefabActor::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	// SBZ stephane.maruejouls - allow edition of seed
	if (e.Property) {
		FName PropertyName = e.Property->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(APrefabActor, Seed)) {
			OnSeedChanged();
		}
	}
	// SBZ
	Super::PostEditChangeProperty(e);
}

void APrefabActor::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	if (DuplicateMode == EDuplicateMode::Normal) {
		RandomizeSeed();
		OnSeedChanged();
	}
}

FName APrefabActor::GetCustomIconName() const
{
	static const FName PrefabIconName("ClassIcon.PrefabActor");
	return PrefabIconName;
}

#endif // WITH_EDITOR

void APrefabActor::LoadPrefab()
{
	FPrefabTools::LoadStateFromPrefabAsset(this);
}

void APrefabActor::SavePrefab()
{
	FPrefabTools::SaveStateToPrefabAsset(this);
}

bool APrefabActor::IsPrefabOutdated()
{
	UPrefabricatorAsset* PrefabAsset = GetPrefabAsset();
	if (!PrefabAsset) {
		return false;
	}

	return PrefabAsset->LastUpdateID != LastUpdateID;
}

UPrefabricatorAsset* APrefabActor::GetPrefabAsset()
{
	FPrefabAssetSelectionConfig SelectionConfig;
	SelectionConfig.Seed = Seed;
	UPrefabricatorAssetInterface* PrefabAssetInterface = PrefabComponent->PrefabAssetInterface.LoadSynchronous();
	return PrefabAssetInterface ? PrefabAssetInterface->GetPrefabAsset(SelectionConfig) : nullptr;
}

void APrefabActor::HandleBuildComplete()
{
	UPrefabricatorAssetInterface* PrefabAssetInterface = PrefabComponent->PrefabAssetInterface.LoadSynchronous();
	if (PrefabAssetInterface && PrefabAssetInterface->EventListener) {
		UPrefabricatorEventListener* EventListenerInstance = NewObject<UPrefabricatorEventListener>(GetTransientPackage(), PrefabAssetInterface->EventListener, NAME_None, RF_Transient);
		if (EventListenerInstance) {
			EventListenerInstance->PostSpawn(this);
		}
	}
}

////////////////////////////////// FPrefabBuildSystem //////////////////////////////////
FPrefabBuildSystem::FPrefabBuildSystem(double InTimePerFrame)
	: TimePerFrame(InTimePerFrame)
{
}

void FPrefabBuildSystem::Tick()
{
	double StartTime = FPlatformTime::Seconds();
	
	
	while (BuildStack.Num() > 0) {
		FPrefabBuildSystemCommandPtr Item = BuildStack.Pop();
		Item->Execute(*this);

		if (TimePerFrame > 0) {
			double ElapsedTime = FPlatformTime::Seconds() - StartTime;
			if (ElapsedTime >= TimePerFrame) {
				break;
			}
		}
	}
}

void FPrefabBuildSystem::Reset()
{
	BuildStack.Reset();
}

void FPrefabBuildSystem::PushCommand(FPrefabBuildSystemCommandPtr InCommand)
{
	BuildStack.Push(InCommand);
}

FPrefabBuildSystemCommand_BuildPrefab::FPrefabBuildSystemCommand_BuildPrefab(TWeakObjectPtr<APrefabActor> InPrefab, FRandomStream* InRandom)
	: Prefab(InPrefab)
	, Random(InRandom)
{
}

void FPrefabBuildSystemCommand_BuildPrefab::Execute(FPrefabBuildSystem& BuildSystem)
{
	if (Prefab.IsValid()) {
		FPrefabLoadSettings LoadSettings;
		LoadSettings.Random = Random;

		// Nested prefabs will be recursively build on the stack over multiple frames
		LoadSettings.bSynchronousBuild = false;

		FPrefabTools::RandomizeState(Prefab.Get(), LoadSettings);

		// Push a build complete notification request. Since this is a stack, it will execute after all the children are processed below
		FPrefabBuildSystemCommandPtr ChildBuildCommand = MakeShareable(new FPrefabBuildSystemCommand_NotifyBuildComplete(Prefab));
		BuildSystem.PushCommand(ChildBuildCommand);
	}

	// Add the child prefabs to the stack
	TArray<AActor*> ChildActors;
	Prefab->GetAttachedActors(ChildActors);
	for (AActor* ChildActor : ChildActors) {
		if (APrefabActor* ChildPrefab = Cast<APrefabActor>(ChildActor)) {
			FPrefabBuildSystemCommandPtr ChildBuildCommand = MakeShareable(new FPrefabBuildSystemCommand_BuildPrefab(ChildPrefab, Random));
			BuildSystem.PushCommand(ChildBuildCommand);
		}
	}
}

FPrefabBuildSystemCommand_NotifyBuildComplete::FPrefabBuildSystemCommand_NotifyBuildComplete(TWeakObjectPtr<APrefabActor> InPrefab)
	: Prefab(InPrefab)
{
}

void FPrefabBuildSystemCommand_NotifyBuildComplete::Execute(FPrefabBuildSystem& BuildSystem)
{
	if (Prefab.IsValid()) {
		// TODO: Execute Post spawn script
		Prefab->HandleBuildComplete();
	}
}



/////////////////////////////////////

AReplicablePrefabActor::AReplicablePrefabActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AReplicablePrefabActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}

void AReplicablePrefabActor::BeginPlay()
{
	if (Role == ROLE_Authority)
	{
		bReplicates = false;
		SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
		SetReplicates(true);
	}

	Super::BeginPlay();
}
