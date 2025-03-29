#include "MyAIController.h"
#include "GameCharacter.h"              // Per la definizione completa di AGameCharacter
#include "Kismet/GameplayStatics.h"       // Per UGameplayStatics::GetAllActorsOfClass
#include "Engine/World.h"

void AMyAIController::RunTurn()
{
    if (!GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("AMyAIController::RunTurn - Nessun Pawn controllato."));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("L'IA (%s) inizia il suo turno."), *GetPawn()->GetName());
    
    AGameCharacter* TargetEnemy = FindClosestEnemy();
    if (TargetEnemy)
    {
        UE_LOG(LogTemp, Log, TEXT("Nemico trovato: %s"), *TargetEnemy->GetName());
        ExecuteAction();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Nessun nemico trovato per %s"), *GetPawn()->GetName());
    }
}

AGameCharacter* AMyAIController::FindClosestEnemy()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundActors);

    AGameCharacter* ClosestEnemy = nullptr;
    float MinDistance = FLT_MAX;
    APawn* MyPawn = GetPawn();
    if (!MyPawn)
    {
        return nullptr;
    }
    FVector MyLocation = MyPawn->GetActorLocation();

    for (AActor* Actor : FoundActors)
    {
        AGameCharacter* GameChar = Cast<AGameCharacter>(Actor);
        if (GameChar && GameChar != MyPawn)
        {
            // Puoi aggiungere qui una condizione per differenziare unità giocatore e IA, ad esempio:
            // if (!GameChar->IsPlayerControlled())
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

void AMyAIController::ExecuteAction()
{
    AGameCharacter* MyCharacter = Cast<AGameCharacter>(GetPawn());
    if (!MyCharacter)
    {
        return;
    }

    AGameCharacter* TargetEnemy = FindClosestEnemy();
    if (!TargetEnemy)
    {
        return;
    }

    float Distance = FVector::Dist(MyCharacter->GetActorLocation(), TargetEnemy->GetActorLocation());
    float MyAttackRange = MyCharacter->GetAttackRange();
    
    if (Distance <= MyAttackRange)
    {
        UE_LOG(LogTemp, Log, TEXT("%s attacca %s"), *MyCharacter->GetName(), *TargetEnemy->GetName());
        MyCharacter->Attack(TargetEnemy);
    }
    else
    {
        // Calcola la direzione verso il nemico e sposta l'unità fino al massimo consentito
        FVector Direction = (TargetEnemy->GetActorLocation() - MyCharacter->GetActorLocation()).GetSafeNormal();
        float MoveDistance = MyCharacter->GetMaxMovement();
        FVector NewLocation = MyCharacter->GetActorLocation() + Direction * MoveDistance;
        UE_LOG(LogTemp, Log, TEXT("%s si muove verso %s"), *MyCharacter->GetName(), *TargetEnemy->GetName());
        MyCharacter->MoveToLocation(NewLocation);
    }
}
