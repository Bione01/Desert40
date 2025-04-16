#pragma once

#include "CoreMinimal.h"
#include "TurnState.generated.h"

UENUM(BlueprintType)
enum class ETurnState : uint8
{
    TS_PlayerTurn UMETA(DisplayName = "player turn"),
    TS_EnemyTurn  UMETA(DisplayName = "enemy turn")
    
};
