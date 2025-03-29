#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TurnState.h" // Contiene l'enum ETurnState
#include "MyGameModebase.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    GP_Placement UMETA(DisplayName = "Fase di Posizionamento"),
    GP_Battle UMETA(DisplayName = "Fase di Battaglia")
};

UCLASS()
class DESERT0_API AMyGameModebase : public AGameModeBase
{
    GENERATED_BODY()
    
public:
    AMyGameModebase();

    virtual void BeginPlay() override;

    // Stato corrente dei turni in battaglia
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turni")
    ETurnState CurrentTurn;

    // Fase attuale della partita
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Phase")
    EGamePhase CurrentPhase;

    // Funzioni per la gestione dei turni in battaglia
    UFUNCTION(BlueprintCallable, Category = "Turni")
    void StartPlayerTurn();

    UFUNCTION(BlueprintCallable, Category = "Turni")
    void StartEnemyTurn();

    UFUNCTION(BlueprintCallable, Category = "Turni")
    void EndTurn();

    // Notifiche di posizionamento (chiamate dal PlayerController o dall'IA)
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyPlayerUnitPlaced();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyAIUnitPlaced();
    
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void PlacePlayerUnit();

protected:
    // Funzione che esegue le azioni dell'IA durante il turno in battaglia
    void ExecuteEnemyTurn();

    // Timer handle per ritardare l'azione dell'IA
    FTimerHandle EnemyTurnTimerHandle;

    // Riferimento al Grid Manager (già presente nella tua logica)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid")
    TSubclassOf<class AGrid_Manager> GridManagerClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    AGrid_Manager* GridManager;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    TSubclassOf<class AGameCharacter> AIUnitClass;
    
    // Contatori per il posizionamento: 2 unità per ciascun lato
    int32 PlayerUnitsPlaced;
    int32 AIUnitsPlaced;

    // Esito del lancio di moneta per decidere chi posiziona per primo
    // true se il giocatore inizia, false se inizia l'IA
    bool bPlayerStartsPlacement;

    // Funzione per simulare il posizionamento di una unità da parte dell'IA durante la fase di posizionamento
    void PlaceAIUnit();
};
