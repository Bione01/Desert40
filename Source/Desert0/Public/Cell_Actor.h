#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Cell_Actor.generated.h"

UENUM(BlueprintType)
enum class ECellType : uint8 {
    Normal    UMETA(DisplayName = "Normal"),
    Obstacle  UMETA(DisplayName = "Obstacle")
};

UCLASS()
class DESERT0_API ACell_Actor : public AActor
{
    GENERATED_BODY()
    
public:
    // Costruttore: imposta i valori di default
    ACell_Actor();

protected:
    // Chiamato quando il gioco inizia o l'attore viene spawnato
    virtual void BeginPlay() override;

public:
    // Chiamato ad ogni frame
    virtual void Tick(float DeltaTime) override;

    // Coordinate della cella nella griglia
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 Row;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 Column;

    // Tipo di cella (Normale o Ostacolo)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    ECellType CellType;

    // Stato di occupazione della cella
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bIsOccupied;

    // Riferimento all'attore che occupa la cella, se presente
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    AActor* OccupyingUnit;
    
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    bool bIsHighlighted;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
    UStaticMeshComponent* MyMesh;
    
    UFUNCTION(BlueprintCallable)
    void SetHighlight(bool bHighlight);
    
    UPROPERTY(EditAnywhere)
    UMaterialInterface* DefaultMaterial;

    UPROPERTY(EditAnywhere)
    UMaterialInterface* HighlightMaterial;


};
