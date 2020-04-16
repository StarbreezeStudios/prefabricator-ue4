// Copyright 2020 Starbreeze AB. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PrefabInstancingActor.generated.h"


class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class AStaticMeshActor;

UCLASS(Blueprintable, ConversionRoot, ComponentWrapperClass)
class PREFABRICATORRUNTIME_API APrefabInstancingActor : public AActor {
	GENERATED_UCLASS_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform) override;

	void Cleanup(int32 MinimalNumberOfInstances);

	void AddReferenceToActor(AStaticMeshActor* ReferencedActor);

	void SetupInstances(bool IsHierarchial, UStaticMesh* StaticMesh, EComponentMobility::Type Mobility);
	void SetMaterial(int32 ElementIndex, UMaterialInterface* Material);
	void AddInstance(const FTransform& InstanceTransform);

	bool IsHierarchical() const { return bISM; }

	UInstancedStaticMeshComponent* GetInstancedStaticMeshComponent();
	UInstancedStaticMeshComponent* GetInstancedStaticMeshComponent() const;

	UStaticMesh* GetStaticMesh() const;
	TArray<UMaterialInterface*> GetMaterials() const;

	virtual void PostInitializeComponents() override;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInstancedStaticMeshComponent* InstancedStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UHierarchicalInstancedStaticMeshComponent* HierarchicalInstancedStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bISM = true;

	UPROPERTY(Transient)
	TArray<AStaticMeshActor*> ReferencedActors;

};
