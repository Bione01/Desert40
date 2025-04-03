#include "MyAIController.h"
#include "GameCharacter.h"
#include "SniperCharacter.h"
#include "BrawlerCharacter.h"
#include "Grid_Manager.h"
#include "Cell_Actor.h"
#include "MyGameModebase.h"
#include "Kismet/GameplayStatics.h"

void AMyAIController::BeginPlay()
{
    Super::BeginPlay();

    GameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(GetWorld()));
    GridManager = Cast<AGrid_Manager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGrid_Manager::StaticClass()));
    LastTarget = nullptr;

    ClearCurrentPath();
}

void AMyAIController::RunTurn()
{
    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter || !GameMode || !GridManager)
    {
        return;
    }

    if (MyCharacter->HasMovedThisTurn && MyCharacter->HasAttackedThisTurn)
    {
        GameMode->NotifyAIUnitMoved();
        return;
    }

    AGameCharacter* Closest = FindClosestEnemy();
    if (!Closest)
    {
        GameMode->NotifyAIUnitMoved();
        return;
    }

    bool bTargetChanged = (Closest != LastTarget);

    if (bTargetChanged || LastPath.Num() <= 1)
    {
        LastTarget = Closest;
        ACell_Actor* StartCell = GridManager->GetCellAt(MyCharacter->CurrentRow, MyCharacter->CurrentColumn);
        ACell_Actor* TargetCell = nullptr;

        if (MyCharacter->IsBrawler())
        {
            TArray<ACell_Actor*> AdjacentCells;
            int32 Row = Closest->CurrentRow;
            int32 Col = Closest->CurrentColumn;

            TArray<FIntPoint> Offsets = { {1,0}, {-1,0}, {0,1}, {0,-1} };
            for (FIntPoint Offset : Offsets)
            {
                ACell_Actor* Cell = GridManager->GetCellAt(Row + Offset.X, Col + Offset.Y);
                if (Cell && !Cell->bIsOccupied && Cell->CellType != ECellType::Obstacle)
                {
                    AdjacentCells.Add(Cell);
                }
            }

            int32 MinLength = INT_MAX;
            for (ACell_Actor* Cell : AdjacentCells)
            {
                TArray<AGameCharacter*> Ignore;
                for (AGameCharacter* Unit : GameMode->GetAIUnits())
                {
                    if (Unit && Unit != MyCharacter)
                    {
                        Ignore.Add(Unit);
                    }
                }

                TArray<ACell_Actor*> Path = GridManager->FindPathAStarIgnoringUnits(StartCell, Cell, Ignore);
                if (Path.Num() > 1 && Path.Num() < MinLength)
                {
                    MinLength = Path.Num();
                    TargetCell = Cell;
                    LastPath = Path;
                }
            }

            if (!TargetCell)
            {
                TargetCell = GridManager->GetCellAt(Closest->CurrentRow, Closest->CurrentColumn);
                TArray<AGameCharacter*> Ignore;
                for (AGameCharacter* Unit : GameMode->GetAIUnits())
                {
                    if (Unit && Unit != MyCharacter)
                    {
                        Ignore.Add(Unit);
                    }
                }
                LastPath = GridManager->FindPathAStarIgnoringUnits(StartCell, TargetCell, Ignore);
            }
        }
        else
        {
            TargetCell = GridManager->GetCellAt(Closest->CurrentRow, Closest->CurrentColumn);
            TArray<AGameCharacter*> Ignore;
            for (AGameCharacter* Unit : GameMode->GetAIUnits())
            {
                if (Unit && Unit != MyCharacter)
                {
                    Ignore.Add(Unit);
                }
            }
            LastPath = GridManager->FindPathAStarIgnoringUnits(StartCell, TargetCell, Ignore);
        }
    }

    // === Se è già in range d'attacco all'inizio
    int32 RowDiff = FMath::Abs(MyCharacter->CurrentRow - Closest->CurrentRow);
    int32 ColDiff = FMath::Abs(MyCharacter->CurrentColumn - Closest->CurrentColumn);
    if (RowDiff + ColDiff <= MyCharacter->GetAttackRange() && !MyCharacter->HasAttackedThisTurn)
    {
        MyCharacter->Attack(Closest);
        MyCharacter->HasAttackedThisTurn = true;
        GameMode->NotifyAIUnitMoved();
        return;
    }

    // === Movimento
    if (!MyCharacter->HasMovedThisTurn && LastPath.Num() > 1)
    {
        int32 MaxSteps = MyCharacter->GetMaxMovement();
        int32 StepsToMove = FMath::Min(MaxSteps, LastPath.Num() - 1);

        ACell_Actor* DestinationCell = nullptr;

        if (MyCharacter->IsSniper())
        {
            for (int32 i = 1; i <= StepsToMove; ++i)
            {
                ACell_Actor* StepCell = LastPath[i];
                int32 DistanceToTarget = FMath::Abs(StepCell->Row - Closest->CurrentRow) + FMath::Abs(StepCell->Column - Closest->CurrentColumn);

                if (DistanceToTarget <= MyCharacter->GetAttackRange())
                {
                    TArray<ACell_Actor*> SubPath;
                    for (int32 j = 0; j <= i; ++j)
                    {
                        SubPath.Add(LastPath[j]);
                    }

                    MyCharacter->OnMovementFinished.Clear();
                    MyCharacter->OnMovementFinished.AddDynamic(this, &AMyAIController::OnCharacterMovementFinished);
                    MyCharacter->StartStepByStepMovement(SubPath);
                    MyCharacter->HasMovedThisTurn = true;
                    LastPath.RemoveAt(0, i);
                    return;
                }
            }
        }

        for (int32 i = 1; i <= StepsToMove; ++i)
        {
            ACell_Actor* NextCell = LastPath[i];
            if (NextCell && !NextCell->bIsOccupied)
            {
                DestinationCell = NextCell;
            }
            else
            {
                break;
            }
        }

        if (DestinationCell)
        {
            int32 Index = LastPath.IndexOfByKey(DestinationCell);
            if (Index != INDEX_NONE)
            {
                TArray<ACell_Actor*> SubPath;
                for (int32 i = 0; i <= Index; ++i)
                {
                    SubPath.Add(LastPath[i]);
                }

                MyCharacter->OnMovementFinished.Clear();
                MyCharacter->OnMovementFinished.AddDynamic(this, &AMyAIController::OnCharacterMovementFinished);
                MyCharacter->StartStepByStepMovement(SubPath);
                MyCharacter->HasMovedThisTurn = true;
                LastPath.RemoveAt(0, Index);
                return;
            }
        }
        else
        {
            LastPath.Empty();
        }
    }

    // === Se non può muoversi e non ha attaccato
    if (!MyCharacter->HasMovedThisTurn && LastPath.Num() <= 1 && !MyCharacter->HasAttackedThisTurn)
    {
        RowDiff = FMath::Abs(MyCharacter->CurrentRow - Closest->CurrentRow);
        ColDiff = FMath::Abs(MyCharacter->CurrentColumn - Closest->CurrentColumn);

        if (RowDiff + ColDiff <= MyCharacter->GetAttackRange())
        {
            MyCharacter->Attack(Closest);
            MyCharacter->HasAttackedThisTurn = true;
        }
    }

    GameMode->NotifyAIUnitMoved();
}


void AMyAIController::OnCharacterMovementFinished()
{
    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter || !GameMode) return;

    // Aggiorna la posizione logica SEMPRE
    MyCharacter->CurrentRow = MyCharacter->CurrentCell->Row;
    MyCharacter->CurrentColumn = MyCharacter->CurrentCell->Column;

    AGameCharacter* Closest = FindClosestEnemy();
    if (Closest)
    {
        float Distance = FVector::Dist(MyCharacter->GetActorLocation(), Closest->GetActorLocation());
        if (Distance <= MyCharacter->GetAttackRange() && !MyCharacter->HasAttackedThisTurn)
        {
            MyCharacter->Attack(Closest);
            MyCharacter->HasAttackedThisTurn = true;
        }
    }

    GameMode->NotifyAIUnitMoved();
}

AGameCharacter* AMyAIController::FindClosestEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("[IA] Sto cercando nemici..."));

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundActors);

    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter)
    {
        return nullptr;
    }

    AGameCharacter* ClosestEnemy = nullptr;
    float MinDistance = FLT_MAX;

    for (AActor* Actor : FoundActors)
    {
        AGameCharacter* GameChar = Cast<AGameCharacter>(Actor);
        if (GameChar && !GameChar->bIsAIControlled)
        {
            float Dist = FVector::Dist(MyCharacter->GetActorLocation(), GameChar->GetActorLocation());
            if (Dist < MinDistance)
            {
                MinDistance = Dist;
                ClosestEnemy = GameChar;
            }
        }
    }

    return ClosestEnemy;
}

void AMyAIController::ClearCurrentPath()
{
    CurrentPath.Empty();
    CurrentTarget = nullptr;
    UE_LOG(LogTemp, Warning, TEXT("[IA] Path e target sono stati azzerati."));
}

AGameCharacter* AMyAIController::GetControlledCharacter() const
{
    return Cast<AGameCharacter>(GetPawn());
}
