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

    // Default value, blueprint editable
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
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: DestinationCell is null"));
        return;
    }

    if (!bIgnoreRange && !CanReachCell(DestinationCell))
    {
        UE_LOG(LogTemp, Warning, TEXT("MoveToCell: Cell (%d, %d) out of movement range"), DestinationCell->Row, DestinationCell->Column);
        return;
    }

    // clear previous Cell
    if (CurrentCell)
    {
        CurrentCell->bIsOccupied = false;
        CurrentCell->OccupyingUnit = nullptr;
    }

    // update logic state
    CurrentCell = DestinationCell;
    CurrentRow = DestinationCell->Row;
    CurrentColumn = DestinationCell->Column;
    DestinationCell->bIsOccupied = true;
    DestinationCell->OccupyingUnit = this;

    UE_LOG(LogTemp, Log, TEXT("%s move to Cell (%d, %d)"), *GetName(), CurrentRow, CurrentColumn);
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
    UE_LOG(LogTemp, Log, TEXT("%s moved to %s"), *GetName(), *NewLocation.ToString());
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

    // Movement
    return (RowDiff + ColDiff) <= MovementRange;
}
void AGameCharacter::StartStepByStepMovement(const TArray<ACell_Actor*>& PathToFollow)
{
    if (PathToFollow.Num() <= 1)
    {
        OnMovementFinished.Broadcast();
        return;
    }

    // Save path
    StepPath = PathToFollow;
    StepPath.RemoveAt(0);

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
        UE_LOG(LogTemp, Warning, TEXT("[MOVEMENT] Movment block: next cell is occupy by %s."), *NextCell->OccupyingUnit->GetName());
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
        GetWorld()->GetTimerManager().ClearTimer(StepMovementTimer);

        // update before placement
        if (StepPath.Num() > 0)
        {
            ACell_Actor* NextCell = StepPath[0];

            // clear previous
            if (CurrentCell)
            {
                CurrentCell->bIsOccupied = false;
                CurrentCell->OccupyingUnit = nullptr;
            }

            // update Current cell
            CurrentCell = NextCell;
            CurrentRow = NextCell->Row;
            CurrentColumn = NextCell->Column;
            NextCell->bIsOccupied = true;
            NextCell->OccupyingUnit = this;
        }

        // center
        AMyGameModebase* GameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
        if (GameMode && CurrentCell)
        {
            FVector FixedLocation = GameMode->GetCellLocationWithOffset(CurrentCell);
            SetActorLocation(FixedLocation);
        }

        StepPath.RemoveAt(0);

        // next cell
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

    // visual effect
    if (HitEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitEffect, GetActorLocation());
    }

    // sound
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
    UE_LOG(LogTemp, Warning, TEXT("%s is dead!"), *GetName());

    //death sound effect
    if (DeathSound)
      {
          UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
      }
    
    // clear cell
    if (CurrentCell)
    {
        CurrentCell->bIsOccupied = false;
        CurrentCell->OccupyingUnit = nullptr;
    }

    // notify Gamemode
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

    UE_LOG(LogTemp, Log, TEXT("%s attack %s dealing %d dmg."), *GetName(), *Target->GetName(), DamageDealt);

    Target->ReceiveDamage(DamageDealt);

    // counterattack
    Target->HandleCounterAttack(this);
}

void AGameCharacter::HandleCounterAttack(AGameCharacter* Attacker)
{
    if (!Attacker || !IsValid(Attacker)) return;

    ASniperCharacter* AttackingSniper = Cast<ASniperCharacter>(Attacker);
    if (!AttackingSniper) return;

    int32 RowDiff = FMath::Abs(CurrentRow - Attacker->CurrentRow);
    int32 ColDiff = FMath::Abs(CurrentColumn - Attacker->CurrentColumn);
    bool bIsAdjacent = (RowDiff + ColDiff == 1);

    if (IsSniper() || bIsAdjacent)
    {
        int32 CounterDamage = FMath::RandRange(1, 3);
        UE_LOG(LogTemp, Log, TEXT("%s counterattack %s dealing %d dmg."),
            *GetName(), *Attacker->GetName(), CounterDamage);

        Attacker->ReceiveDamage(CounterDamage);

        if (IsValid(Attacker))
        {
            Attacker->PlayCounterHitFlash();
        }
    }
}


void AGameCharacter::HandleDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("%s is dead!"), *GetName());

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
