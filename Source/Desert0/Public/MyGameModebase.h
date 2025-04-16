
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TurnState.h"
#include "HealthBarPanelWidget.h"
#include "MoveLogWidget.h"
#include "TurnImageWidget.h"
#include "GameCharacter.h"
#include "Blueprint/UserWidget.h"
#include "MyGameModebase.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    GP_Placement UMETA(DisplayName = "Fase di Posizionamento"),
    GP_Battle UMETA(DisplayName = "Fase di Battaglia"),
    GP_End       UMETA(DisplayName = "End")
};

UENUM(BlueprintType)
enum class EGameDifficulty : uint8
{
    GD_Easy     UMETA(DisplayName = "Facile"),
    GD_Hard     UMETA(DisplayName = "Difficile")
};

UCLASS()
class DESERT0_API AMyGameModebase : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMyGameModebase();

    virtual void BeginPlay() override;
    
    FVector GetCellLocationWithOffset(ACell_Actor* Cell) const;
    
    void DisablePlayerInput();
    void EnablePlayerInput();
    
    ETurnState GetCurrentTurn() const { return CurrentTurn; }
    EGamePhase GetCurrentPhase() const { return CurrentPhase; }

    bool bIsPlayerInputEnabled = false;

    // game phases
    
    UPROPERTY(EditDefaultsOnly, Category = "Coin Flip")
    TSubclassOf<class ACoinFlipActor> CoinFlipActorClass;
    
    UPROPERTY()
    class UTurnImageWidget* TurnImageWidget;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UUserWidget> TurnImageWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Phase")
    EGamePhase CurrentPhase;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn")
    ETurnState CurrentTurn;

    UFUNCTION(BlueprintCallable, Category = "Game Mode")
    void StartBattlePhase();

    UFUNCTION(BlueprintCallable, Category = "Turn")
    void StartPlayerTurn();

    UFUNCTION(BlueprintCallable, Category = "Turn")
    void StartEnemyTurn();

    UFUNCTION(BlueprintCallable, Category = "Turn")
    void EndTurn();

    // placement
    UFUNCTION(BlueprintCallable)
    void StartPlacementPhase();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyPlayerUnitPlaced();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void NotifyAIUnitPlaced();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void PlacePlayerUnit();

    UFUNCTION(BlueprintCallable, Category = "Placement")
    void PlaceAIUnit();

    UFUNCTION(BlueprintCallable)
    void AddMoveToLog(const FString& MoveText);
    
    UPROPERTY()
    UHealthBarPanelWidget* HealthBarPanelWidget;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UHealthBarPanelWidget> HealthBarPanelWidgetClass;
    
    UPROPERTY()
    AGameCharacter* LastSpawnedPlayerUnit;

    UPROPERTY()
    AGameCharacter* LastSpawnedAIUnit;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mode")
    EGameDifficulty GameDifficulty = EGameDifficulty::GD_Hard;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsHardMode = true;

    // AI turn
    void ExecuteEnemyTurn();

    UFUNCTION(BlueprintCallable)
    void NotifyPlayerUnitMoved();

    UFUNCTION(BlueprintCallable)
    void NotifyAIUnitMoved();

    FORCEINLINE const TArray<AGameCharacter*>& GetPlayerUnits() const { return PlayerUnits; }
    FORCEINLINE const TArray<AGameCharacter*>& GetAIUnits() const { return AIUnits; }

    // variables
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
    
    UPROPERTY(BlueprintReadOnly)
    bool bPlayerStartsPlacement;
    
    UPROPERTY()
    UMoveLogWidget* MoveLogWidget;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UMoveLogWidget> MoveLogWidgetClass;
    
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> EndGameWidgetClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* VictorySound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* DefeatSound;

    UPROPERTY()
    UUserWidget* EndGameWidget;

    int32 CurrentAIIndex;
    
    UFUNCTION(BlueprintCallable)
    void AddPlayerUnit(AGameCharacter* Unit);
   
    UFUNCTION()
    void PlaceAIUnit_Internal();

    UFUNCTION()
    void StartTurn();
    
    UFUNCTION()
    void ExecuteNextAIUnit();

    UFUNCTION()
    void OnAIMovementFinished();

    void OnUnitKilled(AGameCharacter* DeadUnit);
    
    UFUNCTION()
    void EndGame(bool bPlayerWon);

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
};
