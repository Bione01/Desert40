#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MyAIController.generated.h"

/**
 * AIController semplice per un'unità in un gioco a turni.
 */
UCLASS()
class DESERT0_API AMyAIController : public AAIController
{
    GENERATED_BODY()
    
public:
    // Funzione chiamata per eseguire il turno dell'IA
    UFUNCTION(BlueprintCallable, Category = "AI")
    void RunTurn();

protected:
    // Cerca l'unità nemica più vicina. (Assicurati che AGameCharacter sia completamente definito)
    class AGameCharacter* FindClosestEnemy();

    // Esegue l'azione: attacca se in range, altrimenti si muove verso il nemico.
    void ExecuteAction();
};
