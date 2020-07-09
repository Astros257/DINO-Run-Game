// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "DINOGameMode.h"
#include "DINOCharacter.h"

ADINOGameMode::ADINOGameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = ADINOCharacter::StaticClass();	
}
