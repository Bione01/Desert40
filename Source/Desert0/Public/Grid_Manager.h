#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid_Manager.generated.h"

class ACell_Actor;

UCLASS()
class DESERT0_API AGrid_Manager : public AActor
{
    GENERATED_BODY()
    
public:
    AGrid_Manager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 NumRows;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 NumColumns;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float CellSize;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float CellSpacing;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    TSubclassOf<ACell_Actor> CellActorClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    TArray<ACell_Actor*> GridCells;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
    TSubclassOf<AActor> ObstacleBlueprint;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void CreateGrid();
    
    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<ACell_Actor*> GetAllCells() const;

    // Nuovi metodi per accedere alle informazioni sulla griglia
    FVector GetStartLocation() const { return StartLocationSaved; }
    float GetCellStep() const { return CellSize + CellSpacing; }

private:
    FVector StartLocationSaved;
};
