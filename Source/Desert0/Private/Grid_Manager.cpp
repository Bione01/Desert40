#include "Grid_Manager.h"
#include "GameCharacter.h"
#include "Cell_Actor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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
ACell_Actor* AGrid_Manager::GetCellAt(int32 Row, int32 Column) const
{
    for (ACell_Actor* Cell : GridCells)
    {
        if (Cell && Cell->Row == Row && Cell->Column == Column)
        {
            return Cell;
        }
    }
    return nullptr;
}
TArray<ACell_Actor*> AGrid_Manager::FindPathAStarAvoidingUnits(ACell_Actor* StartCell, ACell_Actor* TargetCell, const TArray<AGameCharacter*>& UnitsToIgnore)
{
    TArray<FCellNode> Nodes;
    for (ACell_Actor* Cell : GridCells)
    {
        if (Cell)
        {
            Nodes.Add(FCellNode(Cell));
        }
    }

    FCellNode* StartNode = nullptr;
    FCellNode* TargetNode = nullptr;

    for (FCellNode& Node : Nodes)
    {
        if (Node.Cell == StartCell)
        {
            StartNode = &Node;
        }
        if (Node.Cell == TargetCell)
        {
            TargetNode = &Node;
        }
    }

    if (!StartNode || !TargetNode)
    {
        return TArray<ACell_Actor*>();
    }

    StartNode->Cost = 0.f;
    StartNode->Heuristic = FMath::Abs(StartCell->Row - TargetCell->Row) + FMath::Abs(StartCell->Column - TargetCell->Column);

    TArray<FCellNode*> OpenSet;
    TSet<FCellNode*> ClosedSet;
    OpenSet.Add(StartNode);

    while (OpenSet.Num() > 0)
    {
        // Ordina per (Cost + Heuristic)
        OpenSet.Sort([](const FCellNode& A, const FCellNode& B)
        {
            return (A.Cost + A.Heuristic) < (B.Cost + B.Heuristic);
        });

        FCellNode* CurrentNode = OpenSet[0];
        OpenSet.RemoveAt(0);
        ClosedSet.Add(CurrentNode);

        if (CurrentNode == TargetNode)
        {
            break;
        }

        for (FCellNode& Neighbor : Nodes)
        {
            if (!Neighbor.Cell || Neighbor.Cell->CellType == ECellType::Obstacle || ClosedSet.Contains(&Neighbor))
            {
                continue;
            }

            int32 RowDiff = FMath::Abs(Neighbor.Cell->Row - CurrentNode->Cell->Row);
            int32 ColDiff = FMath::Abs(Neighbor.Cell->Column - CurrentNode->Cell->Column);

            if (!((RowDiff == 1 && ColDiff == 0) || (RowDiff == 0 && ColDiff == 1)))
            {
                continue;
            }

            bool bIsOccupied = false;
            if (Neighbor.Cell->bIsOccupied && !UnitsToIgnore.Contains(Neighbor.Cell->OccupyingUnit) && Neighbor.Cell != TargetCell)
            {
                bIsOccupied = true;
            }

            if (bIsOccupied)
            {
                continue;
            }

            float NewCost = CurrentNode->Cost + 1.f;
            if (NewCost < Neighbor.Cost)
            {
                Neighbor.Cost = NewCost;
                Neighbor.Previous = CurrentNode;
                Neighbor.Heuristic = FMath::Abs(Neighbor.Cell->Row - TargetCell->Row) + FMath::Abs(Neighbor.Cell->Column - TargetCell->Column);

                if (!OpenSet.Contains(&Neighbor))
                {
                    OpenSet.Add(&Neighbor);
                }
            }
        }
    }

    // === Ricostruzione percorso ===
    TArray<ACell_Actor*> Path;
    FCellNode* Current = TargetNode;
    while (Current && Current->Previous)
    {
        Path.Insert(Current->Cell, 0);
        Current = Current->Previous;
    }

    if (StartNode)
    {
        Path.Insert(StartNode->Cell, 0);
    }

    return Path;
}
