#include "MoveLogWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"

void UMoveLogWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UMoveLogWidget::AddMoveEntry(const FString& MoveText)
{
    if (!MoveLogScrollBox) return;

    UTextBlock* NewText = NewObject<UTextBlock>(MoveLogScrollBox);
    if (!NewText) return;

    NewText->SetText(FText::FromString(MoveText));

    FSlateFontInfo FontInfo = NewText->GetFont(); // ✅ metodo corretto
    FontInfo.Size = 18;
    NewText->SetFont(FontInfo);                  // ✅ setter corretto

    MoveLogScrollBox->AddChild(NewText);
    MoveLogScrollBox->ScrollToEnd();
}
