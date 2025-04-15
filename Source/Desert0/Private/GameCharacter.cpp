#include "GameCharacter.h"
#include "Grid_Manager.h"
#include "Cell_Actor.h"
#include "MyGameModebase.h"
#include "BrawlerCharacter.h"
#include "SniperCharacter.h"
#include "MyGameModebase.h"
#include "Kismet/GameplayStatics.h"

AGameCharacter::AGameCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Valori di default generici, che poi potrai modificare nelle classi derivate o via Blueprint
    MovementRange = 0;
    AttackRange = 0;
    MaxHealth = 100;
    Health = MaxHealth;
    DamageMin = 0;
    DamageMax = 0;
    AttackType = EAttackType::Melee;
}

void AGameCharacter::BeginPlay()
{
    Super::BeginPlay();

    UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
    if (Mesh)
    {
        OriginalMaterial = Mesh->GetMaterial(0);
    }
    
    if (!bIsAIControlled && SpawnSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation());
    }
}

void AGameCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AGameCharacter::MoveToCell(ACell_Actor* DestinationCell, bool bIgnoreRange)
{
    if (!DestinationCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: DestinationCell è null"));
        return;
    }

    if (!bIgnoreRange && !CanReachCell(DestinationCell))
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: La cella (%d, %d) è fuori dal range di movimento"), DestinationCell->Row, DestinationCell->Column);
        return;
    }

    // Libera la cella precedente
    if (CurrentCell)
    {
        CurrentCell->bIsOccupied = false;
        CurrentCell->OccupyingUnit = nullptr;
    }

    // Aggiorna stato logico
    CurrentCell = DestinationCell;
    CurrentRow = DestinationCell->Row;
    CurrentColumn = DestinationCell->Column;
    DestinationCell->bIsOccupied = true;
    DestinationCell->OccupyingUnit = this;

    UE_LOG(LogTemp, Log, TEXT("%s si è mosso alla cella (%d, %d)"), *GetName(), CurrentRow, CurrentColumn);
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

void AGameCharacter::ResetTurnState()
{
    HasMovedThisTurn = false;
    HasAttackedThisTurn = false;
}

bool AGameCharacter::IsBrawler() const
{
    return Cast<ABrawlerCharacter>(this) != nullptr;
}

bool AGameCharacter::IsSniper() const
{
    return Cast<ASniperCharacter>(this) != nullptr;
}

bool AGameCharacter::CanReachCell(const ACell_Actor* DestinationCell) const
{
    if (!DestinationCell) return false;

    int32 RowDiff = FMath::Abs(CurrentRow - DestinationCell->Row);
    int32 ColDiff = FMath::Abs(CurrentColumn - DestinationCell->Column);

    // Movimento ortogonale, tipo Fire Emblem
    return (RowDiff + ColDiff) <= MovementRange;
}
void AGameCharacter::StartStepByStepMovement(const TArray<ACell_Actor*>& PathToFollow)
{
    if (PathToFollow.Num() <= 1)
    {
        OnMovementFinished.Broadcast();
        return;
    }

    // Salva il path (escludendo la cella attuale)
    StepPath = PathToFollow;
    StepPath.RemoveAt(0); // Prima è la cella attuale

    MoveOneStep();
}

void AGameCharacter::MoveOneStep()
{
    if (StepPath.Num() == 0)
    {
        GetWorld()->GetTimerManager().ClearTimer(StepMovementTimer);
        OnMovementFinished.Broadcast();
        return;
    }

    ACell_Actor* NextCell = StepPath[0];
    if (NextCell && NextCell->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MOVEMENT] Movimento bloccato: la prossima cella è occupata da %s."), *NextCell->OccupyingUnit->GetName());
        GetWorld()->GetTimerManager().ClearTimer(StepMovementTimer);
        OnMovementFinished.Broadcast();
        return;
    }

    if (NextCell)
    {
        AGrid_Manager* GridManager = Cast<AGrid_Manager>(UGameplayStatics::GetActorOfClass(GetWorld(), AGrid_Manager::StaticClass()));
        if (!GridManager) return;

        AMyGameModebase* GameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(GetWorld()));
        if (!GameMode) return;

        EndLocation = GameMode->GetCellLocationWithOffset(NextCell);

        StartLocation = GetActorLocation();
        float Distance = FVector::Dist(StartLocation, EndLocation);
        MaxLerpTime = Distance / MovementSpeed;
        CurrentLerpTime = 0.f;

        GetWorld()->GetTimerManager().SetTimer(StepMovementTimer, this, &AGameCharacter::UpdateSmoothMovement, 0.01f, true);
    }
}

void AGameCharacter::UpdateSmoothMovement()
{
    if (CurrentLerpTime >= MaxLerpTime)
    {
        // Fine interpolazione
        FVector FinalLocation = EndLocation;
        AMyGameModebase* GameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
        if (CurrentCell)
        {
            FVector FixedLocation = GameMode->GetCellLocationWithOffset(CurrentCell);
            SetActorLocation(FixedLocation);
        }
// Mantieni Z corretta
        SetActorLocation(FinalLocation);

        // Aggiorna logica SOLO a fine movimento
        if (StepPath.Num() > 0)
        {
            ACell_Actor* NextCell = StepPath[0];

            // Libera la cella precedente
            if (CurrentCell)
            {
                CurrentCell->bIsOccupied = false;
                CurrentCell->OccupyingUnit = nullptr;
            }

            // Aggiorna stato logico
            CurrentCell = NextCell;
            CurrentRow = NextCell->Row;
            CurrentColumn = NextCell->Column;
            NextCell->bIsOccupied = true;
            NextCell->OccupyingUnit = this;
        }

        StepPath.RemoveAt(0);
        GetWorld()->GetTimerManager().ClearTimer(StepMovementTimer);

        // Prossimo step
        MoveOneStep();
        return;
    }

    CurrentLerpTime += 0.01f;
    float Alpha = FMath::Clamp(CurrentLerpTime / MaxLerpTime, 0.f, 1.f);
    FVector NewLocation = FMath::Lerp(StartLocation, EndLocation, Alpha);
    SetActorLocation(NewLocation);
}



void AGameCharacter::ReceiveDamage(int32 DamageAmount)
{
    Health -= DamageAmount;
    
    Health = FMath::Max(0, Health);

    // Effetto visivo
    if (HitEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, GetActorLocation());
    }

    // Suono
    if (HitSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
    }

    if (Health <= 0)
    {
        Die();
    }
}

void AGameCharacter::Die()
{
    UE_LOG(LogTemp, Warning, TEXT("%s è stato sconfitto!"), *GetName());

    //death sound effect
    if (DeathSound)
      {
          UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
      }
    
    // Libera la cella
    if (CurrentCell)
    {
        CurrentCell->bIsOccupied = false;
        CurrentCell->OccupyingUnit = nullptr;
    }

    // Notifica GameMode
    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(GetWorld()->GetAuthGameMode());
    if (MyGameMode)
    {
        MyGameMode->OnUnitKilled(this);
    }

    Destroy();
}

void AGameCharacter::Attack(AGameCharacter* Target)
{
    if (!Target) return;

    int32 DamageDealt = FMath::RandRange(DamageMin, DamageMax);
    LastDamageDealt = DamageDealt;

    UE_LOG(LogTemp, Log, TEXT("%s attacca %s infliggendo %d danni."), *GetName(), *Target->GetName(), DamageDealt);

    Target->ReceiveDamage(DamageDealt);

    // 🔁 Contrattacco
    Target->HandleCounterAttack(this);
}

void AGameCharacter::HandleCounterAttack(AGameCharacter* Attacker)
{
    if (!Attacker) return;

    // Solo se chi attacca è Sniper
    ASniperCharacter* AttackingSniper = Cast<ASniperCharacter>(Attacker);
    if (!AttackingSniper) return;

    // Il contrattacco avviene solo se adiacenti o se anche il difensore è uno Sniper
    int32 RowDiff = FMath::Abs(CurrentRow - Attacker->CurrentRow);
    int32 ColDiff = FMath::Abs(CurrentColumn - Attacker->CurrentColumn);
    bool bIsAdjacent = (RowDiff + ColDiff == 1);

    if (IsSniper() || bIsAdjacent)
    {
        int32 CounterDamage = FMath::RandRange(1, 3);
        UE_LOG(LogTemp, Warning, TEXT("[COUNTERATTACK] %s contrattacca %s infliggendo %d danni."),
            *GetName(), *Attacker->GetName(), CounterDamage);

        Attacker->ReceiveDamage(CounterDamage);
        FString Prefix = TEXT("CP");
        FString UnitCode = IsSniper() ? TEXT("S") : TEXT("B");
        FString FromCoord = CurrentCell ? CurrentCell->CellName : TEXT("??");
        FString ToCoord = Attacker->CurrentCell ? Attacker->CurrentCell->CellName : TEXT("??");

        FString LogEntry = FString::Printf(TEXT("%s: %s %s -> %s"), *Prefix, *UnitCode, *FromCoord, *ToCoord);

        AMyGameModebase* MyGM = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
        /*if (MyGM)
        {
            MyGM->AddMoveToLog(LogEntry);
        }*/

        Attacker->PlayCounterHitFlash(); // 🔥 Effetto visivo aggiunto
    }
}

void AGameCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("%s è stato sconfitto!"), *GetName());

    if (CurrentCell)
    {
        CurrentCell->bIsOccupied = false;
        CurrentCell->OccupyingUnit = nullptr;
    }

    AMyGameModebase* MyGameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
    if (MyGameMode)
    {
        MyGameMode->OnUnitKilled(this);
    }

    Destroy();
}
void AGameCharacter::PlayCounterHitFlash()
{
    UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
    if (Mesh && CounterHitMaterial)
    {
        Mesh->SetMaterial(0, CounterHitMaterial);

        GetWorld()->GetTimerManager().SetTimer(
            CounterFlashTimer,
            this,
            &AGameCharacter::EndCounterHitFlash,
            0.3f,
            false
        );
    }
}

void AGameCharacter::EndCounterHitFlash()
{
    UStaticMeshComponent* Mesh = FindComponentByClass<UStaticMeshComponent>();
    if (Mesh && OriginalMaterial)
    {
        Mesh->SetMaterial(0, OriginalMaterial);
    }
}
