#include "CoinFlipActor.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameModebase.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

ACoinFlipActor::ACoinFlipActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Coin body
    CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
    RootComponent = CoinMesh;

    // Face "Head"
    Face_Top = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Face_Top"));
    Face_Top->SetupAttachment(RootComponent);
    Face_Top->SetRelativeLocation(FVector(0.f, 0.f, 1.f));

    // Face "Cross"
    Face_Bottom = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Face_Bottom"));
    Face_Bottom->SetupAttachment(RootComponent);
    Face_Bottom->SetRelativeLocation(FVector(0.f, 0.f, -1.f));
    Face_Bottom->SetRelativeRotation(FRotator(180.f, 0.f, 0.f)); // upside down

    bIsFlipping = false;
    RotationSpeed = 720.f;   // grade4second
    FlipDuration = 2.0f;
    ElapsedTime = 0.f;
}

void ACoinFlipActor::BeginPlay()
{
    Super::BeginPlay();
}

void ACoinFlipActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsFlipping)
    {
        ElapsedTime += DeltaTime;
        float RotationAmount = RotationSpeed * DeltaTime;

        // spin the coin
        CoinMesh->AddLocalRotation(FRotator(RotationAmount, 0.f, 0.f));
        Face_Top->AddLocalRotation(FRotator(RotationAmount, 0.f, 0.f));
        Face_Bottom->AddLocalRotation(FRotator(RotationAmount, 0.f, 0.f));

        if (ElapsedTime >= FlipDuration)
        {
            bIsFlipping = false;
 
            // face the result
            FRotator FinalRotation = bPlayerStartsFlip ? FRotator(0.f, 0.f, 0.f) : FRotator(180.f, 0.f, 0.f);
            CoinMesh->SetRelativeRotation(FinalRotation);
            Face_Top->SetRelativeRotation(FinalRotation);
            Face_Bottom->SetRelativeRotation(FinalRotation);

            // wait 2 seconds before start placement
            GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
            {
                GetWorld()->GetTimerManager().SetTimer(
                    TimerHandle_AfterPause,
                    this,
                    &ACoinFlipActor::OnFlipFinished,
                    2.0f, // timer set
                    false
                );
            });
        }
    }
}


void ACoinFlipActor::StartFlip(bool bPlayerStarts)
{
    bIsFlipping = true;
    ElapsedTime = 0.f;
    bPlayerStartsFlip = bPlayerStarts;
}
void ACoinFlipActor::OnFlipFinished()
{
    AMyGameModebase* GM = Cast<AMyGameModebase>(UGameplayStatics::GetGameMode(this));
    if (GM)
    {
        if (GM->TurnImageWidgetClass)
        {
            GM->TurnImageWidget = Cast<UTurnImageWidget>(CreateWidget(GetWorld(), GM->TurnImageWidgetClass));
            if (GM->TurnImageWidget)
            {
                GM->TurnImageWidget->AddToViewport();
                GM->TurnImageWidget->SetTurnImage(GM->bPlayerStartsPlacement);
            }
        }

        GM->StartPlacementPhase();
    }

    Destroy(); // destroy Coin
}
