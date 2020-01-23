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

	void Cleanup(bool DestroySelf);

	void AddReferenceToActor(AStaticMeshActor* ReferencedActor);
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UInstancedStaticMeshComponent* InstancedStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UHierarchicalInstancedStaticMeshComponent* HierarchicalInstancedStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bISM = true;

	UPROPERTY(Transient)
	TArray<AStaticMeshActor*> ReferencedActors;

};
