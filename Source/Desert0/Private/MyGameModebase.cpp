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
    // Imposta il custom PlayerController
    PlayerControllerClass = AMyPlayerController::StaticClass();

    // Inizialmente, il turno (di battaglia) verrà impostato quando la fase passerà a battaglia
    CurrentTurn = ETurnState::TS_PlayerTurn;
    // La partita inizia nella fase di posizionamento
    CurrentPhase = EGamePhase::GP_Placement;

    PlayerUnitsPlaced = 0;
    AIUnitsPlaced = 0;

    // Simula il lancio di moneta per decidere chi posiziona per primo
    bPlayerStartsPlacement = FMath::RandBool();
    UE_LOG(LogTemp, Log, TEXT("Lancio di moneta: %s inizia il posizionamento."), bPlayerStartsPlacement ? TEXT("Il giocatore") : TEXT("L'IA"));
}

void AMyGameModebase::BeginPlay()
{
    Super::BeginPlay();

    // Se la classe del Grid Manager è assegnata, spawnalo
    if (GridManagerClass)
    {
        FVector SpawnLocation = FVector::ZeroVector;
        FRotator SpawnRotation = FRotator::ZeroRotator;
        GridManager = GetWorld()->SpawnActor<AGrid_Manager>(GridManagerClass, SpawnLocation, SpawnRotation);
        if (GridManager)
        {
            UE_LOG(LogTemp, Log, TEXT("Grid Manager spawnato con successo."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Errore nello spawn del Grid Manager."));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Fase di posizionamento iniziata."));

    // Se l'IA deve iniziare il posizionamento, esegui subito la sua azione
    if (!bPlayerStartsPlacement)
    {
        PlaceAIUnit();
    }
    // Altrimenti, il giocatore attenderà di posizionare una unità (il PlayerController gestirà l'input)
}

void AMyGameModebase::StartPlayerTurn()
{
    CurrentTurn = ETurnState::TS_PlayerTurn;
    UE_LOG(LogTemp, Log, TEXT("Turno del giocatore iniziato."));
    // Qui il PlayerController abilita gli input per il giocatore
}

void AMyGameModebase::StartEnemyTurn()
{
    CurrentTurn = ETurnState::TS_EnemyTurn;
    UE_LOG(LogTemp, Log, TEXT("Turno nemico iniziato."));
    // Simula il "pensiero" dell'IA con un ritardo (es. 2 secondi)
    GetWorldTimerManager().SetTimer(EnemyTurnTimerHandle, this, &AMyGameModebase::ExecuteEnemyTurn, 2.0f, false);
}

void AMyGameModebase::ExecuteEnemyTurn()
{
    UE_LOG(LogTemp, Log, TEXT("Esecuzione del turno nemico."));
    
    // Trova tutte le unità di gioco controllate dall'IA (dipende da come distingui le unità IA dalle unità giocatore)
    TArray<AActor*> EnemyUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGameCharacter::StaticClass(), EnemyUnits);

    for (AActor* Actor : EnemyUnits)
    {
        AGameCharacter* GameChar = Cast<AGameCharacter>(Actor);
        if (GameChar && !GameChar->IsPlayerControlled()) // Supponiamo che AGameCharacter abbia IsPlayerControlled()
        {
            if (AAIController* AIContr = Cast<AAIController>(GameChar->GetController()))
            {
                if (AMyAIController* MyAIContr = Cast<AMyAIController>(AIContr))
                {
                    MyAIContr->RunTurn();
                }
            }
        }
    }
    
    // Dopo aver eseguito le azioni per tutte le unità IA, termina il turno
    EndTurn();
}

void AMyGameModebase::EndTurn()
{
    // Durante il turno di battaglia, alterna tra giocatore e IA
    if (CurrentTurn == ETurnState::TS_PlayerTurn)
    {
        StartEnemyTurn();
    }
    else if (CurrentTurn == ETurnState::TS_EnemyTurn)
    {
        StartPlayerTurn();
    }
}

// Notifica che il giocatore ha posizionato una unità
void AMyGameModebase::PlacePlayerUnit()
{
    UE_LOG(LogTemp, Log, TEXT("Il giocatore posiziona una unità."));

    if (GridManager)
    {
        // 📌 Decidi una posizione valida per il giocatore (es: ultima riga, colonna PlayerUnitsPlaced)
        int32 Row = GridManager->NumRows - 1; // ultima riga
        int32 Column = PlayerUnitsPlaced;     // es. 0, poi 1

        // Trova la cella corrispondente
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

        // Calcola la posizione centrata
        FVector StartLocation = GridManager->GetStartLocation();
        float Step = GridManager->GetCellStep();

        FVector SpawnLocation = StartLocation + FVector(
            Column * Step,
            Row * Step,
            0.f
        );

        // Spawna l'unità giocatore (puoi usare una classe PlayerUnitClass se vuoi)
        FRotator SpawnRotation = FRotator::ZeroRotator;
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AGameCharacter* SpawnedPlayerUnit = GetWorld()->SpawnActor<AGameCharacter>(AIUnitClass, SpawnLocation, SpawnRotation, SpawnParams); // se usi anche per il giocatore
        if (SpawnedPlayerUnit)
        {
            // Muovi e assegna la cella
            SpawnedPlayerUnit->MoveToCell(DestinationCell);
            UE_LOG(LogTemp, Log, TEXT("Unità del giocatore spawnata con successo: %s"), *SpawnedPlayerUnit->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Errore nello spawn dell'unità del giocatore."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GridManager non impostato!"));
    }

    NotifyPlayerUnitPlaced();
}


void AMyGameModebase::NotifyPlayerUnitPlaced()
{
    PlayerUnitsPlaced++;
    UE_LOG(LogTemp, Log, TEXT("Unità del giocatore posizionata. Totale: %d"), PlayerUnitsPlaced);

    // Dopo che il giocatore ha posizionato una unità, se l'IA ha ancora unità da posizionare, chiama PlaceAIUnit().
    if (AIUnitsPlaced < 2)
    {
        PlaceAIUnit();
    }
    else if (PlayerUnitsPlaced >= 2 && AIUnitsPlaced >= 2)
    {
        // Completato il posizionamento, passa alla fase di battaglia.
        CurrentPhase = EGamePhase::GP_Battle;
        UE_LOG(LogTemp, Log, TEXT("Fase di battaglia iniziata."));
        if (bPlayerStartsPlacement)
            StartPlayerTurn();
        else
            StartEnemyTurn();
    }
}


// Notifica che l'IA ha posizionato una unità
void AMyGameModebase::NotifyAIUnitPlaced()
{
    AIUnitsPlaced++;
    UE_LOG(LogTemp, Log, TEXT("Unità dell'IA posizionata. Totale: %d"), AIUnitsPlaced);

    // Se il giocatore ha ancora unità da posizionare, attende l'input del giocatore
    if (PlayerUnitsPlaced < 2)
    {
        // Aspetta che il giocatore posizioni la sua unità
    }
    else if (PlayerUnitsPlaced >= 2 && AIUnitsPlaced >= 2)
    {
        CurrentPhase = EGamePhase::GP_Battle;
        UE_LOG(LogTemp, Log, TEXT("Fase di battaglia iniziata."));
        if (bPlayerStartsPlacement)
            StartPlayerTurn();
        else
            StartEnemyTurn();
    }
}

// Funzione per simulare il posizionamento da parte dell'IA
void AMyGameModebase::PlaceAIUnit()
{
    UE_LOG(LogTemp, Log, TEXT("L'IA posiziona una unità."));

    if (AIUnitClass && GridManager)
    {
        // 📌 Decidi una posizione valida per l'IA (esempio: riga 0, colonna AIUnitsPlaced)
        int32 Row = 0;
        int32 Column = AIUnitsPlaced;

        // Trova la cella corrispondente
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

        // Calcola la posizione centrata usando GridManager
        FVector StartLocation = GridManager->GetStartLocation();
        float Step = GridManager->GetCellStep();

        FVector SpawnLocation = StartLocation + FVector(
            Column * Step,
            Row * Step,
            0.f // oppure una Z fissa (es. 50.f)
        );

        // Spawna l'unità
        FRotator SpawnRotation = FRotator::ZeroRotator;
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AGameCharacter* SpawnedAIUnit = GetWorld()->SpawnActor<AGameCharacter>(AIUnitClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (SpawnedAIUnit)
        {
            // Imposta la cella attuale e occupala
            SpawnedAIUnit->MoveToCell(DestinationCell);
            UE_LOG(LogTemp, Log, TEXT("Unità IA spawnata con successo: %s"), *SpawnedAIUnit->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Errore nello spawn dell'unità IA."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AIUnitClass o GridManager non impostati!"));
    }

    NotifyAIUnitPlaced();
}
