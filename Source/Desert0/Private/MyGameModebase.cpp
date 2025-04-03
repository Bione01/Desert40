#include "MyGameModebase.h"
#include "Grid_Manager.h"
#include "Engine/World.h"
#include "MyPlayerController.h"
#include "TimerManager.h"
#include "Cell_Actor.h"
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

    if (GridManagerClass)
    {
        FVector SpawnLocation = FVector::ZeroVector;
        FRotator SpawnRotation = FRotator::ZeroRotator;
        GridManager = GetWorld()->SpawnActor<AGrid_Manager>(GridManagerClass, SpawnLocation, SpawnRotation);
        if (GridManager)
        {
            UE_LOG(LogTemp, Log, TEXT("Grid Manager spawnato con successo."));
        }
    }

    bPlayerStartsPlacement = FMath::RandBool();
    UE_LOG(LogTemp, Log, TEXT("Lancio della moneta: %s inizia il posizionamento."), bPlayerStartsPlacement ? TEXT("Giocatore") : TEXT("IA"));

    if (bPlayerStartsPlacement)
    {
        UE_LOG(LogTemp, Log, TEXT("Attesa input del giocatore per posizionare la prima unità."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("L'IA sta posizionando la prima unità..."));
        PlaceAIUnit();
    }
}

void AMyGameModebase::NotifyPlayerUnitPlaced()
{
    PlayerUnitsPlaced++;
    UE_LOG(LogTemp, Log, TEXT("Unità del Giocatore posizionata. Totale: %d"), PlayerUnitsPlaced);

    if (PlayerUnitsPlaced >= MaxUnitsPerSide && AIUnitsPlaced >= MaxUnitsPerSide)
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::StartBattlePhase);
    }

    else
    {
        if (bPlayerStartsPlacement)
        {
            if (PlayerUnitsPlaced > AIUnitsPlaced)
            {
                PlaceAIUnit();
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Tocca al giocatore a posizionare."));
            }
        }
        else
        {
            if (PlayerUnitsPlaced == AIUnitsPlaced)
            {
                GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::PlaceAIUnit);
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Tocca al giocatore a posizionare."));
            }
        }
    }
}

void AMyGameModebase::NotifyAIUnitPlaced()
{
    AIUnitsPlaced++;
    UE_LOG(LogTemp, Log, TEXT("Unità dell'IA posizionata. Totale: %d"), AIUnitsPlaced);

    if (PlayerUnitsPlaced >= MaxUnitsPerSide && AIUnitsPlaced >= MaxUnitsPerSide)
    {
        // Aspetta un frame per sicurezza
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::StartBattlePhase);
    }

    {
        if (AIUnitsPlaced >= PlayerUnitsPlaced)
        {
            UE_LOG(LogTemp, Log, TEXT("Tocca al giocatore a posizionare."));
        }
        else
        {
            GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::PlaceAIUnit);
        }
    }
}

void AMyGameModebase::PlaceAIUnit()
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
    FVector SpawnLocation = DestinationCell->GetActorLocation();
    SpawnLocation.Z = UnitSpawnZOffset;

    FRotator SpawnRotation = FRotator::ZeroRotator;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AGameCharacter* SpawnedAIUnit = GetWorld()->SpawnActor<AGameCharacter>(UnitToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
    if (SpawnedAIUnit)
    {
        AMyAIController* AIController = GetWorld()->SpawnActor<AMyAIController>(AMyAIController::StaticClass());
        if (AIController)
        {
            AIController->Possess(SpawnedAIUnit);
        }

        SpawnedAIUnit->bIsAIControlled = true;
        SpawnedAIUnit->MoveToCell(DestinationCell);
        SpawnedAIUnit->CurrentRow = DestinationCell->Row;
        SpawnedAIUnit->CurrentColumn = DestinationCell->Column;

        // ✅ Aggiungiamo l'unità all'array
        AIUnits.Add(SpawnedAIUnit);

        UE_LOG(LogTemp, Log, TEXT("Unità IA spawnata: %s nella cella (%d, %d)"), *SpawnedAIUnit->GetName(), DestinationCell->Row, DestinationCell->Column);
        NotifyAIUnitPlaced();
    }
}

void AMyGameModebase::PlacePlayerUnit()
{
    UE_LOG(LogTemp, Log, TEXT("Il giocatore posiziona una unità."));

    if (AIUnitClasses.Num() == 0 || !GridManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("AIUnitClasses vuoto o GridManager non impostato!"));
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
        UE_LOG(LogTemp, Warning, TEXT("Nessuna cella trovata in (%d, %d)"), Row, Column);
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
        SpawnedPlayerUnit->MoveToCell(DestinationCell);
        SpawnedPlayerUnit->bIsAIControlled = false;

        // ✅ Aggiungi all'array delle unità del player
        PlayerUnits.Add(SpawnedPlayerUnit);

        UE_LOG(LogTemp, Log, TEXT("Unità del giocatore spawnata con successo: %s"), *SpawnedPlayerUnit->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Errore nello spawn dell'unità del giocatore."));
    }

    NotifyPlayerUnitPlaced();
}

void AMyGameModebase::StartBattlePhase()
{
    CurrentPhase = EGamePhase::GP_Battle;
    UE_LOG(LogTemp, Log, TEXT("Fase di battaglia iniziata."));
    PlayerUnitsMoved = 0;
    AIUnitsMoved = 0;
    if (bPlayerStartsPlacement)
    {
        // Aggiorna occupazione celle dopo il piazzamento
        AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
        if (PC)
        {
            PC->RefreshCellOccupancy();
        }
        
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
    UE_LOG(LogTemp, Log, TEXT("Turno del giocatore iniziato."));
    
    for (AGameCharacter* Unit : PlayerUnits)
    {
        if (Unit)
        {
            Unit->HasMovedThisTurn = false;
            Unit->HasAttackedThisTurn = false;
        }
    }
}

void AMyGameModebase::StartEnemyTurn()
{
    CurrentTurn = ETurnState::TS_EnemyTurn;
    AIUnitsMoved = 0;
    CurrentAIUnitIndex = 0;
    UE_LOG(LogTemp, Log, TEXT("Turno nemico iniziato."));

    for (AGameCharacter* Unit : AIUnits)
    {
        if (Unit)
        {
            Unit->HasMovedThisTurn = false;
            Unit->HasAttackedThisTurn = false;

            // 🔥 Pulizia del path
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
    if (PlayerUnitsMoved >= MaxUnitsPerSide)
    {
        EndTurn();
    }
}

void AMyGameModebase::NotifyAIUnitMoved()
{
    CurrentAIUnitIndex++;

    if (CurrentAIUnitIndex >= AIUnits.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("Tutte le unità IA hanno agito. Passo il turno al Player."));
        GetWorldTimerManager().SetTimerForNextTick(this, &AMyGameModebase::EndTurn);
    }
    else
    {
        GetWorldTimerManager().SetTimer(
            EnemyTurnTimerHandle,
            this,
            &AMyGameModebase::MoveNextAIUnit,
            0.3f, // Delay visivo
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
        UE_LOG(LogTemp, Log, TEXT("Tutte le unità AI hanno agito. Passo il turno al Player."));
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
            AIController->RunTurn();
        }
    }
}


void AMyGameModebase::EndTurn()
{
    if (CurrentPhase != EGamePhase::GP_Battle) return;

    if (CurrentTurn == ETurnState::TS_PlayerTurn)
    {
        UE_LOG(LogTemp, Log, TEXT("Turno del Player finito. Inizio turno IA."));
        StartEnemyTurn();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Turno dell'IA finito. Inizio turno Player."));
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
        UE_LOG(LogTemp, Log, TEXT("Tutte le unità IA hanno finito di muoversi."));
        EndTurn(); // Qui chiami il passaggio turno al Player
        return;
    }

    AGameCharacter* AIChar = AIUnits[CurrentAIIndex];
    AMyAIController* AIController = Cast<AMyAIController>(AIChar->GetController());

    if (AIChar && AIController)
    {
        UE_LOG(LogTemp, Log, TEXT("IA Unit %d: %s sta iniziando il turno"), CurrentAIIndex, *AIChar->GetName());

        // Scollega eventuali vecchi bind
        AIChar->OnMovementFinished.Clear();

        // Collega l'evento
        AIChar->OnMovementFinished.AddDynamic(this, &AMyGameModebase::OnAIMovementFinished);

        // Avvia il turno
        AIController->RunTurn();
    }
}

void AMyGameModebase::OnAIMovementFinished()
{
    UE_LOG(LogTemp, Log, TEXT("IA Unit %d ha finito di muoversi"), CurrentAIIndex);

    CurrentAIIndex++;
    ExecuteNextAIUnit();
}

void AMyGameModebase::OnUnitKilled(AGameCharacter* DeadUnit)
{
    if (DeadUnit->bIsAIControlled)
    {
        AIUnits.Remove(DeadUnit);
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Unità IA eliminata."));
    }
    else
    {
        PlayerUnits.Remove(DeadUnit);
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Unità del giocatore eliminata."));

        bool bAllPlayerUnitsActed = true;
        for (AGameCharacter* Unit : PlayerUnits)
        {
            if (Unit && (!Unit->HasMovedThisTurn || !Unit->HasAttackedThisTurn))
            {
                bAllPlayerUnitsActed = false;
                break;
            }
        }

        if (bAllPlayerUnitsActed)
        {
            UE_LOG(LogTemp, Log, TEXT("[GameMode] Tutte le unità Player hanno agito dopo la morte. Passo il turno."));
            NotifyPlayerUnitMoved();
        }
    }

    // Check fine partita
    if (PlayerUnits.Num() == 0)
    {
        EndGame(false); // IA vince
    }
    else if (AIUnits.Num() == 0)
    {
        EndGame(true); // Player vince
    }
}

void AMyGameModebase::EndGame(bool bPlayerWon)
{
    CurrentPhase = EGamePhase::GP_End;
    UE_LOG(LogTemp, Warning, TEXT("=== PARTITA TERMINATA ==="));
    if (bPlayerWon)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hai vinto!"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Hai perso!"));
    }

    // Blocca input
    AMyPlayerController* PC = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
    if (PC)
    {
        PC->SetInputMode(FInputModeUIOnly());
        PC->bShowMouseCursor = true;
    }

    // Blocca IA
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

    // TODO: Se vuoi mostra un widget di vittoria/sconfitta qui
}
