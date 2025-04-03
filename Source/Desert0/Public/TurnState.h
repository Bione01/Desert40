#pragma once

#include "CoreMinimal.h"
#include "TurnState.generated.h"

UENUM(BlueprintType)
enum class ETurnState : uint8
{
    TS_PlayerTurn UMETA(DisplayName = "Turno Giocatore"),
    TS_EnemyTurn  UMETA(DisplayName = "Turno Nemico")
    
};
