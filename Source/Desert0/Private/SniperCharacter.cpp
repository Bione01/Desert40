#include "SniperCharacter.h"
#include "Cell_Actor.h"
#include "Grid_Manager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ASniperCharacter::ASniperCharacter()
{
    MovementRange = 3;
    AttackRange = 10;
    Health = 20;
    DamageMin = 4;
    DamageMax = 8;
    AttackType = EAttackType::Distance;

    CurrentCell = nullptr;
}

void ASniperCharacter::MoveToCell(ACell_Actor* DestinationCell, bool bIgnoreRange /* = false */)
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

    if (CurrentCell && !bIgnoreRange) // ⚠️ rispetta l'argomento
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
        FVector StartLocation = GridManager->GetStartLocation();
        float CellStep = GridManager->GetCellStep();

        FVector TargetLocation = StartLocation + FVector(
            DestinationCell->Column * CellStep,
            DestinationCell->Row * CellStep,
            0.f
        );

        SetActorLocation(TargetLocation);
    }

    CurrentCell = DestinationCell;
    DestinationCell->bIsOccupied = true;

    UE_LOG(LogTemp, Log, TEXT("%s si è mosso alla cella (%d, %d)"), *GetName(), DestinationCell->Row, DestinationCell->Column);
}
