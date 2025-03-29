#include "Grid_Manager.h"
#include "Cell_Actor.h"
#include "Engine/World.h"

AGrid_Manager::AGrid_Manager()
{
    PrimaryActorTick.bCanEverTick = false;

    NumRows = 25;
    NumColumns = 25;
    CellSize = 100.f;
    CellSpacing = 0.f; // Puoi modificarlo in Blueprint
}

void AGrid_Manager::BeginPlay()
{
    Super::BeginPlay();
    CreateGrid();
}

void AGrid_Manager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AGrid_Manager::CreateGrid()
{
    if (!CellActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("CellActorClass non è impostata in Grid_Manager!"));
        return;
    }

    if (!ObstacleBlueprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("ObstacleBlueprint non è impostata in Grid_Manager!"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    GridCells.Empty();

    FVector Origin = GetActorLocation();
    float GridWidth = NumColumns * CellSize + (NumColumns - 1) * CellSpacing;
    float GridHeight = NumRows * CellSize + (NumRows - 1) * CellSpacing;
    FVector StartLocation = Origin - FVector(GridWidth / 2, GridHeight / 2, 0.f);
    StartLocationSaved = StartLocation;

    for (int32 RowIndex = 0; RowIndex < NumRows; ++RowIndex)
    {
        for (int32 ColIndex = 0; ColIndex < NumColumns; ++ColIndex)
        {
            FVector CellLocation = StartLocation + FVector(
                ColIndex * (CellSize + CellSpacing),
                RowIndex * (CellSize + CellSpacing),
                0.f);

            FRotator SpawnRotation = FRotator::ZeroRotator;
            FActorSpawnParameters SpawnParams;

            bool bIsObstacle = FMath::RandRange(0, 100) < 10;

            if (bIsObstacle)
            {
                AActor* NewObstacle = World->SpawnActor<AActor>(ObstacleBlueprint, CellLocation, SpawnRotation, SpawnParams);
                if (NewObstacle)
                {
                    UE_LOG(LogTemp, Log, TEXT("Ostacolo generato a %s"), *CellLocation.ToString());
                }
            }
            else
            {
                ACell_Actor* NewCell = World->SpawnActor<ACell_Actor>(CellActorClass, CellLocation, SpawnRotation, SpawnParams);
                if (NewCell)
                {
                    NewCell->Row = RowIndex;
                    NewCell->Column = ColIndex;
                    GridCells.Add(NewCell);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Griglia generata con %d celle e ostacoli casuali."), GridCells.Num());
}

TArray<ACell_Actor*> AGrid_Manager::GetAllCells() const
{
    return GridCells;
}
