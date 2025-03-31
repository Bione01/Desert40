#include "GameCharacter.h"
#include "Grid_Manager.h"
#include "Cell_Actor.h"
#include "Kismet/GameplayStatics.h"

AGameCharacter::AGameCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Valori di default generici, che poi potrai modificare nelle classi derivate o via Blueprint
    MovementRange = 0;
    AttackRange = 0;
    Health = 100;
    DamageMin = 0;
    DamageMax = 0;
    AttackType = EAttackType::Melee;
}

void AGameCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AGameCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AGameCharacter::Attack(AGameCharacter* Target)
{
    if (!Target)
    {
        return;
    }
    
    // Calcola un danno casuale compreso tra DamageMin e DamageMax
    int32 DamageDealt = FMath::RandRange(DamageMin, DamageMax);
    Target->Health -= DamageDealt;
    
    UE_LOG(LogTemp, Log, TEXT("%s ha attaccato %s infliggendo %d danni."), *GetName(), *Target->GetName(), DamageDealt);
    
    // Se la salute del bersaglio scende a zero o sotto, logga che è stato sconfitto
    if (Target->Health <= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("%s è stato sconfitto."), *Target->GetName());
        // Qui potresti aggiungere ulteriori logiche di rimozione o di gestione della sconfitta
    }
}

void AGameCharacter::MoveToCell(ACell_Actor* DestinationCell)
{
    if (!DestinationCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: DestinationCell is null"));
        return;
    }

    AGrid_Manager* GridManager = Cast<AGrid_Manager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGrid_Manager::StaticClass()));
    if (!GridManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: GridManager non trovato!"));
        return;
    }

    FVector StartLocation = GridManager->GetStartLocation();
    float CellStep = GridManager->GetCellStep();

    FVector TargetLocation = StartLocation + FVector(
        DestinationCell->Column * CellStep,
        DestinationCell->Row * CellStep,
        UnitSpawnZOffset // ✅ QUI applichi l'offset
    );

    SetActorLocation(TargetLocation);

    CurrentRow = DestinationCell->Row;
    CurrentColumn = DestinationCell->Column;
    DestinationCell->bIsOccupied = true;

    UE_LOG(LogTemp, Log, TEXT("%s si è mosso alla cella (%d, %d)"), *GetName(), DestinationCell->Row, DestinationCell->Column);
}

int32 AGameCharacter::GetAttackRange() const
{
    return AttackRange;
}

int32 AGameCharacter::GetMaxMovement() const
{
    return MovementRange;
}

void AGameCharacter::MoveToLocation(const FVector& NewLocation)
{
    SetActorLocation(NewLocation);
    UE_LOG(LogTemp, Log, TEXT("%s si è spostato a %s"), *GetName(), *NewLocation.ToString());
}
