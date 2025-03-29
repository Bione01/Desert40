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

    // Funzione chiamata ad ogni frame
    virtual void Tick(float DeltaTime) override;

    // Funzione chiamata all'inizio del gioco o quando l'attore viene spawnato
    virtual void BeginPlay() override;

    // Range di movimento (numero di celle)
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

    // Tipo di attacco (a distanza o a corto raggio)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    EAttackType AttackType;

    // Funzione per effettuare un attacco su un altro personaggio
    UFUNCTION(BlueprintCallable, Category = "Combat")
    virtual void Attack(AGameCharacter* Target);
    
    // Funzione per muoversi verso una cella specificata
    UFUNCTION(BlueprintCallable, Category = "Movement")
    virtual void MoveToCell(class ACell_Actor* DestinationCell);

    // ---- Nuove funzioni richieste dall'IA ----

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
