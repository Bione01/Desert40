#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MyGameModebase.h"
#include "MyAIController.generated.h"

UCLASS()
class DESERT0_API AMyAIController : public AAIController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    // Funzione chiamata per eseguire il turno dell'IA
    UFUNCTION(BlueprintCallable, Category = "AI")
    void RunTurn();

    // Funzione per eseguire l'azione dell'IA (es. attacco o movimento)
    void ExecuteAction();  // Aggiungi questa dichiarazione

    // Funzione per trovare il nemico più vicino
    AGameCharacter* FindClosestEnemy();

protected:
    // Variabile per il GameMode
    UPROPERTY()
    class AMyGameModebase* GameMode;
};
