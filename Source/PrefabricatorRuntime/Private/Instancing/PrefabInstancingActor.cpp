// Copyright 2020 Starbreeze AB. All Rights Reserved.

#include "Instancing/PrefabInstancingActor.h"

#include "Components/BillboardComponent.h"
#include "Engine/PointLight.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

#include "Engine/StaticMeshActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogPrefabActor, Log, All);


APrefabInstancingActor::APrefabInstancingActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InstancedStaticMesh(nullptr)
	, HierarchicalInstancedStaticMesh(nullptr)
	, bISM(true)
{
	InstancedStaticMesh = ObjectInitializer.CreateDefaultSubobject<UInstancedStaticMeshComponent>(this, "InstancedStaticMesh");
	HierarchicalInstancedStaticMesh = ObjectInitializer.CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(this, "HierarchicalInstancedStaticMesh");
}

void APrefabInstancingActor::OnConstruction(const FTransform& Transform)
{
	AActor::OnConstruction(Transform);
	if (bISM)
	{
		if (::IsValid(HierarchicalInstancedStaticMesh))
		{
			HierarchicalInstancedStaticMesh->DestroyComponent();
			HierarchicalInstancedStaticMesh = nullptr;
		}
		SetRootComponent(InstancedStaticMesh);
	}
	else
	{
		if(::IsValid(InstancedStaticMesh))
		{ 
			InstancedStaticMesh->DestroyComponent();
			InstancedStaticMesh = nullptr;
		}
		SetRootComponent(HierarchicalInstancedStaticMesh);
	}
}

void APrefabInstancingActor::Cleanup(bool DestroySelf)
{
	if (DestroySelf)
	{
		Destroy();
	}
	else
	{
		for (AStaticMeshActor* Ref : ReferencedActors)
		{
			if (IsValid(Ref))
			{
				Ref->Destroy();
			}
		}
		ReferencedActors.Empty();
		if (bISM)
		{
			InstancedStaticMesh->InitPerInstanceRenderData(true);
		}
		else
		{
			InstancedStaticMesh->InitPerInstanceRenderData(false);
		}

	}
}

void APrefabInstancingActor::AddReferenceToActor(AStaticMeshActor* ReferencedActor)
{
	ReferencedActors.Add(ReferencedActor);
}