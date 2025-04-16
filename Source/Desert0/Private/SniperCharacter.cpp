#include "SniperCharacter.h"
#include "MyGameModebase.h"
#include "Cell_Actor.h"
#include "Grid_Manager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "MyGameModebase.h"

ASniperCharacter::ASniperCharacter()
{
    MovementRange = 3;
    AttackRange = 10;
    MaxHealth = 20;
    Health = MaxHealth;
    DamageMin = 4;
    DamageMax = 8;
    AttackType = EAttackType::Distance;

    CurrentCell = nullptr;
}

void ASniperCharacter::MoveToCell(ACell_Actor* DestinationCell, bool bIgnoreRange)
{
    if (!DestinationCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: DestinationCell is null"));
        return;
    }

    if (DestinationCell->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: cell (%d, %d) is occupata"), DestinationCell->Row, DestinationCell->Column);
        return;
    }

    if (CurrentCell && !bIgnoreRange)
    {
        int32 DeltaRow = FMath::Abs(DestinationCell->Row - CurrentCell->Row);
        int32 DeltaCol = FMath::Abs(DestinationCell->Column - CurrentCell->Column);
        int32 ManhattanDistance = DeltaRow + DeltaCol;

        if (ManhattanDistance > MovementRange)
        {
            UE_LOG(LogTemp, Warning, TEXT("MoveToCell: cell (%d, %d) out of movement range"), DestinationCell->Row, DestinationCell->Column);
            return;
        }

        CurrentCell->bIsOccupied = false;
        CurrentCell->OccupyingUnit = nullptr;
    }

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(GetWorld()));
    if (MyGameMode)
    {
        FVector TargetLocation = MyGameMode->GetCellLocationWithOffset(DestinationCell);
        SetActorLocation(TargetLocation);
    }

    CurrentCell = DestinationCell;
    DestinationCell->bIsOccupied = true;
    DestinationCell->OccupyingUnit = this;

    UE_LOG(LogTemp, Log, TEXT("%s move to cell (%d, %d)"), *GetName(), DestinationCell->Row, DestinationCell->Column);
}
