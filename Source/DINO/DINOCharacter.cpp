// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "DINOCharacter.h"
#include "PaperFlipbookComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "Sound/SoundCue.h"
#include "Camera/CameraComponent.h"

DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);

//////////////////////////////////////////////////////////////////////////
// ADINOCharacter

ADINOCharacter::ADINOCharacter()
{
	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
	

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 850.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

    // 	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
    // 	TextComponent->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
    // 	TextComponent->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
    // 	TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    // 	TextComponent->SetupAttachment(RootComponent);

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;

	//settomg the status of the characters jumping state
	isJumping = false;

	//defaulting the animation to the idleanimation
	DesiredAnimation = IdleAnimation;
	GetSprite()->SetFlipbook(DesiredAnimation);

	//setting the distance traveled to 0
	DistanceTraveled = 0;

	//default value of the player state 
	isDead = false;
}

//////////////////////////////////////////////////////////////////////////
// Animation

void ADINOCharacter::UpdateAnimation()
{

	const FVector PlayerVelocity = GetVelocity();
	const float PlayerJumpVelocity = GetVelocity().Z;
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();
	
	// Are we moving or jumping?
	if (isJumping == true || PlayerJumpVelocity > 0)
	{
		DesiredAnimation = JumpingAnimation;
	}
	else if(isJumping == false && PlayerSpeedSqr > 0.0f && PlayerJumpVelocity  == 0)
	{
		DesiredAnimation = RunningAnimation;
	}
	 
	if( GetSprite()->GetFlipbook() != DesiredAnimation 	)
	{
		GetSprite()->SetFlipbook(DesiredAnimation);
	}
}

void ADINOCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//this is how we are constantly moving our character forward withouth input
	if (isDead == false)
	{
		MoveRight(DeltaSeconds);
	}
	
	//UpdateCharacter();	
}


//////////////////////////////////////////////////////////////////////////
// Input

void ADINOCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ADINOCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ADINOCharacter::StopJump);
	//PlayerInputComponent->BindAxis("MoveRight", this, &ADINOCharacter::MoveRight);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &ADINOCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ADINOCharacter::TouchStopped);
}

void ADINOCharacter::MoveRight(float value)
{
	UpdateCharacter();

	// Apply the input to the character motion
	AddMovementInput(FVector(100.0f, 0.0f, 0.0f), 1);
}

void ADINOCharacter::Jumping()
{
	isJumping = true;
	Jump();
	UpdateCharacter();
}

void ADINOCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Jump on any touch
	isJumping = true;
	Jump();
	UpdateCharacter();
}

void ADINOCharacter::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Cease jumping once touch stopped
	isJumping = true;
	StopJumping();
	UpdateCharacter();
}

void ADINOCharacter::StopJump()
{
	isJumping = false;
    StopJumping();
	UpdateCharacter();
}

void ADINOCharacter::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimation();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();	
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}
