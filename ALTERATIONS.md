# Alterations Implemented So Far

This document records the project changes made around the CiA 402 integration work.

## Repository-Level Changes

### CANopenNode Submodule Migration

The `CANopenNode` submodule was moved away from the upstream `CANopenNode/CANopenNode` repository and pointed at the CiA 402 implementation fork:

- Submodule URL changed to `git@github.com:alexandre-ulx/CANopenNode-linux-CIA402.git`.
- Submodule branch changed to `CiA_402_implementation`.
- Submodule commit updated from `891852ce39f4be23ddea7aef9bf44546a28dbcae` to `83b074f811fabb1cd515cf46b78fb7cd50c8e4a1`.

The active submodule commit is:

```text
83b074f feat: Add CiA 402 motion config validation
```

There is also one current uncommitted cleanup inside the submodule:

- `CANopenNode/CANopen.c`: removed a debug `printf("CO_CONFIG_CIA402 = %d\n", CO_CONFIG_CIA402);` from `CO_process()`.

## STM32 Wrapper Integration

### CiA 402 Stack Enablement

`CANopenNode_STM32/CO_driver_target.h` now enables CiA 402 support at compile time:

```c
#define CO_CONFIG_CIA402 0x01U
```

This activates CiA 402 code paths from the forked CANopenNode stack when the project is built.

### CANopen App Configuration

`CANopenNode_STM32/CO_app_STM32.c` was updated so the STM32 wrapper configures one CiA 402 instance when CiA 402 is enabled:

- The local `CO_config_t co_config` was made `static`.
- `co_config.CNT_CIA402 = 1` is assigned under the `CO_CONFIG_CIA402` compile-time guard.
- `CO_CANopenInitCiA402Hw()` is called during communication reset before PDO initialization.

This means the hardware abstraction for the CiA 402 drive profile is registered as part of the CANopen reset/reinitialization flow.

### STM32 App State Extension

`CANopenNode_STM32/CO_app_STM32.h` extends `CANopenNodeSTM32` with two CiA 402 hardware binding fields:

```c
void* cia402HwObject;
const CO_CiA402_hwInterface_t* cia402Hw;
```

These allow the application layer to pass a drive-specific object and callback table into the CANopen stack without hardcoding board-specific drive behavior in the wrapper.

## Object Dictionary Changes

### DS402 Profile File

Added:

```text
CANopenNode_STM32/DS402_profile.xpd
```

This is a CANopenEditor-generated XPD file containing DS402/CiA 402 profile definitions. It includes a note that the canonical CiA 402 additions in this branch are maintained in generated OD files and related DS301 profile files, and that the XPD should be regenerated with CANopenEditor before being treated as canonical source.

### Generated OD Expansion

`CANopenNode_STM32/OD.c` and `CANopenNode_STM32/OD.h` were regenerated/expanded for CiA 402.

Major changes:

- Added `OD_CNT_CIA402 1`.
- Replaced the prior simple `0x6000 velocity` application object with CiA 402 drive profile objects.
- Added OD storage, metadata, and list entries for the following CiA 402 indexes:

```text
0x603F errorCode
0x6040 controlword
0x6041 statusword
0x605A quickStopOptionCode
0x605B shutdownOptionCode
0x605C disableOperationOptionCode
0x605D haltOptionCode
0x605E faultReactionOptionCode
0x6060 modesOfOperation
0x6061 modesOfOperationDisplay
0x6064 positionActualValue
0x606C velocityActualValue
0x6071 targetTorque
0x6072 maxTorque
0x6077 torqueActualValue
0x607A targetPosition
0x607D softwarePositionLimit
0x6081 profileVelocity
0x6083 profileAcceleration
0x6084 profileDeceleration
0x6085 quickStopDeceleration
0x6086 motionProfileType
0x6098 homingMethod
0x60FF targetVelocity
```

The generated object dictionary now exposes the standard CiA 402 control, status, mode, motion target, feedback, option-code, and limit objects required by the forked stack.

## STM32G0 RTOS Example Changes

### Eclipse/CDT Project Fixes

`examples/stm32g0xx_fdcan_rtos/.cproject` was corrected so the RTOS example can see the STM32 wrapper and CANopenNode sources consistently:

- Fixed include path casing from `CANOpenNode_STM32` to `CANopenNode_STM32`.
- Added `CANopenNode` and `CANopenNode_STM32` include paths to the other build configuration.
- Added `CANopenNode` and `CANopenNode_STM32` source entries.
- Excluded `example/` and `test/` from the `CANopenNode` source entry to avoid building non-target sources.
- Added the missing final newline.

### Stub CiA 402 Hardware Interface

`examples/stm32g0xx_fdcan_rtos/Core/Src/app_freertos.c` now contains a minimal software-backed CiA 402 drive interface for the example app.

Added `AppCia402Drive`:

```c
typedef struct {
  CO_CiA402_feedback_t feedback;
  int8_t mode;
} AppCia402Drive;
```

Added a static example drive object initialized with:

- `targetReached = true`
- `mode = CO_CiA402_MODE_NONE`

Added callback implementations for:

- voltage enable
- switch-on
- operation enable
- quick stop
- halt
- mode selection
- fault reset
- profile position
- profile velocity
- profile torque
- homing
- cyclic synchronous position
- cyclic synchronous velocity
- cyclic synchronous torque
- feedback readback

The example callback table is assigned to the STM32 wrapper before `canopen_app_init()`:

```c
canOpenNodeSTM32.cia402HwObject = &cia402Drive;
canOpenNodeSTM32.cia402Hw = &cia402Hw;
```

The callbacks are intentionally stubbed: they update software feedback fields and return success, but they do not drive real motor hardware. This gives the CANopen/CiA 402 state machine a valid hardware interface for integration testing and example builds.

## CANopenNode Fork-Level Changes

The selected `CANopenNode` submodule commit includes the CiA 402 stack work itself. The relevant commits in the fork are:

```text
355851a start CIA 402
1dd3d78 news CIA 402 part 2
5f8be73 Add CiA 402 cyclic synchronous mode support
83b074f feat: Add CiA 402 motion config validation
```

### Initial CiA 402 Stack

The fork added:

- `402/CO_CiA402.c`
- `402/CO_CiA402.h`
- `402/README_CiA402.md`

It also modified:

- `301/CO_config.h`
- `CANopen.c`
- `CANopen.h`

Functionally, this introduced:

- `CO_CONFIG_CIA402` compile-time configuration.
- `CO_CiA402_t` drive-profile object.
- CiA 402 state machine states.
- CiA 402 controlword/statusword processing.
- CiA 402 hardware callback interface.
- `CO_CANopenInitCiA402Hw()` registration hook.
- CiA 402 processing from the normal `CO_process()` loop.

### Mode and Hardware Callback Support

The fork added support for these operation modes:

- Profile position
- Profile velocity
- Profile torque
- Homing
- Cyclic synchronous position
- Cyclic synchronous velocity
- Cyclic synchronous torque

The hardware callback interface was expanded so the application can implement mode-specific behavior and feedback reporting.

### Motion Configuration Handling

The fork added `CO_CiA402_motionConfig_t` and passes motion configuration into relevant callbacks.

This includes reading OD-provided configuration such as:

- software position limits
- profile velocity
- profile acceleration
- profile deceleration
- quick-stop deceleration
- motion profile type
- option-code behavior
- max torque

### Validation and Fault Handling

The fork now validates motion commands before dispatch:

- Position targets are checked against configured software position limits.
- Torque targets are checked against configured maximum torque.
- Invalid targets report internal-limit style faults through the CiA 402 fault path.
- Failed hardware callbacks report generic faults.
- Feedback-reported faults transition the drive through fault reaction and fault states.

### Example and Test Additions in the Fork

The fork updated `example/main_blank.c` with a sample CiA 402 hardware interface and callback registration.

It also added test infrastructure:

- `test/Makefile`
- `test/test_cia402.c`
- generated `test/test_cia402` binary in the current submodule commit

The tests cover core CiA 402 behavior such as:

- state transitions
- operation-mode handling
- optional mode availability
- cyclic synchronous mode callbacks
- fault reset behavior
- motion configuration propagation
- position and torque limit validation

## Current Working Tree State

At the time this document was written:

- Top-level repo has this new documentation file plus a dirty `CANopenNode` submodule marker.
- Inside `CANopenNode`, only `CANopen.c` is modified.
- That submodule modification is limited to removing the temporary debug print from `CO_process()`.

