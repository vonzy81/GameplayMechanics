// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GameplayMechanicsCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AGameplayMechanicsCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	USceneComponent* AxePath;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	UAnimMontage* ThrowMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	UAnimMontage* CatchMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	UStaticMeshComponent* AxeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ANIM, meta=(AllowPrivateAccess = "true"))
	TSubclassOf<AActor> AxeActor;

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	AActor* ThrownAxe;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	bool InHand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	float ZoomFOV;
	
	/** INPUTS */
	
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AttackAction;

	/** Aiming Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	float Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	bool isAiming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= ANIM, meta=(AllowPrivateAccess = "true"))
	FRotator AxeRotatingRate;

	bool isReturning;
	float defaultFOV;
	float time;
	FVector initialAxePos;

public:
	AGameplayMechanicsCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= Input)
	bool IsAttacking;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void Attack(const FInputActionValue& Value);

	void AimDownSights(const FInputActionValue& Value);
	void StopAimDownSights(const FInputActionValue& Value);
	void RotateToCameraForward();		

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

	virtual void Tick(float DeltaSeconds) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable)
	void ReturnAxe();

	UFUNCTION(BlueprintCallable)
	void ThrowAxe();

	UFUNCTION(BlueprintCallable)
	FVector BQCurvePath(float t, FVector v1, FVector v2, FVector v3);

	UFUNCTION(BlueprintCallable)
	void AxeReturnPath(FVector location, float deltatime);
};

