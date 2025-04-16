#include "MyAIController.h"
#include "GameCharacter.h"
#include "MyGameInstance.h"
#include "SniperCharacter.h"
#include "BrawlerCharacter.h"
#include "Grid_Manager.h"
#include "UtilityFunctions.h"
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
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (GI && !GI->bIsHardMode)
    {
        EasyRunTurn();
        return;
    }
    
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
    if (MyCharacter->IsSniper() && Closest->IsBrawler())
    {
        int32 RowDiff = FMath::Abs(MyCharacter->CurrentRow - Closest->CurrentRow);
        int32 ColDiff = FMath::Abs(MyCharacter->CurrentColumn - Closest->CurrentColumn);

        if (RowDiff + ColDiff < 8 && !MyCharacter->HasMovedThisTurn)
        {
            TryToEscapeAndAttack(Closest);
            return;
        }
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

                TArray<ACell_Actor*> Path = GridManager->FindPathAStarAvoidingUnits(StartCell, Cell, Ignore);
                if (Path.Num() > 1 && Path.Num() < MinLength)
                {
                    MinLength = Path.Num();
                    TargetCell = Cell;
                    LastPath = Path;
                }
            }

            if (!TargetCell)
            {
                // get closer
                int32 MinDistance = INT_MAX;

                for (ACell_Actor* Cell : GridManager->GetAllCells())
                {
                    if (Cell && !Cell->bIsOccupied && Cell->CellType != ECellType::Obstacle)
                    {
                        int32 DistanceToEnemy = FMath::Abs(Cell->Row - Closest->CurrentRow) + FMath::Abs(Cell->Column - Closest->CurrentColumn);

                        TArray<AGameCharacter*> Ignore;
                        for (AGameCharacter* Unit : GameMode->GetAIUnits())
                        {
                            if (Unit && Unit != MyCharacter)
                            {
                                Ignore.Add(Unit);
                            }
                        }

                        TArray<ACell_Actor*> Path = GridManager->FindPathAStarAvoidingUnits(StartCell, Cell, Ignore);
                        if (Path.Num() > 1 && DistanceToEnemy < MinDistance)
                        {
                            MinDistance = DistanceToEnemy;
                            TargetCell = Cell;
                            LastPath = Path;
                        }
                    }
                }
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
            LastPath = GridManager->FindPathAStarAvoidingUnits(StartCell, TargetCell, Ignore);
        }
    }

    int32 RowDiff = FMath::Abs(MyCharacter->CurrentRow - Closest->CurrentRow);
    int32 ColDiff = FMath::Abs(MyCharacter->CurrentColumn - Closest->CurrentColumn);
    if (RowDiff + ColDiff <= MyCharacter->GetAttackRange() && !MyCharacter->HasAttackedThisTurn)
    {
        MyCharacter->Attack(Closest);
        MyCharacter->HasAttackedThisTurn = true;

        MyCharacter->HighlightedOriginCell = MyCharacter->CurrentCell;
        // hp log
        FString Prefix = TEXT("AI");
        FString UnitCode = MyCharacter->IsSniper() ? TEXT("S") : TEXT("B");
        FString TargetCoord = Closest->CurrentCell ? ConvertToChessNotation(Closest->CurrentCell->Row, Closest->CurrentCell->Column) : TEXT("??");
        int32 Damage = MyCharacter->GetLastDamageDealt();

        FString LogEntry = FString::Printf(TEXT("%s: %s %s %d"), *Prefix, *UnitCode, *TargetCoord, Damage);

        if (GameMode)
        {
            GameMode->AddMoveToLog(LogEntry);
        }

        GameMode->NotifyAIUnitMoved();
        return;
    }



    // movement
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
                    MyCharacter->HighlightedOriginCell = MyCharacter->CurrentCell;
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
                MyCharacter->HighlightedOriginCell = MyCharacter->CurrentCell;
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

    // if can't move and attack
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

void AMyAIController::EasyRunTurn()
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

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundActors);

    AGameCharacter* Target = nullptr;
    int32 MinDistance = INT_MAX;

    for (AActor* Actor : FoundActors)
    {
        AGameCharacter* OtherChar = Cast<AGameCharacter>(Actor);
        if (OtherChar && !OtherChar->bIsAIControlled)
        {
            int32 Distance = FMath::Abs(MyCharacter->CurrentRow - OtherChar->CurrentRow)
                           + FMath::Abs(MyCharacter->CurrentColumn - OtherChar->CurrentColumn);

            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                Target = OtherChar;
            }
        }
    }

    if (!Target)
    {
        GameMode->NotifyAIUnitMoved();
        return;
    }

    int32 RowDiff = FMath::Abs(MyCharacter->CurrentRow - Target->CurrentRow);
    int32 ColDiff = FMath::Abs(MyCharacter->CurrentColumn - Target->CurrentColumn);
    int32 Distance = RowDiff + ColDiff;

    // direct attack
    if (Distance <= MyCharacter->GetAttackRange() && !MyCharacter->HasAttackedThisTurn)
    {
        MyCharacter->Attack(Target);
        MyCharacter->HasAttackedThisTurn = true;

        FString LogEntry = FString::Printf(TEXT("AI: %s %s %d"),
            MyCharacter->IsSniper() ? TEXT("S") : TEXT("B"),
            *ConvertToChessNotation(Target->CurrentCell->Row, Target->CurrentCell->Column),
            MyCharacter->GetLastDamageDealt()
        );

        GameMode->AddMoveToLog(LogEntry);
        GameMode->NotifyAIUnitMoved();
        return;
    }

    // movement toward enemy
    ACell_Actor* StartCell = GridManager->GetCellAt(MyCharacter->CurrentRow, MyCharacter->CurrentColumn);
    ACell_Actor* TargetCell = GridManager->GetCellAt(Target->CurrentRow, Target->CurrentColumn);
    if (!StartCell || !TargetCell)
    {
        GameMode->NotifyAIUnitMoved();
        return;
    }

    TArray<AGameCharacter*> Ignore;
    for (AGameCharacter* Unit : GameMode->GetAIUnits())
    {
        if (Unit && Unit != MyCharacter)
        {
            Ignore.Add(Unit);
        }
    }

    TArray<ACell_Actor*> Path = GridManager->FindPathAStarAvoidingUnits(StartCell, TargetCell, Ignore);
    if (Path.Num() <= 1)
    {
        GameMode->NotifyAIUnitMoved();
        return;
    }

    int32 MaxSteps = MyCharacter->GetMaxMovement();
    int32 StepsToMove = FMath::Min(MaxSteps, Path.Num() - 1);

    TArray<ACell_Actor*> SubPath;
    for (int32 i = 0; i <= StepsToMove; ++i)
    {
        SubPath.Add(Path[i]);
    }

    MyCharacter->OnMovementFinished.Clear();
    MyCharacter->OnMovementFinished.AddDynamic(this, &AMyAIController::OnCharacterMovementFinished);
    MyCharacter->HighlightedOriginCell = MyCharacter->CurrentCell;
    MyCharacter->StartStepByStepMovement(SubPath);
    MyCharacter->HasMovedThisTurn = true;

    LastTarget = Target;
}

void AMyAIController::OnCharacterMovementFinished()
{
    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter || !GameMode) return;

    // update position
    MyCharacter->CurrentRow = MyCharacter->CurrentCell->Row;
    MyCharacter->CurrentColumn = MyCharacter->CurrentCell->Column;

    FVector FixedLocation = GameMode->GetCellLocationWithOffset(MyCharacter->CurrentCell);
    MyCharacter->SetActorLocation(FixedLocation);

    // ia movement log
    FString Prefix = TEXT("AI");
    FString UnitCode = MyCharacter->IsSniper() ? TEXT("S") : TEXT("B");
    FString FromCoord = MyCharacter->HighlightedOriginCell ? ConvertToChessNotation(MyCharacter->HighlightedOriginCell->Row, MyCharacter->HighlightedOriginCell->Column) : TEXT("??");
    FString ToCoord = MyCharacter->CurrentCell ? ConvertToChessNotation(MyCharacter->CurrentCell->Row, MyCharacter->CurrentCell->Column) : TEXT("??");

    FString MoveLogEntry = FString::Printf(TEXT("%s: %s %s -> %s"), *Prefix, *UnitCode, *FromCoord, *ToCoord);
    GameMode->AddMoveToLog(MoveLogEntry);

    // clear HighlightedOriginCell
    MyCharacter->HighlightedOriginCell = nullptr;

    // attack after movement
    if (LastTarget && !MyCharacter->HasAttackedThisTurn)
    {
        int32 RowDiff = FMath::Abs(MyCharacter->CurrentRow - LastTarget->CurrentRow);
        int32 ColDiff = FMath::Abs(MyCharacter->CurrentColumn - LastTarget->CurrentColumn);

        if (RowDiff + ColDiff <= MyCharacter->GetAttackRange())
        {
            MyCharacter->Attack(LastTarget);
            MyCharacter->HasAttackedThisTurn = true;

            // attack log after movement
            FString Prefix = TEXT("AI");
            FString UnitCode = MyCharacter->IsSniper() ? TEXT("S") : TEXT("B");
            FString TargetCoord = LastTarget->CurrentCell ? ConvertToChessNotation(LastTarget->CurrentCell->Row, LastTarget->CurrentCell->Column) : TEXT("??");
            int32 Damage = MyCharacter->GetLastDamageDealt();

            FString LogEntry = FString::Printf(TEXT("%s: %s %s %d"), *Prefix, *UnitCode, *TargetCoord, Damage);
            GameMode->AddMoveToLog(LogEntry);
        }
    }

    GameMode->NotifyAIUnitMoved();
}

AGameCharacter* AMyAIController::FindClosestEnemy()
{
    UE_LOG(LogTemp, Warning, TEXT("[IA] finding enemy..."));

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundActors);

    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter) return nullptr;

    AGameCharacter* ClosestSniper = nullptr;
    AGameCharacter* ClosestOther = nullptr;
    float MinDistanceSniper = FLT_MAX;
    float MinDistanceOther = FLT_MAX;

    for (AActor* Actor : FoundActors)
    {
        AGameCharacter* GameChar = Cast<AGameCharacter>(Actor);
        if (GameChar && !GameChar->bIsAIControlled)
        {
            int32 RowDiff = FMath::Abs(MyCharacter->CurrentRow - GameChar->CurrentRow);
            int32 ColDiff = FMath::Abs(MyCharacter->CurrentColumn - GameChar->CurrentColumn);
            int32 Distance = RowDiff + ColDiff;

            if (GameChar->IsSniper())
            {
                if (Distance < MinDistanceSniper)
                {
                    MinDistanceSniper = Distance;
                    ClosestSniper = GameChar;
                }
            }
            else
            {
                if (Distance < MinDistanceOther)
                {
                    MinDistanceOther = Distance;
                    ClosestOther = GameChar;
                }
            }
        }
    }

    int32 MoveRange = MyCharacter->GetMaxMovement();
    bool bSniperInRange = (MinDistanceSniper <= MoveRange);
    bool bOtherInRange = (MinDistanceOther <= MoveRange);

    // sniper preference
    if (bSniperInRange && bOtherInRange)
    {
        return ClosestSniper;
    }

    if (bSniperInRange) return ClosestSniper;
    if (bOtherInRange) return ClosestOther;

    return (MinDistanceSniper < MinDistanceOther) ? ClosestSniper : ClosestOther;
}


void AMyAIController::ClearCurrentPath()
{
    CurrentPath.Empty();
    CurrentTarget = nullptr;
    UE_LOG(LogTemp, Warning, TEXT("[IA] Path and target cleared."));
}

AGameCharacter* AMyAIController::GetControlledCharacter() const
{
    return Cast<AGameCharacter>(GetPawn());
}

void AMyAIController::TryToEscapeAndAttack(AGameCharacter* Threat)
{
    AGameCharacter* MyCharacter = GetControlledCharacter();
    if (!MyCharacter || !GameMode || !GridManager) return;

    ACell_Actor* StartCell = GridManager->GetCellAt(MyCharacter->CurrentRow, MyCharacter->CurrentColumn);

    ACell_Actor* BestEscapeCell = nullptr;
    int32 MaxCombinedDistance = -1;

    const TArray<ACell_Actor*> AllCells = GridManager->GetAllCells();
    const TArray<AGameCharacter*> PlayerUnits = GameMode->GetPlayerUnits();

    for (ACell_Actor* Cell : AllCells)
    {
        if (Cell && !Cell->bIsOccupied && Cell->CellType != ECellType::Obstacle)
        {
            int32 CombinedDistance = 0;
            for (AGameCharacter* PlayerUnit : PlayerUnits)
            {
                int32 RowDiff = FMath::Abs(Cell->Row - PlayerUnit->CurrentRow);
                int32 ColDiff = FMath::Abs(Cell->Column - PlayerUnit->CurrentColumn);
                CombinedDistance += RowDiff + ColDiff;
            }

            if (CombinedDistance > MaxCombinedDistance)
            {
                TArray<AGameCharacter*> Ignore;
                for (AGameCharacter* Unit : GameMode->GetAIUnits())
                {
                    if (Unit && Unit != MyCharacter)
                    {
                        Ignore.Add(Unit);
                    }
                }

                TArray<ACell_Actor*> Path = GridManager->FindPathAStarAvoidingUnits(StartCell, Cell, Ignore);
                if (Path.Num() > 1 && Path.Num() - 1 <= MyCharacter->GetMaxMovement())
                {
                    MaxCombinedDistance = CombinedDistance;
                    BestEscapeCell = Cell;
                    LastPath = Path;
                }
            }
        }
    }

    if (BestEscapeCell)
    {
        MyCharacter->OnMovementFinished.Clear();
        MyCharacter->OnMovementFinished.AddDynamic(this, &AMyAIController::OnCharacterMovementFinished);
        MyCharacter->HighlightedOriginCell = MyCharacter->CurrentCell;
        MyCharacter->StartStepByStepMovement(LastPath);
        MyCharacter->HasMovedThisTurn = true;

        LastTarget = Threat;
        return;
    }
    else
    {
        MyCharacter->Attack(Threat);
        MyCharacter->HasAttackedThisTurn = true;

        // log
        FString Prefix = TEXT("AI");
        FString UnitCode = MyCharacter->IsSniper() ? TEXT("S") : TEXT("B");
        FString TargetCoord = Threat->CurrentCell ? ConvertToChessNotation(Threat->CurrentCell->Row, Threat->CurrentCell->Column) : TEXT("??");
        int32 Damage = MyCharacter->GetLastDamageDealt();

        FString LogEntry = FString::Printf(TEXT("%s: %s %s %d"), *Prefix, *UnitCode, *TargetCoord, Damage);
        GameMode->AddMoveToLog(LogEntry);

        GameMode->NotifyAIUnitMoved();
    }
}
