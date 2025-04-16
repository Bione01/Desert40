#include "BrawlerCharacter.h"
#include "Cell_Actor.h"
#include "MyGameModebase.h"
#include "Grid_Manager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ABrawlerCharacter::ABrawlerCharacter()
{
    MovementRange = 6;
    AttackRange = 1;
    MaxHealth = 40;
    Health = MaxHealth;
    DamageMin = 1;
    DamageMax = 6;
    AttackType = EAttackType::Melee;

    CurrentCell = nullptr;
}

void ABrawlerCharacter::MoveToCell(ACell_Actor* DestinationCell, bool bIgnoreRange)
{
    if (!DestinationCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: DestinationCell is null"));
        return;
    }

    if (DestinationCell->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: cell (%d, %d) is occupy"), DestinationCell->Row, DestinationCell->Column);
        return;
    }

    if (CurrentCell && !bIgnoreRange)
    {
        int32 DeltaRow = FMath::Abs(DestinationCell->Row - CurrentCell->Row);
        int32 DeltaCol = FMath::Abs(DestinationCell->Column - CurrentCell->Column);
        int32 ManhattanDistance = DeltaRow + DeltaCol;

        if (ManhattanDistance > MovementRange)
        {
            UE_LOG(LogTemp, Warning, TEXT("MoveToCell: Cell (%d, %d) is out of movment range"), DestinationCell->Row, DestinationCell->Column);
            return;
        }

        CurrentCell->bIsOccupied = false;
    }

    AGrid_Manager* GridManager = Cast<AGrid_Manager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGrid_Manager::StaticClass()));
    if (GridManager)
    {
        AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(GetWorld()));
        if (MyGameMode)
        {
            FVector TargetLocation = MyGameMode->GetCellLocationWithOffset(DestinationCell);
            SetActorLocation(TargetLocation);
        }
    }

    CurrentCell = DestinationCell;
    DestinationCell->bIsOccupied = true;
    DestinationCell->OccupyingUnit = this;
    
    UE_LOG(LogTemp, Log, TEXT("%s moved to cell (%d, %d)"), *GetName(), DestinationCell->Row, DestinationCell->Column);
}
