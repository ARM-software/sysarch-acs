# UART Platform Configuration Guide  

This document explains **how to fill the platform override macros** used by SBSA ACS-style firmware code to describe a **console UART** override table.

---

## 1) What override is used for

The override table tells the OS **which serial port** firmware used for early console / redirection, and **how to program it** (base address, interrupt routing, baud rate hints, etc.).

If override is correct, you typically get:
- early boot logs on the same UART firmware uses,
- a reliable serial console even before a full-featured driver loads.

If this table is wrong, you typically see:
- no output on expected UART,
- output but with garbage (wrong baud/clock),
- interrupts misrouted (if interrupt-driven console is used).

---

## 2) Typical platform override mapping

RD N2 sample macros:

```c
#define UART_ADDRESS                     0xF98DFE18
#define BASE_ADDRESS_ADDRESS_SPACE_ID    0x0
#define BASE_ADDRESS_REGISTER_BIT_WIDTH  0x20
#define BASE_ADDRESS_REGISTER_BIT_OFFSET 0x0
#define BASE_ADDRESS_ADDRESS_SIZE        0x3
#define BASE_ADDRESS_ADDRESS             0x2A400000
#define INTERFACE_TYPE                   8
#define UART_IRQ                         0
#define UART_BAUD_RATE                   0x7
#define UART_BAUD_RATE_BPS               115200
#define UART_CLK_IN_HZ                   24000000
#define UART_GLOBAL_SYSTEM_INTERRUPT     0x70
#define UART_PCI_DEVICE_ID               0xFFFF
#define UART_PCI_VENDOR_ID               0xFFFF
#define UART_PCI_BUS_NUMBER              0x0
#define UART_PCI_DEV_NUMBER              0x0
#define UART_PCI_FUNC_NUMBER             0x0
#define UART_PCI_FLAGS                   0x0
#define UART_PCI_SEGMENT                 0x0
```

Conceptually this set covers:

- **Base Address (GAS)**  
  → `BASE_ADDRESS_*` macros  
- **Interface Type**  
  → `INTERFACE_TYPE`  
- **Interrupt routing**  
  → `UART_IRQ` and `UART_GLOBAL_SYSTEM_INTERRUPT` (and the Interrupt Type mask in implementation)  
- **Baud/clock fields**  
  → `UART_BAUD_RATE`, `UART_BAUD_RATE_BPS`, `UART_CLK_IN_HZ`  
- **PCI identity fields (optional)**  
  → `UART_PCI_*` (usually all-FFFF / 0 for MMIO UART)

> **Note:** `UART_ADDRESS` is often *not* a field itself; it is commonly used by platform code as the UART base to program/debug and may be used to derive GAS base address. Treat it as “platform’s UART register base” unless your codebase defines it differently.

---

## 3) How to fill each field for a new platform

### 3.1 `BASE_ADDRESS_*` — “Base Address” Generic Address Structure (GAS)

The offset 40 provides a **12-byte GAS** describing where UART registers live.

Your macros correspond to the GAS members:

| GAS Field | Meaning | Platform Macro |
|---|---|---|
| Address Space ID | MMIO vs I/O space | `BASE_ADDRESS_ADDRESS_SPACE_ID` |
| Register Bit Width | register access width | `BASE_ADDRESS_REGISTER_BIT_WIDTH` |
| Register Bit Offset | bit offset within access | `BASE_ADDRESS_REGISTER_BIT_OFFSET` |
| Access Size | 1/2/3/4/… byte access | `BASE_ADDRESS_ADDRESS_SIZE` |
| Address | base address | `BASE_ADDRESS_ADDRESS` |

#### (A) `BASE_ADDRESS_ADDRESS_SPACE_ID`
Use:
- `0x0` = **System Memory** (MMIO) → *most Arm SoCs*
- `0x1` = **System I/O** (x86 legacy IO ports like COM1 0x3F8)

For almost all modern Arm platforms: **set `0x0`.**

#### (B) `BASE_ADDRESS_ADDRESS`
This is the **UART register block base** used by firmware for console.

How to obtain:
- SoC TRM / platform memory map (UART controller base)
- device tree used in pre-ACPI environments (serial node base)
- bootloader debug config (often “earlycon” address)

**Must match the UART instance actually used for console.**

#### (C) `BASE_ADDRESS_REGISTER_BIT_WIDTH`
This expresses the typical register access width the OS should use.

Common choices:
- `0x20` (32-bit) for MMIO UARTs with 32-bit registers
- `0x08` (8-bit) for byte-register UARTs / 16550-like layouts

**Rule of thumb:** pick the native register width of the UART register interface that firmware uses.

#### (D) `BASE_ADDRESS_REGISTER_BIT_OFFSET`
Usually `0x0`.

Only change if the UART registers are not aligned at bit 0 within the access. That’s rare.

#### (E) `BASE_ADDRESS_ADDRESS_SIZE`
This describes the access size encoding used by GAS (ACPI-defined):
- `0` = undefined
- `1` = byte access
- `2` = word (16-bit)
- `3` = dword (32-bit)
- `4` = qword (64-bit)

For a 32-bit MMIO UART interface: **use `0x3`.**  
For an 8-bit register interface: **use `0x1`.**

---

### 3.2 `INTERFACE_TYPE` — SPCR “Interface Type”
Offset 36 selects the UART programming model.

For revision 2+, it refers to **DBG2 Serial Port Subtypes** (Table 3 of DBG2 spec). In many Arm server platforms:
- `8` commonly indicates **ARM PL011 UART** subtype.

How to fill for a new platform:
1. Identify your UART IP:
   - ARM PL011? SBSA generic UART? 16550-compatible? vendor-specific?
2. Map it to the appropriate DBG2 serial port subtype value expected by your firmware/OS ecosystem.
3. Use that numeric value in `INTERFACE_TYPE`.

**Pitfall:** If you put `8` but your UART is 16550-compatible, OS may program it incorrectly.

---

### 3.3 Interrupt routing fields

This describes interrupt usage via:
- Interrupt Type bitmask (IRQ vs GSI, and which controller model),
- IRQ number (8259 legacy only),
- Global System Interrupt (GSIV) for APIC/SAPIC/GIC/PLIC etc.

Your override set includes:
- `UART_IRQ`
- `UART_GLOBAL_SYSTEM_INTERRUPT`

#### (A) `UART_IRQ`
This is only meaningful if the platform uses a PC-AT 8259-style IRQ routing (legacy x86).

For Arm server platforms:
- set `UART_IRQ = 0` (placeholder / unused) unless your code explicitly requires otherwise.

#### (B) `UART_GLOBAL_SYSTEM_INTERRUPT`
This is the UART’s **GSIV** if you intend to advertise interrupt-driven console.

How to obtain:
- From platform interrupt map (GIC SPI number used by UART)
- From device tree interrupt spec (SPI ID)
- From GIC integration documentation

**Important constraints for Arm GIC in SPCR:**
- GSIV must **not** be in `{0..31}` (SGI/PPI range)  
- and must **not** be in `{1056..1119}` (reserved/forbidden range as noted)

In practice, UART console interrupt is usually an **SPI** ≥ 32.

#### (C) Interrupt Type bitmask (often inside platform code)
You didn’t show a macro for “Interrupt Type”, but your platform code likely sets it based on architecture.

For an Arm GIC-based system:
- set the **ARM GIC** bit (typically Bit[3]) in Interrupt Type.
- If your implementation supports polled console only, set Interrupt Type = 0.

**Recommended:** If your firmware/OS uses polling for early console, it is acceptable to set Interrupt Type = 0 and rely on base address + interface type.

---

### 3.4 Baud rate + UART clock fields

Provides:
- Configured Baud Rate (enumerated)
- Precise Baud Rate (exact)
- UART Clock Frequency (revision-dependent)

Your macros include:
- `UART_BAUD_RATE`
- `UART_BAUD_RATE_BPS`
- `UART_CLK_IN_HZ`

#### (A) `UART_BAUD_RATE`
This is the “Configured Baud Rate” enumerated field.

Common values:
- `0x7` = 115200  
- `0x6` = 57600  
- `0x4` = 19200  
- `0x3` = 9600  
- `0x0` = “as-is” (OS assumes UART already configured by firmware)

**Guidance for new platform:**
- If firmware programs UART to a standard baud and you want OS to keep it: set `UART_BAUD_RATE = 0` (“as-is”).
- If you want to explicitly declare 115200: set `UART_BAUD_RATE = 0x7`.

#### (B) `UART_BAUD_RATE_BPS`
This is typically a *platform helper macro* (not a raw field) used by code to program the UART or to fill “Precise Baud Rate”.

- If your implementation uses “Precise Baud Rate”, put the exact integer rate here (e.g., `115200`).
- If it does not, still keep it consistent with `UART_BAUD_RATE`.

#### (C) `UART_CLK_IN_HZ`
For revision 3+, UART clock frequency can be supplied (in Hz) if known.

Set to:
- the UART reference clock used for divisor generation (e.g., `24000000` for 24 MHz),
- or `0` if indeterminate and you do not want to specify it.

How to obtain:
- SoC clock tree documentation
- firmware clock configuration for that UART instance
- device tree `clock-frequency` property (if known correct)

**Pitfall:** Wrong UART clock leads to wrong divisor calculations → garbled output.

---

### 3.5 PCI identity fields (only if UART is a PCI function)

Table allows describing a UART that lives behind PCI:
- `UART_PCI_DEVICE_ID`, `UART_PCI_VENDOR_ID`
- BDF: `UART_PCI_BUS_NUMBER`, `UART_PCI_DEV_NUMBER`, `UART_PCI_FUNC_NUMBER`
- `UART_PCI_FLAGS`
- `UART_PCI_SEGMENT`

For MMIO UART (SoC-integrated):
- set `UART_PCI_DEVICE_ID = 0xFFFF`
- set `UART_PCI_VENDOR_ID = 0xFFFF`
- set bus/dev/func = 0
- set flags = 0
- segment = 0

For PCI UART:
- fill in Vendor/Device ID from PCI config space,
- fill BDF of the UART function,
- segment is your PCI segment (0 for most systems).

**Rule:** If it is not a PCI device, **Vendor/Device must be `0xFFFF`.**

---

## 4) Minimal checklist for a new platform

Use this as a bring-up checklist:

1. **Pick the console UART instance**
   - Confirm which UART firmware uses for console (UEFI debug / early prints).
2. **Base Address GAS**
   - AddressSpaceID = MMIO (0)
   - Address = UART base
   - AccessSize matches register width
3. **Interface Type**
   - Match actual UART IP (PL011 vs 16550 vs other)
4. **Interrupt fields**
   - If polling console: InterruptType=0 (in table build code), GSIV optional
   - If interrupt-driven: set GSIV to UART SPI (>= 32)
5. **Baud + Clock**
   - Either “as-is” OR declare 115200 explicitly
   - If providing clock, ensure the real UART functional clock frequency
6. **PCI fields**
   - MMIO UART → Vendor/Device = 0xFFFF

---

## 5) Example template to adapt for a new platform

```c
/* SPCR / UART console platform config */

#define BASE_ADDRESS_ADDRESS_SPACE_ID        0x0              /* 0=MMIO */
#define BASE_ADDRESS_REGISTER_BIT_WIDTH      0x20             /* 0x20 for 32-bit regs, 0x08 for 8-bit regs */
#define BASE_ADDRESS_REGISTER_BIT_OFFSET     0x0
#define BASE_ADDRESS_ADDRESS_SIZE            0x3              /* 3=dword access */
#define BASE_ADDRESS_ADDRESS                 0x<UART_BASE>    /* e.g., 0x2A400000 */

#define INTERFACE_TYPE                       <DBG2_SUBTYPE>   /* e.g., 8 for PL011 */

#define UART_IRQ                             0x0              /* usually unused on Arm */
#define UART_GLOBAL_SYSTEM_INTERRUPT         0x<UART_SPI>      /* must be >= 32 if used */

#define UART_BAUD_RATE                       0x0              /* 0=as-is, or 0x7=115200 */
#define UART_BAUD_RATE_BPS                   115200           /* if used by your build */
#define UART_CLK_IN_HZ                       0                /* or <UART_CLK_HZ> if known */

#define UART_PCI_DEVICE_ID                   0xFFFF           /* non-PCI UART */
#define UART_PCI_VENDOR_ID                   0xFFFF
#define UART_PCI_BUS_NUMBER                  0x0
#define UART_PCI_DEV_NUMBER                  0x0
#define UART_PCI_FUNC_NUMBER                 0x0
#define UART_PCI_FLAGS                       0x0
#define UART_PCI_SEGMENT                     0x0
```

---
