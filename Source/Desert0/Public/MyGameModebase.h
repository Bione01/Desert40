#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TurnState.h"
#include "GameCharacter.h"
#include "MyGameModebase.generated.h"  // Questa linea deve essere l'ultima nella sezione degli includes.

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

    // Stato corrente dei turni
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turni")
    ETurnState CurrentTurn;

    // Fase attuale della partita
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Phase")
    EGamePhase CurrentPhase;

    // Gestione dei turni in battaglia
    UFUNCTION(BlueprintCallable, Category = "Turni")
    void StartPlayerTurn();

    UFUNCTION(BlueprintCallable, Category = "Turni")
    void StartEnemyTurn();

    UFUNCTION(BlueprintCallable, Category = "Turni")
    void EndTurn();  // Ensure EndTurn() is declared here

    // Posizionamento
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyPlayerUnitPlaced();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyAIUnitPlaced();
    
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void PlacePlayerUnit();

    // Offset Z per far apparire le unità sopra la griglia
    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    float UnitSpawnZOffset = 50.f;
    
    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    int32 GetAIUnitsPlaced() const { return AIUnitsPlaced; }

        
    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    void IncrementAIUnitsPlaced() { AIUnitsPlaced++; }

    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    void StartBattlePhase();

    // Quante righe usa l'IA per il posizionamento
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    int32 AIRows = 2;

    // Quante righe usa il Player per il posizionamento (facoltativo)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    int32 PlayerRows = 2;

    // Numero massimo di unità da piazzare
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    int32 MaxUnitsPerSide = 2;

    // Variabili per le classi di unità Brawler e Sniper
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Units")
    TSubclassOf<AGameCharacter> BrawlerCharacter;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Units")
    TSubclassOf<AGameCharacter> SniperCharacter;

protected:
    void ExecuteEnemyTurn();

    FTimerHandle EnemyTurnTimerHandle;
    
    FTimerHandle AIPlacementTimerHandle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid")
    TSubclassOf<class AGrid_Manager> GridManagerClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    AGrid_Manager* GridManager;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    TArray<TSubclassOf<AGameCharacter>> AIUnitClasses;

    int32 PlayerUnitsPlaced;
    int32 AIUnitsPlaced;

    bool bPlayerStartsPlacement;

    bool bHasSpawnedBrawler; // Aggiungi una variabile per tracciare se l'IA ha spawnato un Brawler
    bool bHasSpawnedSniper;  // Aggiungi una variabile per tracciare se l'IA ha spawnato un Sniper

    void PlaceAIUnit();
};
