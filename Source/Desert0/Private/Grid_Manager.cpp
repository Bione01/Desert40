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
    CellSpacing = 0.f; // Blueprint editable
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
    if (!CellActorClass || !ObstacleBlueprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("CellActorClass or ObstacleBlueprint not set!"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    // clear
    for (ACell_Actor* Cell : GridCells)
    {
        if (Cell) Cell->Destroy();
    }
    GridCells.Empty();

    FVector Origin = GetActorLocation();
    float GridWidth = NumColumns * CellSize + (NumColumns - 1) * CellSpacing;
    float GridHeight = NumRows * CellSize + (NumRows - 1) * CellSpacing;
    FVector StartLocation = Origin - FVector(GridWidth / 2, GridHeight / 2, 0.f);
    StartLocationSaved = StartLocation;

    // loop for correct asset
    bool bGridIsValid = false;
    while (!bGridIsValid)
    {
        TArray<TArray<int32>> GridArray;
        GridArray.SetNum(NumRows);

        for (int32 Row = 0; Row < NumRows; ++Row)
        {
            GridArray[Row].SetNum(NumColumns);
            for (int32 Col = 0; Col < NumColumns; ++Col)
            {
                GridArray[Row][Col] = (FMath::RandRange(0, 100) < 10) ? 1 : 0;
            }
        }

        // verify is grid is valid
        bGridIsValid = !HasIsolatedCells(GridArray);
        if (bGridIsValid)
        {
            for (int32 Row = 0; Row < NumRows; ++Row)
            {
                for (int32 Col = 0; Col < NumColumns; ++Col)
                {
                    FVector CellLocation = StartLocation + FVector(
                        Col * (CellSize + CellSpacing),
                        Row * (CellSize + CellSpacing),
                        0.f);

                    FRotator SpawnRotation = FRotator::ZeroRotator;
                    FActorSpawnParameters SpawnParams;

                    if (GridArray[Row][Col] == 1)
                    {
                        World->SpawnActor<AActor>(ObstacleBlueprint, CellLocation, SpawnRotation, SpawnParams);
                    }
                    else
                    {
                        ACell_Actor* NewCell = World->SpawnActor<ACell_Actor>(CellActorClass, CellLocation, SpawnRotation, SpawnParams);
                        if (NewCell)
                        {
                            NewCell->Row = Row;
                            NewCell->Column = Col;

                            FString CellName = FString::Printf(TEXT("Cell_%d_%d"), Row, Col);
                            NewCell->Rename(*CellName);
                            #if WITH_EDITOR
                            NewCell->SetActorLabel(CellName);
                            #endif
                            NewCell->CellName = CellName;

                            GridCells.Add(NewCell);
                        }
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Grid correctly generated."));
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

TArray<ACell_Actor*> AGrid_Manager::GetAllCells() const
{
    return GridCells;
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
        // Order
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

    // path building
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
bool AGrid_Manager::HasIsolatedCells(const TArray<TArray<int32>>& GridArray) const
{
    for (int32 Row = 0; Row < NumRows; ++Row)
    {
        for (int32 Col = 0; Col < NumColumns; ++Col)
        {
            if (GridArray[Row][Col] == 0)
            {
                bool bHasFreeNeighbor = false;
                TArray<FIntPoint> Offsets = { {1,0}, {-1,0}, {0,1}, {0,-1} };
                for (FIntPoint Offset : Offsets)
                {
                    int32 NewRow = Row + Offset.X;
                    int32 NewCol = Col + Offset.Y;
                    if (NewRow >= 0 && NewRow < NumRows && NewCol >= 0 && NewCol < NumColumns)
                    {
                        if (GridArray[NewRow][NewCol] == 0)
                        {
                            bHasFreeNeighbor = true;
                            break;
                        }
                    }
                }
                if (!bHasFreeNeighbor)
                {
                    UE_LOG(LogTemp, Warning, TEXT("wrong cell found: (%d, %d)"), Row, Col);
                    return true;
                }
            }
        }
    }
    return false;
}
