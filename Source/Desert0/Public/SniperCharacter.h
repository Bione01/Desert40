#pragma once

#include "CoreMinimal.h"
#include "GameCharacter.h"
#include "SniperCharacter.generated.h"

class ACell_Actor;

UCLASS()
class DESERT0_API ASniperCharacter : public AGameCharacter
{
    GENERATED_BODY()

public:
    ASniperCharacter();

    virtual void MoveToCell(ACell_Actor* DestinationCell) override;
};
