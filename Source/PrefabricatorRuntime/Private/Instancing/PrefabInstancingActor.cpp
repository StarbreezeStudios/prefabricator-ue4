// Copyright 2020 Starbreeze AB. All Rights Reserved.

#include "Instancing/PrefabInstancingActor.h"

#include "Components/BillboardComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"


APrefabInstancingActor::APrefabInstancingActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InstancedStaticMesh(nullptr)
	, HierarchicalInstancedStaticMesh(nullptr)
	, bISM(true)
{
	InstancedStaticMesh = ObjectInitializer.CreateDefaultSubobject<UInstancedStaticMeshComponent>(this, "InstancedStaticMesh");
	HierarchicalInstancedStaticMesh = ObjectInitializer.CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(this, "HierarchicalInstancedStaticMesh");
	InstancedStaticMesh->SetupAttachment(RootComponent);
	HierarchicalInstancedStaticMesh->SetupAttachment(RootComponent);
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

void APrefabInstancingActor::SetupInstances(bool IsHierarchial, UStaticMesh* StaticMesh, EComponentMobility::Type Mobility)
{
	bISM = IsHierarchial;
	if (bISM)
	{
		if (::IsValid(HierarchicalInstancedStaticMesh))
		{
			HierarchicalInstancedStaticMesh->DestroyComponent();
			HierarchicalInstancedStaticMesh = nullptr;
		}
		SetRootComponent(InstancedStaticMesh);
		InstancedStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		InstancedStaticMesh->SetStaticMesh(StaticMesh);
		InstancedStaticMesh->SetMobility(Mobility);

	}
	else
	{
		if (::IsValid(InstancedStaticMesh))
		{
			InstancedStaticMesh->DestroyComponent();
			InstancedStaticMesh = nullptr;
		}
		SetRootComponent(HierarchicalInstancedStaticMesh);
		HierarchicalInstancedStaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HierarchicalInstancedStaticMesh->SetStaticMesh(StaticMesh);
		HierarchicalInstancedStaticMesh->SetMobility(Mobility);
	}
}

UStaticMesh* APrefabInstancingActor::GetStaticMesh() const
{
	return GetInstancedStaticMeshComponent()->GetStaticMesh();
}

TArray<UMaterialInterface*> APrefabInstancingActor::GetMaterials() const
{
	return GetInstancedStaticMeshComponent()->GetMaterials();
}

void APrefabInstancingActor::SetMaterial(int32 ElementIndex, UMaterialInterface* Material)
{
	GetInstancedStaticMeshComponent()->SetMaterial(ElementIndex, Material);
}

void APrefabInstancingActor::AddInstance(const FTransform& InstanceTransform)
{
	GetInstancedStaticMeshComponent()->AddInstance(InstanceTransform);
}

UInstancedStaticMeshComponent* APrefabInstancingActor::GetInstancedStaticMeshComponent()
{
	check(bISM ? InstancedStaticMesh : HierarchicalInstancedStaticMesh);
	return bISM ? InstancedStaticMesh : HierarchicalInstancedStaticMesh;
}

UInstancedStaticMeshComponent* APrefabInstancingActor::GetInstancedStaticMeshComponent() const
{
	check(bISM ? InstancedStaticMesh : HierarchicalInstancedStaticMesh);
	return bISM ? InstancedStaticMesh : HierarchicalInstancedStaticMesh;
}

void APrefabInstancingActor::Cleanup(int32 MinimalNumberOfInstances)
{
	
	bool DestroySelf = GetInstancedStaticMeshComponent()->GetInstanceCount() < MinimalNumberOfInstances;
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
	}
}

void APrefabInstancingActor::AddReferenceToActor(AStaticMeshActor* ReferencedActor)
{
	ReferencedActors.Add(ReferencedActor);
}

void APrefabInstancingActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}