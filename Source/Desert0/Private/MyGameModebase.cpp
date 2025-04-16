#include "MyGameModebase.h"
#include "CoinFlipActor.h"
#include "Grid_Manager.h"
#include "HealthBarPanelWidget.h"
#include "MoveLogWidget.h"
#include "Engine/World.h"
#include "MyPlayerController.h"
#include "TurnImageWidget.h"
#include "TimerManager.h"
#include "Cell_Actor.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameCharacter.h"
#include "AIController.h"
#include "MyAIController.h"

AMyGameModebase::AMyGameModebase()
{
    PlayerControllerClass = AMyPlayerController::StaticClass();
    CurrentTurn = ETurnState::TS_PlayerTurn;
    CurrentPhase = EGamePhase::GP_Placement;
    PlayerUnitsPlaced = 0;
    AIUnitsPlaced = 0;
    PlayerUnitsMoved = 0;
    AIUnitsMoved = 0;
}

void AMyGameModebase::BeginPlay()
{
    Super::BeginPlay();
    
    if (MoveLogWidgetClass)
     {
         MoveLogWidget = CreateWidget<UMoveLogWidget>(GetWorld(), MoveLogWidgetClass);
     }
    
    if (HealthBarPanelWidgetClass)
    {
        HealthBarPanelWidget = CreateWidget<UHealthBarPanelWidget>(GetWorld(), HealthBarPanelWidgetClass);
        if (HealthBarPanelWidget)
        {
            HealthBarPanelWidget->AddToViewport(10);
        }
        UE_LOG(LogTemp, Warning, TEXT("healthbar widget: %s"), HealthBarPanelWidget ? TEXT("Created") : TEXT("nullptr"));
    }

    if (GridManagerClass)
    {
        FVector SpawnLocation = FVector::ZeroVector;
        FRotator SpawnRotation = FRotator::ZeroRotator;
        GridManager = GetWorld()->SpawnActor<AGrid_Manager>(GridManagerClass, SpawnLocation, SpawnRotation);
        if (GridManager)
        {
            UE_LOG(LogTemp, Log, TEXT("Grid Manager successfully spawned."));
        }
    }

    // coin flip
    if (CoinFlipActorClass)
    {
        FVector CoinSpawnLocation = FVector(0.f, 0.f, 200.f);
        FRotator CoinRotation = FRotator::ZeroRotator;

        ACoinFlipActor* Coin = GetWorld()->SpawnActor<ACoinFlipActor>(CoinFlipActorClass, CoinSpawnLocation, CoinRotation);
        if (Coin)
        {
            DisablePlayerInput();

            AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
            if (PC)
            {
                PC->HideCharacterSelectionWidget();
            }

            bPlayerStartsPlacement = FMath::RandBool();
            Coin->StartFlip(bPlayerStartsPlacement);

            return;
        }
    }

    bPlayerStartsPlacement = FMath::RandBool();
    UE_LOG(LogTemp, Log, TEXT("coin flip: %s start placement."), bPlayerStartsPlacement ? TEXT("player") : TEXT("IA"));

    if (bPlayerStartsPlacement)
    {
        UE_LOG(LogTemp, Log, TEXT("waiting player input."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("IA placing first unit.."));
        PlaceAIUnit();
    }
}

void AMyGameModebase::AddMoveToLog(const FString& MoveText)
{
    UE_LOG(LogTemp, Warning, TEXT("[LOG] AddMoveToLog called for: %s"), *MoveText);

    if (MoveLogWidget)
    {
        MoveLogWidget->AddMoveEntry(MoveText);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[LOG] MoveLogWidget is nullptr"));
    }
}

void AMyGameModebase::StartPlacementPhase()
{
    if (TurnImageWidget)
        TurnImageWidget->SetTurnImage(bPlayerStartsPlacement);

    AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

    if (bPlayerStartsPlacement)
    {
        EnablePlayerInput();
        if (PC) PC->SetCharacterSelectionVisibility(true);
    }
    else
    {
        DisablePlayerInput();
        if (PC) PC->HideCharacterSelectionWidget();
        PlaceAIUnit();
    }
}

FVector AMyGameModebase::GetCellLocationWithOffset(ACell_Actor* Cell) const
{
    if (!Cell || !GridManager) return FVector::ZeroVector;

    FVector StartLocation = GridManager->GetStartLocation();
    float CellStep = GridManager->GetCellStep();

    FVector Location = StartLocation + FVector(
        Cell->Column * CellStep,
        Cell->Row * CellStep,
        UnitSpawnZOffset
    );

    return Location;
}

void AMyGameModebase::NotifyPlayerUnitPlaced()
{
    PlayerUnitsPlaced++;

    // add healthbar
    if (HealthBarPanelWidget && PlayerUnits.Num() > 0)
    {
        AGameCharacter* LastPlayerUnit = PlayerUnits.Last();
        UE_LOG(LogTemp, Warning, TEXT("AddHealthBarForCharacter called for: %s"), *LastPlayerUnit->GetName());
        HealthBarPanelWidget->AddHealthBarForCharacter(LastPlayerUnit);
    }

    if (PlayerUnitsPlaced >= MaxUnitsPerSide && AIUnitsPlaced >= MaxUnitsPerSide)
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::StartBattlePhase);
        return;
    }

    AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

    if (bPlayerStartsPlacement)
    {
        if (PlayerUnitsPlaced > AIUnitsPlaced)
        {
            if (TurnImageWidget) TurnImageWidget->SetTurnImage(false);
            DisablePlayerInput();
            if (PC) PC->HideCharacterSelectionWidget();
            GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::PlaceAIUnit);
        }
        else
        {
            if (TurnImageWidget) TurnImageWidget->SetTurnImage(true);
            EnablePlayerInput();
            if (PC) PC->SetCharacterSelectionVisibility(true);
        }
    }
    else
    {
        if (PlayerUnitsPlaced == AIUnitsPlaced)
        {
            if (TurnImageWidget) TurnImageWidget->SetTurnImage(false);
            DisablePlayerInput();
            if (PC) PC->HideCharacterSelectionWidget();
            GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::PlaceAIUnit);
        }
        else
        {
            if (TurnImageWidget) TurnImageWidget->SetTurnImage(true);
            EnablePlayerInput();
            if (PC) PC->SetCharacterSelectionVisibility(true);
        }
    }
}



void AMyGameModebase::NotifyAIUnitPlaced()
{
    AIUnitsPlaced++;

    if (PlayerUnitsPlaced >= MaxUnitsPerSide && AIUnitsPlaced >= MaxUnitsPerSide)
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::StartBattlePhase);
        return;
    }

    AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

    if (AIUnitsPlaced >= PlayerUnitsPlaced)
    {
        if (TurnImageWidget) TurnImageWidget->SetTurnImage(true);
        EnablePlayerInput();
        if (PC) PC->SetCharacterSelectionVisibility(true);
    }
    else
    {
        if (TurnImageWidget) TurnImageWidget->SetTurnImage(false);
        DisablePlayerInput();
        if (PC) PC->HideCharacterSelectionWidget();
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::PlaceAIUnit);
    }
}

void AMyGameModebase::PlaceAIUnit()
{
    GetWorldTimerManager().SetTimer(
        EnemyTurnTimerHandle,
        this,
        &AMyGameModebase::PlaceAIUnit_Internal,
        1.0f,
        false
    );
}

void AMyGameModebase::PlaceAIUnit_Internal()
{
    if (!GridManager || AIUnitClasses.Num() == 0) return;

    TArray<ACell_Actor*> FreeCells;
    for (ACell_Actor* Cell : GridManager->GridCells)
    {
        if (Cell && !Cell->bIsOccupied && Cell->CellType != ECellType::Obstacle)
        {
            bool bOccupied = false;
            TArray<AActor*> FoundCharacters;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), FoundCharacters);
            for (AActor* Actor : FoundCharacters)
            {
                AGameCharacter* Character = Cast<AGameCharacter>(Actor);
                if (Character && Character->CurrentRow == Cell->Row && Character->CurrentColumn == Cell->Column)
                {
                    bOccupied = true;
                    break;
                }
            }
            if (!bOccupied)
            {
                FreeCells.Add(Cell);
            }
        }
    }

    if (FreeCells.Num() == 0 || AIUnitsPlaced >= MaxUnitsPerSide) return;

    TSubclassOf<AGameCharacter> UnitToSpawn = (AIUnitsPlaced == 0) ? BrawlerCharacter : SniperCharacter;

    int32 RandomIndex = FMath::RandRange(0, FreeCells.Num() - 1);
    ACell_Actor* DestinationCell = FreeCells[RandomIndex];
    FVector SpawnLocation = GetCellLocationWithOffset(DestinationCell);

    FRotator SpawnRotation = FRotator::ZeroRotator;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AGameCharacter* SpawnedAIUnit = GetWorld()->SpawnActor<AGameCharacter>(UnitToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
    if (SpawnedAIUnit)
    {
        SpawnedAIUnit->Health = SpawnedAIUnit->MaxHealth;
        
        AMyAIController* AIController = GetWorld()->SpawnActor<AMyAIController>(AMyAIController::StaticClass());
        if (AIController)
        {
            AIController->Possess(SpawnedAIUnit);
        }

        FVector CorrectLocation = GetCellLocationWithOffset(DestinationCell);
        SpawnedAIUnit->SetActorLocation(CorrectLocation);
        SpawnedAIUnit->bIsAIControlled = true;
        SpawnedAIUnit->MoveToCell(DestinationCell);
        SpawnedAIUnit->CurrentRow = DestinationCell->Row;
        SpawnedAIUnit->CurrentColumn = DestinationCell->Column;

        DestinationCell->SetOriginHighlight(true);
        SpawnedAIUnit->HighlightedOriginCell = DestinationCell;
        
        AIUnits.Add(SpawnedAIUnit);
        
        if (HealthBarPanelWidget)
        {
            HealthBarPanelWidget->AddHealthBarForCharacter(SpawnedAIUnit);
        }

        UE_LOG(LogTemp, Log, TEXT("IA unit spawn: %s in cell (%d, %d)"), *SpawnedAIUnit->GetName(), DestinationCell->Row, DestinationCell->Column);
        NotifyAIUnitPlaced();
    }
}

void AMyGameModebase::PlacePlayerUnit()
{
    UE_LOG(LogTemp, Log, TEXT("player place unit in."));

    if (AIUnitClasses.Num() == 0 || !GridManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("AIUnitClasses empty or GridManager not set!"));
        return;
    }

    int32 Row = GridManager->NumRows - 1;
    int32 Column = PlayerUnitsPlaced;

    ACell_Actor* DestinationCell = nullptr;
    for (ACell_Actor* Cell : GridManager->GridCells)
    {
        if (Cell && Cell->Row == Row && Cell->Column == Column)
        {
            DestinationCell = Cell;
            break;
        }
    }

    if (!DestinationCell)
    {
        UE_LOG(LogTemp, Warning, TEXT("no cell found in (%d, %d)"), Row, Column);
        return;
    }

    int32 RandomIndex = FMath::RandRange(0, AIUnitClasses.Num() - 1);
    TSubclassOf<AGameCharacter> SelectedClass = AIUnitClasses[RandomIndex];

    FVector StartLocation = GridManager->GetStartLocation();
    float Step = GridManager->GetCellStep();

    FVector SpawnLocation = StartLocation + FVector(
        Column * Step,
        Row * Step,
        UnitSpawnZOffset
    );

    FRotator SpawnRotation = FRotator::ZeroRotator;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AGameCharacter* SpawnedPlayerUnit = GetWorld()->SpawnActor<AGameCharacter>(SelectedClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (SpawnedPlayerUnit)
    {
        SpawnedPlayerUnit->Health = SpawnedPlayerUnit->MaxHealth;
        
        SpawnedPlayerUnit->MoveToCell(DestinationCell);
        SpawnedPlayerUnit->bIsAIControlled = false;
        
        SpawnedPlayerUnit->HighlightedOriginCell = DestinationCell;
        DestinationCell->SetOriginHighlight(true);
        
        if (HealthBarPanelWidget)
        {
            HealthBarPanelWidget->AddHealthBarForCharacter(SpawnedPlayerUnit);
        }

        // add player units in array
        PlayerUnits.Add(SpawnedPlayerUnit);

        UE_LOG(LogTemp, Log, TEXT("player unit successfully spawned: %s"), *SpawnedPlayerUnit->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("error in player spawn unit."));
    }

    NotifyPlayerUnitPlaced();
}

void AMyGameModebase::StartBattlePhase()
{
    CurrentPhase = EGamePhase::GP_Battle;
    PlayerUnitsMoved = 0;
    AIUnitsMoved = 0;

    // switch off all highlight
    if (GridManager)
    {
        for (ACell_Actor* Cell : GridManager->GetAllCells())
        {
            if (Cell)
            {
                Cell->SetHighlight(false);
                Cell->SetAttackHighlight(false);
                Cell->SetOriginHighlight(false);
            }
        }
    }

    AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
    if (PC) PC->HideCharacterSelectionWidget();

    // show MoveLogWidget
    if (MoveLogWidget && !MoveLogWidget->IsInViewport())
    {
        MoveLogWidget->AddToViewport();
    }

    if (bPlayerStartsPlacement)
    {
        if (PC) PC->RefreshCellOccupancy();
        StartPlayerTurn();
    }
    else
    {
        StartEnemyTurn();
    }
}

void AMyGameModebase::StartPlayerTurn()
{
    CurrentTurn = ETurnState::TS_PlayerTurn;
    PlayerUnitsMoved = 0;
    
    if (TurnImageWidget)
        TurnImageWidget->SetTurnImage(true);
    
    EnablePlayerInput();
    
    for (AGameCharacter* Unit : PlayerUnits)
    {
        if (Unit)
        {
            Unit->HasMovedThisTurn = false;
            Unit->HasAttackedThisTurn = false;
            
            if (Unit->CurrentCell)
            {
                Unit->CurrentCell->SetOriginHighlight(true);
                Unit->HighlightedOriginCell = Unit->CurrentCell;
            }
        }
    }
}

void AMyGameModebase::StartEnemyTurn()
{
    CurrentTurn = ETurnState::TS_EnemyTurn;
    AIUnitsMoved = 0;
    CurrentAIUnitIndex = 0;

    if (TurnImageWidget)
        TurnImageWidget->SetTurnImage(false);

    DisablePlayerInput();

    for (AGameCharacter* Unit : AIUnits)
    {
        if (Unit)
        {
            Unit->HasMovedThisTurn = false;
            Unit->HasAttackedThisTurn = false;

            AMyAIController* AIController = Cast<AMyAIController>(Unit->GetController());
            if (AIController)
            {
                AIController->ClearCurrentPath();
            }
        }
    }

    MoveNextAIUnit();
}



void AMyGameModebase::NotifyPlayerUnitMoved()
{
    PlayerUnitsMoved++;
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] PlayerUnitsMoved: %d / units left: %d"), PlayerUnitsMoved, PlayerUnits.Num());

    if (PlayerUnitsMoved >= PlayerUnits.Num())
    {
        EndTurn();
    }
}

void AMyGameModebase::NotifyAIUnitMoved()
{
    CurrentAIUnitIndex++;

    if (CurrentAIUnitIndex >= AIUnits.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("all IA units moved. player turn."));
        
      
        
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::EndTurn);
    }
    else
    {
        GetWorldTimerManager().SetTimer(
            EnemyTurnTimerHandle,
            this,
            &AMyGameModebase::MoveNextAIUnit,
            0.3f, // delay
            false
        );
    }
}

void AMyGameModebase::AddPlayerUnit(AGameCharacter* Unit)
{
    if (Unit)
    {
        PlayerUnits.Add(Unit);
    }
}

void AMyGameModebase::MoveNextAIUnit()
{
    if (CurrentAIUnitIndex >= AIUnits.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("all IA units moved. player turn."));
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::EndTurn);
        return;
    }

    AGameCharacter* AIUnit = AIUnits[CurrentAIUnitIndex];
    if (AIUnit && AIUnit->IsValidLowLevel())
    {
        AMyAIController* AIController = Cast<AMyAIController>(AIUnit->GetController());
        if (AIController)
        {
            UE_LOG(LogTemp, Log, TEXT("L'IA controlla: %s"), *AIUnit->GetName());

            // delay
            GetWorldTimerManager().SetTimer(
                EnemyTurnTimerHandle,
                FTimerDelegate::CreateLambda([AIController]()
                {
                    AIController->RunTurn();
                }),
                1.0f, // delay
                false
            );
        }
    }
}

void AMyGameModebase::EndTurn()
{
    if (CurrentPhase != EGamePhase::GP_Battle) return;

    // check for pass
    if (PlayerUnits.Num() == 0)
    {
        EndGame(false);
        return;
    }
    if (AIUnits.Num() == 0)
    {
        EndGame(true);
        return;
    }

    // debug log
    UE_LOG(LogTemp, Log, TEXT("end turn"));

    if (CurrentTurn == ETurnState::TS_PlayerTurn)
    {
        UE_LOG(LogTemp, Log, TEXT("player turn finished. IA start"));
        StartEnemyTurn();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("IA turn finished. player start"));
        StartPlayerTurn();
    }
}


void AMyGameModebase::StartTurn()
{
    CurrentAIIndex = 0;
    ExecuteNextAIUnit();
}

void AMyGameModebase::ExecuteNextAIUnit()
{
    if (CurrentAIIndex >= AIUnits.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("all IA units finish movement"));
        EndTurn(); // player
        return;
    }

    AGameCharacter* AIChar = AIUnits[CurrentAIIndex];
    AMyAIController* AIController = Cast<AMyAIController>(AIChar->GetController());

    if (AIChar && AIController)
    {
        UE_LOG(LogTemp, Log, TEXT("IA Unit %d: %s start turn"), CurrentAIIndex, *AIChar->GetName());

        // unplug old bind
        AIChar->OnMovementFinished.Clear();

        // plug event
        AIChar->OnMovementFinished.AddDynamic(this, &AMyGameModebase::OnAIMovementFinished);

        // start turn
        AIController->RunTurn();
    }
}

void AMyGameModebase::OnAIMovementFinished()
{
    UE_LOG(LogTemp, Log, TEXT("IA Unit %d finished movemnt"), CurrentAIIndex);

    CurrentAIIndex++;
    ExecuteNextAIUnit();
}

void AMyGameModebase::OnUnitKilled(AGameCharacter* DeadUnit)
{
    if (DeadUnit->bIsAIControlled)
    {
        AIUnits.Remove(DeadUnit);
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] IA unit dead"));
    }
    else
    {
        bool bHadAlreadyMoved = DeadUnit->HasMovedThisTurn;
        PlayerUnits.Remove(DeadUnit);
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] player unit dead"));

        if (!bHadAlreadyMoved)
        {
            PlayerUnitsMoved++;
        }

        // all player units dead -> end game
        if (PlayerUnits.Num() == 0)
        {
            EndGame(false);
            return;
        }

        bool bAllActed = true;
        for (AGameCharacter* Unit : PlayerUnits)
        {
            if (Unit && (!Unit->HasMovedThisTurn || !Unit->HasAttackedThisTurn))
            {
                bAllActed = false;
                break;
            }
        }

        if (bAllActed)
        {
            UE_LOG(LogTemp, Log, TEXT("[GameMode] after death, all player units moved. pass turn."));
            EndTurn();
        }
    }

    // IA endgame check
    if (AIUnits.Num() == 0)
    {
        EndGame(true);
    }
}

void AMyGameModebase::EndGame(bool bPlayerWon)
{
    CurrentPhase = EGamePhase::GP_End;
    UE_LOG(LogTemp, Warning, TEXT("end game"));
    if (bPlayerWon)
    {
        UE_LOG(LogTemp, Warning, TEXT("you won!"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("you lose!"));
    }
    
    // unable input
    AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
    if (PC)
    {
        PC->SetInputMode(FInputModeUIOnly());
        PC->bShowMouseCursor = true;
    }

    for (AGameCharacter* Unit : AIUnits)
    {
        if (Unit)
        {
            AMyAIController* AIController = Cast<AMyAIController>(Unit->GetController());
            if (AIController)
            {
                AIController->StopMovement();
                AIController->SetActorTickEnabled(false);
            }
        }
    }
    if (EndGameWidgetClass)
    {
        EndGameWidget = CreateWidget<UUserWidget>(GetWorld(), EndGameWidgetClass);
        if (EndGameWidget)
        {
            EndGameWidget->AddToViewport(20);

            // show endgame image
            UImage* Image_YouWon = Cast<UImage>(EndGameWidget->GetWidgetFromName(TEXT("Image_YouWon")));
            UImage* Image_GameOver = Cast<UImage>(EndGameWidget->GetWidgetFromName(TEXT("Image_GameOver")));

            if (bPlayerWon && Image_YouWon)
            {
                Image_YouWon->SetVisibility(ESlateVisibility::Visible);
            }
            else if (!bPlayerWon && Image_GameOver)
            {
                Image_GameOver->SetVisibility(ESlateVisibility::Visible);
            }

            // show mouse unable input
            APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
            if (PC)
            {
                PC->SetInputMode(FInputModeUIOnly());
                PC->SetShowMouseCursor(true);
            }
        }
        if (bPlayerWon && VictorySound)
        {
            UGameplayStatics::PlaySound2D(this, VictorySound);
        }
        else if (!bPlayerWon && DefeatSound)
        {
            UGameplayStatics::PlaySound2D(this, DefeatSound);
        }
    }
    
    if (EndGameWidgetClass)
    {
        EndGameWidget = CreateWidget<UUserWidget>(GetWorld(), EndGameWidgetClass);
        if (EndGameWidget)
        {
            EndGameWidget->AddToViewport();

            // show correct image
            UImage* Image_YouWon = Cast<UImage>(EndGameWidget->GetWidgetFromName(TEXT("Image_YouWon")));
            UImage* Image_GameOver = Cast<UImage>(EndGameWidget->GetWidgetFromName(TEXT("Image_GameOver")));

            if (bPlayerWon && Image_YouWon)
            {
                Image_YouWon->SetVisibility(ESlateVisibility::Visible);
            }
            else if (!bPlayerWon && Image_GameOver)
            {
                Image_GameOver->SetVisibility(ESlateVisibility::Visible);
            }

            // unable inpunt, show mouse
            APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
            if (PC)
            {
                PC->SetInputMode(FInputModeUIOnly());
                PC->SetShowMouseCursor(true);
            }
        }
    }

}

void AMyGameModebase::DisablePlayerInput()
{
    bIsPlayerInputEnabled = false;
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->SetInputMode(FInputModeUIOnly());
        PC->bShowMouseCursor = true;
    }
}

void AMyGameModebase::EnablePlayerInput()
{
    bIsPlayerInputEnabled = true;
    
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->SetInputMode(FInputModeGameAndUI());
        PC->bShowMouseCursor = true;
    }
}
