#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class UUserWidget;
class ACell_Actor;
class AGameCharacter;
class AGrid_Manager;

UCLASS()
class DESERT0_API AMyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    // widget
    
    UFUNCTION()
    void SetCharacterSelectionVisibility(bool bVisible);

    UFUNCTION(BlueprintCallable)
     void HideCharacterSelectionWidget();
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> CharacterSelectionWidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UUserWidget* CharacterSelectionWidget;

    // character class
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characters")
    TSubclassOf<AGameCharacter> SniperCharacterClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characters")
    TSubclassOf<AGameCharacter> BrawlerCharacterClass;

    // placement
    UPROPERTY(BlueprintReadWrite, Category = "Placement")
    int32 PlacementCount = 0;

    UPROPERTY(BlueprintReadWrite)
    bool bIsSniperPlaced = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsBrawlerPlaced = false;

    UPROPERTY(BlueprintReadWrite, Category = "Characters")
    FName SelectedCharacterType;

    UPROPERTY(BlueprintReadWrite, Category = "Spawn")
    FVector DesiredSpawnLocation;

    UPROPERTY(BlueprintReadOnly, Category = "Characters")
    AGameCharacter* SpawnedSniper;

    UPROPERTY(BlueprintReadOnly, Category = "Characters")
    AGameCharacter* SpawnedBrawler;

    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    AGrid_Manager* GridManager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float CellSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float MovementRange = 2.f;

    UPROPERTY()
    bool bIsMoving = false;

    UPROPERTY()
    AGameCharacter* SelectedCharacter = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn State")
    bool bIsWaitingForAttack = false;

    // function
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void OnCharacterSelected(FName CharacterType);

    UFUNCTION(BlueprintCallable, Category = "Input")
    AGameCharacter* GetClickedUnit();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void HighlightReachableCells(AGameCharacter* CharacterToHighlight);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ClearHighlights();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void RefreshCellOccupancy();
    
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void HighlightEnemyCellsInRange();

    UFUNCTION()
    void OnPlayerMovementFinished();
    
    UFUNCTION()
    void OnPlayerMovementFinishedAndCheckAttack();

    UFUNCTION()
    void CheckEndOfPlayerUnitTurn();

    UFUNCTION()
    void DeselectCurrentUnit();
    
    UFUNCTION()
    void HandlePlacementClick(class AMyGameModebase* MyGameMode);
    
    UFUNCTION()
    void UpdateHighlights();
    
    UFUNCTION()
    void SkipCurrentAttack();

    void SetGameInputMode(bool bGameOnly);

protected:
    void HandleLeftMouseClick();
    ACell_Actor* GetClickedCell();
    void ShowCharacterSelectionWidget();

    float ZOffset = 5.f;
    
    
};
