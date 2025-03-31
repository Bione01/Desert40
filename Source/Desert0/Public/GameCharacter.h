#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameCharacter.generated.h"

UENUM(BlueprintType)
enum class EAttackType : uint8
{
    Distance    UMETA(DisplayName = "Attacco a Distanza"),
    Melee       UMETA(DisplayName = "Attacco a Corto Raggio")
};

UCLASS(Abstract)
class DESERT0_API AGameCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // Costruttore
    AGameCharacter();

    // Funzione chiamata ogni frame
    virtual void Tick(float DeltaTime) override;

    // Funzione chiamata all'inizio del gioco o quando l'attore viene spawnato
    virtual void BeginPlay() override;

    // ---- Statistiche dell'unità ----

    // Numero di celle massime che può muovere
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MovementRange;

    // Range d'attacco (numero di celle)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 AttackRange;

    // Punti vita dell'unità
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Health;

    // Danno minimo e massimo
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 DamageMin;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 DamageMax;

    // Tipo di attacco (distanza o melee)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    EAttackType AttackType;
    
    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    int32 CurrentRow;

    UPROPERTY(BlueprintReadWrite, Category = "Grid")
    int32 CurrentColumn;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    float UnitSpawnZOffset = 50.f;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bIsAIControlled = false;

    // ---- Funzioni ----

    // Attacco verso un altro personaggio
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void Attack(AGameCharacter* Target);

    // Movimento verso una cella
    UFUNCTION(BlueprintCallable, Category = "Movement")
    virtual void MoveToCell(class ACell_Actor* DestinationCell);

    // ---- Funzioni per IA e Gameplay ----

    // Restituisce il range d'attacco dell'unità
    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    int32 GetAttackRange() const;

    // Restituisce il range di movimento massimo dell'unità
    UFUNCTION(BlueprintCallable, Category = "Unit Stats")
    int32 GetMaxMovement() const;

    // Muove l'unità a una nuova posizione
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToLocation(const FVector& NewLocation);
};
