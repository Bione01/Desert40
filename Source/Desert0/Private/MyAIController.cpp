#include "MyAIController.h"
#include "GameCharacter.h"
#include "Grid_Manager.h"
#include "Cell_Actor.h"
#include "MyGameModebase.h"
#include "Kismet/GameplayStatics.h"

void AMyAIController::BeginPlay()
{
    Super::BeginPlay();

    // Inizializzazione delle variabili
    GameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(GetWorld()));
    GridManager = Cast<AGrid_Manager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGrid_Manager::StaticClass()));
    LastTarget = nullptr;  // Inizializza LastTarget come nullptr

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
        MyCharacter->OnMovementFinished.Broadcast();
        return;
    }

    AGameCharacter* Closest = FindClosestEnemy();
    if (!Closest)
    {
        MyCharacter->OnMovementFinished.Broadcast();
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

    float Distance = FVector::Dist(MyCharacter->GetActorLocation(), Closest->GetActorLocation());
    if (Distance <= MyCharacter->GetAttackRange() && !MyCharacter->HasAttackedThisTurn)
    {
        MyCharacter->Attack(Closest);
        MyCharacter->HasAttackedThisTurn = true;
        MyCharacter->OnMovementFinished.Broadcast(); // Comunica al GameMode che ha finito
        return;

    }

    if (!MyCharacter->HasMovedThisTurn && LastPath.Num() > 1)
    {
        int32 MaxSteps = MyCharacter->GetMaxMovement();
        int32 StepsToMove = FMath::Min(MaxSteps, LastPath.Num() - 1);

        ACell_Actor* DestinationCell = nullptr;

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
}

void AMyAIController::UpdateTarget(AGameCharacter* NewTarget)
{
    // Aggiorna LastTarget ogni volta che il target cambia
    LastTarget = NewTarget;
}
AGameCharacter* AMyAIController::FindClosestEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("[IA] Sto cercando nemici..."));

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundActors);

    UE_LOG(LogTemp, Warning, TEXT("[IA] Ho trovato %d attori di tipo AGameCharacter."), FoundActors.Num());

    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("[IA] MyCharacter è NULL."));
        return nullptr;
    }

    AGameCharacter* ClosestEnemy = nullptr;
    float MinDistance = FLT_MAX;

    for (AActor* Actor : FoundActors)
    {
        AGameCharacter* GameChar = Cast<AGameCharacter>(Actor);
        if (GameChar && !GameChar->bIsAIControlled)
        {
            UE_LOG(LogTemp, Warning, TEXT("[IA] Trovato: %s, bIsAIControlled = %s"),
                *GameChar->GetName(),
                GameChar->bIsAIControlled ? TEXT("true") : TEXT("false"));

            float Dist = FVector::Dist(MyCharacter->GetActorLocation(), GameChar->GetActorLocation());
            if (Dist < MinDistance)
            {
                MinDistance = Dist;
                ClosestEnemy = GameChar;
            }
        }
    }

    if (ClosestEnemy)
    {
        UE_LOG(LogTemp, Warning, TEXT("[IA] Nemico più vicino: %s"), *ClosestEnemy->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[IA] Nessun nemico trovato."));
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
void AMyAIController::OnCharacterMovementFinished()
{
    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter || !GameMode) return;

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

    // 🔥 Non fare più il Broadcast qui! Chiama direttamente il GameMode
    GameMode->NotifyAIUnitMoved();
}
