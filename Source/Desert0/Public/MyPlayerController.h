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

    // Widget per la selezione dei personaggi
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> CharacterSelectionWidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    UUserWidget* CharacterSelectionWidget;

    // Classi dei personaggi da spawnare
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characters")
    TSubclassOf<AGameCharacter> SniperCharacterClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Characters")
    TSubclassOf<AGameCharacter> BrawlerCharacterClass;

    // Tipo di personaggio selezionato
    UPROPERTY(BlueprintReadWrite, Category = "Characters")
    FName SelectedCharacterType;

    // Posizione desiderata per il posizionamento
    UPROPERTY(BlueprintReadWrite, Category = "Spawn")
    FVector DesiredSpawnLocation;

    // Personaggi spawnati
    UPROPERTY(BlueprintReadOnly, Category = "Characters")
    AGameCharacter* SpawnedSniper;

    UPROPERTY(BlueprintReadOnly, Category = "Characters")
    AGameCharacter* SpawnedBrawler;

    // Stato di piazzamento
    UPROPERTY(BlueprintReadWrite, Category = "Placement")
    int32 PlacementCount;

    UPROPERTY(BlueprintReadWrite)
    bool bIsSniperPlaced = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsBrawlerPlaced = false;

    // Riferimento al Grid Manager
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    AGrid_Manager* GridManager;

    // Valori usati per il calcolo del movimento
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float CellSize = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float MovementRange = 2.f;
    
    UPROPERTY()
    bool bIsMoving = false;

    // Funzioni accessibili dai Blueprint
    UFUNCTION(BlueprintCallable, Category = "Character Selection")
    void OnCharacterSelected(FName CharacterType);

    UFUNCTION(BlueprintCallable, Category = "Input")
    AGameCharacter* GetClickedUnit();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void HighlightReachableCells(AGameCharacter* SelectedCharacter);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ClearHighlights();
    
    UFUNCTION()
    void OnPlayerMovementFinished();

protected:
    void HandleLeftMouseClick();
    ACell_Actor* GetClickedCell();
    void SetGameInputMode();
    void ShowCharacterSelectionWidget();

    // Offset per lo Z spostamento dell'unità (se serve)
    float ZOffset = 5.f;
};
