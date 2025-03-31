// MyAIController.cpp
#include "MyAIController.h"
#include "GameCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Grid_Manager.h"
#include "Cell_Actor.h"
#include "MyGameModebase.h"

void AMyAIController::BeginPlay()
{
    Super::BeginPlay();

    // Ottieni il GameMode
    GameMode = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(GetWorld()));
    if (!GameMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameMode non trovato!"));
    }
}

AGameCharacter* AMyAIController::FindClosestEnemy()
{
    // Recupera tutti gli attori di tipo AGameCharacter nel mondo
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundActors);

    AGameCharacter* ClosestEnemy = nullptr;
    float MinDistance = FLT_MAX;

    // Ottieni la posizione del personaggio controllato dall'IA
    APawn* MyPawn = GetPawn();
    if (!MyPawn)
    {
        return nullptr;
    }

    FVector MyLocation = MyPawn->GetActorLocation();

    // Itera sugli attori trovati e trova il più vicino
    for (AActor* Actor : FoundActors)
    {
        AGameCharacter* GameChar = Cast<AGameCharacter>(Actor);
        if (GameChar && GameChar != MyPawn) // Escludi il proprio personaggio
        {
            float Dist = FVector::Dist(MyLocation, GameChar->GetActorLocation());
            if (Dist < MinDistance)
            {
                MinDistance = Dist;
                ClosestEnemy = GameChar;
            }
        }
    }
    return ClosestEnemy;
}

void AMyAIController::RunTurn()
{
    if (!GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("AMyAIController::RunTurn - Nessun Pawn controllato."));
        return;
    }

    AGameCharacter* TargetEnemy = FindClosestEnemy(); // Assicurati che FindClosestEnemy sia chiamata correttamente
    if (TargetEnemy)
    {
        // Usa GetName() per ottenere il nome dell'unità come stringa
        UE_LOG(LogTemp, Log, TEXT("Nemico trovato: %s"), *TargetEnemy->GetName());
        ExecuteAction();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Nessun nemico trovato"));
    }
}

void AMyAIController::ExecuteAction()
{
    AGameCharacter* MyCharacter = Cast<AGameCharacter>(GetPawn());
    if (!MyCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("No character controlled by AI."));
        return;
    }

    AGameCharacter* TargetEnemy = FindClosestEnemy();
    if (!TargetEnemy)
    {
        UE_LOG(LogTemp, Warning, TEXT("No enemy found for action."));
        return;
    }

    float Distance = FVector::Dist(MyCharacter->GetActorLocation(), TargetEnemy->GetActorLocation());
    float AttackRange = MyCharacter->GetAttackRange();

    if (Distance <= AttackRange)
    {
        // Attacco al nemico
        UE_LOG(LogTemp, Log, TEXT("%s attacca %s"), *MyCharacter->GetName(), *TargetEnemy->GetName());
        MyCharacter->Attack(TargetEnemy);
    }
    else
    {
        // Movimento verso il nemico
        FVector Direction = (TargetEnemy->GetActorLocation() - MyCharacter->GetActorLocation()).GetSafeNormal();
        FVector NewLocation = MyCharacter->GetActorLocation() + Direction * MyCharacter->GetMaxMovement();
        UE_LOG(LogTemp, Log, TEXT("%s si muove verso %s"), *MyCharacter->GetName(), *TargetEnemy->GetName());
        MyCharacter->MoveToLocation(NewLocation);
    }
}
