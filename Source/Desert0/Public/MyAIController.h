#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MyAIController.generated.h"

class AGameCharacter;
class AGrid_Manager;
class ACell_Actor;
class AMyGameModebase;

UCLASS()
class DESERT0_API AMyAIController : public AAIController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    void RunTurn();
    void ClearCurrentPath(); // La funzione ClearCurrentPath è dichiarata una sola volta qui

private:
    AGameCharacter* FindClosestEnemy();

    // Funzione per aggiornare il target
    void UpdateTarget(AGameCharacter* NewTarget);

    UPROPERTY()
    AMyGameModebase* GameMode;

    UPROPERTY()
    AGrid_Manager* GridManager;

    UPROPERTY()
    AGameCharacter* CurrentTarget;

    UPROPERTY()
    TArray<ACell_Actor*> CurrentPath;
    
    UPROPERTY()
    AGameCharacter* LastTarget = nullptr;

    UPROPERTY()
    TArray<ACell_Actor*> LastPath;
    
    UFUNCTION()
    void OnCharacterMovementFinished();
    
    UFUNCTION()
    void TryToEscapeAndAttack(AGameCharacter* Threat);

    AGameCharacter* GetControlledCharacter() const;
};
