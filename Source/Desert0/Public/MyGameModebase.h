#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TurnState.h"
#include "GameCharacter.h"
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

    // === FASI DI GIOCO ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Phase")
    EGamePhase CurrentPhase;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turni")
    ETurnState CurrentTurn;

    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    void StartBattlePhase();

    UFUNCTION(BlueprintCallable, Category = "Turni")
    void StartPlayerTurn();

    UFUNCTION(BlueprintCallable, Category = "Turni")
    void StartEnemyTurn();

    UFUNCTION(BlueprintCallable, Category = "Turni")
    void EndTurn();

    // === POSIZIONAMENTO ===
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyPlayerUnitPlaced();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyAIUnitPlaced();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void PlacePlayerUnit();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void PlaceAIUnit();

    // === AI TURN ===
    void ExecuteEnemyTurn();

    UFUNCTION(BlueprintCallable)
    void NotifyPlayerUnitMoved();

    UFUNCTION(BlueprintCallable)
    void NotifyAIUnitMoved();

    // === ACCESSORS ===
    FORCEINLINE const TArray<AGameCharacter*>& GetPlayerUnits() const { return PlayerUnits; }
    FORCEINLINE const TArray<AGameCharacter*>& GetAIUnits() const { return AIUnits; }

    // === VARIABILI ===
    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    float UnitSpawnZOffset = 50.f;

    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    int32 AIRows = 2;

    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    int32 PlayerRows = 2;

    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    int32 MaxUnitsPerSide = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Units")
    TSubclassOf<AGameCharacter> BrawlerCharacter;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Units")
    TSubclassOf<AGameCharacter> SniperCharacter;
    
    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    void AddPlayerUnit(AGameCharacter* Unit);


protected:

    FTimerHandle EnemyTurnTimerHandle;

    AGameCharacter* CurrentAIUnit;

    void ResetUnitsMoved();

    TArray<AGameCharacter*> GetAllAIUnits();

    UPROPERTY()
    TArray<AGameCharacter*> AIUnits;

    UPROPERTY()
    TArray<AGameCharacter*> PlayerUnits;
    
    void MoveNextAIUnit();
    
    UPROPERTY()
    int32 CurrentAIUnitIndex;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grid")
    TSubclassOf<class AGrid_Manager> GridManagerClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    AGrid_Manager* GridManager;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    TArray<TSubclassOf<AGameCharacter>> AIUnitClasses;

    int32 PlayerUnitsPlaced;
    int32 AIUnitsPlaced;
    int32 PlayerUnitsMoved;
    int32 AIUnitsMoved;

    bool bPlayerStartsPlacement;
};
