// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayMechanicsCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AGameplayMechanicsCharacter

AGameplayMechanicsCharacter::AGameplayMechanicsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	InHand = true;
	Speed = 1000;
	defaultFOV = FollowCamera->FieldOfView;

	AxePath = CreateDefaultSubobject<USceneComponent>(TEXT("AxePath"));
	AxePath->SetupAttachment(GetMesh());
	
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AGameplayMechanicsCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	TArray<UStaticMeshComponent*> staticMeshArray;
	GetComponents<UStaticMeshComponent*>(staticMeshArray);

	if(!staticMeshArray.IsEmpty())
	{
		for (auto staticMesh : staticMeshArray)
		{
			if(staticMesh->GetName() == "Axe")
			{
				AxeMesh = staticMesh;
			}
		}
	}
}

void AGameplayMechanicsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if(isReturning)
		AxeReturnPath(GetActorLocation(), DeltaSeconds);
}

void AGameplayMechanicsCharacter::ReturnAxe()
{
	initialAxePos = ThrownAxe->GetActorLocation();
	isReturning = true;
}

void AGameplayMechanicsCharacter::ThrowAxe()
{
	AxeMesh->SetVisibility(false);

	FVector SpawnLocation = FollowCamera->GetComponentLocation() + FollowCamera->GetForwardVector() * 150;
	FRotator SpawnRotation = FollowCamera->GetComponentRotation();
	
	ThrownAxe = GetWorld()->SpawnActor<AActor>(AxeActor, SpawnLocation, SpawnRotation);
	
	InHand = false;
}

FVector AGameplayMechanicsCharacter::BQCurvePath(float t, FVector v1, FVector v2, FVector v3)
{
	float u = 1-t;
	float tt = t * t;
	float uu = u * u;
	FVector p = (uu * v1) + (2 * u * t * v2) + (tt * v3);
	return p;
}

void AGameplayMechanicsCharacter::AxeReturnPath(FVector location, float deltatime)
{
	if(!ThrownAxe) return;

	if(time <= 1)
	{
		ThrownAxe->SetActorLocation(BQCurvePath(time, initialAxePos, AxePath->GetComponentLocation(),
			AxeMesh->GetComponentLocation()));

		ThrownAxe->GetComponentByClass<URotatingMovementComponent>()->RotationRate = AxeRotatingRate;

		time += deltatime;
	}

	if(time >= 1)
	{
		PlayAnimMontage(CatchMontage);
		ThrownAxe->Destroy();

		InHand = true;
		AxeMesh->SetVisibility(true);
		isReturning = false;
		time = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AGameplayMechanicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGameplayMechanicsCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGameplayMechanicsCharacter::Look);

		// Attacking
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AGameplayMechanicsCharacter::Attack);

		// Aiming
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AGameplayMechanicsCharacter::AimDownSights);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AGameplayMechanicsCharacter::StopAimDownSights);
		
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AGameplayMechanicsCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AGameplayMechanicsCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}

	if(isAiming)
		RotateToCameraForward();
}

void AGameplayMechanicsCharacter::Attack(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Attacking"));

	if(!CanJump()) return;

	if(!InHand)
	{
		ReturnAxe();
		return;
	}

	RotateToCameraForward();
	PlayAnimMontage(ThrowMontage);
}

void AGameplayMechanicsCharacter::AimDownSights(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("AIMING"));
	isAiming = true;
	FollowCamera->SetFieldOfView(ZoomFOV);

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void AGameplayMechanicsCharacter::StopAimDownSights(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("STOP AIMING"));
	isAiming = false;
	FollowCamera->SetFieldOfView(defaultFOV);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void AGameplayMechanicsCharacter::RotateToCameraForward()
{
	FRotator camerarotation = GetFollowCamera()->GetComponentRotation();
	camerarotation.Pitch = 0;

	SetActorRotation(camerarotation);
}
