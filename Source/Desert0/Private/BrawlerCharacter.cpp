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

void ABrawlerCharacter::MoveToCell(ACell_Actor* DestinationCell, bool bIgnoreRange /* puoi anche ignorarlo se non ti serve */)
{
    if (!DestinationCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: DestinationCell is null"));
        return;
    }

    if (DestinationCell->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: La cella (%d, %d) è occupata"), DestinationCell->Row, DestinationCell->Column);
        return;
    }

    if (CurrentCell && !bIgnoreRange) // solo se bIgnoreRange == false controlli la distanza
    {
        int32 DeltaRow = FMath::Abs(DestinationCell->Row - CurrentCell->Row);
        int32 DeltaCol = FMath::Abs(DestinationCell->Column - CurrentCell->Column);
        int32 ManhattanDistance = DeltaRow + DeltaCol;

        if (ManhattanDistance > MovementRange)
        {
            UE_LOG(LogTemp, Warning, TEXT("MoveToCell: La cella (%d, %d) è fuori dal range di movimento"), DestinationCell->Row, DestinationCell->Column);
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
    
    UE_LOG(LogTemp, Log, TEXT("%s si è mosso alla cella (%d, %d)"), *GetName(), DestinationCell->Row, DestinationCell->Column);
}
