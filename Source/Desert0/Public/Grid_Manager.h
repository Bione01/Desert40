#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid_Manager.generated.h"

class ACell_Actor;
class AGameCharacter;

USTRUCT()
struct FCellNode
{
    GENERATED_BODY()

    ACell_Actor* Cell;
    float Cost;
    float Heuristic;
    FCellNode* Previous;

    FCellNode() : Cell(nullptr), Cost(FLT_MAX), Heuristic(0.f), Previous(nullptr) {}
    FCellNode(ACell_Actor* InCell) : Cell(InCell), Cost(FLT_MAX), Heuristic(0.f), Previous(nullptr) {}
};

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    TSubclassOf<AActor> ObstacleBlueprint;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void CreateGrid();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<ACell_Actor*> GetAllCells() const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    ACell_Actor* GetCellAt(int32 Row, int32 Column) const;

    UFUNCTION(BlueprintCallable, Category = "Grid|Pathfinding")
    TArray<ACell_Actor*> FindPathAStarAvoidingUnits(ACell_Actor* StartCell, ACell_Actor* TargetCell, const TArray<AGameCharacter*>& UnitsToIgnore);

    FVector GetStartLocation() const { return StartLocationSaved; }
    float GetCellStep() const { return CellSize + CellSpacing; }

private:
    FVector StartLocationSaved;
};
