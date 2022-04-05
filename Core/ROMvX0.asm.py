#!/usr/bin/env python3
#----------------------------------------------------------------------- 
#
#  Core video, sound and interpreter loop for Gigatron TTL microcomputer
#
#-----------------------------------------------------------------------
#
#  Main characteristics:
#
#  - 6.25 MHz clock
#  - Rendering 160x120 pixels at 6.25MHz with flexible videoline programming
#  - Must stay above 31 kHz horizontal sync --> 200 cycles/scanline
#  - Must stay above 59.94 Hz vertical sync --> 521 scanlines/frame
#  - 4 channels sound
#  - 16-bits vCPU interpreter
#  - 8-bits v6502 emulator
#  - Builtin vCPU programs (Snake, Racer, etc) loaded from unused ROM area
#  - Serial input handler, supporting ASCII input and two game controller types
#  - Serial output handler
#  - Soft reset button (keep 'Start' button down for 2 seconds)
#  - Low-level support for I/O and RAM expander (SPI and banking)
#
#-----------------------------------------------------------------------
#
#  ROM v2: Mimimal changes
#
#  DONE Snake color upgrade (just white, still a bit boring)
#  DONE Sound continuity fix
#  DONE A-C- mode (Also A--- added)
#  DONE Zero-page handling of ROM loader (SYS_Exec_88)
#  DONE Replace Screen test
#  DONE LED stopped mode
#  DONE Update font (69;=@Sc)
#  DONE Retire SYS_Reset_36 from all interfaces (replace with vReset)
#  DONE Added SYS_SetMemory_54 SYS_SetVideoMode_80
#  DONE Put in an example BASIC program? Self list, self start
#  DONE Move SYS_NextByteIn_32 out page 1 and rename SYS_LoaderNextByteIn_32
#       Same for SYS_PayloadCopy_34 -> SYS_LoaderPayloadCopy_34
#  DONE Update version number to v2a
#  DONE Test existing GT1 files, in all scan line modes
#  DONE Sanity test on HW
#  DONE Sanity test on several monitors
#  DONE Update version number to v2
#
#-----------------------------------------------------------------------
#
#  ROM v3: New applications
#
#  DONE vPulse width modulation (for SAVE in BASIC)
#  DONE Bricks
#  DONE Tetronis
#  DONE TinyBASIC v2
#  DONE TicTacToe
#  DONE SYS sprites/memcpy acceleration functions (reflections etc.)
#  DONE Replace Easter egg
#  DONE Update version number to v3
#
#-----------------------------------------------------------------------
#
#  ROM v4: Numerous small updates, no new applications
#
#  DONE #81 Support alternative game controllers (TypeC added)
#  DONE SPI: Setup SPI at power-on and add 'ctrl' instruction to asm.py
#  DONE SPI: Expander control (Enable/disable slave, set bank etc)
#  DONE SPI: SYS Exchange bytes
#  DONE SYS: Reinitialize waveforms at soft reset, not just at power on
#  DONE v6502: Prototype. Retire bootCount to free up zp variables
#  DONE v6502: Allow soft reset when v6502 is active
#  DONE Apple1: As easter egg, preload with WozMon and Munching6502
#  DONE Apple1: Don't use buttonState but serialRaw
#  DONE Snake: Don't use serialRaw but buttonState
#  DONE Snake: Head-only snake shouldn't be allowed to turn around #52
#  DONE Snake: Improve game play and colors in general
#  DONE Snake: Tweak AI. Also autoplayer can't get hiscore anymore
#  DONE Racer: Don't use serialRaw but buttonState
#  DONE Racer: Faster road setup with SYS_SetMemory
#  DONE Makefile: Pass app-specific SYS functions on command line (.py files)
#  DONE Main: "Press [A] to start": accept keyboard also (incl. 'A') #38
#  DONE Add 4 arrows to font to fill up the ROM page
#  DONE Mode 1975 (for "zombie" mode), can do mode -1 to recover
#  DONE TinyBASIC: support larger font and MODE 1975. Fix indent issue #40
#  DONE Add channelMask to switch off the higher sound channels
#  DONE Loader: clear channelMask when loading into sound channels
#  DONE Update romTypeValue and interface.json
#  DONE Update version number to v4
#  DONE Formally Support SPI and RAM expander: publish in interface.json
#  DONE Use `inspect' to make program listing with original comments #127
#
#-----------------------------------------------------------------------
#
#  Ideas for ROM v5:
#
#  DONE v6502: Test with VTL02
#  DONE v6502: Test with Microchess
#  DONE Sound: Better noise by changing wavX every frame (at least in channel 1)
#  DONE Sound demo: Play SMB Underworld tune
#  DONE SPI: Also reset state at soft reset
#  DONE Fix clobbering of 0x81 by SPI SYS functions #103
#  DONE Control variable to black out the area at the top of the screen
#  DONE Fix possible video timing error in Loader #100
#  DONE Fix zero page usage in Bricks and Tetronis #41
#  DONE Add CALLI instruction to vCPU
#  DONE Main: add Apple1 to main menu
#  DONE Replace egg with something new
#  DONE Split interface.json and interface-dev.json
#  DONE MSBASIC
#  DONE Speed up SetMemory by 300% using bursts #126
#  DONE Discoverable ROM contents #46
#  DONE Vertical blank interrupt #125
#  DONE TinyBASIC: Support hexadecimal numbers $....
#  XXX  Expander: Auto-detect banking, 64K and 128K -> needs FIX
#  DONE Cardboot: Boot from *.GT1 file if SDC/MMC detected
#  XXX  CardBoot: Strip non-essentials
#  XXX  CardBoot: Fix card type detection
#  XXX  CardBoot: Read full sector
#  DONE Apple-1: Memory mapped PIA emulation using interrupt (D010-D013)
#  DONE Apple-1: Include A1 Integer BASIC
#  DONE Apple-1: Suppress lower case
#  DONE Apple-1: Include Mastermind and 15-Puzzle
#  DONE Apple-1: Include mini-assembler
#  DONE Apple-1: Intercept cassette interface = menu
#  XXX  Reduce the Pictures application ROM footprint #120
#  XXX  Mandelbrot: add more color schemes, eg. with permutations of RGB
#  XXX  Main: Better startup chime, eg. sequence the 4 notes and then decay
#  XXX  Main: Some startup logo as intro, eg. gigatron letters from the box
#  XXX  Racer: Control speed with up/down (better for TypeC controllers)
#  XXX  Racer: Make noise when crashing
#  XXX  Loader: make noise while loading (only channel 1 is safe to use)
#  XXX  Faster SYS_Exec_88, with start address (GT1)?
#  XXX  Let SYS_Exec_88 clear channelMask when loading into live channels
#  XXX  Investigate: Babelfish sometimes freezes during power-on?
#
#  Ideas for ROM v6+
#  XXX  ROM functions: SYS_PrintString, control codes, SYS_DrawChar, SYS_Newline
#  XXX  v8080 prepping for CP/M
#  XXX  vForth virtual CPU
#  XXX  Video: Increase vertical resolution with same videoTable (160 lines?)
#  XXX  Video mode for 12.5 MHz systems
#  XXX  Pucrunch (well documented) or eximozer 3.0.2 (better compression)
#  XXX  SPI: Think about SPI modes (polarities)
#  XXX  I2C: Turn SPI port 2-3 into a I2C port as suggesred by jwolfram
#  XXX  Reset.c and Main.c (that is: port these from GCL to C, but requires LCC fixed)
#  XXX  Need keymaps in ROM? (perhaps undocumented if not tested)
#  XXX  FrogStroll (not in Contrib/)
#  XXX  How it works memo: brief description of every software function
#  XXX  Adjustable return for LUP trampolines (in case SYS functions need it)
#  XXX  Loader: make noise when data comes in
#  XXX  vCPU: Multiplication (mulShift8?)
#  XXX  vCPU: Interrupts / Task switching (e.g for clock, LED sequencer)
#  XXX  Scroll out the top line of text, or generic vertical scroll SYS call
#  XXX  SYS function for plotting a full character in one go
#  XXX  Multitasking/threading/sleeping (start with date/time clock in GCL)
#-----------------------------------------------------------------------

import importlib
from sys import argv
from os  import getenv

from asm import *
import gcl0x as gcl
import font_v4 as font

enableListing()
#-----------------------------------------------------------------------
#
#  Start of core
#
#-----------------------------------------------------------------------

# Pre-loading the formal interface as a way to get warnings when
# accidentally redefined with a different value
loadBindings('ROMvX0_interface.json')
loadBindings('Core/interface-dev.json') # Provisional values for DEVROM

# Gigatron clock
cpuClock = 6.250e+06

# Output pin assignment for VGA
R, G, B, hSync, vSync = 1, 4, 16, 64, 128
syncBits = hSync+vSync # Both pulses negative

# When the XOUT register is in the circuit, the rising edge triggers its update.
# The loop can therefore not be agnostic to the horizontal pulse polarity.
assert syncBits & hSync != 0

# VGA 640x480 defaults (to be adjusted below!)
vFront = 10     # Vertical front porch
vPulse = 2      # Vertical sync pulse
vBack  = 33     # Vertical back porch
vgaLines = vFront + vPulse + vBack + 480
vgaClock = 25.175e+06

# Video adjustments for Gigatron
# 1. Our clock is (slightly) slower than 1/4th VGA clock. Not all monitors will
#    accept the decreased frame rate, so we restore the frame rate to above
#    minimum 59.94 Hz by cutting some lines from the vertical front porch.
vFrontAdjust = vgaLines - int(4 * cpuClock / vgaClock * vgaLines)
vFront -= vFrontAdjust
# 2. Extend vertical sync pulse so we can feed the game controller the same
#    signal. This is needed for controllers based on the 4021 instead of 74165
vPulseExtension = max(0, 8-vPulse)
vPulse += vPulseExtension
# 3. Borrow these lines from the back porch so the refresh rate remains
#    unaffected
vBack -= vPulseExtension

# Start value of vertical blank counter
videoYline0 = 1-2*(vFront+vPulse+vBack-2)

# Mismatch between video lines and sound channels
soundDiscontinuity = (vFront+vPulse+vBack) % 4

# QQVGA resolution
qqVgaWidth      = 160
qqVgaHeight     = 120

# Game controller bits (actual controllers in kit have negative output)
# +----------------------------------------+
# |       Up                        B*     |
# |  Left    Right               B     A*  |
# |      Down      Select Start     A      |
# +----------------------------------------+ *=Auto fire
buttonRight     = 1
buttonLeft      = 2
buttonDown      = 4
buttonUp        = 8
buttonStart     = 16
buttonSelect    = 32
buttonB         = 64
buttonA         = 128

#-----------------------------------------------------------------------
#
#  RAM page 0: zero-page variables
#
#-----------------------------------------------------------------------

# Memory size in pages from auto-detect
memSize         = zpByte()

# Active interpreter page, swapped with channel, so that we can do a one
# instruction vCpuSelect reset, i.e. st(vCpuSelect,[vCpuSelect])
vCpuSelect      = zpByte()

# Next sound sample being synthesized
sample          = zpByte()
# To save one instruction in the critical inner loop, `sample' is always
# reset with its own address instead of, for example, the value 0. Compare:
# 1 instruction reset
#       st sample,[sample]
# 2 instruction reset:
#       ld 0
#       st [sample]
# The difference is not audible. This is fine when the reset/address
# value is low and doesn't overflow with 4 channels added to it.
# There is an alternative, but it requires pull-down diodes on the data bus:
#       st [sample],[sample]
assert 4*63 + sample < 256
# We pin this reset/address value to 3, so `sample' swings from 3 to 255
assert sample == 3

# Former bootCount and bootCheck (<= ROMv3)
vSPH             = zpByte()
#zpReserved      = zpByte() # Recycled and still unused. Candidate future uses:
                           # - Video driver high address (for alternative video modes)
                           # - v6502: ADH offset ("MMU")
                           # - v8080: ???

# The current channel number for sound generation. Advanced every scan line
# and independent of the vertical refresh to maintain constant oscillation.
channel         = zpByte()

# Entropy harvested from SRAM startup and controller input
entropy         = zpByte(3)

# Visible video
videoY          = zpByte() # Counts up from 0 to 238 in steps of 2
                           # Counts up (and is odd) during vertical blank
videoModeB      = zpByte() # Handler for every 2nd line (pixel burst or vCPU)
videoModeC      = zpByte() # Handler for every 3rd line (pixel burst or vCPU)
videoModeD      = zpByte() # Handler for every 4th line (pixel burst or vCPU)

nextVideo       = zpByte() # Jump offset to scan line handler (videoA, B, C...)
videoPulse      = nextVideo # Used for pulse width modulation

# Frame counter is good enough as system clock
frameCount      = zpByte(1)

# Serial input (game controller)
serialRaw       = zpByte() # New raw serial read
serialLast      = zpByte() # Previous serial read
buttonState     = zpByte() # Clearable button state
resetTimer      = zpByte() # After 2 seconds of holding 'Start', do a soft reset
                           # XXX move to page 1 to free up space

# Extended output (blinkenlights in bit 0:3 and audio in bit 4:7). This
# value must be present in AC during a rising hSync edge. It then gets
# copied to the XOUT register by the hardware. The XOUT register is only
# accessible in this indirect manner because it isn't part of the core
# CPU architecture.
xout            = zpByte()
xoutMask        = zpByte() # The blinkenlights and sound on/off state

# vCPU interpreter
vTicks          = zpByte()  # Interpreter ticks are units of 2 clocks
vPC             = zpByte(2) # Interpreter program counter, points into RAM
vAC             = zpByte(2) # Interpreter accumulator, 16-bits
vLR             = zpByte(2) # Return address, for returning after CALL
vSP             = zpByte(1) # Stack pointer
vTmp            = zpByte()
vReturn         = zpByte()  # Return into video loop (in page of vBlankStart)

# Scratch
frameX          = zpByte() # Starting byte within page
frameY          = zpByte() # Page of current pixel line (updated by videoA)

# Vertical blank (reuse some variables used in the visible part)
videoSync0      = frameX   # Vertical sync type on current line (0xc0 or 0x40)
videoSync1      = frameY   # Same during horizontal pulse (0x80 or 0x00)

# Versioning for GT1 compatibility
# Please refer to Docs/GT1-files.txt for interpreting this variable
romType         = zpByte(1)

# The low 3 bits are repurposed to select the actively updated sound channels.
# Valid bit combinations are:
#  xxxxx011     Default after reset: 4 channels (page 1,2,3,4)
#  xxxxx001     2 channels at double update rate (page 1,2)
#  xxxxx000     1 channel at quadruple update rate (page 1)
# The main application for this is to free up the high bytes of page 2,3,4.
channelMask = symbol('channelMask_v4')
assert romType == channelMask

# SYS function arguments and results/scratch
sysFn           = zpByte(2)
sysArgs         = zpByte(8)

# Play sound if non-zero, count down and stop sound when zero
soundTimer      = zpByte()

# Fow now the LED state machine itself is hard-coded in the program ROM
ledTimer        = zpByte() # Number of ticks until next LED change
ledState_v2     = zpByte() # Current LED state
ledTempo        = zpByte() # Next value for ledTimer after LED state change

# All bytes above userVars, except 0x80, are potentially usable by user code
# that refrains from using ROM facilities introduced since ROMv5a.
userVars        = zpByte(0)

# [0x30-0x35]
# Saved vCPU context during vIRQ (since ROMv5a)
# Code that uses vCPU interrupts should not use these locations.
# area register save area
vIrqSave        = zpByte(6)
userVars1       = zpByte(0)

# [0x80]
# Constant 0x01. 
zpReset(0x80)
oneConst        = zpByte(1)
userVars2       = zpByte(0)  

# [0x82-0xB7]
# Scratch for certain ops and sys calls introduced in ROMvX0.
# Pending more specific information to be inserted here.
zpReset(0x82)
vX0Scratch      = zpByte(0xB8-0x82)

# [0xB8-0xBF]
# State for ROMvX0 SYS calls with vCPU callbacks.
# Pending more specific information to be inserted here.
vX0State        = zpByte(0xC0-0xB8)

# [0xC0-0xCF]
# Fixed locations for ROMvX0 opcodes that operate on long and floats.
# Pending more specific information to be inserted here.
vFAC_reserved   = zpByte(4)     # reserved for float accumulator
vLAC            = zpByte(4)     # long accumulator/continued float accumulator
vTmpL           = zpByte(4)     # long scratch register
vDST            = zpByte(2)     # destination address for copy opcodes.
vTmpW           = zpByte(2)     # word scratch register



#-----------------------------------------------------------------------
#
#  RAM page 1: video line table
#
#-----------------------------------------------------------------------

# Byte 0-239 define the video lines
videoTable      = 0x0100 # Indirection table: Y[0] dX[0]  ..., Y[119] dX[119]

vReset          = 0x01f0
vIRQ_v5         = 0x01f6
ctrlBits        = 0x01f8
videoTop_v5     = 0x01f9 # Number of skip lines

# Highest bytes are for sound channel variables
wavA = 250      # Waveform modulation with `adda'
wavX = 251      # Waveform modulation with `xora'
keyL = 252      # Frequency low 7 bits (bit7 == 0)
keyH = 253      # Frequency high 8 bits
oscL = 254      # Phase low 7 bits
oscH = 255      # Phase high 8 bits

#-----------------------------------------------------------------------
#  Memory layout
#-----------------------------------------------------------------------

userCode = 0x0200       # Application vCPU code
soundTable = 0x0700     # Wave form tables (doubles as right-shift-2 table)
screenMemory = 0x0800   # Default start of screen memory: 0x0800 to 0x7fff

#-----------------------------------------------------------------------
#  Application definitions
#-----------------------------------------------------------------------
 
maxTicks = 30//2                 # Duration of vCPU's slowest virtual opcode (ticks)
minTicks = 14//2                 # vcPU's fastest instruction
v6502_maxTicks = 38//2           # Max duration of v6502 processing phase (ticks)

runVcpu_overhead = 5            # Caller overhead (cycles)
vCPU_overhead = 9               # Callee overhead of jumping in and out (cycles)
v6502_overhead = 11             # Callee overhead for v6502 (cycles)

v6502_adjust = (v6502_maxTicks - maxTicks) + (v6502_overhead - vCPU_overhead)//2
assert v6502_adjust >= 0        # v6502's overhead is a bit more than vCPU

def runVcpu(n, ref=None, returnTo=None):
  """Macro to run interpreter for exactly n cycles. Returns 0 in AC.

  - `n' is the number of available Gigatron cycles including overhead.
    This is converted into interpreter ticks and takes into account
    the vCPU calling overheads. A `nop' is inserted when necessary
    for alignment between cycles and ticks.
  - `returnTo' is where program flow continues after return. If not set
     explicitely, it will be the first instruction behind the expansion.
  - If another interpreter than vCPU is active (v6502...), that one
    must adjust for the timing differences, because runVcpu wouldn't know."""

  overhead = runVcpu_overhead + vCPU_overhead
  if returnTo == 0x100:         # Special case for videoZ
    overhead -= 2

  if n is None:
    # (Clumsily) create a maximum time slice, corresponding to a vTicks
    # value of 127 (giving 282 cycles). A higher value doesn't work because
    # then SYS functions that just need 28 cycles (0 excess) won't start.
    n = (127 + maxTicks) * 2 + overhead

  n -= overhead
  assert n > 0

  if n % 2 == 1:
    nop()                       # Tick alignment
    n -= 1
  assert n % 2 == 0

  print('runVcpu at $%04x net cycles %3s info %s' % (pc(), n, ref))

  if returnTo != 0x100:
    if returnTo is None:
      returnTo = pc() + 5       # Next instruction
    ld(lo(returnTo))            #0
    st([vReturn])               #1

  n //= 2
  n -= maxTicks                 # First instruction always runs
  assert n < 128
  assert n >= v6502_adjust

  ld([vCpuSelect],Y)            #2
  jmp(Y,'ENTER')                #3
  ld(n)                         #4
assert runVcpu_overhead ==       5

#-----------------------------------------------------------------------
#       v6502 definitions
#-----------------------------------------------------------------------

# Registers are zero page variables
v6502_PC        = vLR           # Program Counter
v6502_PCL       = vLR+0         # Program Counter Low
v6502_PCH       = vLR+1         # Program Counter High
v6502_S         = vSP           # Stack Pointer (kept as "S+1")
v6502_A         = vAC+0         # Accumulator
v6502_BI        = vAC+1         # B Input Register (used by SBC)
v6502_ADL       = sysArgs+0     # Low Address Register
v6502_ADH       = sysArgs+1     # High Address Register
v6502_IR        = sysArgs+2     # Instruction Register
v6502_P         = sysArgs+3     # Processor Status Register (V flag in bit 7)
v6502_Qz        = sysArgs+4     # Quick Status Register for Z flag
v6502_Qn        = sysArgs+5     # Quick Status Register for N flag
v6502_X         = sysArgs+6     # Index Register X
v6502_Y         = sysArgs+7     # Index Register Y
v6502_Tmp       = vTmp          # Scratch (may be clobbered outside v6502)

# MOS 6502 definitions for P register
v6502_Cflag = 1                 # Carry Flag (unsigned overflow)
v6502_Zflag = 2                 # Zero Flag (all bits zero)
v6502_Iflag = 4                 # Interrupt Enable Flag (1=Disable)
v6502_Dflag = 8                 # Decimal Enable Flag (aka BCD mode, 1=Enable)
v6502_Bflag = 16                # Break (or PHP) Instruction Flag
v6502_Uflag = 32                # Unused (always 1)
v6502_Vflag = 64                # Overflow Flag (signed overflow)
v6502_Nflag = 128               # Negative Flag (bit 7 of result)

# In emulation it is much faster to keep the V flag in bit 7
# This can be corrected when importing/exporting with PHP, PLP, etc
v6502_Vemu = 128

# On overflow:
#       """Overflow is set if two inputs with the same sign produce
#          a result with a different sign. Otherwise it is clear."""
# Formula (without carry/borrow in!):
#       (A ^ (A+B)) & (B ^ (A+B)) & 0x80
# References:
#       http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html
#       http://6502.org/tutorials/vflag.html

# Memory layout
v6502_Stack     = 0x0000        # 0x0100 is already used in the Gigatron
#v6502_NMI      = 0xfffa
#v6502_RESET    = 0xfffc
#v6502_IRQ      = 0xfffe

#-----------------------------------------------------------------------
#
#  $0000 ROM page 0: Boot
#
#-----------------------------------------------------------------------

align(0x100, size=0x80)

# Give a first sign of life that can be checked with a voltmeter
ld(0b0000)                      # LEDs |OOOO|
ld(syncBits^hSync,OUT)          # Prepare XOUT update, hSync goes down, RGB to black
ld(syncBits,OUT)                # hSync goes up, updating XOUT

# Setup I/O and RAM expander
ctrl(0b01111111)                # Expansion board: (1) reset signal.
ctrl(0b01111100)                # (2) disable SPI slaves, enable RAM bank 1
#      ^^^^^^^^
#      |||||||`-- SCLK
#      ||||||`--- Not connected
#      |||||`---- /SS0
#      ||||`----- /SS1
#      |||`------ /SS2
#      ||`------- /SS3
#      |`-------- B0
#      `--------- B1
# bit15 --------- MOSI = 0

# Simple RAM test and size check by writing to [1<<n] and see if [0] changes or not.
ld(1)                           # Quick RAM test and count
label('.countMem0')
st([memSize],Y)                 # Store in RAM and load AC in Y
ld(255)
xora([Y,0])                     # Invert value from memory
st([Y,0])                       # Test RAM by writing the new value
st([0])                         # Copy result in [0]
xora([Y,0])                     # Read back and compare if written ok
bne(pc())                       # Loop forever on RAM failure here
ld(255)
xora([Y,0])                     # Invert memory value again
st([Y,0])                       # To restore original value
xora([0])                       # Compare with inverted copy
beq('.countMem1')               # If equal, we wrapped around
ld([memSize])
bra('.countMem0')               # Loop to test next address line
adda(AC)                        # Executes in the branch delay slot!
label('.countMem1')

# Momentarily wait to allow for debouncing of the reset switch by spinning
# roughly 2^15 times at 2 clocks per loop: 6.5ms@10MHz to 10ms@6.3MHz
# Real-world switches normally bounce shorter than that.
# "[...] 16 switches exhibited an average 1557 usec of bouncing, with,
#  as I said, a max of 6200 usec" (From: http://www.ganssle.com/debouncing.htm)
# Relevant for the breadboard version, as the kit doesn't have a reset switch.

ld(255)                         # Debounce reset button
label('.debounce')
st([0])
bne(pc())
suba(1)                         # Branch delay slot
ld([0])
bne('.debounce')
suba(1)                         # Branch delay slot

# Update LEDs (memory is present and counted, reset is stable)
ld(0b0001)                      # LEDs |*OOO|
ld(syncBits^hSync,OUT)
ld(syncBits,OUT)

# Scan the entire RAM space to collect entropy for a random number generator.
# The 16-bit address space is scanned, even if less RAM was detected.
ld(0)                           # Collect entropy from RAM
st([vAC+0],X)
st([vAC+1],Y)
label('.initEnt0')
ld([entropy+0])
bpl('.initEnt1')
adda([Y,X])
xora(191)
label('.initEnt1')
st([entropy+0])
ld([entropy+1])
bpl('.initEnt2')
adda([entropy+0])
xora(193)
label('.initEnt2')
st([entropy+1])
adda([entropy+2])
st([entropy+2])
ld([vAC+0])
adda(1)
bne('.initEnt0')
st([vAC+0],X)
ld([vAC+1])
adda(1)
bne('.initEnt0')
st([vAC+1],Y)

# Update LEDs
ld(0b0011)                      # LEDs |**OO|
ld(syncBits^hSync,OUT)
ld(syncBits,OUT)
 
# vCPU reset handler
ld((vReset&255)-2)              # Setup vCPU reset handler
st([vPC])
adda(2,X)
ld(vReset>>8)
st([vPC+1],Y)
st('LDI',             [Y,Xpp])
st('SYS_Reset_88',    [Y,Xpp])
st('STW',             [Y,Xpp])
st(sysFn,             [Y,Xpp])
st('SYS',             [Y,Xpp])  # SYS -> SYS_Reset_88 -> SYS_Exec_88
st(256-88//2+maxTicks,[Y,Xpp])
st(0,                 [Y,Xpp])  # vIRQ_v5: Disable interrupts
st(0,                 [Y,Xpp])  # vIRQ_v5
st(0b11111100,        [Y,Xpp])  # Control register
st(0,                 [Y,Xpp])  # videoTop

ld(hi('ENTER'))                 # Active interpreter (vCPU,v6502) = vCPU
st([vCpuSelect])

ld(255)                         # Setup serial input
st([frameCount])
st([serialRaw])
st([serialLast])
st([buttonState])
st([resetTimer])                # resetTimer<0 when entering Main.gcl

ld(0b0111)                      # LEDs |***O|
ld(syncBits^hSync,OUT)
ld(syncBits,OUT)

ld(0)
st([0])                         # Carry lookup ([0x80] in 1st line of vBlank)
st([channel])
st([soundTimer])

ld(0b1111)                      # LEDs |****|
ld(syncBits^hSync,OUT)
ld(syncBits,OUT)
st([xout])                      # Setup for control by video loop
st([xoutMask])

ld(hi('startVideo'),Y)          # Enter video loop at vertical blank
jmp(Y,'startVideo')
st([ledState_v2])               # Setting to 1..126 means "stopped"

#-----------------------------------------------------------------------
# Extension SYS_Reset_88: Soft reset
#-----------------------------------------------------------------------

# SYS_Reset_88 initiates an immediate Gigatron reset from within the vCPU.
# The reset sequence itself is mostly implemented in GCL by Reset.gcl,
# which must first be loaded into RAM. But as that takes more than 1 scanline,
# some vCPU bootstrapping code gets loaded with SYS_Exec_88.
# !!! This function was REMOVED from interface.json
# !!! Better use vReset as generic entry point for soft reset

# ROM type (see also Docs/GT1-files.txt)
romTypeValue = symbol('romTypeValue_ROMvX0')

label('SYS_Reset_88')
assert pc()>>8 == 0
assert (romTypeValue & 7) == 0
ld(hi('sys_Reset_88'),Y)        #15
jmp(Y,'sys_Reset_88')           #16
ld(romTypeValue)                #17 Set ROM type/version and clear channel mask

 
# STPX implementation
label('stpx#13')
ld([X])                         #13,
st([vTmp])                      #14,
st([Y,Xpp])                     #15, relies on being in page 0!
ld([X])                         #16, y
adda([X])                       #17, y*2
ld(AC,X)                        #18,
ld(1,Y)                         #19, Y,X = 0x0100 + 2*y, (0 >= y <= 127)
ld([Y,X])                       #20,
ld(AC,Y)                        #21, Y = [Y,X]
ld([sysArgs+7],X)               #22,
ld([X])                         #23, colour
ld([vTmp],X)                    #24,
st([Y,X])                       #25, [Y,X] = colour
ld(hi('NEXTY'),Y)               #26,
jmp(Y,'NEXTY')                  #27,
ld(-30/2)                       #28,


align(0x80, size=0x80)
assert pc() == 0x80


# ADDW moved to page0 to free up room for more instruction slots in page3
# This implemetation of ADDW relies on Y=0 and hence cannot be dispatched from any other page
label('addw#13')
ld([vAC])                       #13 Low byte
st([vTmp])                      #14 Store low result
adda([X])                       #15
st([vAC])                       #16
bmi('.addw#19')                 #17 Now figure out if there was a carry
ld([X])                         #18
st([Y,Xpp])                     #29
ora([vTmp])                     #20
bmi('.addw#23')                 #21 If Carry == 1
ld([X])                         #22
adda([vAC+1])                   #23
st([vAC+1])                     #24 Store high result
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27
label('.addw#19')
st([Y,Xpp])                     #19
anda([vTmp])                    #20
bmi('.addw#23')                 #21 If Carry == 1
ld([X])                         #22
adda([vAC+1])                   #23
st([vAC+1])                     #24 Store high result
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27
label('.addw#23')
adda(1)                         #23
adda([vAC+1])                   #24
st([vAC+1])                     #25 Store high result
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28


#-----------------------------------------------------------------------
# Placeholders for future SYS functions. This works as a kind of jump
# table. The indirection allows SYS implementations to be moved around
# between ROM versions, at the expense of 2 clock cycles (or 1). When
# the function is not present it just acts as a NOP. Of course, when a
# SYS function must be patched or extended it needs to have budget for
# that in its declared maximum cycle count.
#
# Technically the same goal can be achieved by starting each function
# with 2 nop's, or by overdeclaring their duration in the first place
# (a bit is still wise to do). But this can result in fragmentation
# of future ROM images. The indirection avoids that.
#
# An added advantage of having these in ROM page 0 is that it saves one
# byte when setting sysFn: LDI+STW (4 bytes) instead of LDWI+STW (5 bytes)
#-----------------------------------------------------------------------

label('SYS_Multiply_s16_vX_66')
ld(hi('sys_Multiply_s16'),Y)    #15 slot 0x9e
jmp(Y,'sys_Multiply_s16')       #16
ld([sysArgs+6])                 #17 load mask.lo

label('SYS_Divide_s16_vX_80')
ld(hi('sys_Divide_s16'),Y)      #15 slot 0xa1
jmp(Y,'sys_Divide_s16')         #16
ld([sysArgs+4])                 #17 

label('SYS_DrawLine_vX_86')
ld(hi('sys_DrawLine'),Y)        #15 slot 0xa4
jmp(Y,'sys_DrawLine')           #16
ld([0xA3])                      #17 fgcolour

ld(hi('REENTER'),Y)             #15 slot 0xa7
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17

label('SYS_WaitVBlank_vX_28')
ld(hi('sys_WaitVBlank'),Y)      #15 slot 0xaa
jmp(Y,'sys_WaitVBlank')         #16
ld([videoY])                    #17 scanline Y

#-----------------------------------------------------------------------
# Extension SYS_Exec_88: Load code from ROM into memory and execute it
#-----------------------------------------------------------------------
#
# This loads the vCPU code with consideration of the current vSP
# Used during reset, but also for switching between applications or for
# loading data from ROM from within an application (overlays).
#
# ROM stream format is [<addrH> <addrL> <n&255> n*<byte>]* 0
# on top of lookup tables.
#
# Variables:
#       sysArgs[0:1]    ROM pointer (in)
#       sysArgs[2:3]    RAM pointer (changed)
#       sysArgs[4]      State counter (changed)
#       vLR             vCPU continues here (in)

label('SYS_Exec_88')
ld(hi('sys_Exec'),Y)            #15 slot 0xad
jmp(Y,'sys_Exec')               #16
ld(0)                           #17 Address of loader on zero page

ld(hi('REENTER'),Y)             #15 slot 0xb0
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17

ld(hi('REENTER'),Y)             #15 slot 0xb3
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17

label('SYS_SpritePattern_vX_134')
ld(hi('sys_SpritePattern'),Y)   #15 slot 0xb6
jmp(Y,'sys_SpritePattern')      #16
ld([sysArgs+0],X)               #17 src.lo

label('SYS_SortUint8Array_vX_52')
ld(hi('sys_SortUint8Array'),Y)  #15 slot 0xb9
jmp(Y,'sys_SortUint8Array')     #16
ld([sysArgs+1],Y)               #17 array pointer is in sysArgs0

label('SYS_SortSprites_vX_62')
ld(hi('sys_SortSprites'),Y)     #15 slot 0xbc
jmp(Y,'sys_SortSprites')        #16
ld([sysArgs+1],Y)               #17 array pointer is in sysArgs0

label('SYS_SortViaIndices_vX_44')
ld(hi('sys_SortViaIndices'),Y)  #15 slot 0xbf
jmp(Y,'sys_SortViaIndices')     #16
ld([sysArgs+1],Y)               #17 src pointer

label('SYS_MemCopyByte_vX_40')
ld(hi('sys_MemCopyByte'),Y)     #15 slot 0xc2
jmp(Y,'sys_MemCopyByte')        #16
ld([sysArgs+1],Y)               #17 src pointer

label('SYS_MemCopyWord_vX_48')
ld(hi('sys_MemCopyWord'),Y)     #15 slot 0xc5
jmp(Y,'sys_MemCopyWord')        #16
ld([sysArgs+1],Y)               #17 src pointer

label('SYS_MemCopyDWord_vX_58')
ld(hi('sys_MemCopyDWord'),Y)    #15 slot 0xc8
jmp(Y,'sys_MemCopyDWord')       #16
ld([sysArgs+1],Y)               #17 src pointer

label('SYS_DrawVLine_vX_66')
ld(hi('sys_DrawVLine'),Y)       #15 slot 0xcb
jmp(Y,'sys_DrawVLine')          #16
ld([sysArgs+3],Y)               #17

label('SYS_DrawSprite_vX_132')
ld(hi('sys_DrawSprite'),Y)      #15 slot 0xce
jmp(Y,'sys_DrawSprite')         #16
ld([0x82],X)                    #17 spriteX

label('SYS_DrawBullet_vX_140')
ld(hi('sys_DrawBullet'),Y)      #15 slot 0xd1
jmp(Y,'sys_DrawBullet')         #16
ld([0x82],X)                    #17, bulletX

label('SYS_CmpByteBounds_vX_54')
ld(hi('sys_CmpByteBounds'),Y)   #15 slot 0xd4
jmp(Y,'sys_CmpByteBounds')      #16
ld([0xBF])                      #17 count

label('SYS_Divide_u168_vX_56')
ld(hi('sys_Divide_u168'),Y)     #15 slot 0xd7
jmp(Y,'sys_Divide_u168')        #16
ld([sysArgs+1])                 #17 

label('SYS_ReadPixel_vX_32')
ld(hi('sys_ReadPixel'),Y)       #15 slot 0xda
jmp(Y,'sys_ReadPixel')          #16
ld([vAC+1])                     #17 y

label('SYS_DrawPixel_vX_30')
ld(hi('sys_DrawPixel'),Y)       #15 slot 0xdd
jmp(Y,'sys_DrawPixel')          #16
ld([sysArgs+1])                 #17 y

ld(hi('REENTER'),Y)             #15 slot 0xe0
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17

#-----------------------------------------------------------------------
# Extension SYS_ScanMemoryExt_vX_50
#-----------------------------------------------------------------------

# SYS function for searching a byte in a 0 to 256 bytes string located
# in a different bank. Doesn't cross page boundaries. Returns a
# pointer to the target if found or zero. Temporarily deselects SPI
# devices.
#
# sysArgs[0:1]            Start address
# sysArgs[2], sysArgs[3]  Bytes to locate in the string
# vACL                    Length of the string (0 means 256)
# vACH                    Bit 6 and 7 contain the bank number

label('SYS_ScanMemoryExt_vX_50')
ld(hi('sys_ScanMemoryExt'),Y)   #15 slot 0xe3
jmp(Y,'sys_ScanMemoryExt')      #16
ld([vAC+1])                     #17


#-----------------------------------------------------------------------
# Extension SYS_ScanMemory_vX_50
#-----------------------------------------------------------------------

# SYS function for searching a byte in a 0 to 256 bytes string.
# Returns a pointer to the target if found or zero.  Doesn't cross
# page boundaries.

#
# sysArgs[0:1]            Start address
# sysArgs[2], sysArgs[3]  Bytes to locate in the string
# vACL                    Length of the string (0 means 256)

label('SYS_ScanMemory_vX_50')
ld(hi('sys_ScanMemory'),Y)      #15 slot 0xe6
jmp(Y,'sys_ScanMemory')         #16
ld([sysArgs+1],Y)               #17

#-----------------------------------------------------------------------
# Extension SYS_CopyMemory_vX_80
#-----------------------------------------------------------------------

# SYS function for copying 1..256 bytes
#
# sysArgs[0:1]    Destination address
# sysArgs[2:3]    Source address
# vAC[0]          Count (0 means 256)
#
# Doesn't cross page boundaries.
# Overwrites sysArgs[4:7] and vLR.

label('SYS_CopyMemory_vX_80')
ld(hi('sys_CopyMemory'),Y)       # 15 slot 0xe9
jmp(Y, 'sys_CopyMemory')         # 16
ld([vAC])                        # 17

#-----------------------------------------------------------------------
# Extension SYS_CopyMemoryExt_vX_100
#-----------------------------------------------------------------------

# SYS function for copying 1..256 bytes across banks
#
# sysArgs[0:1]  Destination address
# sysArgs[2:3]  Source address
# vAC[0]        Count (0 means 256)
# vAC[1]        Bits 7 and 6 contain the destination bank number,
#               and bits 5 and 4 the source bank number.
#
# Doesn't cross page boundaries.
# Overwrites sysArgs[4:7], vLR, and vTmp.
# Temporarily deselect all SPI devices.
# Should not call without expansion board

label('SYS_CopyMemoryExt_vX_100')
ld(hi('sys_CopyMemoryExt'),Y)    # 15 slot 0xec
jmp(Y, 'sys_CopyMemoryExt')      # 16
ld([vAC+1])                      # 17

#-----------------------------------------------------------------------
# Extension SYS_ReadRomDir_v5_80 
#-----------------------------------------------------------------------

# Get next entry from ROM file system. Use vAC=0 to get the first entry.

# Variables:
#       vAC             Start address of current entry (inout)
#       sysArgs[0:7]    File name, padded with zeroes (out)

label('SYS_ReadRomDir_v5_80')
ld(hi('sys_ReadRomDir'),Y)      #15
jmp(Y,'sys_ReadRomDir')         #16
ld([vAC+1])                     #17

fillers(until=symbol('SYS_Out_22') & 255)

#-----------------------------------------------------------------------
# Extension SYS_Out_22
#-----------------------------------------------------------------------

# Send byte to output port
#
# Variables:
#       vAC

label('SYS_Out_22')
ld([sysArgs+0],OUT)             #15
nop()                           #16
ld(hi('REENTER'),Y)             #17
jmp(Y,'REENTER')                #18
ld(-22/2)                       #19

#-----------------------------------------------------------------------
# Extension SYS_In_24
#-----------------------------------------------------------------------

# Read a byte from the input port
#
# Variables:
#       vAC

label('SYS_In_24')
st(IN, [vAC])                   #15
ld(0)                           #16
st([vAC+1])                     #17
nop()                           #18
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

assert pc()&255 == 0

#-----------------------------------------------------------------------
#
#  $0100 ROM page 1: Video loop vertical blank
#
#-----------------------------------------------------------------------
align(0x100, size=0x100)

# Video off mode (also no sound, serial, timer, blinkenlights, ...).
# For benchmarking purposes. This still has the overhead for the vTicks
# administration, time slice granularity etc.
label('videoZ')
videoZ = pc()
runVcpu(None, '---- novideo', returnTo=videoZ)

label('startVideo')             # (Re)start of video signal from idle state
ld(syncBits)

# Start of vertical blank interval
label('vBlankStart')
st([videoSync0])                #32 Start of vertical blank interval
ld(syncBits^hSync)              #33
st([videoSync1])                #34

# Reset line counter before vCPU can see it
ld(videoYline0)                 #35
st([videoY])                    #36

# Update frame count and [0x80] (4 cycles)
ld(1)                           #37 Reinitialize carry lookup, for robustness
st([0x80])                      #38
adda([frameCount])              #39 Frame counter
st([frameCount])                #40

# Mix entropy (11 cycles)
xora([entropy+1])               #41 Mix entropy
xora([serialRaw])               #42 Mix in serial input
adda([entropy+0])               #43
st([entropy+0])                 #44
adda([entropy+2])               #45 Some hidden state
st([entropy+2])                 #46
bmi(pc()+3)                     #47
bra(pc()+3)                     #48
xora(64+16+2+1)                 #49
xora(64+32+8+4)                 #49(!)
adda([entropy+1])               #50
st([entropy+1])                 #51

# LED sequencer (18 cycles)
ld([ledTimer])                  #52 Blinkenlight sequencer
beq(pc()+3)                     #53
bra(pc()+3)                     #54
suba(1)                         #55
ld([ledTempo])                  #55(!)
st([ledTimer])                  #56
beq(pc()+3)                     #57
bra(pc()+3)                     #58
ld(0)                           #59 Don't advance state
ld(1)                           #59(!) Advance state when timer passes through 0
adda([ledState_v2])             #60
bne(pc()+3)                     #61
bra(pc()+3)                     #62
ld(-24)                         #63 State 0 becomes -24, start of sequence
bgt('.leds#65')                 #63(!) Catch the stopped state (>0)
st([ledState_v2])               #64
adda('.leds#69')                #65
bra(AC)                         #66 Jump to lookup table
bra('.leds#69')                 #67 Single-instruction subroutine

label('.leds#65')
ld(0x0f)                        #65 Maintain stopped state
st([ledState_v2])               #66
bra('.leds#69')                 #67
anda([xoutMask])                #68 Always clear sound bits (this is why AC=0x0f)

ld(0b1111)                      #68 LEDs |****| offset -24 Low 4 bits are the LED output
ld(0b0111)                      #68 LEDs |***O|
ld(0b0011)                      #68 LEDs |**OO|
ld(0b0001)                      #68 LEDs |*OOO|
ld(0b0010)                      #68 LEDs |O*OO|
ld(0b0100)                      #68 LEDs |OO*O|
ld(0b1000)                      #68 LEDs |OOO*|
ld(0b0100)                      #68 LEDs |OO*O|
ld(0b0010)                      #68 LEDs |O*OO|
ld(0b0001)                      #68 LEDs |*OOO|
ld(0b0011)                      #68 LEDs |**OO|
ld(0b0111)                      #68 LEDs |***O|
ld(0b1111)                      #68 LEDs |****|
ld(0b1110)                      #68 LEDs |O***|
ld(0b1100)                      #68 LEDs |OO**|
ld(0b1000)                      #68 LEDs |OOO*|
ld(0b0100)                      #68 LEDs |OO*O|
ld(0b0010)                      #68 LEDs |O*OO|
ld(0b0001)                      #68 LEDs |*OOO|
ld(0b0010)                      #68 LEDs |O*OO|
ld(0b0100)                      #68 LEDs |OO*O|
ld(0b1000)                      #68 LEDs |OOO*|
ld(0b1100)                      #68 LEDs |OO**|
ld(0b1110)                      #68 LEDs |O***| offset -1
label('.leds#69')
st([xoutMask])                  #69 Sound bits will be re-enabled below
ld(vPulse*2)                    #70 vPulse default length when not modulated
st([videoPulse])                #71

# When the total number of scan lines per frame is not an exact multiple of the
# (4) channels, there will be an audible discontinuity if no measure is taken.
# This static noise can be suppressed by swallowing the first `lines mod 4'
# partial samples after transitioning into vertical blank. This is easiest if
# the modulo is 0 (do nothing), 1 (reset sample when entering the last visible
# scan line), or 2 (reset sample while in the first blank scan line). For the
# last case there is no solution yet: give a warning.
extra = 0
if soundDiscontinuity == 2:
  st(sample, [sample])          # Sound continuity
  extra += 1
if soundDiscontinuity > 2:
  highlight('Warning: sound discontinuity not suppressed')

# vCPU interrupt
ld([frameCount])                #72
beq('vBlankFirst#75')           #73

runVcpu(186-74-extra,           #74 Application cycles (scan line 0)
    '---D line 0 no timeout',
    returnTo='vBlankFirst#186')

label('vBlankFirst#75')
ld(hi('vBlankFirst#78'),Y)      #75
jmp(Y,'vBlankFirst#78')         #76
ld(hi(vIRQ_v5),Y)               #77
label('vBlankFirst#186')

# Mitigation for rogue channelMask (3 cycles)
ld([channelMask])               #186 Normalize channelMask, for robustness
anda(0b11111011)                #187
st([channelMask])               #188

# Sound on/off (6 cycles)
ld([soundTimer])                #189 Sound on/off
bne(pc()+3)                     #190
bra(pc()+3)                     #191
ld(0)                           #192 Keeps sound unchanged (should be off here)
ld(0xf0)                        #192(!) Turns sound back on
ora([xoutMask])                 #193
st([xoutMask])                  #194

# Sound timer count down (5 cycles)
ld([soundTimer])                #195 Sound timer
beq(pc()+3)                     #196
bra(pc()+3)                     #197
suba(1)                         #198
ld(0)                           #198
st([soundTimer])                #199

ld([videoSync0],OUT)            #0 <New scan line start>
label('sound1')
ld([channel])                   #1 Advance to next sound channel
anda([channelMask])             #2
adda(1)                         #3
ld([videoSync1],OUT)            #4 Start horizontal pulse
st([channel],Y)                 #5
ld(0x7f)                        #6 Update sound channel
anda([Y,oscL])                  #7
adda([Y,keyL])                  #8
st([Y,oscL])                    #9
anda(0x80,X)                    #10
ld([X])                         #11
adda([Y,oscH])                  #12
adda([Y,keyH])                  #13
st([Y,oscH])                    #14
anda(0xfc)                      #15
xora([Y,wavX])                  #16
ld(AC,X)                        #17
ld([Y,wavA])                    #18
ld(soundTable>>8,Y)             #19
adda([Y,X])                     #20
bmi(pc()+3)                     #21
bra(pc()+3)                     #22
anda(63)                        #23
ld(63)                          #23(!)
adda([sample])                  #24
st([sample])                    #25

ld([xout])                      #26 Gets copied to XOUT
ld(hi('vBlankLast#34'),Y)       #27 Prepare jumping out of page in last line
ld([videoSync0],OUT)            #28 End horizontal pulse

# Count through the vertical blank interval until its last scan line
ld([videoY])                    #29
bpl('.vBlankLast#32')           #30
adda(2)                         #31
st([videoY])                    #32

# Determine if we're in the vertical sync pulse
suba(1-2*(vBack+vPulse-1))      #33 Prepare sync values
bne('.prepSync36')              #34 Tests for start of vPulse
suba([videoPulse])              #35
ld(syncBits^vSync)              #36 Entering vertical sync pulse
bra('.prepSync39')              #37
st([videoSync0])                #38
label('.prepSync36')
bne('.prepSync38')              #36 Tests for end of vPulse
ld(syncBits)                    #37
bra('.prepSync40')              #38 Entering vertical back porch
st([videoSync0])                #39
label('.prepSync38')
ld([videoSync0])                #38 Load current value
label('.prepSync39')
nop()                           #39
label('.prepSync40')
xora(hSync)                     #40 Precompute, as during the pulse there is no time
st([videoSync1])                #41

# Capture the serial input before the '595 shifts it out
ld([videoY])                    #42 Capture serial input
xora(1-2*(vBack-1-1))           #43 Exactly when the 74HC595 has captured all 8 controller bits
bne(pc()+3)                     #44
bra(pc()+3)                     #45
st(IN, [serialRaw])             #46
st(0,[0])                       #46(!) Reinitialize carry lookup, for robustness

# Update [xout] with the next sound sample every 4 scan lines.
# Keep doing this on 'videoC equivalent' scan lines in vertical blank.
ld([videoY])                    #47
anda(6)                         #48
beq('vBlankSample')             #49
ld([sample])                    #50

label('vBlankNormal')
runVcpu(199-51, 'AB-D line 1-36')#51 Application cycles (vBlank scan lines without sound sample update)
bra('sound1')                   #199
ld([videoSync0],OUT)            #0 <New scan line start>

label('vBlankSample')
ora(0x0f)                       #51 New sound sample is ready
anda([xoutMask])                #52
st([xout])                      #53
st(sample, [sample])            #54 Reset for next sample

runVcpu(199-55, '--C- line 3-39')#55 Application cycles (vBlank scan lines with sound sample update)
bra('sound1')                   #199
ld([videoSync0],OUT)            #0 <New scan line start>

#-----------------------------------------------------------------------

label('.vBlankLast#32')
jmp(Y,'vBlankLast#34')          #32 Jump out of page for space reasons
#assert hi(controllerType) == hi(pc()) # Assume these share the high address
ld(hi(pc()),Y)                  #33

label('vBlankLast#52')

# Respond to reset button (14 cycles)
# - ResetTimer decrements as long as just [Start] is pressed down
# - Reaching 0 (normal) or 128 (extended) triggers the soft reset sequence
# - Initial value is 128 (or 255 at boot), first decrement, then check
# - This starts vReset -> SYS_Reset_88 -> SYS_Exec_88 -> Reset.gcl -> Main.gcl
# - Main.gcl then recognizes extended presses if resetTimer is 0..127 ("paasei")
# - This requires a full cycle (4s) in the warm boot scenario
# - Or a half cycle (2s) when pressing [Select] down during hard reset
# - This furthermore requires >=1 frame (and <=128) to have passed between
#   reaching 128 and getting through Reset and the start of Main, while [Start]
#   was still pressed so the count reaches <128. Two reasonable expectations.
# - The unintended power-up scenarios of ROMv1 (pulling SER_DATA low, or
#   pressing [Select] together with another button) now don't trigger anymore.

ld([buttonState])               #52 Check [Start] for soft reset
xora(~buttonStart)              #53
bne('.restart#56')              #54
ld([resetTimer])                #55 As long as button pressed
suba(1)                         #56 ... count down the timer
st([resetTimer])                #57
anda(127)                       #58
beq('.restart#61')              #59 Reset at 0 (normal 2s) or 128 (extended 4s)
ld((vReset&255)-2)              #60 Start force reset when hitting 0
bra('.restart#63')              #61 ... otherwise do nothing yet
bra('.restart#64')              #62
label('.restart#56')
wait(62-56)                     #56
ld(128)                         #62 Not pressed, reset the timer
st([resetTimer])                #63
label('.restart#64')
bra('.restart#66')              #64
label('.restart#63')
nop()                           #63,65
label('.restart#61')
st([vPC])                       #61 Point vPC at vReset
ld(vReset>>8)                   #62
st([vPC+1])                     #63
ld(hi('ENTER'))                 #64 Set active interpreter to vCPU
st([vCpuSelect])                #65
label('.restart#66')

# Switch video mode when (only) select is pressed (16 cycles)
# XXX We could make this a vCPU interrupt
ld([buttonState])               #66 Check [Select] to switch modes
xora(~buttonSelect)             #67 Only trigger when just [Select] is pressed
bne('.select#70')               #68
ld([videoModeC])                #69
bmi('.select#72')               #70 Branch when line C is off
ld([videoModeB])                #71 Rotate: Off->D->B->C
st([videoModeC])                #72
ld([videoModeD])                #73
st([videoModeB])                #74
bra('.select#77')               #75
label('.select#72')
ld('nopixels')                  #72,76
ld('pixels')                    #73 Reset: On->D->B->C
st([videoModeC])                #74
st([videoModeB])                #75
nop()                           #76
label('.select#77')
st([videoModeD])                #77
wait(188-78)                    #78 Don't waste code space expanding runVcpu here
# AC==255 now
st([buttonState])               #188
bra('vBlankEnd#191')            #189
ld(0)                           #190
label('.select#70')

# Mitigation of runaway channel variable
ld([channel])                   #70 Normalize channel, for robustness
anda(0b00000011)                #71
st([channel])                   #72 Stop wild channel updates

runVcpu(191-73, '---D line 40') #73 Application cycles (scan line 40)

# AC==0 now
label('vBlankEnd#191')
ld(videoTop_v5>>8,Y)            #191
ld([Y,videoTop_v5])             #192
st([videoY])                    #193
st([frameX])                    #194
bne(pc()+3)                     #195
bra(pc()+3)                     #196
ld('videoA')                    #197
ld('videoF')                    #197(!)
st([nextVideo])                 #198
ld([channel])                   #199 Advance to next sound channel
anda([channelMask])             #0 <New scan line start>
adda(1)                         #1
ld(hi('sound2'),Y)              #2
jmp(Y,'sound2')                 #3
ld(syncBits^hSync,OUT)          #4 Start horizontal pulse

fillers(until=0xff)

#-----------------------------------------------------------------------
# Return point for vCPU slices during visible screen area
#-----------------------------------------------------------------------

assert pc() == 0x1ff            # Enables runVcpu() to re-enter into the next page
bra('sound3')                   #200,0 <New scan line start>

#-----------------------------------------------------------------------
#
#  $0200 ROM page 2: Video loop visible scanlines
#
#-----------------------------------------------------------------------
align(0x100, size=0x100)
ld([channel])                   #1 Advance to next sound channel

# Back porch A: first of 4 repeated scan lines
# - Fetch next Yi and store it for retrieval in the next scan lines
# - Calculate Xi from dXi, but there is no cycle time left to store it as well
label('videoA')
ld('videoB')                    #29 1st scanline of 4 (always visible)
st([nextVideo])                 #30
ld(videoTable>>8,Y)             #31
ld([videoY],X)                  #32
ld([Y,X])                       #33
st([Y,Xpp])                     #34 Just X++
st([frameY])                    #35
ld([Y,X])                       #36
adda([frameX],X)                #37
label('pixels')
ld([frameY],Y)                  #38
ld(syncBits)                    #39

# Stream 160 pixels from memory location <Yi,Xi> onwards
# Superimpose the sync signal bits to be robust against misprogramming
for i in range(qqVgaWidth):
  ora([Y,Xpp],OUT)              #40-199 Pixel burst
ld(syncBits,OUT)                #0 <New scan line start> Back to black

# Front porch
ld([channel])                   #1 Advance to next sound channel
label('sound3')                 # Return from vCPU interpreter
anda([channelMask])             #2
adda(1)                         #3
ld(syncBits^hSync,OUT)          #4 Start horizontal pulse

# Horizontal sync and sound channel update for scanlines outside vBlank
label('sound2')
st([channel],Y)                 #5
ld(0x7f)                        #6
anda([Y,oscL])                  #7
adda([Y,keyL])                  #8
st([Y,oscL])                    #9
anda(0x80,X)                    #10
ld([X])                         #11
adda([Y,oscH])                  #12
adda([Y,keyH])                  #13
st([Y,oscH] )                   #14
anda(0xfc)                      #15
xora([Y,wavX])                  #16
ld(AC,X)                        #17
ld([Y,wavA])                    #18
ld(soundTable>>8,Y)             #19
adda([Y,X])                     #20
bmi(pc()+3)                     #21
bra(pc()+3)                     #22
anda(63)                        #23
ld(63)                          #23(!)
adda([sample])                  #24
st([sample])                    #25

ld([xout])                      #26 Gets copied to XOUT
bra([nextVideo])                #27
ld(syncBits,OUT)                #28 End horizontal pulse

# Back porch B: second of 4 repeated scan lines
# - Recompute Xi from dXi and store for retrieval in the next scan lines
label('videoB')
ld('videoC')                    #29 2nd scanline of 4
st([nextVideo])                 #30
ld(videoTable>>8,Y)             #31
ld([videoY])                    #32
adda(1,X)                       #33
ld([frameX])                    #34
adda([Y,X])                     #35
bra([videoModeB])               #36
st([frameX],X)                  #37 Store in RAM and X

# Back porch C: third of 4 repeated scan lines
# - Nothing new to for video do as Yi and Xi are known,
# - This is the time to emit and reset the next sound sample
label('videoC')
ld('videoD')                    #29 3rd scanline of 4
st([nextVideo])                 #30
ld([sample])                    #31 New sound sample is ready (didn't fit in the audio loop)
ora(0x0f)                       #32
anda([xoutMask])                #33
st([xout])                      #34 Update [xout] with new sample (4 channels just updated)
st(sample, [sample])            #35 Reset for next sample
bra([videoModeC])               #36
ld([frameX],X)                  #37

# Back porch D: last of 4 repeated scan lines
# - Calculate the next frame index
# - Decide if this is the last line or not
label('videoD')                 # Default video mode
ld([frameX], X)                 #29 4th scanline of 4
ld([videoY])                    #30
suba((120-1)*2)                 #31
beq('.lastpixels#34')           #32
adda(120*2)                     #33 More pixel lines to go
st([videoY])                    #34
ld('videoA')                    #35
bra([videoModeD])               #36
st([nextVideo])                 #37

label('.lastpixels#34')
if soundDiscontinuity == 1:
  st(sample, [sample])          #34 Sound continuity
else:
  nop()                         #34
ld('videoE')                    #35 No more pixel lines to go
bra([videoModeD])               #36
st([nextVideo])                 #37

# Back porch "E": after the last line
# - Go back and and enter vertical blank (program page 2)
label('videoE') # Exit visible area
ld(hi('vBlankStart'),Y)         #29 Return to vertical blank interval
jmp(Y,'vBlankStart')            #30
ld(syncBits)                    #31

# Video mode that blacks out one or more pixel lines from the top of screen.
# This yields some speed, but also frees up screen memory for other purposes.
# Note: Sound output becomes choppier the more pixel lines are skipped
# Note: The vertical blank driver leaves 0x80 behind in [videoSync1]
label('videoF')
ld([videoSync1])                #29 Completely black pixel line
adda(0x80)                      #30
st([videoSync1],X)              #31
ld([frameX])                    #32
suba([X])                       #33 Decrements every two VGA scanlines
beq('.videoF#36')               #34
st([frameX])                    #35
bra('nopixels')                 #36
label('.videoF#36')
ld('videoA')                    #36,37 Transfer to visible screen area
st([nextVideo])                 #37
#
# Alternative for pixel burst: faster application mode
label('nopixels')
runVcpu(200-38, 'ABCD line 40-520',
  returnTo=0x1ff)               #38 Application interpreter (black scanlines)

#-----------------------------------------------------------------------
#
#  $0300 ROM page 3: Application interpreter primary page
#
#-----------------------------------------------------------------------

# Enter the timing-aware application interpreter (aka virtual CPU, vCPU)
#
# This routine will execute as many as possible instructions in the
# allotted time. When time runs out, it synchronizes such that the total
# duration matches the caller's request. Durations are counted in `ticks',
# which are multiples of 2 clock cycles.
#
# Synopsis: Use the runVcpu() macro as entry point

# We let 'ENTER' begin one word before the page boundary, for a bit extra
# precious space in the packed interpreter code page. Although ENTER's
# first instruction is bra() which normally doesn't cross page boundaries,
# in this case it will still jump into the right space, because branches
# from $xxFF land in the next page anyway.
while pc()&255 < 255:
  nop()
label('ENTER')
bra('.next2')                   #0 Enter at '.next2' (so no startup overhead)
# --- Page boundary ---
align(0x100,size=0x100)
label('NEXTY')                  # Alternative for REENTER
ld([vPC+1],Y)                   #1

# Fetch next instruction and execute it, but only if there are sufficient
# ticks left for the slowest instruction.
label('NEXT')
adda([vTicks])                  #0 Track elapsed ticks (actually counting down: AC<0)
blt('EXIT')                     #1 Escape near time out
label('.next2')
st([vTicks])                    #2
ld([vPC])                       #3 Advance vPC
adda(2)                         #4
label('.next3')
st([vPC],X)                     #5
ld([Y,X])                       #6 Fetch opcode (actually a branch target)
st([Y,Xpp])                     #7 Just X++
bra(AC)                         #8 Dispatch
ld([Y,X])                       #9 Prefetch operand

# Resync with video driver and transfer control
label('EXIT')
adda(maxTicks)                  #3
label('RESYNC')
bgt(pc()&255)                   #4 Resync
suba(1)                         #5
ld(hi('vBlankStart'),Y)         #6
jmp(Y,[vReturn])                #7 To video driver
ld(0)                           #8 AC should be 0 already. Still..
assert vCPU_overhead ==          9

# pc = 0x0311, Opcode = 0x11
# Instruction LDWI: Load immediate word constant (vAC=D), 24 cycles
label('LDWI')
ld(hi('ldwi#13'),Y)             #10
jmp(Y,'ldwi#13')                #11
ld([vPC+1],Y)                   #12

# pc = 0x0314, Opcode = 0x14
# Instruction DEC: Decrement byte var ([D]--), 22 cycles
label('DEC')
ld(hi('dec#13'),Y)              #10
jmp(Y,'dec#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x0316, Opcode = 0x16
# Instruction MOVQB: Load a byte var with a small constant 0..255, 28 cycles
label('MOVQB')
ld(hi('movqb#13'),Y)            #10 #12
jmp(Y,'movqb#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x0318, Opcode = 0x18
# Instruction LSRB: Logical shift right on a byte var, 28 cycles
label('LSRB')
ld(hi('lsrb#13'),Y)             #10 #12
jmp(Y,'lsrb#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x031a, Opcode = 0x1a
# Instruction LD: Load byte from zero page (vAC=[D]), 22 cycles
label('LD')
ld(hi('ld#13'),Y)               #10 #12
jmp(Y,'ld#13')                  #11
# dummy                         #12 Overlap
#
# pc = 0x031c, Opcode = 0x1c
# Instruction LOKEQI: Loke immediate unsigned word into address contained in [vAC], 34 cycles
label('LOKEQI') 
ld(hi('lokeqi#13'),Y)           #10 #12
jmp(Y,'lokeqi#13')              #11
st([vTmp])                      #12 imm.1

# pc = 0x031f, Opcode = 0x1f
# Instruction CMPHS: Adjust high byte for signed compare (vACH=XXX), 28 cycles
label('CMPHS')
ld(hi('cmphs#13'),Y)            #10
jmp(Y,'cmphs#13')               #11
# dummy                         #12 Overlap, not dependent on ld(AC,X) anymore
#
# pc = 0x0321, Opcode = 0x21
# Instruction LDW: Load word from zero page (vAC=[D]+256*[D+1]), 24 cycles
label('LDW')
ld(hi('ldw#13'),Y)              #10 #12
jmp(Y,'ldw#13')                 #11
# dummy                         #12 Overlap
# 
# pc = 0x0323, Opcode = 0x23
# Instruction PEEK+: Peek byte at address contained in var, inc var, 30 cycles
label('PEEK+') 
ld(hi('peek+#13'),Y)            #10 #12
jmp(Y,'peek+#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x0325, Opcode = 0x25
# Instruction POKEI: Poke immediate byte into address contained in [vAC], 20 cycles
label('POKEI') 
ld(hi('pokei#13'),Y)            #10 #12
jmp(Y,'pokei#13')               #11
# dummy                         #12 Overlap
# 
# pc = 0x0327, Opcode = 0x27
# Instruction LSLV: Logical shift left word var, 28 cycles
label('LSLV')
ld(hi('lslv#13'),Y)             #10 #12
jmp(Y,'lslv#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x0329, Opcode = 0x29
# Instruction ADDBA: vAC += var.lo, 28 cycles
label('ADDBA')
ld(hi('addba#13'),Y)            #10 #12
jmp(Y,'addba#13')               #11
# dummy                         #12 Overlap
# 
# pc = 0x032b, Opcode = 0x2b
# Instruction STW: Store word in zero page ([D],[D+1]=vAC&255,vAC>>8), 24 cycles
label('STW')
ld(hi('stw#13'),Y)              #10 #12
jmp(Y,'stw#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x032d, Opcode = 0x2d
# Instruction ADDBI: Add a constant 0..255 to byte var, 28 cycles
label('ADDBI') 
ld(hi('addbi#13'),Y)            #10 #12
jmp(Y,'addbi#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x032f, Opcode = 0x2f
# Instruction PREFX2
label('PREFX2')
ld(hi('prefx2#13'),Y)           #10 #12
jmp(Y,'prefx2#13')              #11
st([sysArgs+7])                 #12 Operand

# pc = 0x0332, Opcode = 0x32
# Instruction DBNE:  Decrement byte var and branch if not zero, 28 cycles
label('DBNE')
ld(hi('dbne#13'),Y)             #10
jmp(Y,'dbne#13')                #11
ld([vPC+1],Y)                   #12 vPC.hi

# pc = 0x0335, Opcode = 0x35
# Instruction BCC: Test AC sign and branch conditionally, variable, (24-26), cycles
label('BCC')
bra(AC)                         #10 AC is the conditional operand
st([Y,Xpp])                     #11 X++

# pc = 0x0337, Opcode = 0x37
# Instruction DOKEI: Doke immediate word into address contained in [vAC], 30 cycles
label('DOKEI') 
ld(hi('dokei#13'),Y)            #10
jmp(Y,'dokei#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x0339, Opcode = 0x39
# Instruction PEEKV: Read byte from address contained in var, 30 cycles
label('PEEKV')
ld(hi('peekv#13'),Y)            #10
jmp(Y,'peekv#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x033b, Opcode = 0x3b
# Instruction DEEKV: Read word from address contained in var, 28 cycles
label('DEEKV')
ld(hi('deekv#13'),Y)            #10 #12
jmp(Y,'deekv#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x033d, Opcode = 0x3d
# Instruction LOKEI: Loke immediate long into address contained in [vAC], 42 cycles
label('LOKEI') 
ld(hi('lokei#13'),Y)            #10
jmp(Y,'lokei#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x033f, Opcode = 0x3f
# Conditional EQ: Branch if zero (if(vACL==0)vPCL=D)
ld(hi('beq#15'),Y)              #12 #12
jmp(Y,'beq#15')                 #13
ld([vPC+1],Y)                   #14 vPC.hi

# pc = 0x0342, Opcode = 0x42
# Instruction ADDVI: Add 8bit immediate to 16bit zero page var, var += imm, vAC = var, 50 cycles
label('ADDVI')
ld(hi('addvi#13'),Y)            #10
jmp(Y,'addvi#13')               #11
# dummy                         #12 Overlap

# pc = 0x0344, Opcode = 0x44
# Instruction SUBVI: Subtract 8bit immediate from 16bit zero page var, var -= imm, vAC = var, 50 cycles
label('SUBVI')
ld(hi('subvi#13'),Y)            #10 #12
jmp(Y,'subvi#13')               #11
# dummy                         #12 Overlap

# pc = 0x0346, Opcode = 0x46
# Instruction DOKE+: doke word in vAC to address contained in var, var += 2, 30 cycles
label('DOKE+') 
ld(hi('doke+#13'),Y)            #10 #12
jmp(Y,'doke+#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x0348, Opcode = 0x48
# Instruction NOTB: var.lo = ~var.lo, 22 cycles
label('NOTB') 
ld(hi('notb#13'),Y)             #10 #12
jmp(Y,'notb#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x034a, Opcode = 0x4a
# Instruction DJGE:
label('DJGE')
ld(hi('djge#13'),Y)             #10 #12
jmp(Y,'djge#13')                #11
ld([vPC+1],Y)                   #12

# pc = 0x034d, Opcode = 0x4d
# Conditional GT: Branch if positive (if(vACL>0)vPCL=D)
ld(hi('bgt#15'),Y)              #12
jmp(Y,'bgt#15')                 #13
ld([vPC+1],Y)                   #14 vPC.hi

# pc = 0x0350, Opcode = 0x50
# Conditional LT: Branch if negative (if(vACL<0)vPCL=D)
ld(hi('blt#15'),Y)              #12
jmp(Y,'blt#15')                 #13
ld([vPC+1],Y)                   #14 vPC.hi

# pc = 0x0353, Opcode = 0x53
# Conditional GE: Branch if positive or zero (if(vACL>=0)vPCL=D)
ld(hi('bge#15'),Y)              #12
jmp(Y,'bge#15')                 #13
ld([vPC+1],Y)                   #14 vPC.hi

# pc = 0x0356, Opcode = 0x56
# Conditional LE: Branch if negative or zero (if(vACL<=0)vPCL=D)
ld(hi('ble#15'),Y)              #12
jmp(Y,'ble#15')                 #13
ld([vPC+1],Y)                   #14 vPC.hi

# pc = 0x0359, Opcode = 0x59
# Instruction LDI: Load immediate small positive constant (vAC=D), 20 cycles
label('LDI')
ld(hi('ldi#13'),Y)              #10
jmp(Y,'ldi#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x035b, Opcode = 0x5b
# Instruction MOVQW: Load a word var with a small constant 0..255, 30 cycles
label('MOVQW')
ld(hi('movqw#13'),Y)            #10 #12
jmp(Y,'movqw#13')               #11
ld([vPC+1],Y)                   #12 vPC.hi

# pc = 0x035e, Opcode = 0x5e
# Instruction ST: Store byte in zero page ([D]=vAC&255), 20 cycles
label('ST')
ld(hi('st#13'),Y)               #10
jmp(Y,'st#13')                  #11
# dummy                         #12 Overlap
#
# pc = 0x0360, Opcode = 0x60
# Instruction DEEK+: Deek word at address contained in var, var += 2, 30 cycles
label('DEEK+') 
ld(hi('deek+#13'),Y)            #10 #12
jmp(Y,'deek+#13')               #11
ld(0,Y)                         #12

# pc = 0x0363, Opcode = 0x63
# Instruction POP: Pop address from stack (vLR,vSP==[vSP]+256*[vSP+1],vSP+2), 30 cycles
label('POP')
ld(hi('pop#13'),Y)              #10
jmp(Y,'pop#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x0365, Opcode = 0x65
# Instruction MOV: Moves a byte from src var to dst var, 28 cycles
label('MOV')
ld(hi('mov#13'),Y)              #10 #12
jmp(Y,'mov#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x0367, Opcode = 0x67
# Instruction PEEKA: Peek a byte from [vAC] to var, 24 cycles
label('PEEKA') 
ld(hi('peeka#13'),Y)            #10 #12
jmp(Y,'peeka#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x0369, Opcode = 0x69
# Instruction POKEA: Poke a byte from var to [vAC], 22 cycles
label('POKEA') 
ld(hi('pokea#13'),Y)            #10 #12
jmp(Y,'pokea#13')               #11
# dummy                         #12 Overlap

# pc = 0x036b, Opcode = 0x6b
# Instruction TEQ: Test for EQ, returns 0x0000 or 0x0101 in vAC, 28 cycles
label('TEQ')
ld(hi('teq#13'),Y)              #10 #12
jmp(Y,'teq#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x036d, Opcode = 0x6d
# Instruction TNE: Test for NE, returns 0x0000 or 0x0101 in vAC, 28 cycles
label('TNE')
ld(hi('tne#13'),Y)              #10 #12
jmp(Y,'tne#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x036f, Opcode = 0x6f
# Instruction DEEKA: Deek a word from [vAC] to var, 30 cycles
label('DEEKA')
ld(hi('deeka#13'),Y)            #10 #12
jmp(Y,'deeka#13')               #11
st([vTmp])                      #12 mask

# pc = 0x0372, Opcode = 0x72
# Conditional NE: Branch if not zero (if(vACL!=0)vPCL=D)
ld(hi('bne#15'),Y)              #12
jmp(Y,'bne#15')                 #13
ld([vPC+1],Y)                   #14 vPC.hi

# pc = 0x0375, Opcode = 0x75
# Instruction PUSH: Push vLR on stack ([vSP-2],v[vSP-1],vSP=vLR&255,vLR>>8,vLR-2), 30 cycles
label('PUSH')
ld(hi('push#13'),Y)             #10
jmp(Y,'push#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x0377, Opcode = 0x77
# Instruction SUBBA: vAC -= var.lo, 28 cycles
label('SUBBA')
ld(hi('subba#13'),Y)            #10 #12
jmp(Y,'subba#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x0379, Opcode = 0x79
# Instruction INCW: Increment word var, 24-26 cycles
label('INCW')
ld(hi('incw#13'),Y)             #10 #12
jmp(Y,'incw#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x037b, Opcode = 0x7b
# Instruction DECW: Decrement word var, 24-28 cycles
label('DECW')
ld(hi('decw#13'),Y)             #10 #12
jmp(Y,'decw#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x037d, Opcode = 0x7d
# Instruction DOKEA: Doke a word from var to [vAC], 30 cycles
label('DOKEA') 
ld(hi('dokea#13'),Y)            #10 #12
jmp(Y,'dokea#13')               #11
# dummy                         #12 Overlap

# pc = 0x037f, Opcode = 0x7f
# Instruction LUP: ROM lookup (vAC=ROM[vAC+D]), 26 cycles
label('LUP')
ld([vAC+1],Y)                   #10 #12
jmp(Y,251)                      #11 Trampoline offset
adda([vAC])                     #12

# pc = 0x0382, Opcode = 0x82
# Instruction ANDI: Logical-AND with small constant (vAC&=D), 20 cycles
label('ANDI')
ld(hi('andi#13'),Y)             #10
jmp(Y,'andi#13')                #11
anda([vAC])                     #12

# pc = 0x0385, Opcode = 0x85
# Instruction CALLI: Goto immediate address and remember vPC (vLR,vPC=vPC+3,$HHLL-2), 28 cycles
label('CALLI')
ld(hi('calli#13'),Y)            #10
jmp(Y,'calli#13')               #11
ld([vPC])                       #12

# pc = 0x0388, Opcode = 0x88
# Instruction ORI: Logical-OR with small constant (vAC|=D), 20 cycles
label('ORI')
ld(hi('ori#13'),Y)              #10
jmp(Y,'ori#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x038a, Opcode = 0x8a
# Instruction PEEKA+: Peek a byte from [vAC] to var, incw vAC, 28 to 30 cycles
label('PEEKA+') 
ld(hi('peeka+#13'),Y)           #10 #12
jmp(Y,'peeka+#13')              #11
# dummy                         #12 Overlap
# 
# pc = 0x038c, Opcode = 0x8c
# Instruction XORI: Logical-XOR with small constant (vAC^=D), 20 cycles
label('XORI')
ld(hi('xori#13'),Y)             #10 #12
jmp(Y,'xori#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x038e, Opcode = 0x8e
# Instruction DBGE:  Decrement byte var and branch if >= 0, 30 cycles
label('DBGE')
ld(hi('dbge#13'),Y)             #10 #12
jmp(Y,'dbge#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x0390, Opcode = 0x90
# Instruction BRA: Branch unconditionally (vPC=(vPC&0xff00)+D), 18 cycles
label('BRA')
ld(hi('bra#13'),Y)              #10 #12
jmp(Y,'bra#13')                 #11
st([vPC])                       #12

# pc = 0x0393, Opcode = 0x93
# Instruction INC: Increment zero page byte ([D]++), 20 cycles
label('INC')
ld(hi('inc#13'),Y)              #10
jmp(Y,'inc#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x0395, Opcode = 0x95
# Instruction INCWA: Increment word var, vAC=var, 26-28 cycles
label('INCWA')
ld(hi('incwa#13'),Y)            #10 #12
jmp(Y,'incwa#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x0397, Opcode = 0x97
# Instruction CMPHU: Adjust high byte for unsigned compare (vACH=XXX), 28 cycles
label('CMPHU')
ld(hi('cmphu#13'),Y)            #10 #12
jmp(Y,'cmphu#13')               #11
# dummy                         #12 Overlap, not dependent on ld(AC,X) anymore
#
# pc = 0x0399, Opcode = 0x99
# Instruction ADDW: Word addition with zero page (vAC+=[D]+256*[D+1]), 30 cycles
label('ADDW')
ld(hi('addw#13'),Y)             #10 #12
jmp(Y,'addw#13')                #11 Y=0
ld(AC,X)                        #12 Address of low byte to be added

# pc = 0x039c, Opcode = 0x9c
# Instruction LDNI: Load an 8bit immediate as a negative 16bit immediate into vAC, 22 cycles
label('LDNI')
ld(hi('ldni#13'),Y)             #10
jmp(Y,'ldni#13')                #11
# dummy                         #12 Overlap

# pc = 0x039e, Opcode = 0x9e
# Instruction ANDBK: vAC = var & imm, 30 cycles
label('ANDBK')
ld(hi('andbk#13'),Y)            #10 #12
jmp(Y,'andbk#13')               #11
# dummy                         #12 Overlap

# pc = 0x03a0, Opcode = 0xa0
# Instruction ORBK: vAC = var | imm, 30 cycles
label('ORBK')
ld(hi('orbk#13'),Y)             #10 #12
jmp(Y,'orbk#13')                #11
# dummy                         #12 Overlap

# pc = 0x03a2, Opcode = 0xa2
# Instruction XORBK: vAC = var ^ imm, 30 cycles
label('XORBK')
ld(hi('xorbk#13'),Y)            #10 #12
jmp(Y,'xorbk#13')               #11
# dummy                         #12 Overlap

# pc = 0x03a4, Opcode = 0xa4, 26 to 46 cycles
# Instruction DJNE:
label('DJNE')
ld(hi('djne#13'),Y)             #10 #12
jmp(Y,'djne#13')                #11
ld([vPC+1],Y)                   #12

# pc = 0x03a7, Opcode = 0xa7
# Instruction CMPI: Compare byte variable to 8bit immediate, 36 cycles
label('CMPI')
ld(hi('cmpi#13'),Y)             #10
jmp(Y,'cmpi#13')                #11
# dummy                         #12 Overlap

# pc = 0x03a9, Opcode = 0xa9
# Instruction ADDVW: Add two 16bit zero page vars, dst += src, vAC = dst, 28 to 54 cycles
label('ADDVW')
ld(hi('addvw#13'),Y)            #10 #12
jmp(Y,'addvw#13')               #11
# dummy                         #12 Overlap

# pc = 0x03ab, Opcode = 0xab
# Instruction SUBVW: Subtract two 16bit zero page vars, dst -= src, vAC = dst, 30 to 54 cycles
label('SUBVW')
ld(hi('subvw#13'),Y)            #10 #12
jmp(Y,'subvw#13')               #11
# dummy                         #12 Overlap

# pc = 0x03ad, Opcode = 0xad
# Instruction PEEK: Read byte from memory (vAC=[vAC]), 26 cycles
label('PEEK')
ld(hi('peek#13'),Y)             #10 #12
jmp(Y,'peek#13')                #11
# dummy                         #12 Overlap
#
# The 'SYS' vCPU instruction first checks the number of desired ticks given by
# the operand. As long as there are insufficient ticks available in the current
# time slice, the instruction will be retried. This will effectively wait for
# the next scan line if the current slice is almost out of time. Then a jump to
# native code is made. This code can do whatever it wants, but it must return
# to the 'REENTER' label when done. When returning, AC must hold (the negative
# of) the actual consumed number of whole ticks for the entire virtual
# instruction cycle (from NEXT to NEXT). This duration may not exceed the prior
# declared duration in the operand + 28 (or maxTicks). The operand specifies the
# (negative) of the maximum number of *extra* ticks that the native call will
# need. The GCL compiler automatically makes this calculation from gross number
# of cycles to excess number of ticks.
# SYS functions can modify vPC to implement repetition. For example to split
# up work into multiple chucks.
label('.sys#13')
ld(hi('.sys#16'),Y)             #13 #12
jmp(Y,'.sys#16')                #14
# dummy                         #15 Overlap
#
# pc = 0x03b1, Opcode = 0xb1
# Instruction PREFX1
label('PREFX1')
ld(hi('prefx1#13'),Y)           #10 #15
jmp(Y,'prefx1#13')              #11
ld(hi('PREFX1_PAGE'))           #12 ENTER is at $(n-1)ff, where n = instruction page

# pc = 0x03b4, Opcode = 0xb4
# Instruction SYS: Native call, <=256 cycles (<=128 ticks, in reality less)
label('SYS')
adda([vTicks])                  #10
blt('.sys#13')                  #11
ld([sysFn+1],Y)                 #12
jmp(Y,[sysFn])                  #13
# dummy                         #14 Overlap
#
# pc = 0x03b8, Opcode = 0xb8
# Instruction SUBW: Word subtract with zero page (AC-=[D]+256*[D+1]), 30 cycles
label('SUBW')
ld(hi('subw#13'),Y)             #10 #14
jmp(Y,'subw#13')                #11 Y=0
ld(AC,X)                        #12 Address of low byte to be added

# pc = 0x03bb, Opcode = 0xbb
# Instruction JEQ: jump to 16bit address if vAC=0
label('JEQ')
ld(hi('jeq#13'),Y)              #10
jmp(Y,'jeq#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03bd, Opcode = 0xbd
# Instruction JNE: jump to 16bit address if vAC!=0
label('JNE')
ld(hi('jne#13'),Y)              #10 #12
jmp(Y,'jne#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03bf, Opcode = 0xbf
# Instruction JLT: jump to 16bit address if vAC<0
label('JLT')
ld(hi('jlt#13'),Y)              #10 #12
jmp(Y,'jlt#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03c1, Opcode = 0xc1
# Instruction JGT: jump to 16bit address if vAC>0
label('JGT')
ld(hi('jgt#13'),Y)              #10 #12
jmp(Y,'jgt#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03c3, Opcode = 0xc3
# Instruction JLE: jump to 16bit address if vAC<=0
label('JLE')
ld(hi('jle#13'),Y)              #10 #12
jmp(Y,'jle#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03c5, Opcode = 0xc5
# Instruction JGE: jump to 16bit address if vAC>=0
label('JGE')
ld(hi('jge#13'),Y)              #10 #12
jmp(Y,'jge#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03c7, Opcode = 0xc7
# Instruction PREFX3: switches instruction page to 0x2200
# original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2099#p2099
label('PREFX3')
ld(hi('prefx3#13'),Y)           #10 #12
jmp(Y,'prefx3#13')              #11
st([sysArgs+7])                 #12 Second operand

# SYS calls and instruction implementations rely on these labels
label('REENTER_28')
ld(-28/2)                       #25
label('REENTER')
bra('NEXT')                     #26 Return from SYS calls
ld([vPC+1],Y)                   #27

fillers(until=0xcd)

#
# The instructions below are all implemented in the second code page. Jumping
# back and forth makes each 6 cycles slower, but it also saves space in the
# primary page for the instructions above. Most of them are in fact not very
# critical, as evidenced by the fact that they weren't needed for the first
# Gigatron applications (Snake, Racer, Mandelbrot, Loader). By providing them
# in this way, at least they don't need to be implemented as a SYS extension.
#
# pc = 0x03cd, Opcode = 0xcd
# Instruction DEF: Define data or code (vAC,vPC=vPC+2,(vPC&0xff00)+D), 26 cycles
label('DEF')
ld(hi('def#13'),Y)              #10
jmp(Y,'def#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03cf, Opcode = 0xcf
# Instruction CALL: Goto address and remember vPC (vLR,vPC=vPC+2,[D]+256*[D+1]-2), 30 cycles
label('CALL')
ld(hi('call#13'),Y)             #10 #12
jmp(Y,'call#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x03d1, Opcode = 0xd1
# Instruction POKE+: Poke byte in vAC to address contained in var, inc var, 30 cycles
label('POKE+') 
ld(hi('poke+#13'),Y)            #10 #12
jmp(Y,'poke+#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x03d3, Opcode = 0xd3
# Instruction LSRV: Logical shift right word var, 56 cycles
label('LSRV')
ld(hi('lsrv#13'),Y)             #10 #12
jmp(Y,'lsrv#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x03d5, Opcode = 0xd5
# Instruction TGE: Test for GE, returns 0x0000 or 0x0101 in vAC, 26 cycles
label('TGE')
ld(hi('tge#13'),Y)              #10 #12
jmp(Y,'tge#13')                 #11
# dummy                         #12 Overlap
#
# pc = 0x03d7, Opcode = 0xd7
# Instruction TLT: Test for LT, returns 0x0000 or 0x0101 in vAC, 26 cycles
label('TLT')
ld(hi('tlt#13'),Y)             #10 #12
jmp(Y,'tlt#13')                #11
# dummy                        #12 Overlap
#
# pc = 0x03d9, Opcode = 0xd9
# Instruction TGT: Test for GT, returns 0x0000 or 0x0101 in vAC, 28 cycles
label('TGT')
ld(hi('tgt#13'),Y)             #10 #12
jmp(Y,'tgt#13')                #11
# dummy                        #12 Overlap
#
# pc = 0x03db, Opcode = 0xdb
# Instruction TLE: Test for LE, returns 0x0000 or 0x0101 in vAC
label('TLE')
ld(hi('tle#13'),Y)             #10 #12
jmp(Y,'tle#13')                #11
# dummy                        #12 Overlap
#
# pc = 0x03dd, Opcode = 0xdd
# Instruction DECWA: Decrement word var, vAC=var, 28-30 cycles
label('DECWA')
ld(hi('decwa#13'),Y)           #10 #12
jmp(Y,'decwa#13')              #11
# dummy                        #12 Overlap
#
# pc = 0x03df, Opcode = 0xdf
# Instruction ALLOC: Create or destroy stack frame (vSP+=D), 20 cycles
label('ALLOC')
ld(hi('alloc#13'),Y)           #10 #12
jmp(Y,'alloc#13')              #11
# dummy                        #12 Overlap
#
# pc = 0x03e1, Opcode = 0xe1
# Instruction SUBBI: Subtract a constant 0..255 from a byte var, 28 cycles
label('SUBBI')
ld(hi('subbi#13'),Y)            #10 #12
jmp(Y,'subbi#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x03e3, Opcode = 0xe3
# Instruction ADDI: Add small positive constant (vAC+=D), 26 cycles
label('ADDI')
ld(hi('addi#13'),Y)             #10 #12
jmp(Y,'addi#13')                #11
st([vTmp])                      #12

# pc = 0x03e6, Opcode = 0xe6
# Instruction SUBI: Subtract small positive constant (vAC+=D), 26 cycles
label('SUBI')
ld(hi('subi#13'),Y)             #10
jmp(Y,'subi#13')                #11
st([vTmp])                      #12

# pc = 0x03e9, Opcode = 0xe9
# Instruction LSLW: Logical shift left (vAC<<=1), 28 cycles
# Useful, because ADDW can't add vAC to itself. Also more compact.
label('LSLW')
ld(hi('lslw#13'),Y)             #10
jmp(Y,'lslw#13')                #11
ld([vAC])                       #12

# pc = 0x03ec, Opcode = 0xec
# Instruction STLW: Store word in stack frame ([vSP+D],[vSP+D+1]=vAC&255,vAC>>8), 24 cycles
label('STLW')
ld(hi('stlw#13'),Y)             #10
jmp(Y,'stlw#13')                #11
#dummy()                        #12 Overlap
#
# pc = 0x03ee, Opcode = 0xee
# Instruction LDLW: Load word from stack frame (vAC=[vSP+D]+256*[vSP+D+1]), 24 cycles
label('LDLW')
ld(hi('ldlw#13'),Y)             #10 #12
jmp(Y,'ldlw#13')                #11
#dummy()                        #12 Overlap
#
# pc = 0x03f0, Opcode = 0xf0
# Instruction POKE: Write byte in memory ([[D+1],[D]]=vAC&255), 26 cycles
label('POKE')
ld(hi('poke#13'),Y)             #10 #12
jmp(Y,'poke#13')                #11
st([vTmp])                      #12

# pc = 0x03f3, Opcode = 0xf3
# Instruction DOKE: Write word in memory ([[D+1],[D]],[[D+1],[D]+1]=vAC&255,vAC>>8), 28 cycles
label('DOKE')
ld(hi('doke#13'),Y)             #10
jmp(Y,'doke#13')                #11
st([vTmp])                      #12

# pc = 0x03f6, Opcode = 0xf6
# Instruction DEEK: Read word from memory (vAC=[vAC]+256*[vAC+1]), 28 cycles
label('DEEK')
ld(hi('deek#13'),Y)             #10
jmp(Y,'deek#13')                #11
#dummy()                        #12 Overlap
#
# pc = 0x03f8, Opcode = 0xf8
# Instruction ANDW: Word logical-AND with zero page (vAC&=[D]+256*[D+1]), 28 cycles
label('ANDW')
ld(hi('andw#13'),Y)             #10 #12
jmp(Y,'andw#13')                #11
#dummy()                        #12 Overlap
#
# pc = 0x03fa, Opcode = 0xfa
# Instruction ORW: Word logical-OR with zero page (vAC|=[D]+256*[D+1]), 28 cycles
label('ORW')
ld(hi('orw#13'),Y)              #10 #12
jmp(Y,'orw#13')                 #11
#dummy()                        #12 Overlap
#
# pc = 0x03fc, Opcode = 0xfc
# Instruction XORW: Word logical-XOR with zero page (vAC^=[D]+256*[D+1]), 28 cycles
label('XORW')
ld(hi('xorw#13'),Y)             #10 #12
jmp(Y,'xorw#13')                #11
ld(AC,X)                        #12

# pc = 0x03ff, Opcode = 0xff
# Instruction RET: Function return (vPC=vLR-2), 16 cycles
label('RET')
ld([vLR])                       #10
assert pc()&255 == 0


#-----------------------------------------------------------------------
#
#  $0400 ROM page 4: Application interpreter extension
#
#-----------------------------------------------------------------------
align(0x100, size=0x100)

# (Continue RET)
suba(2)                         #11
st([vPC])                       #12
ld([vLR+1])                     #13
st([vPC+1])                     #14
ld(hi('REENTER'),Y)             #15
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17
 
# CALL implementation
label('call#13')
ld(AC,X)                        #13
ld([vPC])                       #14
adda(2)                         #15 Point to instruction after CALL
st([vLR])                       #16
ld([vPC+1])                     #17
st([vLR+1])                     #18
ld(0,Y)                         #19
ld([X])                         #20
st([Y,Xpp])                     #21
suba(2)                         #22 Because NEXT will add 2
st([vPC])                       #23
ld([X])                         #24
st([vPC+1])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# DEF implementation
label('def#13')
st([vTmp])                      #13
ld([vPC])                       #14
adda(2)                         #15
st([vAC])                       #16
ld([vPC+1])                     #17
st([vAC+1])                     #18
ld([vTmp])                      #19
st([vPC])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

# ANDI implementation
label('andi#13')
st([vAC])                       #13
ld(0)                           #14 Clear high byte
st([vAC+1])                     #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# LSLW implementation
label('lslw#13')
anda(128,X)                     #13
adda([vAC])                     #14
st([vAC])                       #15
ld([X])                         #16
adda([vAC+1])                   #17
adda([vAC+1])                   #18
st([vAC+1])                     #19
ld([vPC])                       #20
suba(1)                         #21
ld(hi('REENTER_28'),Y)          #22
jmp(Y,'REENTER_28')             #23
st([vPC])                       #24

# STLW implementation
label('stlw#13')
ld([vSPH],Y)                    #13
adda([vSP],X)                   #14
ld([vAC])                       #15
st([Y,Xpp])                     #16
ld([vAC+1])                     #17
st([Y,X])                       #18
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

# LDLW implementation
label('ldlw#13')
ld([vSPH],Y)                    #13
adda([vSP],X)                   #14
ld([Y,X])                       #15
st([Y,Xpp])                     #16
st([vAC])                       #17
ld([Y,X])                       #18
st([vAC+1])                     #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# POKE implementation
label('poke#13')
adda(1,X)                       #13
ld([X])                         #14
ld(AC,Y)                        #15
ld([vTmp],X)                    #16
ld([X])                         #17
ld(AC,X)                        #18
ld([vAC])                       #19
st([Y,X])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

# PEEK implementation
label('peek#13')
ld([vAC],X)                     #13
ld([vAC+1],Y)                   #14
ld([Y,X])                       #15
st([vAC])                       #16
ld(0)                           #17
st([vAC+1])                     #18
ld([vPC])                       #19
suba(1)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# DOKE implementation
label('doke#13')
adda(1,X)                       #13
ld([X])                         #14
ld(AC,Y)                        #15
ld([vTmp],X)                    #16
ld([X])                         #17
ld(AC,X)                        #18
ld([vAC])                       #19
st([Y,Xpp])                     #20
ld([vAC+1])                     #21
st([Y,X])                       #22 Incompatible with REENTER_28
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25

# DEEK implementation
label('deek#13')
ld([vPC])                       #13
suba(1)                         #14
st([vPC])                       #15
ld([vAC],X)                     #16
ld([vAC+1],Y)                   #17
ld([Y,X])                       #18
st([Y,Xpp])                     #19 Just X++
st([vAC])                       #20
ld([Y,X])                       #21
ld(hi('REENTER_28'),Y)          #22
jmp(Y,'REENTER_28')             #23
st([vAC+1])                     #24

# ANDW implementation
label('andw#13')
ld(AC,X)                        #13
ld(0,Y)                         #14
ld([X])                         #15
st([Y,Xpp])                     #16
anda([vAC])                     #17
st([vAC])                       #18
ld([X])                         #19
anda([vAC+1])                   #20
st([vAC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# ORW implementation
label('orw#13')
ld(AC,X)                        #13
ld(0,Y)                         #14
ld([X])                         #15
st([Y,Xpp])                     #16
ora([vAC])                      #17
st([vAC])                       #18
ld([X])                         #19
ora([vAC+1])                    #20
st([vAC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# PEEK+ implementation
label('peek+#13')
ld(0,Y)                         #13
ld(AC,X)                        #14
ld([X])                         #15 low byte peek address
st([vTmp])                      #16
adda(1)                         #17
st([Y,Xpp])                     #18
ld([X])                         #19 high byte peek address
ld(AC,Y)                        #20
ld([vTmp],X)                    #21
ld([Y,X])                       #22
st([vAC])                       #23
ld(0)                           #24
st([vAC+1])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

fillers(until=0xa7)

# pc = 0x04a7
#-----------------------------------------------------------------------
#
#  vCPU extension functions (for acceleration and compaction) follow below.
#
#  The naming convention is: SYS_<CamelCase>[_v<V>]_<N>
#
#  With <N> the maximum number of cycles the function will run
#  (counted from NEXT to NEXT). This is the same number that must
#  be passed to the 'SYS' vCPU instruction as operand, and it will
#  appear in the GCL code upon use.
#
#  If a SYS extension got introduced after ROM v1, the version number of
#  introduction is included in the name. This helps the programmer to be
#  reminded to verify the acutal ROM version and fail gracefully on older
#  ROMs than required. See also Docs/GT1-files.txt on using [romType].
#
#-----------------------------------------------------------------------

#-----------------------------------------------------------------------
# Extension SYS_Random_34: Update entropy and copy to vAC
#-----------------------------------------------------------------------

# This same algorithm runs automatically once per vertical blank.
# Use this function to get numbers at a higher rate.
#
# Variables:
#       vAC

label('SYS_Random_34')
ld([frameCount])                #15
xora([entropy+1])               #16
xora([serialRaw])               #17
adda([entropy+0])               #18
st([entropy+0])                 #19
st([vAC+0])                     #20
adda([entropy+2])               #21
st([entropy+2])                 #22
bmi('.sysRnd0')                 #23
bra('.sysRnd1')                 #24
xora(64+16+2+1)                 #25
label('.sysRnd0')
xora(64+32+8+4)                 #25
label('.sysRnd1')
adda([entropy+1])               #26
st([entropy+1])                 #27
st([vAC+1])                     #28
ld(hi('REENTER'),Y)             #29
jmp(Y,'REENTER')                #30
ld(-34/2)                       #31

label('SYS_LSRW7_30')
ld([vAC])                       #15
anda(128,X)                     #16
ld([vAC+1])                     #17
adda(AC)                        #18
ora([X])                        #19
st([vAC])                       #20
ld([vAC+1])                     #21
anda(128,X)                     #22
ld([X])                         #23
st([vAC+1])                     #24
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27

label('SYS_LSRW8_24')
ld([vAC+1])                     #15
st([vAC])                       #16
ld(0)                           #17
st([vAC+1])                     #18
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

label('SYS_LSLW8_24')
ld([vAC])                       #15
st([vAC+1])                     #16
ld(0)                           #17
st([vAC])                       #18
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

#-----------------------------------------------------------------------
# Extension SYS_Draw4_30
#-----------------------------------------------------------------------

# Draw 4 pixels on screen, horizontally next to each other
#
# Variables:
#       sysArgs[0:3]    Pixels (in)
#       sysArgs[4:5]    Position on screen (in)

label('SYS_Draw4_30')
ld([sysArgs+4],X)               #15
ld([sysArgs+5],Y)               #16
ld([sysArgs+0])                 #17
st([Y,Xpp])                     #18
ld([sysArgs+1])                 #19
st([Y,Xpp])                     #20
ld([sysArgs+2])                 #21
st([Y,Xpp])                     #22
ld([sysArgs+3])                 #23
st([Y,Xpp])                     #24
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27

#-----------------------------------------------------------------------
# Extension SYS_VDrawBits_134:
#-----------------------------------------------------------------------

# Draw slice of a character, 8 pixels vertical
#
# Variables:
#       sysArgs[0]      Color 0 "background" (in)
#       sysArgs[1]      Color 1 "pen" (in)
#       sysArgs[2]      8 bits, highest bit first (in, changed)
#       sysArgs[4:5]    Position on screen (in)

label('SYS_VDrawBits_134')
ld(hi('sys_VDrawBits'),Y)       #15
jmp(Y,'sys_VDrawBits')          #16
ld([sysArgs+4],X)               #17

#-----------------------------------------------------------------------

# Interrupt handler:
#       STW  $xx        -> optionally store vCpuSelect
#       ... IRQ payload ...
# either:
#       LDWI $400
#       LUP  0          -> vRTI and don't switch interpreter (immediate resume)
# or:
#       LDWI $400
#       LUP  $xx        -> vRTI and switch interpreter type as stored in [$xx]
fillers(until=251-13)
label('vRTI#15')
ld([vIrqSave])                  #15 Continue with vCPU in the same timeslice (faster)
st([vPC])                       #16
ld([vIrqSave+1])                #17
st([vPC+1])                     #18
ld([vIrqSave+2])                #19
st([vAC])                       #20
ld([vIrqSave+3])                #21
st([vAC+1])                     #22
ld([vIrqSave+4])                #23 Restore vCpuSelect for PREFIX
st([vCpuSelect])                #24
adda(1,Y)                       #25 Jump to correct PREFIX page, (or page by default)
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27
# vRTI entry point
assert(pc()&255 == 251)         # The landing offset 251 for LUP trampoline is fixed
beq('vRTI#15')                  #13 vRTI sequence
adda(1,X)                       #14
ld(hi('vRTI#18'),Y)             #15 Switch and wait for end of timeslice (slower)
jmp(Y,'vRTI#18')                #16
st([vTmp])                      #17

#-----------------------------------------------------------------------
#
#  $0500 ROM page 5-6: Shift table and code
#
#-----------------------------------------------------------------------

align(0x100, size=0x200)

# Lookup table for i>>n, with n in 1..6
# Indexing ix = i & ~b | (b-1), where b = 1<<(n-1)
#       ...
#       ld   <.ret
#       st   [vTmp]
#       ld   >shiftTable,y
#       <calculate ix>
#       jmp  y,ac
#       bra  $ff
# .ret: ...
#
# i >> 7 can be always be done with RAM: [i&128]
#       ...
#       anda $80,x
#       ld   [x]
#       ...

label('shiftTable')
shiftTable = pc()

for ix in range(255):
  for n in range(1,7): # Find first zero
    if ~ix & (1 << (n-1)):
      break
  pattern = ['x' if i<n else '1' if ix&(1<<i) else '0' for i in range(8)]
  ld(ix>>n); C('0b%s >> %d' % (''.join(reversed(pattern)), n))

assert pc()&255 == 255
bra([vTmp])                     # Jumps back into next page

label('SYS_LSRW1_48')
assert pc()&255 == 0            # First instruction on this page *must* be a nop
nop()                           #15
ld(hi('shiftTable'),Y)          #16 Logical shift right 1 bit (X >> 1)
ld('.sysLsrw1a')                #17 Shift low byte
st([vTmp])                      #18
ld([vAC])                       #19
anda(0b11111110)                #20
jmp(Y,AC)                       #21
bra(255)                        #22 bra shiftTable+255
label('.sysLsrw1a')
st([vAC])                       #26
ld([vAC+1])                     #27 Transfer bit 8
anda(1)                         #28
adda(127)                       #29
anda(128)                       #30
ora([vAC])                      #31
st([vAC])                       #32
ld('.sysLsrw1b')                #33 Shift high byte
st([vTmp])                      #34
ld([vAC+1])                     #35
anda(0b11111110)                #36
jmp(Y,AC)                       #37
bra(255)                        #38 bra shiftTable+255
label('.sysLsrw1b')
st([vAC+1])                     #42
ld(hi('REENTER'),Y)             #43
jmp(Y,'REENTER')                #44
ld(-48/2)                       #45

label('SYS_LSRW2_52')
ld(hi('shiftTable'),Y)          #15 Logical shift right 2 bit (X >> 2)
ld('.sysLsrw2a')                #16 Shift low byte
st([vTmp])                      #17
ld([vAC])                       #18
anda(0b11111100)                #19
ora( 0b00000001)                #20
jmp(Y,AC)                       #21
bra(255)                        #22 bra shiftTable+255
label('.sysLsrw2a')
st([vAC])                       #26
ld([vAC+1])                     #27 Transfer bit 8:9
adda(AC)                        #28
adda(AC)                        #29
adda(AC)                        #30
adda(AC)                        #31
adda(AC)                        #32
adda(AC)                        #33
ora([vAC])                      #34
st([vAC])                       #35
ld('.sysLsrw2b')                #36 Shift high byte
st([vTmp])                      #37
ld([vAC+1])                     #38
anda(0b11111100)                #39
ora( 0b00000001)                #40
jmp(Y,AC)                       #41
bra(255)                        #42 bra shiftTable+255
label('.sysLsrw2b')
st([vAC+1])                     #46
ld(hi('REENTER'),Y)             #47
jmp(Y,'REENTER')                #48
ld(-52/2)                       #49

label('SYS_LSRW3_52')
ld(hi('shiftTable'),Y)          #15 Logical shift right 3 bit (X >> 3)
ld('.sysLsrw3a')                #16 Shift low byte
st([vTmp])                      #17
ld([vAC])                       #18
anda(0b11111000)                #19
ora( 0b00000011)                #20
jmp(Y,AC)                       #21
bra(255)                        #22 bra shiftTable+255
label('.sysLsrw3a')
st([vAC])                       #26
ld([vAC+1])                     #27 Transfer bit 8:10
adda(AC)                        #28
adda(AC)                        #29
adda(AC)                        #30
adda(AC)                        #31
adda(AC)                        #32
ora([vAC])                      #33
st([vAC])                       #34
ld('.sysLsrw3b')                #35 Shift high byte
st([vTmp])                      #36
ld([vAC+1])                     #37
anda(0b11111000)                #38
ora( 0b00000011)                #39
jmp(Y,AC)                       #40
bra(255)                        #41 bra shiftTable+255
label('.sysLsrw3b')
st([vAC+1])                     #45
ld(-52/2)                       #46
ld(hi('REENTER'),Y)             #47
jmp(Y,'REENTER')                #48
#nop()                          #49

label('SYS_LSRW4_50')
ld(hi('shiftTable'),Y)          #15,49 Logical shift right 4 bit (X >> 4)
ld('.sysLsrw4a')                #16 Shift low byte
st([vTmp])                      #17
ld([vAC])                       #18
anda(0b11110000)                #19
ora( 0b00000111)                #20
jmp(Y,AC)                       #21
bra(255)                        #22 bra shiftTable+255
label('.sysLsrw4a')
st([vAC])                       #26
ld([vAC+1])                     #27 Transfer bit 8:11
adda(AC)                        #28
adda(AC)                        #29
adda(AC)                        #30
adda(AC)                        #31
ora([vAC])                      #32
st([vAC])                       #33
ld('.sysLsrw4b')                #34 Shift high byte'
st([vTmp])                      #35
ld([vAC+1])                     #36
anda(0b11110000)                #37
ora( 0b00000111)                #38
jmp(Y,AC)                       #39
bra(255)                        #40 bra shiftTable+255
label('.sysLsrw4b')
st([vAC+1])                     #44
ld(hi('REENTER'),Y)             #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47

label('SYS_LSRW5_50')
ld(hi('shiftTable'),Y)          #15 Logical shift right 5 bit (X >> 5)
ld('.sysLsrw5a')                #16 Shift low byte
st([vTmp])                      #17
ld([vAC])                       #18
anda(0b11100000)                #19
ora( 0b00001111)                #20
jmp(Y,AC)                       #21
bra(255)                        #22 bra shiftTable+255
label('.sysLsrw5a')
st([vAC])                       #26
ld([vAC+1])                     #27 Transfer bit 8:13
adda(AC)                        #28
adda(AC)                        #29
adda(AC)                        #30
ora([vAC])                      #31
st([vAC])                       #32
ld('.sysLsrw5b')                #33 Shift high byte
st([vTmp])                      #34
ld([vAC+1])                     #35
anda(0b11100000)                #36
ora( 0b00001111)                #37
jmp(Y,AC)                       #38
bra(255)                        #39 bra shiftTable+255
label('.sysLsrw5b')
st([vAC+1])                     #44
ld(-50/2)                       #45
ld(hi('REENTER'),Y)             #46
jmp(Y,'REENTER')                #47
#nop()                          #48

label('SYS_LSRW6_48')
ld(hi('shiftTable'),Y)          #15,44 Logical shift right 6 bit (X >> 6)
ld('.sysLsrw6a')                #16 Shift low byte
st([vTmp])                      #17
ld([vAC])                       #18
anda(0b11000000)                #19
ora( 0b00011111)                #20
jmp(Y,AC)                       #21
bra(255)                        #22 bra shiftTable+255
label('.sysLsrw6a')
st([vAC])                       #26
ld([vAC+1])                     #27 Transfer bit 8:13
adda(AC)                        #28
adda(AC)                        #29
ora([vAC])                      #30
st([vAC])                       #31
ld('.sysLsrw6b')                #32 Shift high byte
st([vTmp])                      #33
ld([vAC+1])                     #34
anda(0b11000000)                #35
ora( 0b00011111)                #36
jmp(Y,AC)                       #37
bra(255)                        #38 bra shiftTable+255
label('.sysLsrw6b')
st([vAC+1])                     #42
ld(hi('REENTER'),Y)             #43
jmp(Y,'REENTER')                #44
ld(-48/2)                       #45

label('SYS_LSLW4_46')
ld(hi('shiftTable'),Y)          #15 Logical shift left 4 bit (X << 4)
ld('.sysLsrl4')                 #16
st([vTmp])                      #17
ld([vAC+1])                     #18
adda(AC)                        #19
adda(AC)                        #20
adda(AC)                        #21
adda(AC)                        #22
st([vAC+1])                     #23
ld([vAC])                       #24
anda(0b11110000)                #25
ora( 0b00000111)                #26
jmp(Y,AC)                       #27
bra(255)                        #28 bra shiftTable+255
label('.sysLsrl4')
ora([vAC+1])                    #32
st([vAC+1])                     #33
ld([vAC])                       #34
adda(AC)                        #35
adda(AC)                        #36
adda(AC)                        #37
adda(AC)                        #38
st([vAC])                       #39
ld(-46/2)                       #40
ld(hi('REENTER'),Y)             #41
jmp(Y,'REENTER')                #42
#nop()                          #43

#-----------------------------------------------------------------------
#       v6502 right shift instruction
#-----------------------------------------------------------------------

label('v6502_lsr#30')
ld([v6502_ADH],Y)               #30 Result
st([Y,X])                       #31
st([v6502_Qz])                  #32 Z flag
st([v6502_Qn])                  #33 N flag
ld(hi('v6502_next'),Y)          #34
ld(-38/2)                       #35
jmp(Y,'v6502_next')             #36
#nop()                          #37 Overlap
#
label('v6502_ror#38')
ld([v6502_ADH],Y)               #38,38 Result
ora([v6502_BI])                 #39 Transfer bit 8
st([Y,X])                       #40
st([v6502_Qz])                  #41 Z flag
st([v6502_Qn])                  #42 N flag
ld(hi('v6502_next'),Y)          #43
jmp(Y,'v6502_next')             #44
ld(-46/2)                       #45

#-----------------------------------------------------------------------
#       vCPU LSRB
#-----------------------------------------------------------------------

label('.lsrb#24')
st([X])                         #24
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27

#-----------------------------------------------------------------------
#       vCPU LSRV
#-----------------------------------------------------------------------

label('.lsrv#27')
ld(hi('.lsrv#30'),Y)            #27
jmp(Y,'.lsrv#30')               #28
st([X])                         #29 shifted byte low

label('.lsrv#51')
st([X])                         #51 shifted byte hi
ld(hi('NEXTY'),Y)               #52
jmp(Y,'NEXTY')                  #53
ld(-56/2)                       #54

#-----------------------------------------------------------------------
#       vCPU LSRVL
#-----------------------------------------------------------------------

label('.lsrvl#26')
ld(hi('.lsrvl#29'),Y)           #26
jmp(Y,'.lsrvl#29')              #27
st([X])                         #28 shifted byte 0

label('.lsrvl#54')
ld(hi('.lsrvl#57'),Y)           #54
jmp(Y,'.lsrvl#57')              #55
st([X])                         #56 shifted byte 1

label('.lsrvl#76')
ld(hi('.lsrvl#79'),Y)           #76
jmp(Y,'.lsrvl#79')              #77
st([X])                         #78 shifted byte 2

label('.lsrvl#98')
st([X])                         #98 shifted byte 3
ld(hi('REENTER'),Y)             #99
jmp(Y,'REENTER')                #100
ld(-104/2)                      #101

#-----------------------------------------------------------------------
#       vCPU NROR (lb3361)
#-----------------------------------------------------------------------

label('nror#35')
ora([vAC])                      #35
st([X])                         #36
ld([sysArgs+7])                 #37
xora([sysArgs+6])               #38
bne('nror#41')                  #39
ld(hi('NEXTY'),Y)               #40
jmp(Y,'NEXTY')                  #41
ld(-44/2)                       #42
label('nror#41')
ld(-30/2)                       #41
adda([vTicks])                  #12=42-30
ld(hi('nror#16'),Y)             #13=43-30
jmp(Y,'nror#16')                #14=44-30
st([vTicks])                    #15=45-30



#-----------------------------------------------------------------------
#
#  $0700 ROM page 7-8: Gigatron font data
#
#-----------------------------------------------------------------------

align(0x100, size=0x100)

label('font32up')
for ch in range(32, 32+50):
  comment = 'Char %s' % repr(chr(ch))
  for byte in font.font[ch-32]:
    ld(byte)
    comment = C(comment)

trampoline()

#-----------------------------------------------------------------------

align(0x100, size=0x100)

label('font82up')
for ch in range(32+50, 132):
  comment = 'Char %s' % repr(chr(ch))
  for byte in font.font[ch-32]:
    ld(byte)
    comment = C(comment)

trampoline()

#-----------------------------------------------------------------------
#
#  $0900 ROM page 9: Key table for music
#
#-----------------------------------------------------------------------

align(0x100, size=0x100)
notes = 'CCDDEFFGGAAB'
sampleRate = cpuClock / 200.0 / 4
label('notesTable')
ld(0)
ld(0)
for i in range(0, 250, 2):
  j = i//2-1
  freq = 440.0*2.0**((j-57)/12.0)
  if j>=0 and freq <= sampleRate/2.0:
    key = int(round(32768 * freq / sampleRate))
    octave, note = j//12, notes[j%12]
    sharp = '-' if notes[j%12-1] != note else '#'
    comment = '%s%s%s (%0.1f Hz)' % (note, sharp, octave, freq)
    ld(key&127); C(comment); ld(key>>7)

# NOTE trampoline, (0x09c0), (cycles times are +1 for midi#13)
label('noteTrampoline')
bra(AC)                         #22,#23 #35,#36 fetches ROM note byte, (low then high)
bra(pc()+1)                     #24,#37
ld(hi('note#13'),Y)             #25,#38 midi#13 lives in the same page
jmp(Y,[vTmp])                   #26,#39
nop()                           #27,#40

trampoline()

#-----------------------------------------------------------------------
#
#  $0a00 ROM page 10: Inversion table
#
#-----------------------------------------------------------------------

align(0x100, size=0x100)
label('invTable')

# Unit 64, table offset 16 (=1/4), value offset 1: (x+16)*(y+1) == 64*64 - e
for i in range(251):
  ld(4096//(i+16)-1)

trampoline()

#-----------------------------------------------------------------------
#
#  $0b00 ROM page 11: More SYS functions
#
#-----------------------------------------------------------------------

align(0x100, size=0x100)

#-----------------------------------------------------------------------
# Extension SYS_SetMode_v2_80
#-----------------------------------------------------------------------

# Set video mode to 0 to 3 black scanlines per pixel line.
#
# Mainly for making the MODE command available in Tiny BASIC, so that
# the user can experiment. It's adviced to refrain from using
# SYS_SetMode_v2_80 in regular applications. Video mode is a deeply
# personal preference, and the programmer shouldn't overrule the user
# in that choice. The Gigatron philisophy is that the end user has
# the final say on what happens on the system, not the application,
# even if that implies a degraded performance. This doesn't mean that
# all applications must work well in all video modes: mode 1 is still
# the default. If an application really doesn't work at all in that
# mode, it's acceptable to change mode once after loading.
#
# There's no "SYS_GetMode" function.
#
# Variables:
#       vAC bit 0:1     Mode:
#                         0      "ABCD" -> Full mode (slowest)
#                         1      "ABC-" -> Default mode after reset
#                         2      "A-C-" -> at67's mode
#                         3      "A---" -> HGM's mode
#       vAC bit 2:15    Ignored bits and should be 0
#
# Special values (ROM v4):
#       vAC = 1975      Zombie mode (no video signals, no input,
#                        no blinkenlights).
#       vAC = -1        Leave zombie mode and restore previous mode.

# Actual duration is <80 cycles, but keep some room for future extensions
label('SYS_SetMode_v2_80')
ld(hi('sys_SetMode'),Y)         #15
jmp(Y,'sys_SetMode')            #16
ld([vReturn])                   #17

#-----------------------------------------------------------------------
# Extension SYS_SetMemory_v2_54
#-----------------------------------------------------------------------

# SYS function for setting 1..256 bytes
#
# sysArgs[0]   Copy count (in, changed)
# sysArgs[1]   Copy value (in)
# sysArgs[2:3] Destination address (in, changed)
#
# Sets up to 8 bytes per invocation before restarting itself through vCPU.
# Doesn't wrap around page boundary. Can run 3 times per 148-cycle time slice.
# All combined that gives a 300% speedup over ROMv4 and before.

label('SYS_SetMemory_v2_54')
ld([sysArgs+0])                 #15
bra('sys_SetMemory#18')         #16
ld([sysArgs+2],X)               #17

#-----------------------------------------------------------------------
# Extension SYS_SendSerial1_v3_80
#-----------------------------------------------------------------------

# SYS function for sending data over serial controller port using
# pulse width modulation of the vertical sync signal.
#
# Variables:
#       sysArgs[0:1]    Source address               (in, changed)
#       sysArgs[2]      Start bit mask (typically 1) (in, changed)
#       sysArgs[3]      Number of send frames X      (in, changed)
#
# The sending will abort if input data is detected on the serial port.
# Returns 0 in case of all bits sent, or <>0 in case of abort
#
# This modulates the next upcoming X vertical pulses with the supplied
# data. A zero becomes a 7 line vPulse, a one will be 9 lines.
# After that, the vPulse width falls back to 8 lines (idle).

label('SYS_SendSerial1_v3_80')
ld([videoY])                    #15
bra('sys_SendSerial1')          #16
xora(videoYline0)               #17 First line of vertical blank

#-----------------------------------------------------------------------
# Extension SYS_ExpanderControl_v4_40
#-----------------------------------------------------------------------

# Sets the I/O and RAM expander's control register
#
# Variables:
#       vAC bit 2       Device enable /SS0
#           bit 3       Device enable /SS1
#           bit 4       Device enable /SS2
#           bit 5       Device enable /SS3
#           bit 6       Banking B0
#           bit 7       Banking B1
#           bit 15      Data out MOSI
#       sysArgs[7]      Cache for control state (written to)
#
# Intended for prototyping, and probably too low-level for most applications
# Still there's a safeguard: it's not possible to disable RAM using this

label('SYS_ExpanderControl_v4_40')
ld(hi('sys_ExpanderControl'),Y) #15
jmp(Y,'sys_ExpanderControl')    #16
ld(0b00001100)                  #17
#    ^^^^^^^^
#    |||||||`-- SCLK
#    ||||||`--- Not connected
#    |||||`---- /SS0
#    ||||`----- /SS1
#    |||`------ /SS2
#    ||`------- /SS3
#    |`-------- B0
#    `--------- B1

#-----------------------------------------------------------------------
# Extension SYS_Run6502_v4_80
#-----------------------------------------------------------------------

# Transfer control to v6502
#
# Calling 6502 code from vCPU goes (only) through this SYS function.
# Directly modifying the vCpuSelect variable is unreliable. The
# control transfer is immediate, without waiting for the current
# time slice to end or first returning to vCPU.
#
# vCPU code and v6502 code can interoperate without much hassle:
# - The v6502 program counter is vLR, and v6502 doesn't touch vPC
# - Returning to vCPU is with the BRK instruction
# - BRK doesn't dump process state on the stack
# - vCPU can save/restore the vLR with PUSH/POP
# - Stacks are shared, vAC is shared
# - vAC can indicate what the v6502 code wants. vAC+1 will be cleared
# - Alternative is to leave a word in sysArgs[6:7] (v6502 X and Y registers)
# - Another way is to set vPC before BRK, and vCPU will continue there(+2)
#
# Calling v6502 code from vCPU looks like this:
#       LDWI  SYS_Run6502_v4_80
#       STW   sysFn
#       LDWI  $6502_start_address
#       STW   vLR
#       SYS   80
#
# Variables:
#       vAC             Accumulator
#       vLR             Program Counter
#       vSP             Stack Pointer (+1)
#       sysArgs[6]      Index Register X
#       sysArgs[7]      Index Register Y
# For info:
#       sysArgs[0:1]    Address Register, free to clobber
#       sysArgs[2]      Instruction Register, free to clobber
#       sysArgs[3:5]    Flags, don't touch
#
# Implementation details::
#
#  The time to reserve for this transition is the maximum time
#  between NEXT and v6502_check. This is
#       SYS call duration + 2*v6502_maxTicks + (v6502_overhead - vCPU_overhead)
#     = 22 + 38 + (11 - 9) = 62 cycles.
#  So reserving 80 cycles is future proof. This isn't overhead, as it includes
#  the fetching of the first 6502 opcode and its operands..
#
#                      0            10                 28=0         9
#    ---+----+---------+------------+------------------+-----------+---
# video | nop| runVcpu |   ENTER    | At least one ins |   EXIT    | video
#    ---+----+---------+------------+------------------+-----------+---
#        sync  prelude  ENTER-to-ins    ins-to-NEXT     NEXT-to-video
#       |<-->|
#        0/1 |<------->|
#                 5    |<----------------------------->|
#          runVCpu_overhead           28               |<--------->|
#                                 2*maxTicks                 9
#                                                      vCPU_overhead
#
#                      0                21                    38=0       11
#    ---+----+---------+----------------+--------------------+-----------+---
# video | nop| runVcpu |   v6502_ENTER  | At least one fetch |v6502_exitB| video
#    ---+----+---------+----------------+--------------------+-----------+---
#        sync  prelude   enter-to-fetch     fetch-to-check    check-to-video
#       |<-->|
#        0/1 |<------->|
#                 5    |<----------------------------------->|
#          runVcpu_overhead           38                     |<--------->|
#                              2*v6520_maxTicks                    11
#                                                            v6502_overhead

label('SYS_Run6502_v4_80')
ld(hi('sys_v6502'),Y)           #15
jmp(Y,'sys_v6502')              #16
ld(hi('v6502_ENTER'))           #17 Activate v6502

#-----------------------------------------------------------------------
# Extension SYS_ResetWaveforms_v4_50
#-----------------------------------------------------------------------

# soundTable[4x+0] = sawtooth, to be modified into metallic/noise
# soundTable[4x+1] = pulse
# soundTable[4x+2] = triangle
# soundTable[4x+3] = sawtooth, also useful to right shift 2 bits

label('SYS_ResetWaveforms_v4_50')
ld(hi('sys_ResetWaveforms'),Y)  #15 Initial setup of waveforms. [vAC+0]=i
jmp(Y,'sys_ResetWaveforms')     #16
ld(soundTable>>8,Y)             #17

#-----------------------------------------------------------------------
# Extension SYS_ShuffleNoise_v4_46
#-----------------------------------------------------------------------

# Use simple 6-bits variation of RC4 to permutate waveform 0 in soundTable

label('SYS_ShuffleNoise_v4_46')
ld(hi('sys_ShuffleNoise'),Y)    #15 Shuffle soundTable[4i+0]. [vAC+0]=4j, [vAC+1]=4i
jmp(Y,'sys_ShuffleNoise')       #16
ld(soundTable>>8,Y)             #17

#-----------------------------------------------------------------------
# Extension SYS_SpiExchangeBytes_v4_134
#-----------------------------------------------------------------------

# Send AND receive 1..256 bytes over SPI interface

# Variables:
#       sysArgs[0]      Page index start, for both send/receive (in, changed)
#       sysArgs[1]      Memory page for send data (in)
#       sysArgs[2]      Page index stop (in)
#       sysArgs[3]      Memory page for receive data (in)
#       sysArgs[4]      Scratch (changed)

label('SYS_SpiExchangeBytes_v4_134')
ld(hi('sys_SpiExchangeBytes'),Y)#15
jmp(Y,'sys_SpiExchangeBytes')   #16
ld(hi(ctrlBits),Y)              #17 Control state as saved by SYS_ExpanderControl


#-----------------------------------------------------------------------
#  SYS Implementations
#-----------------------------------------------------------------------

# SYS_SetMemory_54 implementation
label('sys_SetMemory#18')
ld([sysArgs+3],Y)               #18
ble('.sysSb#21')                #19 Enter fast lane if >=128 or at 0 (-> 256)
suba(8)                         #20
bge('.sysSb#23')                #21 Or when >=8
st([sysArgs+0])                 #22
anda(4)                         #23
beq('.sysSb#26')                #24
ld([sysArgs+1])                 #25 Set 4 pixels
st([Y,Xpp])                     #26
st([Y,Xpp])                     #27
st([Y,Xpp])                     #28
bra('.sysSb#31')                #29
st([Y,Xpp])                     #30
label('.sysSb#26')
wait(31-26)                     #26
label('.sysSb#31')
ld([sysArgs+0])                 #31
anda(2)                         #32
beq('.sysSb#35')                #33
ld([sysArgs+1])                 #34 Set 2 pixels
st([Y,Xpp])                     #35
bra('.sysSb#38')                #36
st([Y,Xpp])                     #37
label('.sysSb#35')
wait(38-35)                     #35
label('.sysSb#38')
ld([sysArgs+0])                 #38
anda(1)                         #39
beq(pc()+3)                     #40
bra(pc()+3)                     #41
ld([sysArgs+1])                 #42 Set 1 pixel
ld([Y,X])                       #42(!) No change
st([Y,X])                       #43
ld(hi('NEXTY'),Y)               #44 Return
jmp(Y,'NEXTY')                  #45 All done
ld(-48/2)                       #46
label('.sysSb#21')
nop()                           #21
st([sysArgs+0])                 #22
label('.sysSb#23')
ld([sysArgs+1])                 #23 Set 8 pixels
st([Y,Xpp])                     #24
st([Y,Xpp])                     #25
st([Y,Xpp])                     #26
st([Y,Xpp])                     #27
st([Y,Xpp])                     #28
st([Y,Xpp])                     #29
st([Y,Xpp])                     #30
st([Y,Xpp])                     #31
ld([sysArgs+2])                 #32 Advance write pointer
adda(8)                         #33
st([sysArgs+2])                 #34
ld([sysArgs+0])                 #35
beq(pc()+3)                     #36
bra(pc()+3)                     #37
ld(-2)                          #38 Self-restart when more to do
ld(0)                           #38(!)
adda([vPC])                     #39
st([vPC])                       #40
ld(hi('REENTER'),Y)             #41
jmp(Y,'REENTER')                #42
ld(-46/2)                       #43

# SYS_SetMode_80 implementation
label('sys_SetMode')
bne(pc()+3)                     #18
bra(pc()+2)                     #19
ld('startVideo')                #20 First enable video if disabled
st([vReturn])                   #20,21
ld([vAC+1])                     #22
beq('.sysSm#25')                #23
ld(hi('REENTER'),Y)             #24
xora([vAC])                     #25
xora((1975>>8)^(1975&255))      #26 Poor man\'s 1975 detection
bne(pc()+3)                     #27
bra(pc()+3)                     #28
assert videoZ == 0x0100
st([vReturn])                   #29 DISABLE video/audio/serial/etc
nop()                           #29(!) Ignore and return
jmp(Y,'REENTER')                #30
ld(-34/2)                       #31
label('.sysSm#25')
ld([vAC])                       #25 Mode 0,1,2,3
anda(3)                         #26
adda('.sysSm#30')               #27
bra(AC)                         #28
bra('.sysSm#31')                #29
label('.sysSm#30')
ld('pixels')                    #30 videoB lines
ld('pixels')                    #30
ld('nopixels')                  #30
ld('nopixels')                  #30
label('.sysSm#31')
st([videoModeB])                #31
ld([vAC])                       #32
anda(3)                         #33
adda('.sysSm#37')               #34
bra(AC)                         #35
bra('.sysSm#38')                #36
label('.sysSm#37')
ld('pixels')                    #37 videoC lines
ld('pixels')                    #37
ld('pixels')                    #37
ld('nopixels')                  #37
label('.sysSm#38')
st([videoModeC])                #38
ld([vAC])                       #39
anda(3)                         #40
adda('.sysSm#44')               #41
bra(AC)                         #42
bra('.sysSm#45')                #43
label('.sysSm#44')
ld('pixels')                    #44 videoD lines
ld('nopixels')                  #44
ld('nopixels')                  #44
ld('nopixels')                  #44
label('.sysSm#45')
st([videoModeD])                #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47

# SYS_SendSerial1_v3_80 implementation
label('sys_SendSerial1')
beq('.sysSs#20')                #18
ld([sysArgs+0],X)               #19
ld([vPC])                       #20 Wait for vBlank
suba(2)                         #21
ld(hi('REENTER_28'),Y)          #22
jmp(Y,'REENTER_28')             #23
st([vPC])                       #24
label('.sysSs#20')
ld([sysArgs+1],Y)               #20 Synchronized with vBlank
ld([Y,X])                       #21 Copy next bit
anda([sysArgs+2])               #22
bne(pc()+3)                     #23
bra(pc()+3)                     #24
ld(7*2)                         #25
ld(9*2)                         #25
st([videoPulse])                #26
ld([sysArgs+2])                 #27 Rotate input bit
adda(AC)                        #28
bne(pc()+3)                     #29
bra(pc()+2)                     #30
ld(1)                           #31
st([sysArgs+2])                 #31,32 (must be idempotent)
anda(1)                         #33 Optionally increment pointer
adda([sysArgs+0])               #34
st([sysArgs+0],X)               #35
ld([sysArgs+3])                 #36 Frame counter
suba(1)                         #37
beq('.sysSs#40')                #38
ld(hi('REENTER'),Y)             #39
st([sysArgs+3])                 #40
ld([serialRaw])                 #41 Test for anything being sent back
xora(255)                       #42
beq('.sysSs#45')                #43
st([vAC])                       #44 Abort after key press with non-zero error
st([vAC+1])                     #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47
label('.sysSs#45')
ld([vPC])                       #45 Continue sending bits
suba(2)                         #46
st([vPC])                       #47
jmp(Y,'REENTER')                #48
ld(-52/2)                       #49
label('.sysSs#40')
st([vAC])                       #40 Stop sending bits, no error
st([vAC+1])                     #41
jmp(Y,'REENTER')                #42
ld(-46/2)                       #43


#-----------------------------------------------------------------------
#  vCPU Implementations
#-----------------------------------------------------------------------

# LDWI implementation
label('ldwi#13')
st([vAC])                       #13 Operand
st([Y,Xpp])                     #14 Just X++
ld([Y,X])                       #15 Fetch second operand
st([vAC+1])                     #16
ld([vPC])                       #17 Advance vPC one more
adda(1)                         #18
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# POP implementation
label('pop#13')
ld([vSPH],Y)                    #13 vSP.hi
ld([vSP],X)                     #14
ld([Y,X])                       #15
st([Y,Xpp])                     #16
st([vLR])                       #17
ld([Y,X])                       #18
st([vLR+1])                     #19
ld([vSP])                       #20
adda(2)                         #21
st([vSP])                       #22
ld([vPC])                       #23
suba(1)                         #24
st([vPC])                       #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# PUSH implementation
label('push#13')
ld([vSPH],Y)                    #13 vSP.hi
ld([vSP])                       #14
suba(2)                         #15
st([vSP],X)                     #16
ld([vLR])                       #17
st([Y,Xpp])                     #18
ld([vLR+1])                     #19
st([Y,X])                       #20
ld([vPC])                       #21
suba(1)                         #22
st([vPC])                       #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# -------------------------------------------------------------
# vCPU instructions for comparisons between two 16-bit operands
# -------------------------------------------------------------
#
# vCPU's conditional branching (BCC) always compares vAC against 0,
# treating vAC as a two's complement 16-bit number. When we need to
# compare two arbitrary numnbers we normally first take their difference
# with SUBW.  However, when this difference is too large, the subtraction
# overflows and we get the wrong outcome. To get it right over the
# entire range, an elaborate sequence is needed. TinyBASIC uses this
# blurp for its relational operators. (It compares stack variable $02
# with zero page variable $3a.)
#
#       0461  ee 02            LDLW  $02
#       0463  fc 3a            XORW  $3a
#       0465  35 53 6a         BGE   $046c
#       0468  ee 02            LDLW  $02
#       046a  90 6e            BRA   $0470
#       046c  ee 02            LDLW  $02
#       046e  b8 3a            SUBW  $3a
#       0470  35 56 73         BLE   $0475
#
# The CMPHS and CMPHU instructions were introduced to simplify this.
# They inspect both operands to see if there is an overflow risk. If
# so, they modify vAC such that their difference gets smaller, while
# preserving the relation between the two operands. After that, the
# SUBW instruction can't overflow and we achieve a correct comparison.
# Use CMPHS for signed comparisons and CMPHU for unsigned. With these,
# the sequence above becomes:
#
#       0461  ee 02            LDLW  $02
#       0463  1f 3b            CMPHS $3b        Note: high byte of operand
#       0465  b8 3a            SUBW  $3a
#       0467  35 56 73         BLE   $0475
#
# CMPHS/CMPHU don't make much sense other than in combination with
# SUBW. These modify vACH, if needed, as given in the following table:
#
#       vACH  varH  |     vACH
#       bit7  bit7  | CMPHS  CMPHU
#       ---------------------------
#         0     0   |  vACH   vACH      no change needed
#         0     1   | varH+1 varH-1     narrowing the range
#         1     0   | varH-1 varH+1     narrowing the range
#         1     1   |  vACH   vACH      no change needed
#       ---------------------------

# CMPHS implementation
label('cmphs#13')
ld(AC,X)                        #13
ld([X])                         #14
xora([vAC+1])                   #15
bpl('.cmphu#18')                #16 Skip if same sign
ld([vAC+1])                     #17
bmi(pc()+3)                     #18
bra(pc()+3)                     #19
label('.cmphs#20')
ld(+1)                          #20    vAC < variable
ld(-1)                          #20(!) vAC > variable
label('.cmphs#21')
adda([X])                       #21
st([vAC+1])                     #22
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25

# CMPHU implementation
label('cmphu#13')
ld(AC,X)                        #13
ld([X])                         #14
xora([vAC+1])                   #15
bpl('.cmphu#18')                #16 Skip if same sign
ld([vAC+1])                     #17
bmi('.cmphs#20')                #18
bra('.cmphs#21')                #19
ld(-1)                          #20    vAC > variable

# No-operation for CMPHS/CMPHU when high bits are equal
label('.cmphu#18')
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20


#-----------------------------------------------------------------------
#
#  $0c00 ROM page 12: More SYS functions (sprites)
#
#       Page 1: vertical blank interval
#       Page 2: visible scanlines
#
#-----------------------------------------------------------------------

align(0x100, size=0x100)

#-----------------------------------------------------------------------
# Extension SYS_Sprite6_v3_64
# Extension SYS_Sprite6x_v3_64
# Extension SYS_Sprite6y_v3_64
# Extension SYS_Sprite6xy_v3_64
#-----------------------------------------------------------------------

# Blit sprite in screen memory
#
# Variables
#       vAC             Destination address in screen
#       sysArgs[0:1]    Source address of 6xY pixels (colors 0..63) terminated
#                       by negative byte value N (typically N = -Y)
#       sysArgs[2:7]    Scratch (user as copy buffer)
#
# This SYS function draws a sprite of 6 pixels wide and Y pixels high.
# The pixel data is read sequentually from RAM, in horizontal chunks
# of 6 pixels at a time, and then written to the screen through the
# destination pointer (each chunk underneath the previous), thus
# drawing a 6xY stripe. Pixel values should be non-negative. The first
# negative byte N after a chunk signals the end of the sprite data.
# So the sprite's height Y is determined by the source data and is
# therefore flexible. This negative byte value, typically N == -Y,
# is then used to adjust the destination pointer's high byte, to make
# it easier to draw sprites wider than 6 pixels: just repeat the SYS
# call for as many 6-pixel wide stripes you need. All arguments are
# already left in place to facilitate this. After one call, the source
# pointer will point past that source data, effectively:
#       src += Y * 6 + 1
# The destination pointer will have been adjusted as:
#       dst += (Y + N) * 256 + 6
# (With arithmetic wrapping around on the same memory page)
#
# Y is only limited by source memory, not by CPU cycles. The
# implementation is such that the SYS function self-repeats, each
# time drawing the next 6-pixel chunk. It can typically draw 12
# pixels per scanline this way.

label('SYS_Sprite6_v3_64')

ld([sysArgs+0],X)               #15 Pixel data source address
ld([sysArgs+1],Y)               #16
ld([Y,X])                       #17 Next pixel or stop
bpl('.sysDpx0')                 #18
st([Y,Xpp])                     #19 Just X++

adda([vAC+1])                   #20 Adjust dst for convenience
st([vAC+1])                     #21
ld([vAC])                       #22
adda(6)                         #23
st([vAC])                       #24
ld([sysArgs+0])                 #25 Adjust src for convenience
adda(1)                         #26
st([sysArgs+0])                 #27
nop()                           #28
ld(hi('REENTER'),Y)             #29 Normal exit (no self-repeat)
jmp(Y,'REENTER')                #30
ld(-34/2)                       #31

label('.sysDpx0')
st([sysArgs+2])                 #20 Gobble 6 pixels into buffer
ld([Y,X])                       #21
st([Y,Xpp])                     #22 Just X++
st([sysArgs+3])                 #23
ld([Y,X])                       #24
st([Y,Xpp])                     #25 Just X++
st([sysArgs+4])                 #26
ld([Y,X])                       #27
st([Y,Xpp])                     #28 Just X++
st([sysArgs+5])                 #29
ld([Y,X])                       #30
st([Y,Xpp])                     #31 Just X++
st([sysArgs+6])                 #32
ld([Y,X])                       #33
st([Y,Xpp])                     #34 Just X++
st([sysArgs+7])                 #35

ld([vAC],X)                     #36 Screen memory destination address
ld([vAC+1],Y)                   #37
ld([sysArgs+2])                 #38 Write 6 pixels
st([Y,Xpp])                     #39
ld([sysArgs+3])                 #40
st([Y,Xpp])                     #41
ld([sysArgs+4])                 #42
st([Y,Xpp])                     #43
ld([sysArgs+5])                 #44
st([Y,Xpp])                     #45
ld([sysArgs+6])                 #46
st([Y,Xpp])                     #47
ld([sysArgs+7])                 #48
st([Y,Xpp])                     #49

ld([sysArgs+0])                 #50 src += 6
adda(6)                         #51
st([sysArgs+0])                 #52
ld([vAC+1])                     #53 dst += 256
adda(1)                         #54
st([vAC+1])                     #55

ld([vPC])                       #56 Self-repeating SYS call
suba(2)                         #57
st([vPC])                       #58
ld(hi('REENTER'),Y)             #59
jmp(Y,'REENTER')                #60
ld(-64/2)                       #61

align(64)
label('SYS_Sprite6x_v3_64')

ld([sysArgs+0],X)               #15 Pixel data source address
ld([sysArgs+1],Y)               #16
ld([Y,X])                       #17 Next pixel or stop
bpl('.sysDpx1')                 #18
st([Y,Xpp])                     #19 Just X++

adda([vAC+1])                   #20 Adjust dst for convenience
st([vAC+1])                     #21
ld([vAC])                       #22
suba(6)                         #23
st([vAC])                       #24
ld([sysArgs+0])                 #25 Adjust src for convenience
adda(1)                         #26
st([sysArgs+0])                 #27
nop()                           #28
ld(hi('REENTER'),Y)             #29 Normal exit (no self-repeat)
jmp(Y,'REENTER')                #30
ld(-34/2)                       #31

label('.sysDpx1')
st([sysArgs+7])                 #20 Gobble 6 pixels into buffer (backwards)
ld([Y,X])                       #21
st([Y,Xpp])                     #22 Just X++
st([sysArgs+6])                 #23
ld([Y,X])                       #24
st([Y,Xpp])                     #25 Just X++
st([sysArgs+5])                 #26
ld([Y,X])                       #27
st([Y,Xpp])                     #28 Just X++
st([sysArgs+4])                 #29
ld([Y,X])                       #30
st([Y,Xpp])                     #31 Just X++
st([sysArgs+3])                 #32
ld([Y,X])                       #33
st([Y,Xpp])                     #34 Just X++

ld([vAC],X)                     #35 Screen memory destination address
ld([vAC+1],Y)                   #36
st([Y,Xpp])                     #37 Write 6 pixels
ld([sysArgs+3])                 #38
st([Y,Xpp])                     #39
ld([sysArgs+4])                 #40
st([Y,Xpp])                     #41
ld([sysArgs+5])                 #42
st([Y,Xpp])                     #43
ld([sysArgs+6])                 #44
st([Y,Xpp])                     #45
ld([sysArgs+7])                 #46
st([Y,Xpp])                     #47

ld([sysArgs+0])                 #48 src += 6
adda(6)                         #49
st([sysArgs+0])                 #50
ld([vAC+1])                     #51 dst += 256
adda(1)                         #52
st([vAC+1])                     #53

ld([vPC])                       #54 Self-repeating SYS call
suba(2)                         #55
st([vPC])                       #56
ld(hi('REENTER'),Y)             #57
jmp(Y,'REENTER')                #58
ld(-62/2)                       #59

align(64)
label('SYS_Sprite6y_v3_64')

ld([sysArgs+0],X)               #15 Pixel data source address
ld([sysArgs+1],Y)               #16
ld([Y,X])                       #17 Next pixel or stop
bpl('.sysDpx2')                 #18
st([Y,Xpp])                     #19 Just X++

xora(255)                       #20 Adjust dst for convenience
adda(1)                         #21
adda([vAC+1])                   #22
st([vAC+1])                     #23
ld([vAC])                       #24
adda(6)                         #25
st([vAC])                       #26
ld([sysArgs+0])                 #27 Adjust src for convenience
adda(1)                         #28
st([sysArgs+0])                 #29
nop()                           #30
ld(hi('REENTER'),Y)             #31 Normal exit (no self-repeat)
jmp(Y,'REENTER')                #32
ld(-36/2)                       #33

label('.sysDpx2')
st([sysArgs+2])                 #20 Gobble 6 pixels into buffer
ld([Y,X])                       #21
st([Y,Xpp])                     #22 Just X++
st([sysArgs+3])                 #23
ld([Y,X])                       #24
st([Y,Xpp])                     #25 Just X++
st([sysArgs+4])                 #26
ld([Y,X])                       #27
st([Y,Xpp])                     #28 Just X++
st([sysArgs+5])                 #29
ld([Y,X])                       #30
st([Y,Xpp])                     #31 Just X++
st([sysArgs+6])                 #32
ld([Y,X])                       #33
st([Y,Xpp])                     #34 Just X++
st([sysArgs+7])                 #35

ld([vAC],X)                     #36 Screen memory destination address
ld([vAC+1],Y)                   #37
ld([sysArgs+2])                 #38 Write 6 pixels
st([Y,Xpp])                     #39
ld([sysArgs+3])                 #40
st([Y,Xpp])                     #41
ld([sysArgs+4])                 #42
st([Y,Xpp])                     #43
ld([sysArgs+5])                 #44
st([Y,Xpp])                     #45
ld([sysArgs+6])                 #46
st([Y,Xpp])                     #47
ld([sysArgs+7])                 #48
st([Y,Xpp])                     #49

ld([sysArgs+0])                 #50 src += 6
adda(6)                         #51
st([sysArgs+0])                 #52
ld([vAC+1])                     #53 dst -= 256
suba(1)                         #54
st([vAC+1])                     #55

ld([vPC])                       #56 Self-repeating SYS call
suba(2)                         #57
st([vPC])                       #58
ld(hi('REENTER'),Y)             #59
jmp(Y,'REENTER')                #60
ld(-64/2)                       #61

align(64)
label('SYS_Sprite6xy_v3_64')

ld([sysArgs+0],X)               #15 Pixel data source address
ld([sysArgs+1],Y)               #16
ld([Y,X])                       #17 Next pixel or stop
bpl('.sysDpx3')                 #18
st([Y,Xpp])                     #19 Just X++

xora(255)                       #20 Adjust dst for convenience
adda(1)                         #21
adda([vAC+1])                   #22
st([vAC+1])                     #23
ld([vAC])                       #24
suba(6)                         #25
st([vAC])                       #26
ld([sysArgs+0])                 #27 Adjust src for convenience
adda(1)                         #28
st([sysArgs+0])                 #29
nop()                           #30
ld(hi('REENTER'),Y)             #31 Normal exit (no self-repeat)
jmp(Y,'REENTER')                #32
ld(-36/2)                       #33

label('.sysDpx3')
st([sysArgs+7])                 #20 Gobble 6 pixels into buffer (backwards)
ld([Y,X])                       #21
st([Y,Xpp])                     #22 Just X++
st([sysArgs+6])                 #23
ld([Y,X])                       #24
st([Y,Xpp])                     #25 Just X++
st([sysArgs+5])                 #26
ld([Y,X])                       #27
st([Y,Xpp])                     #28 Just X++
st([sysArgs+4])                 #29
ld([Y,X])                       #30
st([Y,Xpp])                     #31 Just X++
st([sysArgs+3])                 #32
ld([Y,X])                       #33
st([Y,Xpp])                     #34 Just X++

ld([vAC],X)                     #35 Screen memory destination address
ld([vAC+1],Y)                   #36
st([Y,Xpp])                     #37 Write 6 pixels
ld([sysArgs+3])                 #38
st([Y,Xpp])                     #39
ld([sysArgs+4])                 #40
st([Y,Xpp])                     #41
ld([sysArgs+5])                 #42
st([Y,Xpp])                     #43
ld([sysArgs+6])                 #44
st([Y,Xpp])                     #45
ld([sysArgs+7])                 #46
st([Y,Xpp])                     #47

ld([sysArgs+0])                 #48 src += 6
adda(6)                         #49
st([sysArgs+0])                 #50
ld([vAC+1])                     #51 dst -= 256
suba(1)                         #52
st([vAC+1])                     #53

ld([vPC])                       #54 Self-repeating SYS call
suba(2)                         #55
st([vPC])                       #56
ld(hi('REENTER'),Y)             #57
jmp(Y,'REENTER')                #58
ld(-62/2)                       #59

#-----------------------------------------------------------------------

align(0x100)

label('sys_ExpanderControl')

ld(hi(ctrlBits),Y)                  #18
anda([vAC])                         #19 check for special ctrl code space
beq('sysEx#22')                     #20
ld([vAC])                           #21 load low byte of ctrl code in delay slot
anda(0xfc)                          #22 sanitize normal ctrl code
st([Y,ctrlBits])                    #23 store in ctrlBits
st([vTmp])                          #24 store in vTmp
bra('sysEx#27')                     #25 jump to issuing normal ctrl code
ld([vAC+1],Y)                       #26 load high byte of ctrl code in delay slot
label('sysEx#22')
anda(0xfc,X)                        #22 * special ctrl code
ld([Y,ctrlBits])                    #23 get previous normal code from ctrlBits
st([vTmp])                          #24 save it in vTmp
ld([vAC+1],Y)                       #25 load high byte of ctrl code
ctrl(Y,X)                           #26 issue special ctrl code
label('sysEx#27')
ld([vTmp])                          #27 load saved normal ctrl code
anda(0xfc,X)                        #28 sanitize ctrlBits
ctrl(Y,X)                           #29 issue normal ctrl code
ld([vTmp])                          #30 always load something after ctrl
ld(hi('REENTER'),Y)                 #31
jmp(Y,'REENTER')                    #32
ld(-36/2)                           #33

#-----------------------------------------------------------------------

label('sys_SpiExchangeBytes')

ld([Y,ctrlBits])                #18
st([sysArgs+4])                 #19

ld([sysArgs+0],X)               #20 Fetch byte to send
ld([sysArgs+1],Y)               #21
ld([Y,X])                       #22

for i in range(8):
  st([vTmp],Y);C('Bit %d'%(7-i))#23+i*12
  ld([sysArgs+4],X)             #24+i*12
  ctrl(Y,Xpp)                   #25+i*12 Set MOSI
  ctrl(Y,Xpp)                   #26+i*12 Raise SCLK, disable RAM!
  ld([0])                       #27+i*12 Get MISO
  anda(0b00001111)              #28+i*12 This is why R1 as pull-DOWN is simpler
  beq(pc()+3)                   #29+i*12
  bra(pc()+2)                   #30+i*12
  ld(1)                         #31+i*12
  ctrl(Y,X)                     #32+i*12,29+i*12 (Must be idempotent) Lower SCLK
  adda([vTmp])                  #33+i*12 Shift
  adda([vTmp])                  #34+i*12

ld([sysArgs+0],X)               #119 Store received byte
ld([sysArgs+3],Y)               #120
st([Y,X])                       #121

ld([sysArgs+0])                 #122 Advance pointer
adda(1)                         #123
st([sysArgs+0])                 #124

xora([sysArgs+2])               #125 Reached end?
beq('.sysSpi#128')              #126

ld([vPC])                       #127 Self-repeating SYS call
suba(2)                         #128
st([vPC])                       #129
ld(hi('NEXTY'),Y)               #130
jmp(Y,'NEXTY')                  #131
ld(-134/2)                      #132

label('.sysSpi#128')
ld(hi('NEXTY'),Y)               #128 Continue program
jmp(Y,'NEXTY')                  #129
ld(-132/2)                      #130

#-----------------------------------------------------------------------

label('sys_v6502')

st([vCpuSelect],Y)              #18 Activate v6502
ld(-22/2)                       #19
jmp(Y,'v6502_ENTER')            #20 Transfer control in the same time slice
adda([vTicks])                  #21
assert (38 - 22)//2 >= v6502_adjust

#-----------------------------------------------------------------------
#       MOS 6502 emulator
#-----------------------------------------------------------------------

# Some quirks:
# - Stack in zero page instead of page 1
# - No interrupts
# - No decimal mode (may never be added). D flag is emulated but ignored.
# - BRK switches back to running 16-bits vCPU
# - Illegal opcodes map to BRK, but can read ghost operands before trapping
# - Illegal opcode $ff won't be trapped and cause havoc instead

# Big things TODO:
# XXX Tuning, put most frequent instructions in the primary page

label('v6502_ror')
assert v6502_Cflag == 1
ld([v6502_ADH],Y)               #12
ld(-46//2+v6502_maxTicks)       #13 Is there enough time for the excess ticks?
adda([vTicks])                  #14
blt('.recheck17')               #15
ld([v6502_P])                   #16 Transfer C to "bit 8"
anda(1)                         #17
adda(127)                       #18
anda(128)                       #19
st([v6502_BI])                  #20 The real 6502 wouldn't use BI for this
ld([v6502_P])                   #21 Transfer bit 0 to C
anda(~1)                        #22
st([v6502_P])                   #23
ld([Y,X])                       #24
anda(1)                         #25
ora([v6502_P])                  #26
st([v6502_P])                   #27
ld('v6502_ror#38')              #28 Shift table lookup
st([vTmp])                      #29
ld([Y,X])                       #30
anda(~1)                        #31
ld(hi('shiftTable'),Y)          #32
jmp(Y,AC)                       #33
bra(255)                        #34 bra shiftTable+255
label('.recheck17')
ld(hi('v6502_check'),Y)         #17 Go back to time check before dispatch
jmp(Y,'v6502_check')            #18
ld(-20/2)                       #19

label('v6502_lsr')
assert v6502_Cflag == 1
ld([v6502_ADH],Y)               #12
ld([v6502_P])                   #13 Transfer bit 0 to C
anda(~1)                        #14
st([v6502_P])                   #15
ld([Y,X])                       #16
anda(1)                         #17
ora([v6502_P])                  #18
st([v6502_P])                   #19
ld('v6502_lsr#30')              #20 Shift table lookup
st([vTmp])                      #21
ld([Y,X])                       #22
anda(~1)                        #23
ld(hi('shiftTable'),Y)          #24
jmp(Y,AC)                       #25
bra(255)                        #26 bra shiftTable+255

label('v6502_rol')
assert v6502_Cflag == 1
ld([v6502_ADH],Y)               #12
ld([Y,X])                       #13
anda(0x80)                      #14
st([v6502_Tmp])                 #15
ld([v6502_P])                   #16
anda(1)                         #17
label('.rol#18')
adda([Y,X])                     #18
adda([Y,X])                     #19
st([Y,X])                       #20
st([v6502_Qz])                  #21 Z flag
st([v6502_Qn])                  #22 N flag
ld([v6502_P])                   #23 C Flag
anda(~1)                        #24
ld([v6502_Tmp],X)               #25
ora([X])                        #26
st([v6502_P])                   #27
ld(hi('v6502_next'),Y)          #28
ld(-32/2)                       #29
jmp(Y,'v6502_next')             #30
#nop()                          #31 Overlap
#
label('v6502_asl')
ld([v6502_ADH],Y)               #12,32
ld([Y,X])                       #13
anda(0x80)                      #14
st([v6502_Tmp])                 #15
bra('.rol#18')                  #16
ld(0)                           #17

label('v6502_jmp1')
nop()                           #12
ld([v6502_ADL])                 #13
st([v6502_PCL])                 #14
ld([v6502_ADH])                 #15
st([v6502_PCH])                 #16
ld(hi('v6502_next'),Y)          #17
jmp(Y,'v6502_next')             #18
ld(-20/2)                       #19

label('v6502_jmp2')
nop()                           #12
ld([v6502_ADH],Y)               #13
ld([Y,X])                       #14
st([Y,Xpp])                     #15 (Just X++) Wrap around: bug compatible with NMOS
st([v6502_PCL])                 #16
ld([Y,X])                       #17
st([v6502_PCH])                 #18
ld(hi('v6502_next'),Y)          #19
jmp(Y,'v6502_next')             #20
ld(-22/2)                       #21

label('v6502_pla')
ld([v6502_S])                   #12
ld(AC,X)                        #13
adda(1)                         #14
st([v6502_S])                   #15
ld([X])                         #16
st([v6502_A])                   #17
st([v6502_Qz])                  #18 Z flag
st([v6502_Qn])                  #19 N flag
ld(hi('v6502_next'),Y)          #20
ld(-24/2)                       #21
jmp(Y,'v6502_next')             #22
#nop()                          #23 Overlap
#
label('v6502_pha')
ld(hi('v6502_next'),Y)          #12,24
ld([v6502_S])                   #13
suba(1)                         #14
st([v6502_S],X)                 #15
ld([v6502_A])                   #16
st([X])                         #17
jmp(Y,'v6502_next')             #18
ld(-20/2)                       #19

label('v6502_brk')
ld(hi('ENTER'))                 #12 Switch to vCPU
st([vCpuSelect])                #13
assert v6502_A == vAC
ld(0)                           #14
st([vAC+1])                     #15
ld(hi('REENTER'),Y)             #16 Switch in the current time slice
ld(-22//2+v6502_adjust)         #17
jmp(Y,'REENTER')                #18
nop()                           #19

# All interpreter entry points must share the same page offset, because
# this offset is hard-coded as immediate operand in the video driver.
# The Gigatron's original vCPU's 'ENTER' label is already at $2ff, so we
# just use $dff for 'v6502_ENTER'. v6502 actually has two entry points.
# The other is 'v6502_RESUME' at $10ff. It is used for instructions
# that were fetched but not yet executed. Allowing the split gives finer
# granulariy, and hopefully more throughput for the simpler instructions.
# (There is no "overhead" for allowing instruction splitting, because
#  both emulation phases must administer [vTicks] anyway.)
while pc()&255 < 255:
  nop()
label('v6502_ENTER')
bra('v6502_next2')              #0 v6502 primary entry point
# --- Page boundary ---
suba(v6502_adjust)              #1,19 Adjust for vCPU/v6520 timing differences

#19 Addressing modes
(   'v6502_mode0'  ); bra('v6502_modeIZX'); bra('v6502_modeIMM'); bra('v6502_modeILL') # $00 xxx000xx
bra('v6502_modeZP');  bra('v6502_modeZP');  bra('v6502_modeZP');  bra('v6502_modeILL') # $04 xxx001xx
bra('v6502_modeIMP'); bra('v6502_modeIMM'); bra('v6502_modeACC'); bra('v6502_modeILL') # $08 xxx010xx
bra('v6502_modeABS'); bra('v6502_modeABS'); bra('v6502_modeABS'); bra('v6502_modeILL') # $0c xxx011xx
bra('v6502_modeREL'); bra('v6502_modeIZY'); bra('v6502_modeIMM'); bra('v6502_modeILL') # $10 xxx100xx
bra('v6502_modeZPX'); bra('v6502_modeZPX'); bra('v6502_modeZPX'); bra('v6502_modeILL') # $14 xxx101xx
bra('v6502_modeIMP'); bra('v6502_modeABY'); bra('v6502_modeIMP'); bra('v6502_modeILL') # $18 xxx110xx
bra('v6502_modeABX'); bra('v6502_modeABX'); bra('v6502_modeABX'); bra('v6502_modeILL') # $1c xxx111xx

# Special encoding cases for emulator:
#     $00 BRK -         but gets mapped to #$DD      handled in v6502_mode0
#     $20 JSR $DDDD     but gets mapped to #$DD      handled in v6502_mode0 and v6502_JSR
#     $40 RTI -         but gets mapped to #$DD      handled in v6502_mode0
#     $60 RTS -         but gets mapped to #$DD      handled in v6502_mode0
#     $6C JMP ($DDDD)   but gets mapped to $DDDD     handled in v6502_JMP2
#     $96 STX $DD,Y     but gets mapped to $DD,X     handled in v6502_STX2
#     $B6 LDX $DD,Y     but gets mapped to $DD,X     handled in v6502_LDX2
#     $BE LDX $DDDD,Y   but gets mapped to $DDDD,X   handled in v6502_modeABX

label('v6502_next')
adda([vTicks])                  #0
blt('v6502_exitBefore')         #1 No more ticks
label('v6502_next2')
st([vTicks])                    #2
#
# Fetch opcode
ld([v6502_PCL],X)               #3
ld([v6502_PCH],Y)               #4
ld([Y,X])                       #5 Fetch IR
st([v6502_IR])                  #6
ld([v6502_PCL])                 #7 PC++
adda(1)                         #8
st([v6502_PCL],X)               #9
beq(pc()+3)                     #10
bra(pc()+3)                     #11
ld(0)                           #12
ld(1)                           #12(!)
adda([v6502_PCH])               #13
st([v6502_PCH],Y)               #14
#
# Get addressing mode and fetch operands
ld([v6502_IR])                  #15 Get addressing mode
anda(31)                        #16
bra(AC)                         #17
bra('.next20')                  #18
# (jump table)                  #19
label('.next20')
ld([Y,X])                       #20 Fetch L
# Most opcodes branch away at this point, but IR & 31 == 0 falls through
#
# Implicit Mode for  BRK JSR RTI RTS (<  0x80) -- 26 cycles
# Immediate Mode for LDY CPY CPX     (>= 0x80) -- 36 cycles
label('v6502_mode0')
ld([v6502_IR])                  #21 'xxx0000'
bmi('.imm24')                   #22
ld([v6502_PCH])                 #23
bra('v6502_check')              #24
ld(-26/2)                       #25

# Resync with video driver. At this point we're returning BEFORE
# fetching and executing the next instruction.
label('v6502_exitBefore')
adda(v6502_maxTicks)            #3 Exit BEFORE fetch
bgt(pc()&255)                   #4 Resync
suba(1)                         #5
ld(hi('v6502_ENTER'))           #6 Set entry point to before 'fetch'
st([vCpuSelect])                #7
ld(hi('vBlankStart'),Y)         #8
jmp(Y,[vReturn])                #9 To video driver
ld(0)                           #10
assert v6502_overhead ==         11

# Immediate Mode: #$FF -- 36 cycles
label('v6502_modeIMM')
nop()                           #21 Wait for v6502_mode0 to join
nop()                           #22
ld([v6502_PCH])                 #23 Copy PC
label('.imm24')
st([v6502_ADH])                 #24
ld([v6502_PCL])                 #25
st([v6502_ADL],X)               #26
adda(1)                         #27 PC++
st([v6502_PCL])                 #28
beq(pc()+3)                     #29
bra(pc()+3)                     #30
ld(0)                           #31
ld(1)                           #31(!)
adda([v6502_PCH])               #32
st([v6502_PCH])                 #33
bra('v6502_check')              #34
ld(-36/2)                       #35

# Accumulator Mode: ROL ROR LSL ASR -- 28 cycles
label('v6502_modeACC')
ld(v6502_A&255)                 #21 Address of AC
st([v6502_ADL],X)               #22
ld(v6502_A>>8)                  #23
st([v6502_ADH])                 #24
ld(-28/2)                       #25
bra('v6502_check')              #26
#nop()                          #27 Overlap
#
# Implied Mode: no operand -- 24 cycles
label('v6502_modeILL')
label('v6502_modeIMP')
nop()                           #21,27
bra('v6502_check')              #22
ld(-24/2)                       #23

# Zero Page Modes: $DD $DD,X $DD,Y -- 36 cycles
label('v6502_modeZPX')
bra('.zp23')                    #21
adda([v6502_X])                 #22
label('v6502_modeZP')
bra('.zp23')                    #21
nop()                           #22
label('.zp23')
st([v6502_ADL],X)               #23
ld(0)                           #24 H=0
st([v6502_ADH])                 #25
ld(1)                           #26 PC++
adda([v6502_PCL])               #27
st([v6502_PCL])                 #28
beq(pc()+3)                     #29
bra(pc()+3)                     #30
ld(0)                           #31
ld(1)                           #31(!)
adda([v6502_PCH])               #32
st([v6502_PCH])                 #33
bra('v6502_check')              #34
ld(-36/2)                       #35

# Possible retry loop for modeABS and modeIZY. Because these need
# more time than the v6502_maxTicks of 38 Gigatron cycles, we may
# have to restart them after the next horizontal pulse.
label('.retry28')
beq(pc()+3)                     #28,37 PC--
bra(pc()+3)                     #29
ld(0)                           #30
ld(-1)                          #30(!)
adda([v6502_PCH])               #31
st([v6502_PCH])                 #32
ld([v6502_PCL])                 #33
suba(1)                         #34
st([v6502_PCL])                 #35
bra('v6502_next')               #36 Retry until sufficient time
ld(-38/2)                       #37

# Absolute Modes: $DDDD $DDDD,X $DDDD,Y -- 64 cycles
label('v6502_modeABS')
bra('.abs23')                   #21
ld(0)                           #22
label('v6502_modeABX')
bra('.abs23')                   #21
label('v6502_modeABY')
ld([v6502_X])                   #21,22
ld([v6502_Y])                   #22
label('.abs23')
st([v6502_ADL])                 #23
ld(-64//2+v6502_maxTicks)       #24 Is there enough time for the excess ticks?
adda([vTicks])                  #25
blt('.retry28')                 #26
ld([v6502_PCL])                 #27
ld([v6502_IR])                  #28 Special case $BE: LDX $DDDD,Y (we got X in ADL)
xora(0xbe)                      #29
beq(pc()+3)                     #30
bra(pc()+3)                     #31
ld([v6502_ADL])                 #32
ld([v6502_Y])                   #32(!)
adda([Y,X])                     #33 Fetch and add L
st([v6502_ADL])                 #34
bmi('.abs37')                   #35 Carry?
suba([Y,X])                     #36 Gets back original operand
bra('.abs39')                   #37
ora([Y,X])                      #38 Carry in bit 7
label('.abs37')
anda([Y,X])                     #37 Carry in bit 7
nop()                           #38
label('.abs39')
anda(0x80,X)                    #39 Move carry to bit 0
ld([X])                         #40
st([v6502_ADH])                 #41
ld([v6502_PCL])                 #42 PC++
adda(1)                         #43
st([v6502_PCL],X)               #44
beq(pc()+3)                     #45
bra(pc()+3)                     #46
ld(0)                           #47
ld(1)                           #47(!)
adda([v6502_PCH])               #48
st([v6502_PCH],Y)               #49
ld([Y,X])                       #50 Fetch H
adda([v6502_ADH])               #51
st([v6502_ADH])                 #52
ld([v6502_PCL])                 #53 PC++
adda(1)                         #54
st([v6502_PCL])                 #55
beq(pc()+3)                     #56
bra(pc()+3)                     #57
ld(0)                           #58
ld(1)                           #58(!)
adda([v6502_PCH])               #59
st([v6502_PCH])                 #60
ld([v6502_ADL],X)               #61
bra('v6502_check')              #62
ld(-64/2)                       #63

# Indirect Indexed Mode: ($DD),Y -- 54 cycles
label('v6502_modeIZY')
ld(AC,X)                        #21 $DD
ld(0,Y)                         #22 $00DD
ld(-54//2+v6502_maxTicks)       #23 Is there enough time for the excess ticks?
adda([vTicks])                  #24
nop()                           #25
blt('.retry28')                 #26
ld([v6502_PCL])                 #27
adda(1)                         #28 PC++
st([v6502_PCL])                 #29
beq(pc()+3)                     #30
bra(pc()+3)                     #31
ld(0)                           #32
ld(1)                           #32(!)
adda([v6502_PCH])               #33
st([v6502_PCH])                 #34
ld([Y,X])                       #35 Read word from zero-page
st([Y,Xpp])                     #36 (Just X++) Wrap-around is correct
st([v6502_ADL])                 #37
ld([Y,X])                       #38
st([v6502_ADH])                 #39
ld([v6502_Y])                   #40 Add Y
adda([v6502_ADL])               #41
st([v6502_ADL])                 #42
bmi('.izy45')                   #43 Carry?
suba([v6502_Y])                 #44 Gets back original operand
bra('.izy47')                   #45
ora([v6502_Y])                  #46 Carry in bit 7
label('.izy45')
anda([v6502_Y])                 #45 Carry in bit 7
nop()                           #46
label('.izy47')
anda(0x80,X)                    #47 Move carry to bit 0
ld([X])                         #48
adda([v6502_ADH])               #49
st([v6502_ADH])                 #50
ld([v6502_ADL],X)               #51
bra('v6502_check')              #52
ld(-54/2)                       #53

# Relative Mode: BEQ BNE BPL BMI BCC BCS BVC BVS -- 36 cycles
label('v6502_modeREL')
st([v6502_ADL],X)               #21 Offset (Only needed for branch)
bmi(pc()+3)                     #22 Sign extend
bra(pc()+3)                     #23
ld(0)                           #24
ld(255)                         #24(!)
st([v6502_ADH])                 #25
ld([v6502_PCL])                 #26 PC++ (Needed for both cases)
adda(1)                         #27
st([v6502_PCL])                 #28
beq(pc()+3)                     #29
bra(pc()+3)                     #30
ld(0)                           #31
ld(1)                           #31(!)
adda([v6502_PCH])               #32
st([v6502_PCH])                 #33
bra('v6502_check')              #34
ld(-36/2)                       #53

# Indexed Indirect Mode: ($DD,X) -- 38 cycles
label('v6502_modeIZX')
adda([v6502_X])                 #21 Add X
st([v6502_Tmp])                 #22
adda(1,X)                       #23 Read word from zero-page
ld([X])                         #24
st([v6502_ADH])                 #25
ld([v6502_Tmp],X)               #26
ld([X])                         #27
st([v6502_ADL],X)               #28
ld([v6502_PCL])                 #29 PC++
adda(1)                         #30
st([v6502_PCL])                 #31
beq(pc()+3)                     #32
bra(pc()+3)                     #33
ld(0)                           #34
ld(1)                           #34(!)
adda([v6502_PCH])               #35
st([v6502_PCH])                 #36
ld(-38/2)                       #37 !!! Fall through to v6502_check !!!
#
# Update elapsed time for the addressing mode processing.
# Then check if we can immediately execute this instruction.
# Otherwise transfer control to the video driver.
label('v6502_check')
adda([vTicks])                  #0
blt('v6502_exitAfter')          #1 No more ticks
st([vTicks])                    #2
ld(hi('v6502_execute'),Y)       #3
jmp(Y,[v6502_IR])               #4
bra(255)                        #5

# Otherwise resync with video driver. At this point we're returning AFTER
# addressing mode decoding, but before executing the instruction.
label('v6502_exitAfter')
adda(v6502_maxTicks)            #3 Exit AFTER fetch
bgt(pc()&255)                   #4 Resync
suba(1)                         #5
ld(hi('v6502_RESUME'))          #6 Set entry point to before 'execute'
st([vCpuSelect])                #7
ld(hi('vBlankStart'),Y)         #8
jmp(Y,[vReturn])                #9 To video driver
ld(0)                           #10
assert v6502_overhead ==         11

align(0x100,size=0x100)
label('v6502_execute')
# This page works as a 255-entry (0..254) jump table for 6502 opcodes.
# Jumping into this page must have 'bra 255' in the branch delay slot
# in order to get out again and dispatch to the right continuation.
# X must hold [v6502_ADL],
# Y will hold hi('v6502_execute'),
# A will be loaded with the code offset (this is skipped at offset $ff)
ld('v6502_BRK'); ld('v6502_ORA'); ld('v6502_ILL'); ld('v6502_ILL') #6 $00
ld('v6502_ILL'); ld('v6502_ORA'); ld('v6502_ASL'); ld('v6502_ILL') #6
ld('v6502_PHP'); ld('v6502_ORA'); ld('v6502_ASL'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_ORA'); ld('v6502_ASL'); ld('v6502_ILL') #6
ld('v6502_BPL'); ld('v6502_ORA'); ld('v6502_ILL'); ld('v6502_ILL') #6 $10
ld('v6502_ILL'); ld('v6502_ORA'); ld('v6502_ASL'); ld('v6502_ILL') #6
ld('v6502_CLC'); ld('v6502_ORA'); ld('v6502_ILL'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_ORA'); ld('v6502_ASL'); ld('v6502_ILL') #6
ld('v6502_JSR'); ld('v6502_AND'); ld('v6502_ILL'); ld('v6502_ILL') #6 $20
ld('v6502_BIT'); ld('v6502_AND'); ld('v6502_ROL'); ld('v6502_ILL') #6
ld('v6502_PLP'); ld('v6502_AND'); ld('v6502_ROL'); ld('v6502_ILL') #6
ld('v6502_BIT'); ld('v6502_AND'); ld('v6502_ROL'); ld('v6502_ILL') #6
ld('v6502_BMI'); ld('v6502_AND'); ld('v6502_ILL'); ld('v6502_ILL') #6 $30
ld('v6502_ILL'); ld('v6502_AND'); ld('v6502_ROL'); ld('v6502_ILL') #6
ld('v6502_SEC'); ld('v6502_AND'); ld('v6502_ILL'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_AND'); ld('v6502_ROL'); ld('v6502_ILL') #6
ld('v6502_RTI'); ld('v6502_EOR'); ld('v6502_ILL'); ld('v6502_ILL') #6 $40
ld('v6502_ILL'); ld('v6502_EOR'); ld('v6502_LSR'); ld('v6502_ILL') #6
ld('v6502_PHA'); ld('v6502_EOR'); ld('v6502_LSR'); ld('v6502_ILL') #6
ld('v6502_JMP1');ld('v6502_EOR'); ld('v6502_LSR'); ld('v6502_ILL') #6
ld('v6502_BVC'); ld('v6502_EOR'); ld('v6502_ILL'); ld('v6502_ILL') #6 $50
ld('v6502_ILL'); ld('v6502_EOR'); ld('v6502_LSR'); ld('v6502_ILL') #6
ld('v6502_CLI'); ld('v6502_EOR'); ld('v6502_ILL'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_EOR'); ld('v6502_LSR'); ld('v6502_ILL') #6
ld('v6502_RTS'); ld('v6502_ADC'); ld('v6502_ILL'); ld('v6502_ILL') #6 $60
ld('v6502_ILL'); ld('v6502_ADC'); ld('v6502_ROR'); ld('v6502_ILL') #6
ld('v6502_PLA'); ld('v6502_ADC'); ld('v6502_ROR'); ld('v6502_ILL') #6
ld('v6502_JMP2');ld('v6502_ADC'); ld('v6502_ROR'); ld('v6502_ILL') #6
ld('v6502_BVS'); ld('v6502_ADC'); ld('v6502_ILL'); ld('v6502_ILL') #6 $70
ld('v6502_ILL'); ld('v6502_ADC'); ld('v6502_ROR'); ld('v6502_ILL') #6
ld('v6502_SEI'); ld('v6502_ADC'); ld('v6502_ILL'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_ADC'); ld('v6502_ROR'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_STA'); ld('v6502_ILL'); ld('v6502_ILL') #6 $80
ld('v6502_STY'); ld('v6502_STA'); ld('v6502_STX'); ld('v6502_ILL') #6
ld('v6502_DEY'); ld('v6502_ILL'); ld('v6502_TXA'); ld('v6502_ILL') #6
ld('v6502_STY'); ld('v6502_STA'); ld('v6502_STX'); ld('v6502_ILL') #6
ld('v6502_BCC'); ld('v6502_STA'); ld('v6502_ILL'); ld('v6502_ILL') #6 $90
ld('v6502_STY'); ld('v6502_STA'); ld('v6502_STX2');ld('v6502_ILL') #6
ld('v6502_TYA'); ld('v6502_STA'); ld('v6502_TXS'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_STA'); ld('v6502_ILL'); ld('v6502_ILL') #6
ld('v6502_LDY'); ld('v6502_LDA'); ld('v6502_LDX'); ld('v6502_ILL') #6 $A0
ld('v6502_LDY'); ld('v6502_LDA'); ld('v6502_LDX'); ld('v6502_ILL') #6
ld('v6502_TAY'); ld('v6502_LDA'); ld('v6502_TAX'); ld('v6502_ILL') #6
ld('v6502_LDY'); ld('v6502_LDA'); ld('v6502_LDX'); ld('v6502_ILL') #6
ld('v6502_BCS'); ld('v6502_LDA'); ld('v6502_ILL'); ld('v6502_ILL') #6 $B0
ld('v6502_LDY'); ld('v6502_LDA'); ld('v6502_LDX2');ld('v6502_ILL') #6
ld('v6502_CLV'); ld('v6502_LDA'); ld('v6502_TSX'); ld('v6502_ILL') #6
ld('v6502_LDY'); ld('v6502_LDA'); ld('v6502_LDX'); ld('v6502_ILL') #6
ld('v6502_CPY'); ld('v6502_CMP'); ld('v6502_ILL'); ld('v6502_ILL') #6 $C0
ld('v6502_CPY'); ld('v6502_CMP'); ld('v6502_DEC'); ld('v6502_ILL') #6
ld('v6502_INY'); ld('v6502_CMP'); ld('v6502_DEX'); ld('v6502_ILL') #6
ld('v6502_CPY'); ld('v6502_CMP'); ld('v6502_DEC'); ld('v6502_ILL') #6
ld('v6502_BNE'); ld('v6502_CMP'); ld('v6502_ILL'); ld('v6502_ILL') #6 $D0
ld('v6502_ILL'); ld('v6502_CMP'); ld('v6502_DEC'); ld('v6502_ILL') #6
ld('v6502_CLD'); ld('v6502_CMP'); ld('v6502_ILL'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_CMP'); ld('v6502_DEC'); ld('v6502_ILL') #6
ld('v6502_CPX'); ld('v6502_SBC'); ld('v6502_ILL'); ld('v6502_ILL') #6 $E0
ld('v6502_CPX'); ld('v6502_SBC'); ld('v6502_INC'); ld('v6502_ILL') #6
ld('v6502_INX'); ld('v6502_SBC'); ld('v6502_NOP'); ld('v6502_ILL') #6
ld('v6502_CPX'); ld('v6502_SBC'); ld('v6502_INC'); ld('v6502_ILL') #6
ld('v6502_BEQ'); ld('v6502_SBC'); ld('v6502_ILL'); ld('v6502_ILL') #6 $F0
ld('v6502_ILL'); ld('v6502_SBC'); ld('v6502_INC'); ld('v6502_ILL') #6
ld('v6502_SED'); ld('v6502_SBC'); ld('v6502_ILL'); ld('v6502_ILL') #6
ld('v6502_ILL'); ld('v6502_SBC'); ld('v6502_INC')                  #6
bra(AC)                         #6,7 Dispatch into next page
# --- Page boundary ---
align(0x100,size=0x100)
ld(hi('v6502_next'),Y)          #8 Handy for instructions that don't clobber Y

label('v6502_ADC')
assert pc()&255 == 1
assert v6502_Cflag == 1
assert v6502_Vemu == 128
ld([v6502_ADH],Y)               #9 Must be at page offset 1, so A=1
anda([v6502_P])                 #10 Carry in (AC=1 because lo('v6502_ADC')=1)
adda([v6502_A])                 #11 Sum
beq('.adc14')                   #12 Danger zone for dropping a carry
adda([Y,X])                     #13
st([v6502_Qz])                  #14 Z flag, don't overwrite left-hand side (A) yet
st([v6502_Qn])                  #15 N flag
xora([v6502_A])                 #16 V flag, (Q^A) & (B^Q) & 0x80
st([v6502_A])                   #17
ld([Y,X])                       #18
xora([v6502_Qz])                #19
anda([v6502_A])                 #20
anda(0x80)                      #21
st([v6502_Tmp])                 #22
ld([v6502_Qz])                  #23 Update A
st([v6502_A])                   #24
bmi('.adc27')                   #25 C flag
suba([Y,X])                     #26
bra('.adc29')                   #27
ora([Y,X])                      #28
label('.adc27')
anda([Y,X])                     #27
nop()                           #28
label('.adc29')
anda(0x80,X)                    #29
ld([v6502_P])                   #30 Update P
anda(~v6502_Vemu&~v6502_Cflag)  #31
ora([X])                        #32
ora([v6502_Tmp])                #33
st([v6502_P])                   #34
ld(hi('v6502_next'),Y)          #35
jmp(Y,'v6502_next')             #36
ld(-38/2)                       #37
# Cin=1, A=$FF, B=$DD --> Result=$DD, Cout=1, V=0
# Cin=0, A=$00, B=$DD --> Result=$DD, Cout=0, V=0
label('.adc14')
st([v6502_A])                   #14 Special case
st([v6502_Qz])                  #15 Z flag
st([v6502_Qn])                  #16 N flag
ld([v6502_P])                   #17
anda(0x7f)                      #18 V=0, keep C
st([v6502_P])                   #19
ld(hi('v6502_next'),Y)          #20
ld(-24/2)                       #21
jmp(Y,'v6502_next')             #22
#nop()                          #23 Overlap
#
label('v6502_SBC')
# No matter how hard we try, v6502_SBC always comes out a lot clumsier
# than v6502_ADC. And that one already barely fits in 38 cycles and is
# hard to follow. So we use a hack: transmorph our SBC into an ADC with
# inverted operand, and then dispatch again. Simple and effective.
ld([v6502_ADH],Y)               #9,24
ld([Y,X])                       #10
xora(255)                       #11 Invert right-hand side operand
st([v6502_BI])                  #12 Park modified operand for v6502_ADC
ld(v6502_BI&255)                #13 Address of BI
st([v6502_ADL],X)               #14
ld(v6502_BI>>8)                 #15
st([v6502_ADH])                 #16
ld(0x69)                        #17 ADC #$xx (Any ADC opcode will do)
st([v6502_IR])                  #18
ld(hi('v6502_check'),Y)         #20 Go back to time check before dispatch
jmp(Y,'v6502_check')            #20
ld(-22/2)                       #21

# Carry calculation table
#   L7 R7 C7   RX UC SC
#   -- -- -- | -- -- --
#    0  0  0 |  0  0  0
#    0  0  1 |  0  0  0
#    1  0  0 |  0  1 +1
#    1  0  1 |  0  0  0
#    0  1  0 | -1  1  0
#    0  1  1 | -1  0 -1
#    1  1  0 | -1  1  0
#    1  1  1 | -1  1  0
#   -- -- -- | -- -- --
#    ^  ^  ^    ^  ^  ^
#    |  |  |    |  |  `--- Carry of unsigned L + signed R: SC = RX + UC
#    |  |  |    |  `----- Carry of unsigned L + unsigned R: UC = C7 ? L7&R7 : L7|R7
#    |  |  |    `------- Sign extension of signed R
#    |  |  `--------- MSB of unextended L + R
#    |  `----------- MSB of right operand R
#    `------------- MSB of left operand L

label('v6502_CLC')
ld([v6502_P])                   #9
bra('.sec12')                   #10
label('v6502_SEC')
anda(~v6502_Cflag)              #9,11 Overlap
ld([v6502_P])                   #10
ora(v6502_Cflag)                #11
label('.sec12')
st([v6502_P])                   #12
nop()                           #13
label('.next14')
jmp(Y,'v6502_next')             #14
ld(-16/2)                       #15

label('v6502_BPL')
ld([v6502_Qn])                  #9
bmi('.next12')                  #10
bpl('.branch13')                #11
#nop()                          #12 Overlap
label('v6502_BMI')
ld([v6502_Qn])                  #9,12
bpl('.next12')                  #10
bmi('.branch13')                #11
#nop()                          #12 Overlap
label('v6502_BVC')
ld([v6502_P])                   #9,12
anda(v6502_Vemu)                #10
beq('.branch13')                #11
bne('.next14')                  #12
#nop()                          #13 Overlap
label('v6502_BVS')
ld([v6502_P])                   #9,13
anda(v6502_Vemu)                #10
bne('.branch13')                #11
beq('.next14')                  #12
#nop()                          #13 Overlap
label('v6502_BCC')
ld([v6502_P])                   #9,13
anda(v6502_Cflag)               #10
beq('.branch13')                #11
bne('.next14')                  #12
#nop()                          #13 Overlap
label('v6502_BCS')
ld([v6502_P])                   #9,13
anda(v6502_Cflag)               #10
bne('.branch13')                #11
beq('.next14')                  #12
#nop()                          #13 Overlap
label('v6502_BNE')
ld([v6502_Qz])                  #9,13
beq('.next12')                  #10
bne('.branch13')                #11
#nop()                          #12 Overlap
label('v6502_BEQ')
ld([v6502_Qz])                  #9,12
bne('.next12')                  #10
beq('.branch13')                #11
#nop()                          #12 Overlap
label('.branch13')
ld([v6502_ADL])                 #13,12 PC + offset
adda([v6502_PCL])               #14
st([v6502_PCL])                 #15
bmi('.bra0')                    #16 Carry
suba([v6502_ADL])               #17
bra('.bra1')                    #18
ora([v6502_ADL])                #19
label('.bra0')
anda([v6502_ADL])               #18
nop()                           #19
label('.bra1')
anda(0x80,X)                    #20
ld([X])                         #21
adda([v6502_ADH])               #22
adda([v6502_PCH])               #23
st([v6502_PCH])                 #24
nop()                           #25
jmp(Y,'v6502_next')             #26
ld(-28/2)                       #27

label('v6502_INX')
nop()                           #9
ld([v6502_X])                   #10
adda(1)                         #11
st([v6502_X])                   #12
label('.inx13')
st([v6502_Qz])                  #13 Z flag
st([v6502_Qn])                  #14 N flag
ld(-18/2)                       #15
jmp(Y,'v6502_next')             #16
nop()                           #17

label('.next12')
jmp(Y,'v6502_next')             #12
ld(-14/2)                       #13

label('v6502_DEX')
ld([v6502_X])                   #9
suba(1)                         #10
bra('.inx13')                   #11
st([v6502_X])                   #12

label('v6502_INY')
ld([v6502_Y])                   #9
adda(1)                         #10
bra('.inx13')                   #11
st([v6502_Y])                   #12

label('v6502_DEY')
ld([v6502_Y])                   #9
suba(1)                         #10
bra('.inx13')                   #11
st([v6502_Y])                   #12

label('v6502_NOP')
ld(-12/2)                       #9
jmp(Y,'v6502_next')             #10
#nop()                          #11 Overlap
#
label('v6502_AND')
ld([v6502_ADH],Y)               #9,11
ld([v6502_A])                   #10
bra('.eor13')                   #11
anda([Y,X])                     #12

label('v6502_ORA')
ld([v6502_ADH],Y)               #9
ld([v6502_A])                   #10
bra('.eor13')                   #11
label('v6502_EOR')
ora([Y,X])                      #12,9
#
#label('v6502_EOR')
#nop()                          #9 Overlap
ld([v6502_ADH],Y)               #10
ld([v6502_A])                   #11
xora([Y,X])                     #12
label('.eor13')
st([v6502_A])                   #13
st([v6502_Qz])                  #14 Z flag
st([v6502_Qn])                  #15 N flag
ld(hi('v6502_next'),Y)          #16
ld(-20/2)                       #17
jmp(Y,'v6502_next')             #18
#nop()                          #19 Overlap
#
label('v6502_JMP1')
ld(hi('v6502_jmp1'),Y)          #9,19 JMP $DDDD
jmp(Y,'v6502_jmp1')             #10
#nop()                          #11 Overlap
label('v6502_JMP2')
ld(hi('v6502_jmp2'),Y)          #9 JMP ($DDDD)
jmp(Y,'v6502_jmp2')             #10
#nop()                          #11 Overlap
label('v6502_JSR')
ld([v6502_S])                   #9,11
suba(2)                         #10
st([v6502_S],X)                 #11
ld(v6502_Stack>>8,Y)            #12
ld([v6502_PCH])                 #13 Let ADL,ADH point to L operand
st([v6502_ADH])                 #14
ld([v6502_PCL])                 #15
st([v6502_ADL])                 #16
adda(1)                         #17 Push ++PC
st([v6502_PCL])                 #18 Let PCL,PCH point to H operand
st([Y,Xpp])                     #19
beq(pc()+3)                     #20
bra(pc()+3)                     #21
ld(0)                           #22
ld(1)                           #22(!)
adda([v6502_PCH])               #23
st([v6502_PCH])                 #24
st([Y,X])                       #25
ld([v6502_ADL],X)               #26 Fetch L
ld([v6502_ADH],Y)               #27
ld([Y,X])                       #28
ld([v6502_PCL],X)               #29 Fetch H
st([v6502_PCL])                 #30
ld([v6502_PCH],Y)               #31
ld([Y,X])                       #32
st([v6502_PCH])                 #33
ld(hi('v6502_next'),Y)          #34
ld(-38/2)                       #35
jmp(Y,'v6502_next')             #36
#nop()                          #37 Overlap
#
label('v6502_INC')
ld(hi('v6502_inc'),Y)           #9,37
jmp(Y,'v6502_inc')              #10
#nop()                          #11 Overlap
label('v6502_LDA')
ld(hi('v6502_lda'),Y)           #9,11
jmp(Y,'v6502_lda')              #10
#nop()                          #11 Overlap
label('v6502_LDX')
ld(hi('v6502_ldx'),Y)           #9,11
jmp(Y,'v6502_ldx')              #10
#nop()                          #11 Overlap
label('v6502_LDX2')
ld(hi('v6502_ldx2'),Y)          #9,11
jmp(Y,'v6502_ldx2')             #10
#nop()                          #11 Overlap
label('v6502_LDY')
ld(hi('v6502_ldy'),Y)           #9,11
jmp(Y,'v6502_ldy')              #10
#nop()                          #11 Overlap
label('v6502_STA')
ld(hi('v6502_sta'),Y)           #9,11
jmp(Y,'v6502_sta')              #10
#nop()                          #11 Overlap
label('v6502_STX')
ld(hi('v6502_stx'),Y)           #9,11
jmp(Y,'v6502_stx')              #10
#nop()                          #11 Overlap
label('v6502_STX2')
ld(hi('v6502_stx2'),Y)          #9,11
jmp(Y,'v6502_stx2')             #10
#nop()                          #11 Overlap
label('v6502_STY')
ld(hi('v6502_sty'),Y)           #9,11
jmp(Y,'v6502_sty')              #10
#nop()                          #11 Overlap
label('v6502_TAX')
ld(hi('v6502_tax'),Y)           #9,11
jmp(Y,'v6502_tax')              #10
#nop()                          #11 Overlap
label('v6502_TAY')
ld(hi('v6502_tay'),Y)           #9,11
jmp(Y,'v6502_tay')              #10
#nop()                          #11 Overlap
label('v6502_TXA')
ld(hi('v6502_txa'),Y)           #9,11
jmp(Y,'v6502_txa')              #10
#nop()                          #11 Overlap
label('v6502_TYA')
ld(hi('v6502_tya'),Y)           #9,11
jmp(Y,'v6502_tya')              #10
#nop()                          #11 Overlap
label('v6502_CLV')
ld(hi('v6502_clv'),Y)           #9,11
jmp(Y,'v6502_clv')              #10
#nop()                          #11 Overlap
label('v6502_RTI')
ld(hi('v6502_rti'),Y)           #9,11
jmp(Y,'v6502_rti')              #10
#nop()                          #11 Overlap
label('v6502_ROR')
ld(hi('v6502_ror'),Y)           #9,11
jmp(Y,'v6502_ror')              #10
#nop()                          #11 Overlap
label('v6502_LSR')
ld(hi('v6502_lsr'),Y)           #9,11
jmp(Y,'v6502_lsr')              #10
#nop()                          #11 Overlap
label('v6502_PHA')
ld(hi('v6502_pha'),Y)           #9,11
jmp(Y,'v6502_pha')              #10
#nop()                          #11 Overlap
label('v6502_CLI')
ld(hi('v6502_cli'),Y)           #9,11
jmp(Y,'v6502_cli')              #10
#nop()                          #11 Overlap
label('v6502_RTS')
ld(hi('v6502_rts'),Y)           #9,11
jmp(Y,'v6502_rts')              #10
#nop()                          #11 Overlap
label('v6502_PLA')
ld(hi('v6502_pla'),Y)           #9,11
jmp(Y,'v6502_pla')              #10
#nop()                          #11 Overlap
label('v6502_SEI')
ld(hi('v6502_sei'),Y)           #9,11
jmp(Y,'v6502_sei')              #10
#nop()                          #11 Overlap
label('v6502_TXS')
ld(hi('v6502_txs'),Y)           #9,11
jmp(Y,'v6502_txs')              #10
#nop()                          #11 Overlap
label('v6502_TSX')
ld(hi('v6502_tsx'),Y)           #9,11
jmp(Y,'v6502_tsx')              #10
#nop()                          #11 Overlap
label('v6502_CPY')
ld(hi('v6502_cpy'),Y)           #9,11
jmp(Y,'v6502_cpy')              #10
#nop()                          #11 Overlap
label('v6502_CMP')
ld(hi('v6502_cmp'),Y)           #9,11
jmp(Y,'v6502_cmp')              #10
#nop()                          #11 Overlap
label('v6502_DEC')
ld(hi('v6502_dec'),Y)           #9,11
jmp(Y,'v6502_dec')              #10
#nop()                          #11 Overlap
label('v6502_CLD')
ld(hi('v6502_cld'),Y)           #9,11
jmp(Y,'v6502_cld')              #10
#nop()                          #11 Overlap
label('v6502_CPX')
ld(hi('v6502_cpx'),Y)           #9,11
jmp(Y,'v6502_cpx')              #10
#nop()                          #11 Overlap
label('v6502_ASL')
ld(hi('v6502_asl'),Y)           #9,11
jmp(Y,'v6502_asl')              #10
#nop()                          #11 Overlap
label('v6502_PHP')
ld(hi('v6502_php'),Y)           #9,11
jmp(Y,'v6502_php')              #10
#nop()                          #11 Overlap
label('v6502_BIT')
ld(hi('v6502_bit'),Y)           #9
jmp(Y,'v6502_bit')              #10
#nop()                          #11 Overlap
label('v6502_ROL')
ld(hi('v6502_rol'),Y)           #9
jmp(Y,'v6502_rol')              #10
#nop()                          #11 Overlap
label('v6502_PLP')
ld(hi('v6502_plp'),Y)           #9
jmp(Y,'v6502_plp')              #10
#nop()                          #11 Overlap
label('v6502_SED')              # Decimal mode not implemented
ld(hi('v6502_sed'),Y)           #9,11
jmp(Y,'v6502_sed')              #10
#nop()                          #11 Overlap
label('v6502_ILL') # All illegal opcodes map to BRK, except $FF which will crash
label('v6502_BRK')
ld(hi('v6502_brk'),Y)           #9
jmp(Y,'v6502_brk')              #10
#nop()                          #11 Overlap

while pc()&255 < 255:
  nop()

# `v6502_RESUME' is the interpreter's secondary entry point for when
# the opcode and operands were already fetched, just before the last hPulse.
# It must be at $xxff, prefably somewhere in v6502's own code pages.
label('v6502_RESUME')
assert (pc()&255) == 255
suba(v6502_adjust)              #0,11 v6502 secondary entry point
# --- Page boundary ---
align(0x100,size=0x200)
st([vTicks])                    #1
ld([v6502_ADL],X)               #2
ld(hi('v6502_execute'),Y)       #3
jmp(Y,[v6502_IR])               #4
bra(255)                        #5

label('v6502_dec')
ld([v6502_ADH],Y)               #12
ld([Y,X])                       #13
suba(1)                         #14
st([Y,X])                       #15
st([v6502_Qz])                  #16 Z flag
st([v6502_Qn])                  #17 N flag
ld(hi('v6502_next'),Y)          #18
ld(-22/2)                       #19
jmp(Y,'v6502_next')             #20
#nop()                          #21 Overlap
#
label('v6502_inc')
ld([v6502_ADH],Y)               #12,22
ld([Y,X])                       #13
adda(1)                         #14
st([Y,X])                       #15
st([v6502_Qz])                  #16 Z flag
st([v6502_Qn])                  #17 N flag
ld(hi('v6502_next'),Y)          #18
ld(-22/2)                       #19
jmp(Y,'v6502_next')             #20
nop()                           #21

label('v6502_lda')
nop()                           #12
ld([v6502_ADH],Y)               #13
ld([Y,X])                       #14
st([v6502_A])                   #15
label('.lda16')
st([v6502_Qz])                  #16 Z flag
st([v6502_Qn])                  #17 N flag
nop()                           #18
ld(hi('v6502_next'),Y)          #19
jmp(Y,'v6502_next')             #20
ld(-22/2)                       #21

label('v6502_ldx')
ld([v6502_ADH],Y)               #12
ld([Y,X])                       #13
bra('.lda16')                   #14
st([v6502_X])                   #15

label('v6502_ldy')
ld([v6502_ADH],Y)               #12
ld([Y,X])                       #13
bra('.lda16')                   #14
st([v6502_Y])                   #15

label('v6502_ldx2')
ld([v6502_ADL])                 #12 Special case $B6: LDX $DD,Y
suba([v6502_X])                 #13 Undo X offset
adda([v6502_Y],X)               #14 Apply Y instead
ld([X])                         #15
st([v6502_X])                   #16
st([v6502_Qz])                  #17 Z flag
st([v6502_Qn])                  #18 N flag
ld(hi('v6502_next'),Y)          #19
jmp(Y,'v6502_next')             #20
ld(-22/2)                       #21

label('v6502_sta')
ld([v6502_ADH],Y)               #12
ld([v6502_A])                   #13
st([Y,X])                       #14
ld(hi('v6502_next'),Y)          #15
jmp(Y,'v6502_next')             #16
ld(-18/2)                       #17

label('v6502_stx')
ld([v6502_ADH],Y)               #12
ld([v6502_X])                   #13
st([Y,X])                       #14
ld(hi('v6502_next'),Y)          #15
jmp(Y,'v6502_next')             #16
ld(-18/2)                       #17

label('v6502_stx2')
ld([v6502_ADL])                 #12 Special case $96: STX $DD,Y
suba([v6502_X])                 #13 Undo X offset
adda([v6502_Y],X)               #14 Apply Y instead
ld([v6502_X])                   #15
st([X])                         #16
ld(hi('v6502_next'),Y)          #17
jmp(Y,'v6502_next')             #18
ld(-20/2)                       #19

label('v6502_sty')
ld([v6502_ADH],Y)               #12
ld([v6502_Y])                   #13
st([Y,X])                       #14
ld(hi('v6502_next'),Y)          #15
jmp(Y,'v6502_next')             #16
label('v6502_tax')
ld(-18/2)                       #17,12
#
#label('v6502_tax')
#nop()                          #12 Overlap
ld([v6502_A])                   #13
st([v6502_X])                   #14
label('.tax15')
st([v6502_Qz])                  #15 Z flag
st([v6502_Qn])                  #16 N flag
ld(hi('v6502_next'),Y)          #17
jmp(Y,'v6502_next')             #18
label('v6502_tsx')
ld(-20/2)                       #19
#
#label('v6502_tsx')
#nop()                          #12 Overlap
ld([v6502_S])                   #13
suba(1)                         #14 Shift down on export
st([v6502_X])                   #15
label('.tsx16')
st([v6502_Qz])                  #16 Z flag
st([v6502_Qn])                  #17 N flag
nop()                           #18
ld(hi('v6502_next'),Y)          #19
jmp(Y,'v6502_next')             #20
ld(-22/2)                       #21

label('v6502_txs')
ld([v6502_X])                   #12
adda(1)                         #13 Shift up on import
bra('.tsx16')                   #14
st([v6502_S])                   #15

label('v6502_tay')
ld([v6502_A])                   #12
bra('.tax15')                   #13
st([v6502_Y])                   #14

label('v6502_txa')
ld([v6502_X])                   #12
bra('.tax15')                   #13
st([v6502_A])                   #14

label('v6502_tya')
ld([v6502_Y])                   #12
bra('.tax15')                   #13
st([v6502_A])                   #14

label('v6502_cli')
ld([v6502_P])                   #12
bra('.clv15')                   #13
anda(~v6502_Iflag)              #14

label('v6502_sei')
ld([v6502_P])                   #12
bra('.clv15')                   #13
ora(v6502_Iflag)                #14

label('v6502_cld')
ld([v6502_P])                   #12
bra('.clv15')                   #13
anda(~v6502_Dflag)              #14

label('v6502_sed')
ld([v6502_P])                   #12
bra('.clv15')                   #13
label('v6502_clv')
ora(v6502_Dflag)                #14,12 Overlap
#
#label('v6502_clv')
#nop()                          #12
ld([v6502_P])                   #13
anda(~v6502_Vemu)               #14
label('.clv15')
st([v6502_P])                   #15
ld(hi('v6502_next'),Y)          #16
ld(-20/2)                       #17
jmp(Y,'v6502_next')             #18
label('v6502_bit')
nop()                           #19,12
#
#label('v6502_bit')
#nop()                          #12 Overlap
ld([v6502_ADL],X)               #13
ld([v6502_ADH],Y)               #14
ld([Y,X])                       #15
st([v6502_Qn])                  #16 N flag
anda([v6502_A])                 #17 This is a reason we keep N and Z in separate bytes
st([v6502_Qz])                  #18 Z flag
ld([v6502_P])                   #19
anda(~v6502_Vemu)               #20
st([v6502_P])                   #21
ld([Y,X])                       #22
adda(AC)                        #23
anda(v6502_Vemu)                #24
ora([v6502_P])                  #25
st([v6502_P])                   #26 Update V
ld(hi('v6502_next'),Y)          #27
jmp(Y,'v6502_next')             #28
ld(-30/2)                       #29

label('v6502_rts')
ld([v6502_S])                   #12
ld(AC,X)                        #13
adda(2)                         #14
st([v6502_S])                   #15
ld(0,Y)                         #16
ld([Y,X])                       #17
st([Y,Xpp])                     #18 Just X++
adda(1)                         #19
st([v6502_PCL])                 #20
beq(pc()+3)                     #21
bra(pc()+3)                     #22
ld(0)                           #23
ld(1)                           #23(!)
adda([Y,X])                     #24
st([v6502_PCH])                 #25
nop()                           #26
ld(hi('v6502_next'),Y)          #27
jmp(Y,'v6502_next')             #28
ld(-30/2)                       #29

label('v6502_php')
ld([v6502_S])                   #12
suba(1)                         #13
st([v6502_S],X)                 #14
ld([v6502_P])                   #15
anda(~v6502_Vflag&~v6502_Zflag) #16 Keep Vemu,B,D,I,C
bpl(pc()+3)                     #17 V to bit 6 and clear N
bra(pc()+2)                     #18
xora(v6502_Vflag^v6502_Vemu)    #19
st([X])                         #19,20
ld([v6502_Qz])                  #21 Z flag
beq(pc()+3)                     #22
bra(pc()+3)                     #23
ld(0)                           #24
ld(v6502_Zflag)                 #24(!)
ora([X])                        #25
st([X])                         #26
ld([v6502_Qn])                  #27 N flag
anda(0x80)                      #28
ora([X])                        #29
ora(v6502_Uflag)                #30 Unused bit
st([X])                         #31
nop()                           #32
ld(hi('v6502_next'),Y)          #33
jmp(Y,'v6502_next')             #34
ld(-36/2)                       #35

label('v6502_cpx')
bra('.cmp14')                   #12
ld([v6502_X])                   #13

label('v6502_cpy')
bra('.cmp14')                   #12
label('v6502_cmp')
ld([v6502_Y])                   #13,12
#
#label('v6502_cmp')             #12 Overlap
assert v6502_Cflag == 1
ld([v6502_A])                   #13
label('.cmp14')
ld([v6502_ADH],Y)               #14
bmi('.cmp17')                   #15 Carry?
suba([Y,X])                     #16
st([v6502_Qz])                  #17 Z flag
st([v6502_Qn])                  #18 N flag
bra('.cmp21')                   #19
ora([Y,X])                      #20
label('.cmp17')
st([v6502_Qz])                  #17 Z flag
st([v6502_Qn])                  #18 N flag
anda([Y,X])                     #19
nop()                           #20
label('.cmp21')
xora(0x80)                      #21
anda(0x80,X)                    #22 Move carry to bit 0
ld([v6502_P])                   #23 C flag
anda(~1)                        #24
ora([X])                        #25
st([v6502_P])                   #26
ld(hi('v6502_next'),Y)          #27
jmp(Y,'v6502_next')             #28
ld(-30/2)                       #29

label('v6502_plp')
assert v6502_Nflag == 128
assert 2*v6502_Vflag == v6502_Vemu
ld([v6502_S])                   #12
ld(AC,X)                        #13
adda(1)                         #14
st([v6502_S])                   #15
ld([X])                         #16
st([v6502_Qn])                  #17 N flag
anda(v6502_Zflag)               #18
xora(v6502_Zflag)               #19
st([v6502_Qz])                  #20 Z flag
ld([X])                         #21
anda(~v6502_Vemu)               #22 V to bit 7
adda(v6502_Vflag)               #23
st([v6502_P])                   #24 All other flags
ld(hi('v6502_next'),Y)          #25
jmp(Y,'v6502_next')             #26
ld(-28/2)                       #27

label('v6502_rti')
ld([v6502_S])                   #12
ld(AC,X)                        #13
adda(3)                         #14
st([v6502_S])                   #15
ld([X])                         #16
st([v6502_Qn])                  #17 N flag
anda(v6502_Zflag)               #18
xora(v6502_Zflag)               #19
st([v6502_Qz])                  #20 Z flag
ld(0,Y)                         #21
ld([Y,X])                       #22
st([Y,Xpp])                     #23 Just X++
anda(~v6502_Vemu)               #24 V to bit 7
adda(v6502_Vflag)               #25
st([v6502_P])                   #26 All other flags
ld([Y,X])                       #27
st([Y,Xpp])                     #28 Just X++
st([v6502_PCL])                 #29
ld([Y,X])                       #30
st([v6502_PCH])                 #31
nop()                           #32
ld(hi('v6502_next'),Y)          #33
jmp(Y,'v6502_next')             #34
ld(-36/2)                       #35

#-----------------------------------------------------------------------
#       Extended vertical blank logic: interrupts
#-----------------------------------------------------------------------
align(0x100)

# Check if an IRQ handler is defined
label('vBlankFirst#78')
ld([Y,vIRQ_v5])                 #78
ora([Y,vIRQ_v5+1])              #79
bne('vBlankFirst#82')           #80
ld([vPC])                       #81
runVcpu(186-82-extra,           #82 Application cycles (scan line 0)
    '---D line 0 timeout but no irq',
    returnTo='vBlankFirst#186')

label('vBlankFirst#82')
st([vIrqSave])                  #82 Save vPC
ld([vPC+1])                     #83
st([vIrqSave+1])                #84
ld([vAC])                       #85 Save vAC
st([vIrqSave+2])                #86
ld([vAC+1])                     #87
st([vIrqSave+3])                #88
ld([vCpuSelect])                #89 Save vCpuSelect for PREFIX
st([vIrqSave+4])                #90
ld([Y,vIRQ_v5])                 #91 Set vPC to vIRQ
suba(2)                         #92
st([vPC])                       #93
ld([Y,vIRQ_v5+1])               #94
st([vPC+1])                     #95
ld([vCpuSelect])                #96 Handler must save this if needed
st([vAC+1])                     #97
ld(0)                           #98
st([vAC])                       #99
ld(hi('ENTER'))                 #100 Set vCpuSelect to ENTER (=regular vCPU)
st([vCpuSelect])                #101
runVcpu(186-102-extra,          #102 Application cycles (scan line 0)
    '---D line 0 timeout with irq',
    returnTo='vBlankFirst#186')

# vIRQ sequence WITH interpreter switch
label('vRTI#18')
ld([X])                         #18
st([vCpuSelect])                #19
ld([0x30])                      #20
st([vPC])                       #21
ld([0x31])                      #22
st([vPC+1])                     #23
ld([0x32])                      #24
st([vAC])                       #25
ld([0x33])                      #26
st([vAC+1])                     #27
nop()                           #28
nop()                           #29
nop()                           #30 #0 This MUST match maxTicks, (ie maxTicks=30)
ld(hi('RESYNC'),Y)              #1
jmp(Y,'RESYNC')                 #2
ld([vTicks])                    #3

# Entered last line of vertical blank (line 40)
label('vBlankLast#34')

#-----------------------------------------------------------------------
#       Extended vertical blank logic: game controller decoding
#-----------------------------------------------------------------------

# Game controller types
# TypeA: Based on 74LS165 shift register (not supported)
# TypeB: Based on CD4021B shift register (standard)
# TypeC: Based on priority encoder
#
# Notes:
# - TypeA was only used during development and first beta test, before ROM v1
# - TypeB appears as type A with negative logic levels
# - TypeB is the game controller type that comes with the original kit and ROM v1
# - TypeB is mimicked by BabelFish / Pluggy McPlugface
# - TypeB requires a prolonged /SER_LATCH, therefore vPulse is 8 scanlines, not 2
# - TypeB and TypeC can be sampled in the same scanline
# - TypeA is 1 scanline shifted as it looks at a different edge (XXX up or down?)
# - TypeC gives incomplete information: lower buttons overshadow higher ones
#
#       TypeC    Alias    Button TypeB
#       00000000  ^@   -> Right  11111110
#       00000001  ^A   -> Left   11111101
#       00000011  ^C   -> Down   11111011
#       00000111  ^G   -> Up     11110111
#       00001111  ^O   -> Start  11101111
#       00011111  ^_   -> Select 11011111
#       00111111  ?    -> B      10111111
#       01111111  DEL  -> A      01111111
#       11111111       -> (None) 11111111
#
#       Conversion formula:
#               f(x) := 254 - x

# Detect controller TypeC codes
ld([serialRaw])                 #34 if serialRaw in [0,1,3,7,15,31,63,127,255]
adda(1)                         #35
anda([serialRaw])               #36
bne('.buttons#39')              #37

# TypeC
ld([serialRaw])                 #38 [TypeC] if serialRaw < serialLast
adda(1)                         #39
anda([serialLast])              #40
bne('.buttons#43')              #41
ld(254)                         #42 then clear the selected bit
nop()                           #43
bra('.buttons#46')              #44
label('.buttons#43')
suba([serialRaw])               #43,45
anda([buttonState])             #44
st([buttonState])               #45
label('.buttons#46')
ld([serialRaw])                 #46 Set the lower bits
ora([buttonState])              #47
label('.buttons#48')
st([buttonState])               #48
ld([serialRaw])                 #49 Update serialLast for next pass
jmp(Y,'vBlankLast#52')          #50
st([serialLast])                #51

# TypeB
# pChange = pNew & ~pOld
# nChange = nNew | ~nOld {DeMorgan}
label('.buttons#39')
ld(255)                         #39 [TypeB] Bitwise edge-filter to detect button presses
xora([serialLast])              #40
ora([serialRaw])                #41 Catch button-press events
anda([buttonState])             #42 Keep active button presses
ora([serialRaw])                #43
nop()                           #44
nop()                           #45
bra('.buttons#48')              #46
nop()                           #47


#-----------------------------------------------------------------------
#       More SYS functions, (0x1200)
#-----------------------------------------------------------------------

# SYS_Exec_88 implementation
label('sys_Exec')
st([vPC+1],Y)                   #18 Clear vPCH and Y
ld([vSP])                       #19 Place ROM loader below current stack pointer
suba(53+2)                      #20 (AC -> *+0) One extra word for PUSH
st([vTmp],X)                    #21
adda(-2)                        #22 (AC -> *-2)
st([vPC])                       #23
# Start of manually compiled vCPU section
st('PUSH',    [Y,Xpp])          #24 *+0
st('CALL',    [Y,Xpp])          #25 *+26 Fetch first byte
adda(33-(-2))                   #26 (AC -> *+33)
st(           [Y,Xpp])          #27 *+27
st('ST',      [Y,Xpp])          #28 *+3 Chunk copy loop
st(sysArgs+3, [Y,Xpp])          #29 *+4 High-address comes first
st('CALL',    [Y,Xpp])          #30 *+5
st(           [Y,Xpp])          #31 *+6
st('ST',      [Y,Xpp])          #32 *+7
st(sysArgs+2, [Y,Xpp])          #33 *+8 Then the low address
st('CALL',    [Y,Xpp])          #34 *+9
st(           [Y,Xpp])          #35 *+10
st('ST',      [Y,Xpp])          #36 *+11 Byte copy loop
st(sysArgs+4, [Y,Xpp])          #37 *+12 Byte count (0 means 256)
st('CALL',    [Y,Xpp])          #38 *+13
st(           [Y,Xpp])          #39 *+14
st('POKE',    [Y,Xpp])          #40 *+15
st(sysArgs+2, [Y,Xpp])          #41 *+16
st('INC',     [Y,Xpp])          #42 *+17
st(sysArgs+2, [Y,Xpp])          #43 *+18
st('LD',      [Y,Xpp])          #44 *+19
st(sysArgs+4, [Y,Xpp])          #45 *+20
st('SUBI',    [Y,Xpp])          #46 *+21
st(1,         [Y,Xpp])          #47 *+22
st('BCC',     [Y,Xpp])          #48 *+23
st('NE',      [Y,Xpp])          #49 *+24
adda(11-2-33)                   #50 (AC -> *+9)
st(           [Y,Xpp])          #51 *+25
st('CALL',    [Y,Xpp])          #52 *+26 Go to next block
adda(33-9)                      #53 (AC -> *+33)
st(           [Y,Xpp])          #54 *+27
st('BCC',     [Y,Xpp])          #55 *+28
st('NE',      [Y,Xpp])          #56 *+29
adda(3-2-33)                    #57 (AC -> *+1)
st(           [Y,Xpp])          #58 *+30
st('POP',     [Y,Xpp])          #59 *+31 End
st('RET',     [Y,Xpp])          #60 *+32
# Pointer constant pointing to the routine below (for use by CALL)
adda(35-1)                      #61 (AC -> *+35)
st(           [Y,Xpp])          #62 *+33
st(0,         [Y,Xpp])          #63 *+34
# Routine to read next byte from ROM and advance read pointer
st('LD',      [Y,Xpp])          #64 *+35 Test for end of ROM table
st(sysArgs+0, [Y,Xpp])          #65 *+36
st('XORI',    [Y,Xpp])          #66 *+37
st(251,       [Y,Xpp])          #67 *+38
st('BCC',     [Y,Xpp])          #68 *+39
st('NE',      [Y,Xpp])          #69 *+40
adda(46-2-35)                   #70 (AC -> *+44)
st(           [Y,Xpp])          #71 *+41
st('ST',      [Y,Xpp])          #72 *+42 Wrap to next ROM page
st(sysArgs+0, [Y,Xpp])          #73 *+43
st('INC',     [Y,Xpp])          #74 *+44
st(sysArgs+1, [Y,Xpp])          #75 *+45
st('LDW',     [Y,Xpp])          #76 *+46 Read next byte from ROM table
st(sysArgs+0, [Y,Xpp])          #77 *+47
st('LUP',     [Y,Xpp])          #78 *+48
st(0,         [Y,Xpp])          #79 *+49
st('INC',     [Y,Xpp])          #80 *+50 Increment read pointer
st(sysArgs+0, [Y,Xpp])          #81 *+51
st('RET',     [Y,Xpp])          #82 *+52 Return
# Return to interpreter
ld(hi('REENTER'),Y)             #83
jmp(Y,'REENTER')                #84
ld(-88/2)                       #85

# SYS_VDrawBits_134 implementation
label('sys_VDrawBits')
ld(0)                           #18
label('.sysVdb0')
st([vTmp])                      #19+i*25
adda([sysArgs+5],Y)             #20+i*25 Y=[sysPos+1]+[vTmp]
ld([sysArgs+2])                 #21+i*25 Select color
bmi(pc()+3)                     #22+i*25
bra(pc()+3)                     #23+i*25
ld([sysArgs+0])                 #24+i*25
ld([sysArgs+1])                 #24+i*25(!)
st([Y,X])                       #25+i*25 Draw pixel
ld([sysArgs+2])                 #26+i*25 Shift byte left
adda(AC)                        #27+i*25
st([sysArgs+2])                 #28+i*25
ld([vTmp])                      #29+i*25 Unrolled loop (once)
adda([sysArgs+5])               #31+i*25
adda(1,Y)                       #30+i*25 Y=[sysPos+1]+[vTmp]+1
ld([sysArgs+2])                 #32+i*25 Select color
bmi(pc()+3)                     #33+i*25
bra(pc()+3)                     #34+i*25
ld([sysArgs+0])                 #35+i*25
ld([sysArgs+1])                 #35+i*25(!)
st([Y,X])                       #36+i*25 Draw pixel
ld([sysArgs+2])                 #37+i*25 Shift byte left
adda(AC)                        #38+i*25
st([sysArgs+2])                 #39+i*25
ld([vTmp])                      #40+i*25 Loop counter
suba(6)                         #41+i*25
bne('.sysVdb0')                 #42+i*25
adda(8)                         #43+i*25 Steps of 2
ld(hi('REENTER'),Y)             #119
jmp(Y,'REENTER')                #120
ld(-124/2)                      #121

# SYS_ResetWaveforms_v4_50 implementation
label('sys_ResetWaveforms')
ld([vAC+0])                     #18 X=4i
adda(AC)                        #19
adda(AC,X)                      #20
ld([vAC+0])                     #21
st([Y,Xpp])                     #22 Sawtooth: T[4i+0] = i
anda(0x20)                      #23 Triangle: T[4i+1] = 2i if i<32 else 127-2i
bne(pc()+3)                     #24
ld([vAC+0])                     #25
bra(pc()+3)                     #26
adda([vAC+0])                   #26,27
xora(127)                       #27
st([Y,Xpp])                     #28
ld([vAC+0])                     #29 Pulse: T[4i+2] = 0 if i<32 else 63
anda(0x20)                      #30
bne(pc()+3)                     #31
bra(pc()+3)                     #32
ld(0)                           #33
ld(63)                          #33(!)
st([Y,Xpp])                     #34
ld([vAC+0])                     #35 Sawtooth: T[4i+3] = i
st([Y,X])                       #36
adda(1)                         #37 i += 1
st([vAC+0])                     #38
xora(64)                        #39 For 64 iterations
beq(pc()+3)                     #40
bra(pc()+3)                     #41
ld(-2)                          #42
ld(0)                           #42(!)
adda([vPC])                     #43
st([vPC])                       #44
ld(hi('REENTER'),Y)             #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47

# SYS_ShuffleNoise_v4_46 implementation
label('sys_ShuffleNoise')
ld([vAC+0],X)                   #18 tmp = T[4j]
ld([Y,X])                       #19
st([vTmp])                      #20
ld([vAC+1],X)                   #21 T[4j] = T[4i]
ld([Y,X])                       #22
ld([vAC+0],X)                   #23
st([Y,X])                       #24
adda(AC)                        #25 j += T[4i]
adda(AC,)                       #26
adda([vAC+0])                   #27
st([vAC+0])                     #28
ld([vAC+1],X)                   #29 T[4i] = tmp
ld([vTmp])                      #30
st([Y,X])                       #31
ld([vAC+1])                     #32 i += 1
adda(4)                         #33
st([vAC+1])                     #34
beq(pc()+3)                     #35 For 64 iterations
bra(pc()+3)                     #36
ld(-2)                          #37
ld(0)                           #37(!)
adda([vPC])                     #38
st([vPC])                       #39
ld(hi('NEXTY'),Y)               #40
jmp(Y,'NEXTY')                  #41
ld(-44/2)                       #42


fillers(until=0xff)
align(0x100, size=0x100)


#-----------------------------------------------------------------------
#       More SYS functions, (0x1300)
#-----------------------------------------------------------------------

# sys_Multiply_u8, res:u16 = x:u8 * y:u8
# sysArgs0,1=res, sysArgs2=x, sysArgs3=y, sysArgs4=count
label('SYS_Multiply_u8_vX_48')
ld([sysArgs+4])                 #15,
suba(1)                         #16, count--
bge('.sys_multiply_u8_19')      #17,
st([sysArgs+4])                 #18,
ld(hi('REENTER'),Y)             #19,
jmp(Y,'REENTER')                #20,
ld(-24/2)                       #21,

label('.sys_multiply_u8_19')
ld([sysArgs+0])                 #19,
anda(128,X)                     #20,
adda([sysArgs+0])               #21,
st([sysArgs+0])                 #22,
ld([X])                         #23,
adda([sysArgs+1])               #24,
adda([sysArgs+1])               #25,
st([sysArgs+1])                 #26, result <<1
ld([sysArgs+2])                 #27,
bmi('.sys_multiply_u8_30')      #28, if(x & 0x80)
adda([sysArgs+2])               #29,
st([sysArgs+2])                 #30, x <<= 1
ld([vPC])                       #31,
suba(2)                         #32,
st([vPC])                       #33, restart
ld(hi('NEXTY'),Y)               #34,
jmp(Y,'NEXTY')                  #35,
ld(-38/2)                       #36,

label('.sys_multiply_u8_30')
st([sysArgs+2])                 #30, x <<= 1
ld([sysArgs+0])                 #31,
adda([sysArgs+3])               #32,
st([sysArgs+0])                 #33, res.lo += y
bmi('.sys_multiply_u8_36')      #34, check carry
suba([sysArgs+3])               #35, initial value of res.lo
ora([sysArgs+3])                #36, carry in bit 7
anda(0x80,X)                    #37, move carry to bit 0
ld([X])                         #38,
adda([sysArgs+1])               #39, 
st([sysArgs+1])                 #40, res.hi += carry
ld([vPC])                       #41,
suba(2)                         #42,
st([vPC])                       #43, restart
ld(hi('NEXTY'),Y)               #44,
jmp(Y,'NEXTY')                  #45,
ld(-48/2)                       #46,

label('.sys_multiply_u8_36')
anda([sysArgs+3])               #36, carry in bit 7
anda(0x80,X)                    #37, move carry to bit 0
ld([X])                         #38,
adda([sysArgs+1])               #39, 
st([sysArgs+1])                 #40, res.hi += carry
ld([vPC])                       #41,
suba(2)                         #42,
st([vPC])                       #43, restart
ld(hi('NEXTY'),Y)               #44,
jmp(Y,'NEXTY')                  #45,
ld(-48/2)                       #46,

# leave some room for updates
fillers(until=0x40)


# sys_OffsetVTableY, sysArgs0 = scanline:8, sysArgs1 = scanCount:8, sysArgs2,3 = videoTable:16
label('SYS_OffsetVTableY_vX_36')
ld([sysArgs+0])                 #15, scanline
ld([sysArgs+2],X)               #16,
ld([sysArgs+3],Y)               #17,
st([Y,X])                       #18, [videoTable] = scanline
adda(1)                         #19,
st([sysArgs+0])                 #20, scanline++
ld([sysArgs+2])                 #21,
adda(2)                         #22,
st([sysArgs+2])                 #23, videoTable += 2
ld([sysArgs+1])                 #24,
suba(1)                         #25,
beq('.sys_offsetvty_28')        #26,
st([sysArgs+1])                 #27, scanCount--
ld([vPC])                       #28,
suba(2)                         #29,
st([vPC])                       #30, restart
ld(hi('REENTER'),Y)             #31,
jmp(Y,'REENTER')                #32,
ld(-36/2)                       #33,
label('.sys_offsetvty_28')
ld(hi('NEXTY'),Y)               #28,
jmp(Y,'NEXTY')                  #29,
ld(-32/2)                       #30,

# leave some room for updates
fillers(until=0x60)


# sys_FillByteSeq, sysArgs0,1=dst, sysArg2=start, sysArg3=step, sysArg4=count
label('SYS_FillByteSeq_vX_36')
ld([sysArgs+1],Y)               #15 dst pointer
ld([sysArgs+0],X)               #16,
ld([sysArgs+0])                 #17,
adda(1)                         #18,
st([sysArgs+0])                 #19, dst++
ld([sysArgs+2])                 #20,
st([Y,X])                       #21, start
adda([sysArgs+3])               #22,
st([sysArgs+2])                 #23, start += step
ld([sysArgs+4])                 #24,
suba(1)                         #25,
beq('.sys_fillbyteseq_28')      #26,
st([sysArgs+4])                 #27,
ld([vPC])                       #28,
suba(2)                         #29,
st([vPC])                       #30, restart
ld(hi('REENTER'),Y)             #31,
jmp(Y,'REENTER')                #32,
ld(-36/2)                       #33,
label('.sys_fillbyteseq_28')
ld(hi('NEXTY'),Y)               #28,
jmp(Y,'NEXTY')                  #29,
ld(-32/2)                       #30,

# leave some room for updates
fillers(until=0x80)


# sys_AddInt8Array, sysArgs0,1=src, sysArgs2,3=dst, sysArgs4=index, sysArgs5=count, dst(i) += src(i)
label('SYS_AddInt8Array_vX_40')
ld([sysArgs+0])                 #15,
adda([sysArgs+4],X)             #16,
ld([sysArgs+1],Y)               #17,
ld([Y,X])                       #18,
st([vTmp])                      #19
ld([sysArgs+2])                 #20,
adda([sysArgs+4],X)             #21,
ld([sysArgs+3],Y)               #22,
ld([vTmp])                      #23
adda([Y,X])                     #24,
st([Y,X])                       #25,
ld([sysArgs+5])                 #26,
suba(1)                         #27,
beq('.sys_addint8Array_30')     #28,
st([sysArgs+5])                 #29,
ld([sysArgs+4])                 #30,
adda(1)                         #31,
st([sysArgs+4])                 #32, index++
ld([vPC])                       #33,
suba(2)                         #34,
st([vPC])                       #35,
ld(hi('NEXTY'),Y)               #36,
jmp(Y,'NEXTY')                  #37,
ld(-40/2)                       #38,
label('.sys_addint8Array_30')
ld(hi('NEXTY'),Y)               #30,
jmp(Y,'NEXTY')                  #31,
ld(-34/2)                       #32,

# leave some room for updates
fillers(until=0xa0)


# sys_ParityFill, sysArgs0,1=borderColour:fillColour, sysArgs2,3=X:Y start address
#                 sysArgs4=countXY, sysArgs6=fill toggle, sysArgs7=replace colour
label('SYS_ParityFill_vX_44')
ld([sysArgs+3],Y)               #15,
ld([sysArgs+2],X)               #16,
ld([sysArgs+2])                 #17,
adda(1)                         #18,
st([sysArgs+2])                 #19, x++
ld([sysArgs+4])                 #20,
suba(1)                         #21, countX--
bge('.sys_parityfill_24')       #22,
st([sysArgs+4])                 #23,
ld([sysArgs+3])                 #24,
adda(1)                         #25,
st([sysArgs+3])                 #26, y++
ld([sysArgs+5])                 #27,
suba(1)                         #28, countY--
st([sysArgs+5])                 #29,
bne('.sys_parityfill_32')       #30,
ld(hi('REENTER'),Y)             #31,
jmp(Y,'REENTER')                #32,
ld(-36/2)                       #33,
label('.sys_parityfill_32')
ld([0x82])                      #32,
st([sysArgs+2])                 #33, restore x 
ld([0x83])                      #34,
st([sysArgs+4])                 #35, restore countX
ld([0x84])                      #36,
st([sysArgs+6])                 #37, restore fill toggle
ld([vPC])                       #38,
suba(2)                         #39,
st([vPC])                       #40, restart
jmp(Y,'NEXTY')                  #41,
ld(-44/2)                       #42,

label('.sys_parityfill_24')
ld([sysArgs+6])                 #24, check fill toggle
bne('.sys_parityfill_27')       #25,
ld([Y,X])                       #26, get pixel
xora([sysArgs+0])               #27, check border colour
beq('.sys_parityfill_30')       #28,
ld([vPC])                       #29,
suba(2)                         #30,
st([vPC])                       #31, restart
ld(hi('NEXTY'),Y)               #32,
jmp(Y,'NEXTY')                  #33,
ld(-36/2)                       #34,

label('.sys_parityfill_27')
xora([sysArgs+0])               #27, check border colour
beq('.sys_parityfill_30')       #28,
ld([Y,X])                       #29, get pixel
xora([sysArgs+7])               #30, check replace colour
bne(pc()+3)                     #31, ignore replace colour
bra(pc()+3)                     #32,
ld([sysArgs+1])                 #33, fill colour
ld([Y,X])                       #33! get pixel
st([Y,X])                       #34, set pixel
ld([vPC])                       #35,
suba(2)                         #36,
st([vPC])                       #37, restart
ld(hi('NEXTY'),Y)               #38,
jmp(Y,'NEXTY')                  #39,
ld(-42/2)                       #40,

label('.sys_parityfill_30')
ld([sysArgs+6])                 #30,
xora(0x01)                      #31,
st([sysArgs+6])                 #32, toggle fill
ld([vPC])                       #33,
suba(2)                         #34,
st([vPC])                       #35, restart
ld(hi('NEXTY'),Y)               #36,
jmp(Y,'NEXTY')                  #37,
ld(-40/2)                       #38,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x1400)
#-----------------------------------------------------------------------

# JGT implementation
label('jgt#13')
ld([vAC+1])                     #13
blt('.jgt#16')                  #14 if vAC.hi < 0
ora([vAC])                      #15
bne('.jgt#18')                  #16 if (vAC.hi OR vAC.lo) != 0
ld([vPC])                       #17
adda(1)                         #18
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.jgt#16')
ld([vPC])                       #16
adda(1)                         #17
st([vPC])                       #18
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21
label('.jgt#18')
ld([vPC+1],Y)                   #18 vPC.hi
ld([Y,X])                       #19 address of branch destination
st([vPC])                       #20
st([Y,Xpp])                     #21
ld([Y,X])                       #22 hi address of jump destination
st([vPC+1])                     #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# JLE implementation
label('jle#13')
ld([vAC+1])                     #13
blt('.jle#16')                  #14 if vAC.hi < 0
ora([vAC])                      #15
bne('.jle#18')                  #16 if (vAC.hi OR vAC.lo) != 0
ld([vPC])                       #17
ld([vPC+1],Y)                   #18 vPC.hi
ld([Y,X])                       #19 address of branch destination
st([vPC])                       #20
st([Y,Xpp])                     #21
ld([Y,X])                       #22 hi address of jump destination
st([vPC+1])                     #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26
label('.jle#16')
ld([vPC+1],Y)                   #16 vPC.hi
ld([Y,X])                       #17 address of branch destination
st([vPC])                       #18
st([Y,Xpp])                     #19
ld([Y,X])                       #20 hi address of jump destination
st([vPC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.jle#18')
adda(1)                         #18
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# JGE implementation
label('jge#13')
ld([vAC+1])                     #13
blt('.jge#16')                  #14 if vAC.hi < 0
ld([vPC])                       #15
ld([vPC+1],Y)                   #16 vPC.hi
ld([Y,X])                       #17 address of branch destination
st([vPC])                       #18
st([Y,Xpp])                     #19
ld([Y,X])                       #20 hi address of jump destination
st([vPC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.jge#16')
adda(1)                         #16
st([vPC])                       #17
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# PEEKA+ implementation
label('peeka+#13')
st([vTmp])                      #13
ld([vAC+1],Y)                   #14
ld([vAC])                       #15
ld(AC,X)                        #16
adda(1)                         #17
st([vAC])                       #18
beq('.peeka+#21')               #19 if low byte is 0x00
ld([Y,X])                       #20 peek [vAC]
ld([vTmp],X)                    #21 dst var
st([X])                         #22 dst = peek [vAC]
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.peeka+#21')
ld([vTmp],X)                    #21 dst var
st([X])                         #22 dst = peek [vAC]
ld([vAC+1])                     #23
adda(1)                         #24
st([vAC+1])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# ADDVW implementation
label('addvw#13')
ld([vPC+1],Y)                   #13 vPC.hi
st([sysArgs+6])                 #14 src var addr
st([Y,Xpp])                     #15 X++
ld(min(0,maxTicks-54/2))        #16
adda([vTicks])                  #17
blt('.addvw#20')                #18 not enough time left, so retry
ld([Y,X])                       #19
st([sysArgs+7])                 #20 dst var addr
ld(0,Y)                         #21
ld([sysArgs+6],X)               #22
ld([X])                         #23 src low value
st([vAC])                       #24
st([Y,Xpp])                     #25 X++
ld([X])                         #26 src hi value
st([vAC+1])                     #27
ld([sysArgs+7],X)               #28
ld([vAC])                       #29 Low byte
st([vTmp])                      #30
adda([X])                       #31
st([vAC])                       #32 vAC.lo = low result
bmi('.addvw#35')                #33 Now figure out if there was a carry
ld([X])                         #34
st([Y,Xpp])                     #35
ora([vTmp])                     #36
bmi('.addvw#39')                #37 If Carry == 1
ld([X])                         #38
adda([vAC+1])                   #39
st([vAC+1])                     #40 vAC.hi = high result
st([X])                         #41 dst.hi = high result
ld([sysArgs+7],X)               #42
ld([vAC])                       #43
st([X])                         #44 dst.lo = low result
ld([vPC])                       #45
adda(1)                         #46
st([vPC])                       #47
ld(hi('NEXTY'),Y)               #48
jmp(Y,'NEXTY')                  #49
ld(-52/2)                       #50

label('.addvw#35')
st([Y,Xpp])                     #35
anda([vTmp])                    #36
bmi('.addvw#39')                #37 If Carry == 1
ld([X])                         #38
adda([vAC+1])                   #39
st([vAC+1])                     #40 vAC.hi = high result
st([X])                         #41 dst.hi = high result
ld([sysArgs+7],X)               #42
ld([vAC])                       #43
st([X])                         #44 dst.lo = low result
ld([vPC])                       #45
adda(1)                         #46
st([vPC])                       #47
ld(hi('NEXTY'),Y)               #48
jmp(Y,'NEXTY')                  #49
ld(-52/2)                       #50

label('.addvw#39')
adda(1)                         #39
adda([vAC+1])                   #40
st([vAC+1])                     #41 vAC.hi = high result
st([X])                         #42 dst.hi = high result
ld([sysArgs+7],X)               #43
ld([vAC])                       #44
st([X])                         #45 dst.lo = low result
ld([vPC])                       #46
adda(1)                         #47
st([vPC])                       #48
ld(hi('REENTER'),Y)             #49
jmp(Y,'REENTER')                #50
ld(-54/2)                       #51

label('.addvw#20')
ld([vPC])                       #20 retry instruction
suba(2)                         #21
st([vPC])                       #22
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25


# SUBVW implementation
label('subvw#13')
ld([vPC+1],Y)                   #13 vPC.hi
st([sysArgs+6])                 #14 src var addr
st([Y,Xpp])                     #15 X++
adda(1)                         #16
st([vTmp])                      #17 src var high addr
ld(min(0,maxTicks-54/2))        #18
adda([vTicks])                  #19
blt('.subvw#22')                #20 not enough time left, so retry
ld([Y,X])                       #21
st([sysArgs+7])                 #22 dst var addr
ld(0,Y)                         #23
ld([sysArgs+7],X)               #24
ld([X])                         #25 dst low value
st([vAC])                       #26
st([Y,Xpp])                     #27 X++
ld([X])                         #28 dst hi value
st([vAC+1])                     #29
ld([sysArgs+6],X)               #30
ld([vAC])                       #31 Low byte
bmi('.subvw#34')                #32
suba([X])                       #33
st([vAC])                       #34 Store low result
ora([X])                        #35 Carry in bit 7
anda(0x80,X)                    #36 Move carry to bit 0
ld([vAC+1])                     #37
suba([X])                       #38
ld([vTmp],X)                    #39
suba([X])                       #40
st([vAC+1])                     #41
ld([sysArgs+7],X)               #42
ld([vAC])                       #43
st([Y,Xpp])                     #44
ld([vAC+1])                     #45
st([X])                         #46
ld([vPC])                       #47
adda(1)                         #48
st([vPC])                       #49
ld(hi('NEXTY'),Y)               #50
jmp(Y,'NEXTY')                  #51
ld(-54/2)                       #52

label('.subvw#34')
st([vAC])                       #34 Store low result
anda([X])                       #35 Carry in bit 7
anda(0x80,X)                    #36 Move carry to bit 0
ld([vAC+1])                     #37
suba([X])                       #38
ld([vTmp],X)                    #39
suba([X])                       #40
st([vAC+1])                     #41
ld([sysArgs+7],X)               #42
ld([vAC])                       #43
st([Y,Xpp])                     #44
ld([vAC+1])                     #45
st([X])                         #46
ld([vPC])                       #47
adda(1)                         #48
st([vPC])                       #49
ld(hi('NEXTY'),Y)               #50
jmp(Y,'NEXTY')                  #51
ld(-54/2)                       #52

label('.subvw#22')
ld([vPC])                       #22 retry instruction
suba(2)                         #23
st([vPC])                       #24
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27


# PREFX3 implementation
# Original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2099#p2099
label('prefx3#13')
ld([vPC])                       #13
adda(2)                         #14
st([vPC])                       #15 Advance vPC
ld(hi('PREFX3_PAGE'))           #16 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #17
adda(1,Y)                       #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# PREFX2 implementation
label('prefx2#13')
ld([vPC])                       #13
adda(1)                         #14
st([vPC])                       #15 Advance vPC
ld(hi('PREFX2_PAGE'))           #16 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #17
adda(1,Y)                       #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# PREFX1 implementation
label('prefx1#13')
st([vCpuSelect])                #13
adda(1,Y)                       #14
jmp(Y,'NEXTY')                  #15
ld(-18/2)                       #16


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x1500)
#-----------------------------------------------------------------------

# ADDI implementation
label('addi#13')
adda([vAC])                     #13
st([vAC])                       #14 Store low result
bmi('.addi#17')                 #15 Now figure out if there was a carry
suba([vTmp])                    #16 Gets back the initial value of vAC
ora([vTmp])                     #17 Carry in bit 7
anda(0x80,X)                    #18 Move carry to bit 0
ld([X])                         #19
adda([vAC+1])                   #20 Add the high bytes with carry
st([vAC+1])                     #21 Store high result
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.addi#17')
anda([vTmp])                    #17 Carry in bit 7
anda(0x80,X)                    #18 Move carry to bit 0
ld([X])                         #19
adda([vAC+1])                   #20 Add the high bytes with carry
st([vAC+1])                     #21 Store high result
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# SUBI implementation
label('subi#13')
ld([vAC])                       #13
bmi('.subi#16')                 #14
suba([vTmp])                    #15
st([vAC])                       #16 Store low result
ora([vTmp])                     #17 Carry in bit 7
anda(0x80,X)                    #18 Move carry to bit 0
ld([vAC+1])                     #19
suba([X])                       #20
st([vAC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.subi#16')
st([vAC])                       #16 Store low result
anda([vTmp])                    #17 Carry in bit 7
anda(0x80,X)                    #18 Move carry to bit 0
ld([vAC+1])                     #19
suba([X])                       #20
st([vAC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# XORW implementation
label('xorw#13')
ld(0,Y)                         #13
ld([X])                         #14
st([Y,Xpp])                     #15
xora([vAC])                     #16
st([vAC])                       #17
ld([X])                         #18
xora([vAC+1])                   #19
st([vAC+1])                     #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

# INCW implementation
label('incw#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 address of low byte to be added
ld([X])                         #15
adda(1)                         #16
beq('.incw#19')                 #17 if low byte is 0x00
st([Y,Xpp])                     #18 inc low byte
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21
label('.incw#19')
ld([X])                         #19
adda(1)                         #20
st([X])                         #21 inc high byte
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# DECW implementation
label('decw#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 address of low byte to be added
ld([X])                         #15
suba(1)                         #16
st([Y,Xpp])                     #17 dec low byte
xora(0xff)                      #18 if low byte is 0xff
beq('.decw#21')                 #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.decw#21')
ld([X])                         #21
suba(1)                         #22
st([X])                         #23 dec high byte
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# DBNE implementation
label('dbne#13')
st([vTmp])                      #13 branch offset
st([Y,Xpp])                     #14 X++, address of counter
ld([Y,X])                       #15
ld(AC,X)                        #16 fetch counter
ld([X])                         #17
suba(1)                         #18 decrement counter
beq('.dbne#21')                 #19 if zero BRA
st([X])                         #20
ld([vTmp])                      #21
st([vPC])                       #22 BRA to branch offset
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.dbne#21')
ld([vPC])                       #21
adda(1)                         #22
st([vPC])                       #23 advance vPC by 1
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26
 
# TEQ implementation
label('teq#13')
ld(AC,X)                        #13 address of var
ld([vAC+1])                     #14
ora([vAC])                      #15 
beq('.teq#18')                  #16 if (vAC.hi OR vAC.lo) == 0
ld(0,Y)                         #17
st(0,[Y,Xpp])                   #18 var.lo = 0
st(0,[Y,X])                     #19 var.hi = 0
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.teq#18')
st(1,[Y,Xpp])                   #18 var.lo = 1
st(0,[Y,X])                     #19 var.hi = 0
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# TNE implementation
label('tne#13')
ld(AC,X)                        #13 address of var
ld([vAC+1])                     #14
ora([vAC])                      #15
beq('.tne#18')                  #16 if (vAC.hi OR vAC.lo) == 0
ld(0,Y)                         #17
st(1,[Y,Xpp])                   #18 var.lo = 1
st(0,[Y,X])                     #19 var.hi = 0
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.tne#18')
st(0,[Y,Xpp])                   #18 var.lo = 0
st(0,[Y,X])                     #19 var.hi = 0
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# TGE implementation
label('tge#13')
ld(AC,X)                        #13 address of var
ld([vAC+1])                     #14
bmi('.tge#17')                  #15 if vAC.hi < 0
ld(0,Y)                         #16
st(1,[Y,Xpp])                   #17 var.lo = 1
st(0,[Y,X])                     #18 var.hi = 0
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21
label('.tge#17')
st(0,[Y,Xpp])                   #17 var.lo = 0
st(0,[Y,X])                     #18 var.hi = 0
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

# TLT implementation
label('tlt#13')
ld(AC,X)                        #13 address of var
ld([vAC+1])                     #14
bmi('.tlt#17')                  #15 if vAC.hi < 0
ld(0,Y)                         #16
st(0,[Y,Xpp])                   #17 var.lo = 0
st(0,[Y,X])                     #18 var.hi = 0
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21
label('.tlt#17')
st(1,[Y,Xpp])                   #17 var.lo = 1
st(0,[Y,X])                     #18 var.hi = 0
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

# TGT implementation
label('tgt#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 address of var
ld([vAC+1])                     #15
blt('.tgt#18')                  #16 if vAC.hi < 0
ora([vAC])                      #17
bne('.tgt#20')                  #18 if (vAC.hi OR vAC.lo) != 0
nop()                           #19
st(0,[Y,Xpp])                   #20 var.lo = 0
st(0,[Y,X])                     #21 var.hi = 0
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.tgt#18')
st(0,[Y,Xpp])                   #18 var.lo = 0
st(0,[Y,X])                     #19 var.hi = 0
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.tgt#20')
st(1,[Y,Xpp])                   #20 var.lo = 1
st(0,[Y,X])                     #21 var.hi = 0
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# TLE implementation
label('tle#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 address of var
ld([vAC+1])                     #15
blt('.tle#18')                  #16 if vAC.hi < 0
ora([vAC])                      #17
bne('.tle#20')                  #18 if (vAC.hi OR vAC.lo) != 0
nop()                           #19
st(1,[Y,Xpp])                   #20 var.lo = 1
st(0,[Y,X])                     #21 var.hi = 0
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.tle#18')
st(1,[Y,Xpp])                   #18 var.lo = 1
st(0,[Y,X])                     #19 var.hi = 0
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.tle#20')
st(0,[Y,Xpp])                   #20 var.lo = 0
st(0,[Y,X])                     #21 var.hi = 0
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# INCWA implementation
label('incwa#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 address of low byte to be added
ld([X])                         #15
adda(1)                         #16
st([Y,Xpp])                     #17 inc low byte
st([vAC])                       #18
beq('.incwa#21')                #19 if low byte is 0x00
ld([X])                         #20
st([vAC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.incwa#21')
adda(1)                         #21
st([X])                         #22 inc high byte
st([vAC+1])                     #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# DECWA implementation
label('decwa#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 address of low byte to be added
ld([X])                         #15
suba(1)                         #16
st([Y,Xpp])                     #17 dec low byte
st([vAC])                       #18
xora(0xff)                      #19 if low byte is 0xff
beq('.decwa#22')                #20
ld([X])                         #21
st([vAC+1])                     #22
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.decwa#22')
suba(1)                         #22
st([X])                         #23 dec high byte
st([vAC+1])                     #24
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x1600)
#-----------------------------------------------------------------------

# INC implementation
label('inc#13')
ld(AC,X)                        #13
ld([X])                         #14
adda(1)                         #15
st([X])                         #16
ld(hi('REENTER'),Y)             #17
jmp(Y,'REENTER')                #18
ld(-22/2)                       #19

# DEC implementation
label('dec#13')
ld(AC,X)                        #13
ld([X])                         #14
suba(1)                         #15
st([X])                         #16
ld(hi('REENTER'),Y)             #17
jmp(Y,'REENTER')                #18
ld(-22/2)                       #19

# LD implementation
label('ld#13')
ld(AC,X)                        #13
ld([X])                         #14
st([vAC])                       #15
ld(0)                           #16
st([vAC+1])                     #17
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# LDI implementation
label('ldi#13')
st([vAC])                       #13
ld(0)                           #14
st([vAC+1])                     #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# ST implementation
label('st#13')
ld(AC,X)                        #13
ld([vAC])                       #14
st([X])                         #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# ORI implementation
label('ori#13')
ora([vAC])                      #13
st([vAC])                       #14
ld(hi('REENTER'),Y)             #15
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17

# ALLOC implementation
label('alloc#13')
adda([vSP])                     #13
st([vSP])                       #14
ld(hi('REENTER'),Y)             #15
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17

# MOV implementation
label('mov#13')
ld([vPC+1],Y)                   #13
st([vTmp])                      #14 save address of dst var
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16 address of src var
ld(AC,X)                        #17 
ld([X])                         #18 get src
ld([vTmp],X)                    #19 address of dst var
st([X])                         #20 dst = src
ld([vPC])                       #21
adda(1)                         #22
st([vPC])                       #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# PEEKA implementation
label('peeka#13')
st([vTmp])                      #13
ld([vAC+1],Y)                   #14
ld([vAC],X)                     #15
ld([Y,X])                       #16 peek [vAC]
ld([vTmp],X)                    #17 dst var
st([X])                         #18 dst = peek [vAC]
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

# DEEKA implementation
label('deeka#13')
ld([vAC+1],Y)                   #13
ld([vAC],X)                     #14
ld([Y,X])                       #15 peek [vAC]
st([Y,Xpp])                     #16
st([sysArgs+6])                 #17
ld([Y,X])                       #18 peek [vAC+1]
st([sysArgs+7])                 #19
ld(0,Y)                         #20
ld([vTmp],X)                    #21 dst var
ld([sysArgs+6])                 #22
st([Y,Xpp])                     #23 [var.lo] = peek [vAC]
ld([sysArgs+7])                 #24
st([Y,X])                       #25 [var.hi] = peek [vAC+1]
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# PEEKA implementation
#label('peeka#13')
#ld(0,Y)                         #13
#ld(AC,X)                        #14 address of var
#ld([Y,X])                       #15
#st([vTmp])                      #16 low byte of src pointer
#st([Y,Xpp])                     #17 X++
#ld([Y,X])                       #18
#ld(AC,Y)                        #19 high byte of src pointer
#ld([vTmp],X)                    #20
#ld([Y,X])                       #21 peek src
#ld([vAC],X)                     #22 low byte of dst pointer
#ld([vAC+1],Y)                   #23 high byte of dst pointer
#st([Y,X])                       #24 poke dst
#ld(hi('REENTER'),Y)             #25
#jmp(Y,'REENTER')                #26
#ld(-30/2)                       #27

# SUBBI implementation, var -= imm, does NOT modify var.hi
label('subbi#13')
ld([vPC+1],Y)                   #13
st([vTmp])                      #14 immediate value
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16
ld(AC,X)                        #17 address of var
ld([X])                         #18
suba([vTmp])                    #19
st([X])                         #20
ld([vPC])                       #21
adda(1)                         #22
st([vPC])                       #23 advance vPC by 1
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# MOVQB implementation
label('movqb#13')
ld([vPC+1],Y)                   #13 vPC.hi
st([vTmp])                      #14 immediate value
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16
ld(AC,X)                        #17 address of var
ld([vTmp])                      #18
st([X])                         #19 var.lo = immediate value
ld([vPC])                       #20
adda(1)                         #21
st([vPC])                       #22 advance vPC by 1
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
 
# MOVQW implementation
label('movqw#13')
st([vTmp])                      #13 immediate value
st([Y,Xpp])                     #14 X++
ld([Y,X])                       #15
ld(AC,X)                        #16 address of var
ld([vTmp])                      #17
st([X])                         #18 var.lo = immediate value
ld(0,Y)                         #19
st([Y,Xpp])                     #20 X++
ld(0)                           #21
st([X])                         #22 var.hi = 0
ld([vPC])                       #23
adda(1)                         #24
st([vPC])                       #25 advance vPC by 1
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# LSRB implementation
label('lsrb#13')
ld(AC,X)                        #13 var
ld(hi('shiftTable'),Y)          #14 logical shift right 1 bit (X >> 1)
ld('.lsrb#24')                  #15 continuation address
st([vTmp])                      #16
ld([X])                         #17 load byte from var
anda(0xfe)                      #18
jmp(Y,AC)                       #19
bra(255)                        #20 bra shiftTable+255
#dummy
# continues in page 0x0600 at label('.lsrb#24') after fetching shifted byte from 0x0500
 
# PEEKV implementation
label('peekv#13')
ld(AC,X)                        #13
ld(0,Y)                         #14
ld([Y,X])                       #15 low byte peek address
st([Y,Xpp])                     #16 X++
st([vTmp])                      #17
ld([Y,X])                       #18 high byte peek address
ld(AC,Y)                        #19
ld([vTmp],X)                    #20
ld([Y,X])                       #21 peek byte
st([vAC])                       #22
ld(0)                           #23
st([vAC+1])                     #24
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27

# DEEKV implementation
label('deekv#13')
ld(AC,X)                        #13
ld(0,Y)                         #14
ld([Y,X])                       #15 low byte deek address
st([Y,Xpp])                     #16 X++
st([vTmp])                      #17
ld([Y,X])                       #18 high byte deek address
ld(AC,Y)                        #19
ld([vTmp],X)                    #20
ld([Y,X])                       #21 peek low byte
st([vAC])                       #22 
st([Y,Xpp])                     #23 X++
ld([Y,X])                       #24 peek high byte
st([vAC+1])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# LSLV implementation
label('lslv#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 var
ld([X])                         #15
bge('.lslv#18')                 #16
adda([X])                       #17
st([Y,Xpp])                     #18 var.lo <<1
ld([X])                         #19
adda([X])                       #20
ora(1)                          #21
st([X])                         #22 var.hi <<1 | 1
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.lslv#18')
st([Y,Xpp])                     #18
ld([X])                         #19
adda([X])                       #20
st([X])                         #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# LDW implementation
label('ldw#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 Operand
ld([X])                         #15
st([vAC])                       #16
st([Y,Xpp])                     #17 X++
ld([X])                         #18
st([vAC+1])                     #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# STW implementation
label('stw#13')
ld(0,Y)                         #13
ld(AC,X)                        #14 Operand
ld([vAC])                       #15
st([Y,Xpp])                     #16 
ld([vAC+1])                     #17
st([X])                         #18
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

# POKE+ implementation
label('poke+#13')
ld(AC,X)                        #13 Operand
ld(0,Y)                         #14    
ld([X])                         #15 low byte poke address
st([vTmp])                      #16
adda(1)                         #17
st([Y,Xpp])                     #18
ld([X])                         #19 high byte poke address
ld(AC,Y)                        #20
ld([vTmp],X)                    #21
ld([vAC])                       #22
st([Y,X])                       #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# ADDBI implementation, var += imm, does NOT modify var.hi
label('addbi#13')
ld([vPC+1],Y)                   #13
st([vTmp])                      #14 immediate value
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16
ld(AC,X)                        #17 var
ld([X])                         #18
adda([vTmp])                    #19
st([X])                         #20
ld([vPC])                       #21
adda(1)                         #22
st([vPC])                       #23 advance vPC by 1
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x1700)
#-----------------------------------------------------------------------

# CALLI implementation
label('calli#13')
adda(3)                         #13
st([vLR])                       #14
ld([vPC+1])                     #15
st([vLR+1],Y)                   #16
ld([Y,X])                       #17
st([Y,Xpp])                     #18 Just X++
suba(2)                         #19
st([vPC])                       #20
ld([Y,X])                       #21
ld(hi('REENTER_28'),Y)          #22
jmp(Y,'REENTER_28')             #23
st([vPC+1])                     #24

# SUBW implemetation
label('subw#13')
adda(1)                         #13
st([vTmp])                      #14 Address of high byte to be subtracted
ld([vAC])                       #15 Low byte
bmi('.subw#18')                 #16
suba([X])                       #17
st([vAC])                       #18 Store low result
ora([X])                        #19 Carry in bit 7
anda(0x80,X)                    #20 Move carry to bit 0
ld([vAC+1])                     #21
suba([X])                       #22
ld([vTmp],X)                    #23
suba([X])                       #24
st([vAC+1])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28
label('.subw#18')
st([vAC])                       #18 Store low result
anda([X])                       #19 Carry in bit 7
anda(0x80,X)                    #20 Move carry to bit 0
ld([vAC+1])                     #21
suba([X])                       #22
ld([vTmp],X)                    #23
suba([X])                       #24
st([vAC+1])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# BEQ implementation
label('beq#15')
ld([vAC+1])                     #15
ora([vAC])                      #16 
beq('.beq#19')                  #17 if (vAC.hi OR vAC.lo) == 0
ld([Y,X])                       #18 address of branch destination
ld([vPC])                       #19
adda(1)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.beq#19')
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# BNE implementation
label('bne#15')
ld([vAC+1])                     #15
ora([vAC])                      #16 
bne('.bne#19')                  #17 if (vAC.hi OR vAC.lo) != 0
ld([Y,X])                       #18 address of branch destination
ld([vPC])                       #19
adda(1)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.bne#19')
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# BGT implementation
label('bgt#15')
ld([vAC+1])                     #15
blt('.bgt#18')                  #16 if vAC.hi < 0
ora([vAC])                      #17
bne('.bgt#20')                  #18 if (vAC.hi OR vAC.lo) != 0
ld([vPC])                       #19
adda(1)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.bgt#18')
ld([vPC])                       #18
adda(1)                         #19
st([vPC])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23
label('.bgt#20')
ld([Y,X])                       #20 address of branch destination
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# BLT implementation
label('blt#15')
ld([vAC+1])                     #15
blt('.blt#18')                  #16 if vAC.hi < 0
ld([vPC])                       #17
adda(1)                         #18
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.blt#18')
ld([Y,X])                       #18 address of branch destination
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# BGE implementation
label('bge#15')
ld([vAC+1])                     #15
blt('.bge#18')                  #16 if vAC.hi < 0
ld([vPC])                       #17
ld([Y,X])                       #18 address of branch destination
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.bge#18')
adda(1)                         #18
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# BLE implementation
label('ble#15')
ld([vAC+1])                     #15
blt('.ble#18')                  #16 if vAC.hi < 0
ora([vAC])                      #17
bne('.ble#20')                  #18 if (vAC.hi OR vAC.lo) != 0
ld([vPC])                       #19
ld([Y,X])                       #20 address of branch destination
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24
label('.ble#18')
ld([Y,X])                       #18 address of branch destination
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.ble#20')
adda(1)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# ADDBA implementation
label('addba#13')
ld(AC,X)                        #13 src var
ld([vAC])                       #14
adda([X])                       #15
st([vAC])                       #16 vAC.lo += var.lo
bmi('.addba#19')                #17 Now figure out if there was a carry
suba([X])                       #18 Gets back the initial value of vAC
ora([X])                        #19 Carry in bit 7
anda(0x80,X)                    #20 Move carry to bit 0
ld([X])                         #21
adda([vAC+1])                   #22 Add the high byte and carry
st([vAC+1])                     #23 Store high result
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26
label('.addba#19')
anda([X])                       #19 Carry in bit 7
anda(0x80,X)                    #20 Move carry to bit 0
ld([X])                         #21
adda([vAC+1])                   #22 Add high byte and carry
st([vAC+1])                     #23 Store high result
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# SUBBA implementation
label('subba#13')
ld(AC,X)                        #13 src var
ld([vAC])                       #14
bmi('.subba#17')                 #15
suba([X])                       #16
st([vAC])                       #17 vAC.lo -= var.lo
ora([X])                        #18 Borrow in bit 7
anda(0x80,X)                    #19 Move borrow to bit 0
ld([vAC+1])                     #20
suba([X])                       #21 Sub borrow from high byte
st([vAC+1])                     #22 Store high result
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.subba#17')
st([vAC])                       #17
anda([X])                       #18 Borrow in bit 7
anda(0x80,X)                    #19 Move borrow to bit 0
ld([vAC+1])                     #20
suba([X])                       #21 Sub borrow from high byte
st([vAC+1])                     #22 Store high result
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25

# ADDB implementation, dst += src, does NOT modify dst.hi
#label('addb#13')
#ld([vPC+1],Y)                   #13
#st([vTmp])                      #14 dst var
#st([Y,Xpp])                     #15 X++
#ld([Y,X])                       #16
#ld(AC,X)                        #17 src var
#ld([X])                         #18
#ld([vTmp],X)                    #19
#adda([X])                       #20
#st([X])                         #21 dst += src
#ld([vPC])                       #22
#adda(1)                         #23
#st([vPC])                       #24 advance vPC by 1
#ld(hi('REENTER'),Y)             #25
#jmp(Y,'REENTER')                #26
#ld(-30/2)                       #27

# ADDBA implementation, vAC.lo = dst + src, does NOT modify vAC.hi
#label('addba#13')
#ld([vPC+1],Y)                   #13
#st([vTmp])                      #14 dst var
#st([Y,Xpp])                     #15 X++
#ld([Y,X])                       #16
#ld(AC,X)                        #17 src var
#ld([X])                         #18
#ld([vTmp],X)                    #19
#adda([X])                       #20
#st([vAC])                       #21 vAC.lo = dst + src
#ld([vPC])                       #22
#adda(1)                         #23
#st([vPC])                       #24 advance vPC by 1
#ld(hi('REENTER'),Y)             #25
#jmp(Y,'REENTER')                #26
#ld(-30/2)                       #27

# POKEA implementation
label('pokea#13')
ld(AC,X)                        #13
ld([X])                         #14 var.lo
ld([vAC],X)                     #15
ld([vAC+1],Y)                   #16
st([Y,X])                       #17 poke [vAC], var.lo
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# DOKEA implementation
label('dokea#13')
st([vTmp],X)                    #13
ld([X])                         #14 var.lo
ld([vAC],X)                     #15
ld([vAC+1],Y)                   #16
st([Y,X])                       #17 poke [vAC], var.lo
ld([vTmp])                      #18
adda(1,X)                       #19
ld([X])                         #20 var.hi
st([vTmp])                      #21
ld([vAC])                       #22
adda(1,X)                       #23
ld([vTmp])                      #24
st([Y,X])                       #25 poke [vAC+1], var.hi
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# POKEI implementation
label('pokei#13')
ld([vAC+1],Y)                   #13
ld([vAC],X)                     #14
st([Y,Xpp])                     #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# DOKEI implementation
label('dokei#13')
ld([vPC+1],Y)                   #13
st([vTmp])                      #14 imm.hi
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16 imm.lo
ld([vAC+1],Y)                   #17
ld([vAC],X)                     #18
st([Y,Xpp])                     #19
ld([vTmp])                      #20
st([Y,X])                       #21
ld([vPC])                       #22
adda(1)                         #23
st([vPC])                       #24
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27

# DEEK+ implementation
label('deek+#13')
ld(AC,X)                        #13
ld([X])                         #14 low byte deek address
st([vTmp])                      #15
adda(2)                         #16
st([Y,Xpp])                     #17
ld([X])                         #18 high byte deek address
ld(AC,Y)                        #19
ld([vTmp],X)                    #20
ld([Y,X])                       #21
st([Y,Xpp])                     #22 X++
st([vAC])                       #23
ld([Y,X])                       #24
st([vAC+1])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x1800)
#-----------------------------------------------------------------------

# SYS retry implementation
label('.sys#16')
ld([vPC])                       #16
suba(2)                         #17
st([vPC])                       #18
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

# LUP return, no longer part of PEEK
label('lupReturn#19')
ld(0)                           #19
st([vAC+1])                     #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

# NOTB implementation
label('notb#13')
ld(AC,X)                        #13 address of var
ld([X])                         #14
xora(255)                       #15
st([X])                         #16
ld(hi('REENTER'),Y)             #17
jmp(Y,'REENTER')                #18
ld(-22/2)                       #19

# DOKE+ implementation
label('doke+#13')
ld(AC,X)                        #13
ld(0,Y)                         #14
ld([X])                         #15 low byte poke address
st([vTmp])                      #16
adda(2)                         #17
st([Y,Xpp])                     #18
ld([X])                         #19 high byte poke address
ld(AC,Y)                        #20
ld([vTmp],X)                    #21
ld([vAC])                       #22
st([Y,Xpp])                     #23
ld([vAC+1])                     #24
st([Y,X])                       #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# XORI implementation
label('xori#13')
xora([vAC])                     #13
st([vAC])                       #14
ld(hi('REENTER'),Y)             #15
jmp(Y,'REENTER')                #16
ld(-20/2)                       #17

# NOP implementation
label('nop#13')
ld([vPC])                       #13
suba(1)                         #14
st([vPC])                       #15 vPC--
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# BRA implementation
label('bra#13')
ld(hi('REENTER'),Y)             #13
jmp(Y,'REENTER')                #14
ld(-18/2)                       #15

# DBGE implementation
label('dbge#13')
ld([vPC+1],Y)                   #13 vPC.hi
st([vTmp])                      #14 branch offset
st([Y,Xpp])                     #15 X++, address of counter
ld([Y,X])                       #16
ld(AC,X)                        #17 fetch counter
ld([X])                         #18
suba(1)                         #19 decrement counter
blt('.dbge#22')                 #20 if < 0 BRA
st([X])                         #21
ld([vTmp])                      #22
st([vPC])                       #23 BRA to branch offset
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26
label('.dbge#22')
ld([vPC])                       #22
adda(1)                         #23
st([vPC])                       #24 advance vPC by 1
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27

# LDNI implementation
label('ldni#13')
xora(255)                       #13
adda(1)                         #14
st([vAC])                       #15
ld(255)                         #16
st([vAC+1])                     #17
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# ANDBK implementation
label('andbk#13')
ld([vPC+1],Y)                   #13
st([vTmp])                      #14 immediate value
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16
ld(AC,X)                        #17 address of var
ld([X])                         #18
anda([vTmp])                    #19
st([vAC])                       #20
ld(0)                           #21
st([vAC+1])                     #22 Store high result
ld([vPC])                       #23
adda(1)                         #24
st([vPC])                       #25 advance vPC by 1
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28
 
# ORBK implementation
label('orbk#13')
ld([vPC+1],Y)                   #13
st([vTmp])                      #14 immediate value
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16
ld(AC,X)                        #17 address of var
ld([X])                         #18
ora([vTmp])                     #19
st([vAC])                       #20
ld(0)                           #21
st([vAC+1])                     #22 Store high result
ld([vPC])                       #23
adda(1)                         #24
st([vPC])                       #25 advance vPC by 1
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# XORBK implementation
label('xorbk#13')
ld([vPC+1],Y)                   #13
st([vTmp])                      #14 immediate value
st([Y,Xpp])                     #15 X++
ld([Y,X])                       #16
ld(AC,X)                        #17 address of var
ld([X])                         #18
xora([vTmp])                    #19
st([vAC])                       #20
ld(0)                           #21
st([vAC+1])                     #22 Store high result
ld([vPC])                       #23
adda(1)                         #24
st([vPC])                       #25 advance vPC by 1
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# CMPI implementation
label('cmpi#13')
st([vTmp])                      #13 immediate value
ld(min(0,maxTicks-36/2))        #14
adda([vTicks])                  #15
blt('.cmpi#18')                 #16 not enough time left, so retry
ld([vPC+1],Y)                   #17
ld([vTmp])                      #18 immediate value
st([Y,Xpp])                     #19 X++
ld([Y,X])                       #20 address of var
ld(AC,X)                        #21
ld([X])                         #22 low byte of var
bmi('.cmpi#25')                 #23
suba([vTmp])                    #24
st([vAC])                       #25 store low result
ora([vTmp])                     #26
anda(0x80)                      #27   
st([vAC+1])                     #28 [vAC.hi] = sign bit, numerical accuracy is not important
ld([vPC])                       #29
adda(1)                         #30
st([vPC])                       #31 advance vPC by 1
ld(hi('NEXTY'),Y)               #32
jmp(Y,'NEXTY')                  #33
ld(-36/2)                       #34
label('.cmpi#25')
st([vAC])                       #25 store low result
anda([vTmp])                    #26
anda(0x80)                      #27   
st([vAC+1])                     #28 [vAC.hi] = sign bit, numerical accuracy is not important
ld([vPC])                       #29
adda(1)                         #30
st([vPC])                       #31 advance vPC by 1
ld(hi('NEXTY'),Y)               #32
jmp(Y,'NEXTY')                  #33
ld(-36/2)                       #34
label('.cmpi#18')
ld([vPC])                       #18 retry instruction
suba(2)                         #19
st([vPC])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

# JEQ implementation
label('jeq#13')
ld([vPC+1],Y)                   #13 vPC.hi
ld([vAC+1])                     #14
ora([vAC])                      #15 
beq('.jeq#18')                  #16 if (vAC.hi OR vAC.lo) == 0
ld([Y,X])                       #17 lo address of jump destination
ld([vPC])                       #18
adda(1)                         #19
st([vPC])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23
label('.jeq#18')
st([vPC])                       #18
st([Y,Xpp])                     #19
ld([Y,X])                       #20 hi address of jump destination
st([vPC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# JNE implementation
label('jne#13')
ld([vPC+1],Y)                   #13 vPC.hi
ld([vAC+1])                     #14
ora([vAC])                      #15 
bne('.jne#18')                  #16 if (vAC.hi OR vAC.lo) != 0
ld([Y,X])                       #17 address of branch destination
ld([vPC])                       #18
adda(1)                         #19
st([vPC])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23
label('.jne#18')
st([vPC])                       #18
st([Y,Xpp])                     #19
ld([Y,X])                       #20 hi address of jump destination
st([vPC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# JLT implementation
label('jlt#13')
ld([vPC+1],Y)                   #13 vPC.hi
ld([vAC+1])                     #14
blt('.jlt#17')                  #15 if vAC.hi < 0
ld([Y,X])                       #16 address of branch destination
ld([vPC])                       #17
adda(1)                         #18
st([vPC])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22
label('.jlt#17')
st([vPC])                       #17
st([Y,Xpp])                     #18
ld([Y,X])                       #19 hi address of jump destination
st([vPC+1])                     #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

# BBEQ implementation
#label('bbeq#13')
#ld([vPC+1],Y)                   #13 vPC.hi
#st([vTmp])                      #14 branch offset
#st([Y,Xpp])                     #15 X++
#ld([Y,X])                       #16
#ld(AC,X)                        #17
#ld([X])                         #18 low byte of var
#beq('.bbeq#21')                 #19
#ld([vPC])                       #20
#adda(1)                         #21
#st([vPC])                       #22 advance vPC by 1
#ld(hi('REENTER'),Y)             #23
#jmp(Y,'REENTER')                #24
#ld(-28/2)                       #25
#label('.bbeq#21')
#ld([vTmp])                      #21
#st([vPC])                       #22 branch to offset
#ld(hi('REENTER'),Y)             #23
#jmp(Y,'REENTER')                #24
#ld(-28/2)                       #25

# BWEQ implementation
#label('bweq#13')
#ld([vPC+1],Y)                   #13 vPC.hi
#st([vTmp])                      #14 branch offset
#st([Y,Xpp])                     #15 X++
#ld([Y,X])                       #16
#ld(AC,X)                        #17
#ld([X])                         #18 low byte of var
#ld(0,Y)                         #19
#st([Y,Xpp])                     #20 X++
#ora([Y,X])                      #21 or with high byte of var
#beq('.bweq#24')                 #22
#ld([vPC])                       #23
#adda(1)                         #24
#st([vPC])                       #25 advance vPC by 1
#ld(hi('NEXTY'),Y)               #26
#jmp(Y,'NEXTY')                  #27
#ld(-30/2)                       #28
#label('.bweq#24')
#ld([vTmp])                      #24
#st([vPC])                       #25 branch to offset
#ld(hi('NEXTY'),Y)               #26
#jmp(Y,'NEXTY')                  #27
#ld(-30/2)                       #28


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x1900)
#-----------------------------------------------------------------------
# sys_Multiply_s16, sum:s16 = x:s16 * y:s16
# x:args0:1 y:args2:3 sum:args4:5 mask:args6:7
label('sys_Multiply_s16')
anda([sysArgs+2])               #18,
st([vAC])                       #19, AC.lo = mask.lo AND y.lo
ld([sysArgs+7])                 #20, load mask.hio
anda([sysArgs+3])               #21, 
st([vAC+1])                     #22, AC.hi = mask.hi AND y.hi
ora([vAC])                      #23, 
beq('.sys_ms16_26')             #24, AC = 0 then skip
ld([sysArgs+4])                 #25, load sum.lo
adda([sysArgs+0])               #26, load x.lo
st([sysArgs+4])                 #27, sum.lo = sum.lo + x.lo
blt('.sys_ms16_30')             #28, check for carry
suba([sysArgs+0])               #29, get original sum.lo back
bra('.sys_ms16_32')             #30, 
ora([sysArgs+0])                #31, carry in bit 7

label('.sys_ms16_26')
bra('.sys_ms16_28')             #26,
ld(-56/2)                       #27, no accumulate sys ticks

label('.sys_ms16_30')
anda([sysArgs+0])               #30, carry in bit 7
nop()                           #31, 

label('.sys_ms16_32')
anda(0x80,X)                    #32, 
ld([X])                         #33, move carry to bit 0
adda([sysArgs+5])               #34, 
adda([sysArgs+1])               #35, 
st([sysArgs+5])                 #36, sum.hi = sum.hi + x.hi
ld(-66/2)                       #37, accumulate sys ticks

label('.sys_ms16_28')
st([vTmp])                      #28,#38,
ld([sysArgs+0])                 #29,#39, AC = x.lo
anda(0x80,X)                    #30,#40, X = AC & 0x80
adda([sysArgs+0])               #31,#41, AC = x.lo <<1
st([sysArgs+0])                 #32,#42, x.lo = AC
ld([X])                         #33,#43, AC = X >>7
adda([sysArgs+1])               #34,#44, 
adda([sysArgs+1])               #35,#45, 
st([sysArgs+1])                 #36,#46, x.hi = x.hi <<1 + AC
ld([sysArgs+6])                 #37,#47, AC = mask.lo
anda(0x80,X)                    #38,#48, X = AC & 0x80
adda([sysArgs+6])               #39,#49, AC = mask.lo <<1
st([sysArgs+6])                 #40,#50, mask.lo = AC
ld([X])                         #41,#51, AC = X >>7
adda([sysArgs+7])               #42,#52, 
adda([sysArgs+7])               #43,#53, 
st([sysArgs+7])                 #44,#54, mask.hi = mask.hi <<1 + AC
ora([sysArgs+6])                #45,#55, 
beq('.sys_ms16_48')             #46,#56, if mask = 0
ld([sysArgs+4])                 #47,#57
ld([vPC])                       #48,#58, 
suba(2)                         #49,#59, 
st([vPC])                       #50,#60, restart SYS function
ld(hi('REENTER'),Y)             #51,#61,
jmp(Y,'REENTER')                #52,#62,
ld([vTmp])                      #53,#63,

label('.sys_ms16_48')
st([vAC])                       #48,#58,
ld([sysArgs+5])                 #49,#59,
st([vAC+1])                     #50,#60,
ld(hi('REENTER'),Y)             #51,#61,
jmp(Y,'REENTER')                #52,#62,
ld([vTmp])                      #53,#63,


#-----------------------------------------------------------------------
# sys_Divide_s16, x:s16 = x:s16 / y:s16, rem:s16 = x:s16 % y:s16
# x:args0:1 y:args2:3 rem:args4:5 mask:args6:7
label('sys_Divide_s16')
anda(0x80,X)                    #18, X = AC & 0x80
adda([sysArgs+4])               #19, AC = rem.lo <<1
st([sysArgs+4])                 #20, rem.lo = AC
ld([X])                         #21, AC = X >>7
adda([sysArgs+5])               #22, 
adda([sysArgs+5])               #23, 
st([sysArgs+5])                 #24, rem.hi = rem.hi <<1 + AC
ld([sysArgs+1])                 #25,
anda(0x80)                      #26, sign of x
beq('.sys_ds16_29')             #27, if x >= 0
ld([sysArgs+4])                 #28,
adda(1)                         #29,
bra('.sys_ds16_32')             #30,
st([sysArgs+4])                 #31, rem.lo++

label('.sys_ds16_29')
nop()                           #29
nop()                           #30
nop()                           #31

label('.sys_ds16_32')
ld([sysArgs+0])                 #32, AC = x.lo
anda(0x80,X)                    #33, X = AC & 0x80
adda([sysArgs+0])               #34, AC = x.lo <<1
st([sysArgs+0])                 #35, x.lo = AC
ld([X])                         #36, AC = X >>7
adda([sysArgs+1])               #37, 
adda([sysArgs+1])               #38, 
st([sysArgs+1])                 #39, x.hi = x.hi <<1 + AC

ld([sysArgs+4])                 #40, load rem.lo
blt('.sys_ds16_43')             #41, check for borrow
suba([sysArgs+2])               #42, 
st([vAC])                       #43, vAC.lo = rem.lo - y.lo
bra('.sys_ds16_46')             #44,
ora([sysArgs+2])                #45,

label('.sys_ds16_43')
st([vAC])                       #43,
anda([sysArgs+2])               #44,
nop()                           #45,

label('.sys_ds16_46')
anda(0x80,X)                    #46, move borrow to bit 0
ld([sysArgs+5])                 #47, load rem.hi
suba([X])                       #48,
suba([sysArgs+3])               #49,
st([vAC+1])                     #50, vAC.hi = rem.hi - y.hi
blt('.sys_ds16_53')             #51,
ld(-72/2)                       #52
ld([vAC])                       #53,
st([sysArgs+4])                 #54,
ld([vAC+1])                     #55,
st([sysArgs+5])                 #56, rem = vAC
ld([sysArgs+0])                 #57,
adda(1)                         #58,
st([sysArgs+0])                 #59, x.lo++
ld(-80/2)                       #60,

label('.sys_ds16_53')
st([vTmp])                      #53, #61, 
ld([sysArgs+6])                 #54, #62, AC = mask.lo
anda(0x80,X)                    #55, #63, X = AC & 0x80
adda([sysArgs+6])               #56, #64, AC = mask.lo <<1
st([sysArgs+6])                 #57, #65, mask.lo = AC
ld([X])                         #58, #66, AC = X >>7
adda([sysArgs+7])               #59, #67, 
adda([sysArgs+7])               #60, #68, 
st([sysArgs+7])                 #61, #69, mask.hi = mask.hi <<1 + AC
ora([sysArgs+6])                #62, #70, 
bne('.sys_ds16_65')             #63, #71, 
ld([vPC])                       #64, #72, 
nop()                           #65, #73, 
nop()                           #66, #74, 
ld(hi('REENTER'),Y)             #67, #75, 
jmp(Y,'REENTER')                #68, #76, 
ld([vTmp])                      #69, #77, 

label('.sys_ds16_65')
suba(2)                         #65, #73,
st([vPC])                       #66, #74, restart SYS function
ld(hi('REENTER'),Y)             #67, #75, 
jmp(Y,'REENTER')                #68, #76, 
ld([vTmp])                      #69, #77, 


#-----------------------------------------------------------------------
# sys_DrawLine
label('sys_DrawLine')
ld([0x82],X)                    #18, X = [xy1].lo
ld([0x83],Y)                    #19, Y = [xy1].hi
st([Y,X])                       #20, [Y, X] = fgColour
ld([0x84],X)                    #21, X = [xy2].lo
ld([0x85],Y)                    #22, Y = [xy2].hi
st([Y,X])                       #23, [Y, X] = fgColour

ld([0xA0])                      #24, num.lo
adda([0x9C])                    #25, sy.lo
st([0xA0])                      #26, num.lo = num.lo + sy.lo
blt('.sys_drawl_29')            #27, check for carry
suba([0x9C])                    #28, get original num.lo back
bra('.sys_drawl_31')            #29, 
ora([0x9C])                     #30, carry in bit 7

label('.sys_drawl_29')
anda([0x9C])                    #29, carry in bit 7
nop()                           #30, 

label('.sys_drawl_31')
anda(0x80,X)                    #31, 
ld([X])                         #32, move carry to bit 0
adda([0xA1])                    #33, 
adda([0x9D])                    #34, 
st([0xA1])                      #35, num.hi = num.hi + sy.hi

ld([0xA0])                      #36, num.lo
blt('.sys_drawl_39')            #37, check for borrow
suba([0x9A])                    #38, sx.lo
st([vAC])                       #39, vAC.lo = num.lo - sx.lo
bra('.sys_drawl_42')            #40,
ora([0x9A])                     #41,

label('.sys_drawl_39')
st([vAC])                       #39,
anda([0x9A])                    #40,
nop()                           #41,

label('.sys_drawl_42')
anda(0x80,X)                    #42, move borrow to bit 0
ld([0xA1])                      #43, num.hi
suba([X])                       #44,
suba([0x9B])                    #45,
blt('.sys_drawl_48')            #46,
st([vAC+1])                     #47, vAC.hi = num.hi - sx.hi
ld([vAC])                       #48,
st([0xA0])                      #49,
ld([vAC+1])                     #50,
st([0xA1])                      #51, num = vAC

ld([0x82])                      #52, xy1.lo
adda([0x86])                    #53, dxy1.lo
st([0x82])                      #54, xy1.lo = xy1.lo + dxy1.lo
blt('.sys_drawl_57')            #55, check for carry
suba([0x86])                    #56, get original xy1.lo back
bra('.sys_drawl_59')            #57, 
ora([0x86])                     #58, carry in bit 7

label('.sys_drawl_57')
anda([0x86])                    #57, carry in bit 7
nop()                           #58, 

label('.sys_drawl_59')
anda(0x80,X)                    #59, 
ld([X])                         #60, move carry to bit 0
adda([0x83])                    #61, 
adda([0x87])                    #62, 
st([0x83])                      #63, xy1.hi = xy1.hi + dxy1.hi

ld([0x84])                      #64, xy2.lo
blt('.sys_drawl_67')            #65, check for borrow
suba([0x86])                    #66, dxy1.lo
st([0x84])                      #67, xy2.lo = xy2.lo - dxy1.lo
bra('.sys_drawl_70')            #68,
ora([0x86])                     #69,

label('.sys_drawl_67')
st([0x84])                      #67,
anda([0x86])                    #68,
nop()                           #69,

label('.sys_drawl_70')
anda(0x80,X)                    #70, move borrow to bit 0
ld([0x85])                      #71, xy2.hi
suba([X])                       #72,
suba([0x87])                    #73,
st([0x85])                      #74, xy2.hi = xy2.hi - dxy1.hi

ld([0x9E])                      #75,
suba(1)                         #76,
bne('.sys_drawl_79')            #77,
st([0x9E])                      #78, drawLine_count--
ld(hi('REENTER'),Y)             #79,
jmp(Y,'REENTER')                #80,
ld(-84/2)                       #81,

label('.sys_drawl_79')
ld([vPC])                       #79,
suba(2)                         #80,
st([vPC])                       #81, restart
ld(hi('NEXTY'),Y)               #82,
jmp(Y,'NEXTY')                  #83,
ld(-86/2)                       #84,

label('.sys_drawl_48')
ld([0x82])                      #48, xy1.lo
adda([0x88])                    #49, dxy2.lo
st([0x82])                      #50, xy1.lo = xy1.lo + dxy2.lo
blt('.sys_drawl_53')            #51, check for carry
suba([0x88])                    #52, get original xy1.lo back
bra('.sys_drawl_55')            #53, 
ora([0x88])                     #54, carry in bit 7

label('.sys_drawl_53')
anda([0x88])                    #53, carry in bit 7
nop()                           #54, 

label('.sys_drawl_55')
anda(0x80,X)                    #55, 
ld([X])                         #56, move carry to bit 0
adda([0x83])                    #57, 
adda([0x89])                    #58, 
st([0x83])                      #59, xy1.hi = xy1.hi + dxy2.hi

ld([0x84])                      #60, xy2.lo
blt('.sys_drawl_63')            #61, check for borrow
suba([0x88])                    #62, dxy2.lo
st([0x84])                      #63, xy2.lo = xy2.lo - dxy2.lo
bra('.sys_drawl_66')            #64,
ora([0x88])                     #65,

label('.sys_drawl_63')
st([0x84])                      #63,
anda([0x88])                    #64,
nop()                           #65,

label('.sys_drawl_66')
anda(0x80,X)                    #66, move borrow to bit 0
ld([0x85])                      #67, xy2.hi
suba([X])                       #68,
suba([0x89])                    #69,
st([0x85])                      #70, xy2.hi = xy2.hi - dxy2.hi

ld([0x9E])                      #71,
suba(1)                         #72,
bne('.sys_drawl_75')            #73,
st([0x9E])                      #74, drawLine_count--
ld(hi('REENTER'),Y)             #75,
jmp(Y,'REENTER')                #76,
ld(-80/2)                       #77,

label('.sys_drawl_75')
ld([vPC])                       #75,
suba(2)                         #76,
st([vPC])                       #77, restart
ld(hi('NEXTY'),Y)               #78,
jmp(Y,'NEXTY')                  #79,
ld(-82/2)                       #80,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x1A00)
#-----------------------------------------------------------------------


# sys_WaitVBlank
label('sys_WaitVBlank')
xora(videoYline0)               #18,
beq('.sys_waitvblank_21')       #19,
ld([vPC])                       #20,
suba(2)                         #21,
st([vPC])                       #22, restart
ld(hi('REENTER'),Y)             #23,
jmp(Y,'REENTER')                #24,
ld(-28/2)                       #25,
label('.sys_waitvblank_21')
ld(hi('REENTER'),Y)             #21,
jmp(Y,'REENTER')                #22,
ld(-28/2)                       #23,


# sys_SortUint8Array, sysArgs0,1=array, sysArg2=i, sysArg3=j, sysArg4=key, sysArg5=length
label('sys_SortUint8Array')
ld([sysArgs+3])                 #18,
ble('.sys_sortuint8array_21')   #19, j == 0
ld([sysArgs+0])                 #20,
adda([sysArgs+3])               #21,
suba(1,X)                       #22,
ld([Y,X])                       #23,
bmi('.sys_sortuint8array_26')   #24, convert signed < to unsigned <
suba([sysArgs+4])               #25,
bra('.sys_sortuint8array_28')   #26,
ora([sysArgs+4])                #27, borrow
label('.sys_sortuint8array_26')
anda([sysArgs+4])               #26, borrow
nop()                           #27,
label('.sys_sortuint8array_28')
bmi('.sys_sortuint8array_30_0') #28, a[j-1] < key
ld([Y,X])                       #29,
st([Y,Xpp])                     #30,
st([Y,X])                       #31, a[j] = a[j-1]
ld([sysArgs+3])                 #32,
suba(1)                         #33,
st([sysArgs+3])                 #34, j--
ld([vPC])                       #35,
suba(2)                         #36,
st([vPC])                       #37, restart
ld(hi('NEXTY'),Y)               #38,
jmp(Y,'NEXTY')                  #39,
ld(-42/2)                       #40,
label('.sys_sortuint8array_21')
adda([sysArgs+3],X)             #21,
ld([sysArgs+4])                 #22,
st([Y,X])                       #23, a[j] = key
ld([sysArgs+2])                 #24,
adda(1)                         #25,
st([sysArgs+2])                 #26, i++
suba([sysArgs+5])               #27,
blt('.sys_sortuint8array_30_1') #28, i < length
ld([sysArgs+0])                 #29,
ld(hi('NEXTY'),Y)               #30,
jmp(Y,'NEXTY')                  #31,
ld(-34/2)                       #32,
label('.sys_sortuint8array_30_0')
st([Y,Xpp])                     #30,
ld([sysArgs+4])                 #31,
st([Y,X])                       #32, a[j] = key
ld([sysArgs+2])                 #33,
adda(1)                         #34,
st([sysArgs+2])                 #35, i++
suba([sysArgs+5])               #36,
blt('.sys_sortuint8array_39')   #37, i < length
ld([sysArgs+0])                 #38,
ld(hi('REENTER'),Y)             #39,
jmp(Y,'REENTER')                #40,
ld(-44/2)                       #41,
label('.sys_sortuint8array_30_1')
adda([sysArgs+2],X)             #30,
ld([Y,X])                       #31,
st([sysArgs+4])                 #32, key = a[i]
ld([sysArgs+2])                 #33,
st([sysArgs+3])                 #34, j = i
ld([vPC])                       #35,
suba(2)                         #36,
st([vPC])                       #37, restart
ld(hi('NEXTY'),Y)               #38,
jmp(Y,'NEXTY')                  #39,
ld(-42/2)                       #40,
label('.sys_sortuint8array_39')
adda([sysArgs+2],X)             #39,
ld([Y,X])                       #40,
st([sysArgs+4])                 #41, key = a[i]
ld([sysArgs+2])                 #42,
st([sysArgs+3])                 #43, j = i
ld([vPC])                       #44,
suba(2)                         #45,
st([vPC])                       #46, restart
ld(hi('REENTER'),Y)             #47,
jmp(Y,'REENTER')                #48,
ld(-52/2)                       #49,


# sys_SortViaIndices, sysArgs0,1=indices, sysArgs2,3=src, sysArg4,5=dst, sysArg6=dst step, vAC=count
label('sys_SortViaIndices')
ld([sysArgs+0],X)               #18,
ld([Y,X])                       #19, [index]
ld([sysArgs+3],Y)               #20,
adda([sysArgs+2],X)             #21, src
ld([Y,X])                       #22, peek(src + index)
ld([sysArgs+5],Y)               #23,
ld([sysArgs+4],X)               #24, dst
st([Y,X])                       #25,
ld([sysArgs+0])                 #26,
adda(1)                         #27,
st([sysArgs+0])                 #28, indices++
ld([sysArgs+4])                 #29,
adda([sysArgs+6])               #30,
st([sysArgs+4])                 #31, dst += step
ld([vAC])                       #32,
suba(1)                         #33,
beq('.sys_sortviaindices_36')   #34,
st([vAC])                       #35, count--
ld([vPC])                       #36,
suba(2)                         #37,
st([vPC])                       #38, restart
ld(hi('REENTER'),Y)             #39,
jmp(Y,'REENTER')                #40,
ld(-44/2)                       #41,
label('.sys_sortviaindices_36')
ld(hi('NEXTY'),Y)               #36,
jmp(Y,'NEXTY')                  #37,
ld(-40/2)                       #38,


# SYS_LoaderNextByteIn_32
# sysArgs[0:1] Current address
# sysArgs[2]   Checksum
# sysArgs[3]   Wait value (videoY)
label('SYS_LoaderNextByteIn_32')
ld([videoY])                    #15
xora([sysArgs+3])               #16
bne('.sysNbi#19')               #17
ld([sysArgs+0],X)               #18
ld([sysArgs+1],Y)               #19
ld(IN)                          #20
st([Y,X])                       #21
st([vAC])                       #22
adda([sysArgs+2])               #23
st([sysArgs+2])                 #24
ld([sysArgs+0])                 #25
adda(1)                         #26
st([sysArgs+0])                 #27
ld(hi('NEXTY'),Y)               #28
jmp(Y,'NEXTY')                  #29
ld(-32/2)                       #30

# Restart the instruction in the next timeslice
label('.sysNbi#19')
ld([vPC])                       #19
suba(2)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


# SYS_LoaderProcessInput_64
# sysArgs[0:1] Source address
# sysArgs[2]   Checksum
# sysArgs[4]   Copy count
# sysArgs[5:6] Destination address
label('SYS_LoaderProcessInput_64')
ld([sysArgs+1],Y)               #15
ld([sysArgs+2])                 #16
bne('.sysPi#19')                #17
ld([sysArgs+0])                 #18
suba(65,X)                      #19 Point at first byte of buffer
ld([Y,X])                       #20 Command byte
st([Y,Xpp]);                    C('Just X++')#21
xora(ord('L'))                  #22 This loader lumps everything under 'L'
bne('.sysPi#25')                #23
ld([Y,X]);                      C('Valid command')#24 Length byte
st([Y,Xpp]);                    C('Just X++')#25
anda(63)                        #26 Bit 6:7 are garbage
st([sysArgs+4])                 #27 Copy count 0..60
adda([Y,X])                     #28 One location past (+1) the last byte of fragment
adda(1)                         #29 254+1 = $ff becomes 0, 255+1 = $00 becomes 1
anda(0xfe)                      #30 Will be zero only when writing in top 2 bytes of page
st([vTmp])                      #31 Remember as first condition
ld([Y,X])                       #32 Low copy address
st([Y,Xpp]);                    C('Just X++')#33
st([sysArgs+5])                 #34
ld([Y,X])                       #35 High copy address
st([Y,Xpp]);                    C('Just X++')#36
st([sysArgs+6])                 #37
suba(1)                         #38 Check if writing into sound channel page (1..4)
anda(0xfc)                      #39
ora([vTmp])                     #40 Combine second condition with first
st([vTmp])                      #41 Zero when overwriting one of oscL[1..4] or oscH[1..4]
ld([sysArgs+4])                 #42 Check copy count
bne('.sysPi#45')                #43
# Execute code (don't care about checksum anymore)
ld([sysArgs+5]);                C('Execute')#44 Low run address
st([vLR])                       #45 https://forum.gigatron.io/viewtopic.php?p=29#p29
suba(2)                         #46
st([vPC])                       #47
ld([sysArgs+6])                 #48 High run address
st([vPC+1])                     #49
st([vLR+1])                     #50
ld(hi('REENTER'),Y)             #51
jmp(Y,'REENTER')                #52
ld(-56/2)                       #53

# Invalid checksum
label('.sysPi#19')
wait(25-19);                    C('Invalid checksum')#19 Reset checksum

# Unknown command
label('.sysPi#25')
ld(ord('g'));                   C('Unknown command')#25 Reset checksum
st([sysArgs+2])                 #26
ld(hi('REENTER'),Y)             #27
jmp(Y,'REENTER')                #28
ld(-32/2)                       #29

# Loading data
label('.sysPi#45')
ld([vTmp]);                     C('Loading data')#45
bne(pc()+3)                     #46
bra(pc()+3)                     #47
ld(0xfc);                       C('Unsafe')#48  Clear low channelMask bits so it becomes safe
ld(0xff);                       C('Safe')#48(!) No change to channelMask because already safe
anda([channelMask])             #49
st([channelMask])               #50
ld([sysArgs+0])                 #51 Continue checksum
suba(1,X)                       #52 Point at last byte
ld([Y,X])                       #53
st([sysArgs+2])                 #54
ld(hi('REENTER'),Y)             #55
jmp(Y,'REENTER')                #56
ld(-60/2)                       #57
 

# SYS_LoaderPayloadCopy_34
# sysArgs[0:1] Source address
# sysArgs[4]   Copy count
# sysArgs[5:6] Destination address
label('SYS_LoaderPayloadCopy_34')
ld([sysArgs+4])                 #15 Copy count
beq('.sysCc#18')                #16
suba(1)                         #17
st([sysArgs+4])                 #18
ld([sysArgs+0],X)               #19 Current pointer
ld([sysArgs+1],Y)               #20
ld([Y,X])                       #21
ld([sysArgs+5],X)               #22 Target pointer
ld([sysArgs+6],Y)               #23
st([Y,X])                       #24
ld([sysArgs+5])                 #25 Increment target
adda(1)                         #26
st([sysArgs+5])                 #27
bra('.sysCc#30')                #28

label('.sysCc#18')
ld(hi('REENTER'),Y)             #18,29
wait(30-19)                     #19
label('.sysCc#30')
jmp(Y,'REENTER')                #30
ld(-34/2)                       #31


# SYS_Unpack_56
# Unpack 3 bytes into 4 pixels
#
# Variables:
#       sysArgs[0:2]    Packed bytes (in)
#       sysArgs[0:3]    Pixels (out)
label('SYS_Unpack_56')
ld(soundTable>>8,Y)             #15
ld([sysArgs+2])                 #16 a[2]>>2
ora(0x03,X)                     #17
ld([Y,X])                       #18
st([sysArgs+3])                 #19 -> Pixel 3

ld([sysArgs+2])                 #20 (a[2]&3)<<4
anda(0x03)                      #21
adda(AC)                        #22
adda(AC)                        #23
adda(AC)                        #24
adda(AC)                        #25
st([sysArgs+2])                 #26
ld([sysArgs+1])                 #27 | a[1]>>4
ora(0x03,X)                     #28
ld([Y,X])                       #29
ora(0x03,X)                     #30
ld([Y,X])                       #31
ora([sysArgs+2])                #32
st([sysArgs+2])                 #33 -> Pixel 2

ld([sysArgs+1])                 #34 (a[1]&15)<<2
anda(0x0f)                      #35
adda(AC)                        #36
adda(AC)                        #37
st([sysArgs+1])                 #38

ld([sysArgs+0])                 #39 | a[0]>>6
ora(0x03,X)                     #40
ld([Y,X])                       #41
ora(0x03,X)                     #42
ld([Y,X])                       #43
ora(0x03,X)                     #44
ld([Y,X])                       #45
ora([sysArgs+1])                #46
st([sysArgs+1])                 #47 -> Pixel 1

ld([sysArgs+0])                 #48 a[1]&63
anda(0x3f)                      #49
st([sysArgs+0])                 #50 -> Pixel 0

ld(hi('REENTER'),Y)             #51
jmp(Y,'REENTER')                #52
ld(-56/2)                       #53


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x1B00)
#-----------------------------------------------------------------------

# sys_MemCopyByte, sysArgs0,1=src, sysArg2,3=dst, sysArg4=src step, sysArg5=dst step, vAC=count
label('sys_MemCopyByte')
ld([sysArgs+0],X)               #18,
ld([Y,X])                       #19, [src]
ld([sysArgs+3],Y)               #20,
ld([sysArgs+2],X)               #21,
st([Y,X])                       #22, [dst] = [src]
ld([sysArgs+0])                 #23,
adda([sysArgs+4])               #24,
st([sysArgs+0])                 #25, src++
ld([sysArgs+2])                 #26,
adda([sysArgs+5])               #27,
st([sysArgs+2])                 #28, dst++
ld([vAC])                       #29,
suba(1)                         #30,
beq('.sys_memcopybyte_33')      #31,
st([vAC])                       #32, count--
ld([vPC])                       #33,
suba(2)                         #34,
st([vPC])                       #35, restart
ld(hi('NEXTY'),Y)               #36,
jmp(Y,'NEXTY')                  #37,
ld(-40/2)                       #38,
label('.sys_memcopybyte_33')
ld(hi('REENTER'),Y)             #33,
jmp(Y,'REENTER')                #34,
ld(-38/2)                       #35,


# sys_MemCopyWord, sysArgs0,1=src, sysArg2,3=dst, vAC=count, sysArg4,5=tmp
label('sys_MemCopyWord')
ld([sysArgs+0],X)               #18,
ld([Y,X])                       #19, [src + 0]
st([Y,Xpp])                     #20
st([sysArgs+4])                 #21,
ld([Y,X])                       #22, [src + 1]
st([sysArgs+5])                 #23,
ld([sysArgs+3],Y)               #24,
ld([sysArgs+2],X)               #25,
ld([sysArgs+4])                 #26,
st([Y,Xpp])                     #27, [dst + 0] = [src + 0]
ld([sysArgs+5])                 #28,
st([Y,X])                       #29, [dst + 1] = [src + 1]
ld([sysArgs+0])                 #30,
adda(2)                         #31,
st([sysArgs+0])                 #32, src += 2
ld([sysArgs+2])                 #33,
adda(2)                         #34,
st([sysArgs+2])                 #35, dst += 2
ld([vAC])                       #36,
suba(1)                         #37,
beq('.sys_memcopyword_40')      #38,
st([vAC])                       #39, count--
ld([vPC])                       #40,
suba(2)                         #41,
st([vPC])                       #42, restart
ld(hi('REENTER'),Y)             #43,
jmp(Y,'REENTER')                #44,
ld(-48/2)                       #45,
label('.sys_memcopyword_40')
ld(hi('NEXTY'),Y)               #40,
jmp(Y,'NEXTY')                  #41,
ld(-44/2)                       #42,


# sys_MemCopyDWord, sysArgs0,1=src, sysArg2,3=dst, vAC=count, sysArg4,5=tmp, sysArg6,7=tmp
label('sys_MemCopyDWord')
ld([sysArgs+0],X)               #18,
ld([Y,X])                       #19, [src + 0]
st([Y,Xpp])                     #20
st([sysArgs+4])                 #21,
ld([Y,X])                       #22, [src + 1]
st([Y,Xpp])                     #23
st([sysArgs+5])                 #24,
ld([Y,X])                       #25, [src + 2]
st([Y,Xpp])                     #26
st([sysArgs+6])                 #27,
ld([Y,X])                       #28, [src + 3]
st([sysArgs+7])                 #29,
ld([sysArgs+3],Y)               #30,
ld([sysArgs+2],X)               #31,
ld([sysArgs+4])                 #32,
st([Y,Xpp])                     #33, [dst + 0] = [src + 0]
ld([sysArgs+5])                 #34,
st([Y,Xpp])                     #35, [dst + 1] = [src + 1]
ld([sysArgs+6])                 #36,
st([Y,Xpp])                     #37, [dst + 2] = [src + 2]
ld([sysArgs+7])                 #38,
st([Y,X])                       #39, [dst + 3] = [src + 3]
ld([sysArgs+0])                 #40,
adda(4)                         #41,
st([sysArgs+0])                 #42, src += 4
ld([sysArgs+2])                 #43,
adda(4)                         #44,
st([sysArgs+2])                 #45, dst += 4
ld([vAC])                       #46,
suba(1)                         #47,
beq('.sys_memcopydword_50')     #48,
st([vAC])                       #49, count--
ld([vPC])                       #50,
suba(2)                         #51,
st([vPC])                       #52, restart
ld(hi('REENTER'),Y)             #53,
jmp(Y,'REENTER')                #54,
ld(-58/2)                       #55,
label('.sys_memcopydword_50')
ld(hi('NEXTY'),Y)               #50,
jmp(Y,'NEXTY')                  #51,
ld(-54/2)                       #52,


# sys_ReadPixel
label('sys_ReadPixel')
adda([vAC+1])                   #18,
ld(AC,X)                        #19,
ld(1,Y)                         #20, Y,X = 0x0100 + 2*y, (0 >= y <= 127)
ld([Y,X])                       #21,
ld(AC,Y)                        #22, Y = [Y,X]
ld([vAC],X)                     #23, X = x
ld([Y,X])                       #24,
st([vAC])                       #25, vAC = [Y,X]
ld(0)                           #26,
st([vAC+1])                     #27,
ld(hi('NEXTY'),Y)               #28,
jmp(Y,'NEXTY')                  #29,
ld(-32/2)                       #30,


# sys_DrawPixel
label('sys_DrawPixel')
adda([sysArgs+1])               #18,
ld(AC,X)                        #19,
ld(1,Y)                         #20, Y,X = 0x0100 + 2*y, (0 >= y <= 127)
ld([Y,X])                       #21,
ld(AC,Y)                        #22, Y = [Y,X]
ld([sysArgs+0],X)               #23, X = x
ld([sysArgs+2])                 #24, colour
st([Y,X])                       #25, [Y,X] = colour
ld(hi('NEXTY'),Y)               #26,
jmp(Y,'NEXTY')                  #27,
ld(-30/2)                       #28,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x1C00)
#-----------------------------------------------------------------------

# sys_SortSprites, sysArgs0,1=_y_array, sysArg2=i, sysArg3=j, sysArg4=key0, sysArg5=length
#                  sysArgs6,7=_is_array, vAC.lo=key1
label('sys_SortSprites')
ld([sysArgs+3])                 #18,
ble('.sys_sortsprites_21')      #19, j <= 0
ld([sysArgs+0])                 #20,
adda([sysArgs+3])               #21,
suba(1,X)                       #22,
ld([Y,X])                       #23,
bmi('.sys_sortsprites_26')      #24, convert signed < to unsigned <
suba([sysArgs+4])               #25,
bra('.sys_sortsprites_28')      #26,
ora([sysArgs+4])                #27, borrow
label('.sys_sortsprites_26')
anda([sysArgs+4])               #26, borrow
nop()                           #27,
label('.sys_sortsprites_28')
bmi('.sys_sortsprites_30')      #28, y[j-1] < key
ld([Y,X])                       #29,
st([Y,Xpp])                     #30,
st([Y,X])                       #31, y[j] = y[j-1]
ld([sysArgs+7],Y)               #32,
ld([sysArgs+6])                 #33,
adda([sysArgs+3])               #34,
suba(1,X)                       #35,
ld([Y,X])                       #36,
st([Y,Xpp])                     #37,
st([Y,X])                       #38, is[j] = is[j-1]
ld([sysArgs+3])                 #39,
suba(1)                         #40,
st([sysArgs+3])                 #41, j--
ld([vPC])                       #42,
suba(2)                         #43,
st([vPC])                       #44, restart
ld(hi('REENTER'),Y)             #45,
jmp(Y,'REENTER')                #46,
ld(-50/2)                       #47,
label('.sys_sortsprites_21')
adda([sysArgs+3],X)             #21,
ld([sysArgs+4])                 #22,
st([Y,X])                       #23, y[j] = key0
ld([sysArgs+7],Y)               #24,
ld([sysArgs+6])                 #25,
adda([sysArgs+3],X)             #26,
ld([0x82])                      #27,
st([Y,X])                       #28, is[j] = key1
ld([sysArgs+2])                 #29,
adda(1)                         #30,
st([sysArgs+2])                 #31, i++
suba([sysArgs+5])               #32,
blt('.sys_sortsprites_35')      #33, i < length
ld([sysArgs+1],Y)               #34,
ld(hi('REENTER'),Y)             #35,
jmp(Y,'REENTER')                #36,
ld(-40/2)                       #37,
label('.sys_sortsprites_30')
st([Y,Xpp])                     #30,
ld([sysArgs+4])                 #31,
st([Y,X])                       #32, y[j] = key0
ld([sysArgs+7],Y)               #33,
ld([sysArgs+6])                 #34,
adda([sysArgs+3],X)             #35,
ld([0x82])                      #36,
st([Y,X])                       #37, is[j] = key1
ld([sysArgs+2])                 #38,
adda(1)                         #39,
st([sysArgs+2])                 #40, i++
suba([sysArgs+5])               #41,
blt('.sys_sortsprites_44')      #42, i < length
ld([sysArgs+1],Y)               #43
ld(hi('NEXTY'),Y)               #44,
jmp(Y,'NEXTY')                  #45,
ld(-48/2)                       #46,
label('.sys_sortsprites_35')
ld([sysArgs+0])                 #35,
adda([sysArgs+2],X)             #36,
ld([Y,X])                       #37,
st([sysArgs+4])                 #38, key0 = y[i]
ld([sysArgs+7],Y)               #39,
ld([sysArgs+6])                 #40,
adda([sysArgs+2],X)             #41,
ld([Y,X])                       #42,
st([0x82])                      #43, key1 = is[i]
ld([sysArgs+2])                 #44,
st([sysArgs+3])                 #45, j = i
ld([vPC])                       #46,
suba(2)                         #47,
st([vPC])                       #48, restart
ld(hi('REENTER'),Y)             #49,
jmp(Y,'REENTER')                #50,
ld(-54/2)                       #51,
label('.sys_sortsprites_44')
ld([sysArgs+0])                 #44,
adda([sysArgs+2],X)             #45,
ld([Y,X])                       #46,
st([sysArgs+4])                 #47, key0 = y[i]
ld([sysArgs+7],Y)               #48,
ld([sysArgs+6])                 #49,
adda([sysArgs+2],X)             #50,
ld([Y,X])                       #51,
st([0x82])                      #52, key1 = is[i]
ld([sysArgs+2])                 #53,
st([sysArgs+3])                 #54, j = i
ld([vPC])                       #55,
suba(2)                         #56,
st([vPC])                       #57, restart
ld(hi('NEXTY'),Y)               #58,
jmp(Y,'NEXTY')                  #59,
ld(-62/2)                       #60,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x1D00)
#-----------------------------------------------------------------------

# sys_DrawSprite
label('sys_DrawSprite')
ld([0x83],Y)                    #18, spriteY

ld([Y,X])                       #19, get back0
ld([0x87],Y)                    #20, spriteData
st([Y,0xA2])                    #21, save back0
ld([Y,0xA3])                    #22, load pixel0
ld([0x83],Y)                    #23, spriteY
bne(pc()+3)                     #24, colourkey
bra(pc()+3)                     #25,
ld([Y,X])                       #26,
nop()                           #26, (!)
st([Y,Xpp])                     #27, draw pixel0

ld([Y,X])                       #28, get back1
ld([0x87],Y)                    #29, spriteData
st([Y,0xA4])                    #30, save back1
ld([Y,0xA5])                    #31, load pixel1
ld([0x83],Y)                    #32, spriteY
bne(pc()+3)                     #33, colourkey
bra(pc()+3)                     #34,
ld([Y,X])                       #35,
nop()                           #35, (!)
st([Y,Xpp])                     #36, draw pixel1

ld([Y,X])                       #37, get back2
ld([0x87],Y)                    #38, spriteData
st([Y,0xA6])                    #39, save back2
ld([Y,0xA7])                    #40, load pixel2
ld([0x83],Y)                    #41, spriteY
bne(pc()+3)                     #42, colourkey
bra(pc()+3)                     #43,
ld([Y,X])                       #44,
nop()                           #44, (!)
st([Y,Xpp])                     #45, draw pixel2

ld([Y,X])                       #46, get back3
ld([0x87],Y)                    #47, spriteData
st([Y,0xA8])                    #48, save back3
ld([Y,0xA9])                    #49, load pixel3
ld([0x83],Y)                    #50, spriteY
bne(pc()+3)                     #51, colourkey
bra(pc()+3)                     #52,
ld([Y,X])                       #53,
nop()                           #53, (!)
st([Y,Xpp])                     #54, draw pixel3

ld([Y,X])                       #55, get back4
ld([0x87],Y)                    #56, spriteData
st([Y,0xAA])                    #57, save back4
ld([Y,0xAB])                    #58, load pixel4
ld([0x83],Y)                    #59, spriteY
bne(pc()+3)                     #60, colourkey
bra(pc()+3)                     #61,
ld([Y,X])                       #62,
nop()                           #62, (!)
st([Y,Xpp])                     #63, draw pixel4

ld([Y,X])                       #64, get back5
ld([0x87],Y)                    #65, spriteData
st([Y,0xAC])                    #66, save back5
ld([Y,0xAD])                    #67, load pixel5
ld([0x83],Y)                    #68, spriteY
bne(pc()+3)                     #69, colourkey
bra(pc()+3)                     #70,
ld([Y,X])                       #71,
nop()                           #71, (!)
st([Y,Xpp])                     #72, draw pixel5

ld([Y,X])                       #73, get back6
ld([0x87],Y)                    #74, spriteData
st([Y,0xAE])                    #75, save back6
ld([Y,0xAF])                    #76, load pixel6
ld([0x83],Y)                    #77, spriteY
bne(pc()+3)                     #78, colourkey
bra(pc()+3)                     #79,
ld([Y,X])                       #80,
nop()                           #80, (!)
st([Y,Xpp])                     #81, draw pixel6

ld([Y,X])                       #82, get back7
ld([0x87],Y)                    #83, spriteData
st([Y,0xB0])                    #84, save back7
ld([Y,0xB1])                    #85, load pixel7 
ld([0x83],Y)                    #86, spriteY
bne(pc()+3)                     #87, colourkey
bra(pc()+3)                     #88,
ld([Y,X])                       #89,
nop()                           #89, (!)
st([Y,Xpp])                     #90, draw pixel7

ld([Y,X])                       #91, get back8
ld([0x87],Y)                    #92, spriteData
st([Y,0xB2])                    #93, save back8
ld([Y,0xB3])                    #94, load pixel8
ld([0x83],Y)                    #95, spriteY
bne(pc()+3)                     #96, colourkey
bra(pc()+3)                     #97,
ld([Y,X])                       #98,
nop()                           #98, (!)
st([Y,Xpp])                     #99, draw pixel8

ld([0x87],Y)                    #100, spriteData
ld([0x82])                      #101, spriteX
st([Y,0xA0])                    #102, spriteOldX
ld([0x83])                      #103, spriteY
st([Y,0xA1])                    #104, spriteOldY
ld([0x86])                      #105,
suba(1)                         #106,
beq('.sys_drawsprite_109')      #107,
st([0x86])                      #108, scanLines--
ld([0x83])                      #109, spriteY
adda(1)                         #110,
st([0x83])                      #111, spriteY++
ld([0x87])                      #112,
adda(1)                         #113,
st([0x87])                      #114, spriteData++
ld([vPC])                       #115,
suba(2)                         #116,
st([vPC])                       #117, restart
ld(hi('NEXTY'),Y)               #118,
jmp(Y,'NEXTY')                  #119,
ld(-122/2)                      #120,
label('.sys_drawsprite_109')
ld([0x9C])                      #109,
adda(1)                         #110,
st([0x9C],X)                    #111, register13++
ld([0x9D],Y)                    #112,
ld([Y,X])                       #113, peek(register13)
st([0x82])                      #114, spriteX
ld([0x9E])                      #115,
adda(1)                         #116,
st([0x9E],X)                    #117, register14++
ld([0x9F],Y)                    #118,
ld([Y,X])                       #119, peek(register14)
st([0x83])                      #120, spriteY
ld([0xA0])                      #121,
adda(1)                         #122,
st([0xA0],X)                    #123, register15++
ld([0xA1],Y)                    #124,
ld([Y,X])                       #125, peek(register15)
st([0x87])                      #126, spriteData
ld(hi('REENTER'),Y)             #127,
jmp(Y,'REENTER')                #128,
ld(-132/2)                      #129,


# sys_DrawVLine, sysArgs0,1=count:colour, sysArgs2,3=X:Y address
label('sys_DrawVLine')
ld([sysArgs+2],X)               #18,
ld([sysArgs+0])                 #19,
suba(8)                         #20,
blt('.sys_drawvline_23')        #21,
ld([sysArgs+0])                 #22,
ld([sysArgs+1])                 #23, 8 pixels
st([Y,X])                       #24,
ld([sysArgs+3])                 #25,
adda(1,Y)                       #26,
ld([sysArgs+1])                 #27,
st([Y,X])                       #28,
ld([sysArgs+3])                 #29,
adda(2,Y)                       #30,
ld([sysArgs+1])                 #31,
st([Y,X])                       #32,
ld([sysArgs+3])                 #33,
adda(3,Y)                       #34,
ld([sysArgs+1])                 #35,
st([Y,X])                       #36,
ld([sysArgs+3])                 #37,
adda(4,Y)                       #38,
ld([sysArgs+1])                 #39,
st([Y,X])                       #40,
ld([sysArgs+3])                 #41,
adda(5,Y)                       #42,
ld([sysArgs+1])                 #43,
st([Y,X])                       #44,
ld([sysArgs+3])                 #45,
adda(6,Y)                       #46,
ld([sysArgs+1])                 #47,
st([Y,X])                       #48,
ld([sysArgs+3])                 #49,
adda(7,Y)                       #50,
adda(8)                         #51,
st([sysArgs+3])                 #52,
ld([sysArgs+1])                 #53,
st([Y,X])                       #54,
ld([sysArgs+0])                 #55,
suba(8)                         #56,
st([sysArgs+0])                 #57,
ld([vPC])                       #58,
suba(2)                         #59,
st([vPC])                       #60, restart
ld(hi('REENTER'),Y)             #61,
jmp(Y,'REENTER')                #62,
ld(-66/2)                       #63,
label('.sys_drawvline_23')
suba(4)                         #23,
blt('.sys_drawvline_26')        #24,
ld([sysArgs+0])                 #25,
ld([sysArgs+1])                 #26, 4 pixels
st([Y,X])                       #27,
ld([sysArgs+3])                 #28,
adda(1,Y)                       #29,
ld([sysArgs+1])                 #30,
st([Y,X])                       #31,
ld([sysArgs+3])                 #32,
adda(2,Y)                       #33,
ld([sysArgs+1])                 #34,
st([Y,X])                       #35,
ld([sysArgs+3])                 #36,
adda(3,Y)                       #37,
adda(4)                         #38,
st([sysArgs+3])                 #39,
ld([sysArgs+1])                 #40,
st([Y,X])                       #41,
ld([sysArgs+0])                 #42,
suba(4)                         #43,
st([sysArgs+0])                 #44,
ld([vPC])                       #45,
suba(2)                         #46,
st([vPC])                       #47, restart
ld(hi('NEXTY'),Y)               #48,
jmp(Y,'NEXTY')                  #49,
ld(-52/2)                       #50,
label('.sys_drawvline_26')
suba(2)                         #26,
blt('.sys_drawvline_29')        #27,
ld([sysArgs+0])                 #28,
ld([sysArgs+1])                 #29, 2 pixels
st([Y,X])                       #30,
ld([sysArgs+3])                 #31,
adda(1,Y)                       #32,
adda(2)                         #33,
st([sysArgs+3])                 #34,
ld([sysArgs+1])                 #35,
st([Y,X])                       #36,
ld([sysArgs+0])                 #37,
suba(2)                         #38,
st([sysArgs+0])                 #39,
ld([vPC])                       #40,
suba(2)                         #41,
st([vPC])                       #42, restart
ld(hi('REENTER'),Y)             #43,
jmp(Y,'REENTER')                #44,
ld(-48/2)                       #45,
label('.sys_drawvline_29')
suba(1)                         #29,
blt('.sys_drawvline_32')        #30,
ld([sysArgs+0])                 #31,
ld([sysArgs+1])                 #32, 1 pixel
st([Y,X])                       #33,
ld([sysArgs+0])                 #34,
suba(1)                         #35,
st([sysArgs+0])                 #36,
ld([vPC])                       #37,
suba(2)                         #38,
st([vPC])                       #39, restart
ld(hi('NEXTY'),Y)               #40,
jmp(Y,'NEXTY')                  #41,
ld(-44/2)                       #42,
label('.sys_drawvline_32')
ld(hi('NEXTY'),Y)               #32, done
jmp(Y,'NEXTY')                  #33,
ld(-36/2)                       #34,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x1E00)
#-----------------------------------------------------------------------

# sys_SpritePattern, sysArgs0,1=src, sysArgs2=dstY, sysArgs3=height/2, sysArgs4,5=patternLut
label('sys_SpritePattern')
ld([sysArgs+1],Y)               #18, src.hi
ld([Y,X])                       #19,
st([Y,Xpp])                     #20, src.lo++
ld([sysArgs+2],Y)               #21,
st([Y,0xA3])                    #22, pixel0
ld([sysArgs+1],Y)               #23, src.hi
ld([Y,X])                       #24,
st([Y,Xpp])                     #25, src.lo++
ld([sysArgs+2],Y)               #26,
st([Y,0xA5])                    #27, pixel1
ld([sysArgs+1],Y)               #28, src.hi
ld([Y,X])                       #29,
st([Y,Xpp])                     #30, src.lo++
ld([sysArgs+2],Y)               #31,
st([Y,0xA7])                    #32, pixel2
ld([sysArgs+1],Y)               #33, src.hi
ld([Y,X])                       #34,
st([Y,Xpp])                     #35, src.lo++
ld([sysArgs+2],Y)               #36,
st([Y,0xA9])                    #37, pixel3
ld([sysArgs+1],Y)               #38, src.hi
ld([Y,X])                       #39,
st([Y,Xpp])                     #40, src.lo++
ld([sysArgs+2],Y)               #41,
st([Y,0xAB])                    #42, pixel4
ld([sysArgs+1],Y)               #43, src.hi
ld([Y,X])                       #44,
st([Y,Xpp])                     #45, src.lo++
ld([sysArgs+2],Y)               #46,
st([Y,0xAD])                    #47, pixel5
ld([sysArgs+1],Y)               #48, src.hi
ld([Y,X])                       #49,
st([Y,Xpp])                     #50, src.lo++
ld([sysArgs+2],Y)               #51,
st([Y,0xAF])                    #52, pixel6
ld([sysArgs+1],Y)               #53, src.hi
ld([Y,X])                       #54,
st([Y,Xpp])                     #55, src.lo++
ld([sysArgs+2],Y)               #56,
st([Y,0xB1])                    #57, pixel7
ld([sysArgs+1],Y)               #58, src.hi
ld([Y,X])                       #59,
st([Y,Xpp])                     #60, src.lo++
ld([sysArgs+2],Y)               #61,
st([Y,0xB3])                    #62, pixel8
ld([sysArgs+2])                 #63,
adda(1)                         #64,
st([sysArgs+2])                 #65, dstY++
ld([sysArgs+1],Y)               #66, src.hi
ld([Y,X])                       #67,
st([Y,Xpp])                     #68, src.lo++
ld([sysArgs+2],Y)               #69,
st([Y,0xA3])                    #70, pixel9
ld([sysArgs+1],Y)               #71, src.hi
ld([Y,X])                       #72,
st([Y,Xpp])                     #73, src.lo++
ld([sysArgs+2],Y)               #74,
st([Y,0xA5])                    #75, pixel10
ld([sysArgs+1],Y)               #76, src.hi
ld([Y,X])                       #77,
st([Y,Xpp])                     #78, src.lo++
ld([sysArgs+2],Y)               #79,
st([Y,0xA7])                    #80, pixel11
ld([sysArgs+1],Y)               #81, src.hi
ld([Y,X])                       #82,
st([Y,Xpp])                     #83, src.lo++
ld([sysArgs+2],Y)               #84,
st([Y,0xA9])                    #85, pixel12
ld([sysArgs+1],Y)               #86, src.hi
ld([Y,X])                       #87,
st([Y,Xpp])                     #88, src.lo++
ld([sysArgs+2],Y)               #89,
st([Y,0xAB])                    #90, pixel13
ld([sysArgs+1],Y)               #91, src.hi
ld([Y,X])                       #92,
st([Y,Xpp])                     #93, src.lo++
ld([sysArgs+2],Y)               #94,
st([Y,0xAD])                    #95, pixel14
ld([sysArgs+1],Y)               #96, src.hi
ld([Y,X])                       #97,
st([Y,Xpp])                     #98, src.lo++
ld([sysArgs+2],Y)               #99,
st([Y,0xAF])                    #100, pixel15
ld([sysArgs+1],Y)               #101, src.hi
ld([Y,X])                       #102,
st([Y,Xpp])                     #103, src.lo++
ld([sysArgs+2],Y)               #104,
st([Y,0xB1])                    #105, pixel16
ld([sysArgs+1],Y)               #106, src.hi
ld([Y,X])                       #107,
st([Y,Xpp])                     #108, src.lo++
ld([sysArgs+2],Y)               #109,
st([Y,0xB3])                    #110, pixel17
ld([sysArgs+2])                 #111,
adda(1)                         #112,
st([sysArgs+2])                 #113, dstY++

ld([sysArgs+4])                 #114,
adda(2)                         #115,
st([sysArgs+4],X)               #116, patternLut += 2
ld([sysArgs+5],Y)               #117,
ld([Y,X])                       #118,
st([Y,Xpp])                     #119,
st([sysArgs+0])                 #120, src.lo
ld([Y,X])                       #121,
st([sysArgs+1])                 #122, src.hi

ld([sysArgs+3])                 #123,
suba(1)                         #124,
st([sysArgs+3])                 #125, height/2 --
beq('.sys_spritepattern_128')   #126,
ld([vPC])                       #127,
suba(2)                         #128,
st([vPC])                       #129, restart
ld(hi('NEXTY'),Y)               #130,
jmp(Y,'NEXTY')                  #131,
ld(-134/2)                      #132,

label('.sys_spritepattern_128')
ld(hi('NEXTY'),Y)               #128,
jmp(Y,'NEXTY')                  #129,
ld(-132/2)                      #130,


# sys_DrawBullet
label('sys_DrawBullet')
ld([0x83],Y)                    #18, bulletY
ld([Y,X])                       #19, AC = screen
ld([0x87],Y)                    #20, bulletData
xora([Y,0xB4])                  #21, AC ^= bullet
ld([0x83],Y)                    #22, bulletY
st([Y,Xpp])                     #23, screen = AC

ld([Y,X])                       #24, AC = screen
ld([0x87],Y)                    #25, bulletData
xora([Y,0xB5])                  #26, AC ^= bullet
ld([0x83],Y)                    #27, bulletY
st([Y,Xpp])                     #28, screen = AC

ld([Y,X])                       #29, AC = screen
ld([0x87],Y)                    #30, bulletData
xora([Y,0xB6])                  #31, AC ^= bullet
ld([0x83],Y)                    #32, bulletY
st([Y,Xpp])                     #33, screen = AC

ld([Y,X])                       #34, AC = screen
ld([0x87],Y)                    #35, bulletData
xora([Y,0xB7])                  #36, AC ^= bullet
ld([0x83],Y)                    #37, bulletY
st([Y,Xpp])                     #38, screen = AC

ld([0x82],X)                    #39, bulletX
ld([0x83])                      #40,
adda(1)                         #41,
st([0x83],Y)                    #42, bulletY++

ld([Y,X])                       #43, AC = screen
ld([0x87],Y)                    #44, bulletData
xora([Y,0xB8])                  #45, AC ^= bullet
ld([0x83],Y)                    #46, bulletY
st([Y,Xpp])                     #47, screen = AC

ld([Y,X])                       #48, AC = screen
ld([0x87],Y)                    #49, bulletData
xora([Y,0xB9])                  #50, AC ^= bullet
ld([0x83],Y)                    #51, bulletY
st([Y,Xpp])                     #52, screen = AC

ld([Y,X])                       #53, AC = screen
ld([0x87],Y)                    #54, bulletData
xora([Y,0xBA])                  #55, AC ^= bullet
ld([0x83],Y)                    #56, bulletY
st([Y,Xpp])                     #57, screen = AC

ld([Y,X])                       #58, AC = screen
ld([0x87],Y)                    #59, bulletData
xora([Y,0xBB])                  #60, AC ^= bullet
ld([0x83],Y)                    #61, bulletY
st([Y,Xpp])                     #62, screen = AC

ld([0x82],X)                    #63, bulletX
ld([0x83])                      #64,
adda(1)                         #65,
st([0x83],Y)                    #66, bulletY++

ld([Y,X])                       #67, AC = screen
ld([0x87],Y)                    #68, bulletData
xora([Y,0xBC])                  #69, AC ^= bullet
ld([0x83],Y)                    #70, bulletY
st([Y,Xpp])                     #71, screen = AC

ld([Y,X])                       #72, AC = screen
ld([0x87],Y)                    #73, bulletData
xora([Y,0xBD])                  #74, AC ^= bullet
ld([0x83],Y)                    #75, bulletY
st([Y,Xpp])                     #76, screen = AC

ld([Y,X])                       #77, AC = screen
ld([0x87],Y)                    #78, bulletData
xora([Y,0xBE])                  #79, AC ^= bullet
ld([0x83],Y)                    #80, bulletY
st([Y,Xpp])                     #81, screen = AC

ld([Y,X])                       #82, AC = screen
ld([0x87],Y)                    #83, bulletData
xora([Y,0xBF])                  #84, AC ^= bullet
ld([0x83],Y)                    #85, bulletY
st([Y,Xpp])                     #86, screen = AC

ld([0x82],X)                    #87, bulletX
ld([0x83])                      #88,
adda(1)                         #89,
st([0x83],Y)                    #90, bulletY++

ld([Y,X])                       #91, AC = screen
ld([0x87],Y)                    #92, bulletData
xora([Y,0xC0])                  #93, AC ^= bullet
ld([0x83],Y)                    #94, bulletY
st([Y,Xpp])                     #95, screen = AC

ld([Y,X])                       #96, AC = screen
ld([0x87],Y)                    #97, bulletData
xora([Y,0xC1])                  #98, AC ^= bullet
ld([0x83],Y)                    #99, bulletY
st([Y,Xpp])                     #100, screen = AC

ld([Y,X])                       #101, AC = screen
ld([0x87],Y)                    #102, bulletData
xora([Y,0xC2])                  #103, AC ^= bullet
ld([0x83],Y)                    #104, bulletY
st([Y,Xpp])                     #105, screen = AC

ld([Y,X])                       #106, AC = screen
ld([0x87],Y)                    #107, bulletData
xora([Y,0xC3])                  #108, AC ^= bullet
ld([0x83],Y)                    #109, bulletY
st([Y,Xpp])                     #110, screen = AC

ld([0x9C])                      #111, register13
adda(1)                         #112,
st([0x9C],X)                    #113, register13++
ld([0x9D],Y)                    #114,
ld([Y,X])                       #115, peek(register13)
st([0x82])                      #116, bulletX

ld([0x9E])                      #117,
adda(1)                         #118,
st([0x9E],X)                    #119, register14++
ld([0x9F],Y)                    #120,
ld([Y,X])                       #121, peek(register14)
st([0x83])                      #122, bulletY

ld([0xA0])                      #123,
adda(1)                         #124,
st([0xA0],X)                    #125, register15++
ld([0xA1],Y)                    #126,
ld([Y,X])                       #127, peek(register15)
st([0x87])                      #128, bulletData

ld([0x9A])                      #129,
suba(1)                         #130,
beq('.sys_drawbullet_133')      #131,
st([0x9A])                      #132, register12--
ld(hi('NEXTY'),Y)               #133,
ld([vPC])                       #134,
suba(2)                         #135,
st([vPC])                       #136, restart
jmp(Y,'NEXTY')                  #137,
ld(-140/2)                      #138,
label('.sys_drawbullet_133')
ld(hi('REENTER'),Y)             #133,
jmp(Y,'REENTER')                #134,
ld(-138/2)                      #135,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x1F00)
#-----------------------------------------------------------------------

# sys_CmpByteBounds, 0xB8,B9=src, 0xBA,BB=vPC_bounds, 0xBC,BD=bounds, 0xBE=index,
#                    0xBF=count, output:vAC.lo=index
label('sys_CmpByteBounds')
suba(1)                         #18,
bge('.sys_cmpbytebounds_21')    #19,
st([0xBF])                      #20, count--
ld(hi('REENTER'),Y)             #21,
jmp(Y,'REENTER')                #22,
ld(-26/2)                       #23,
label('.sys_cmpbytebounds_21')
ld([0xB8])                      #21,
adda([0xBE],X)                  #22,
ld([0xB9],Y)                    #23,
ld([Y,X])                       #24,
bmi('.sys_cmpbytebounds_27')    #25,
suba([0xBC])                    #26,
bra('.sys_cmpbytebounds_29')    #27,
ora([0xBC])                     #28,
label('.sys_cmpbytebounds_27')
anda([0xBC])                    #27,
nop()                           #28,

label('.sys_cmpbytebounds_29')
bpl('.sys_cmpbytebounds_31')    #29,
ld([Y,X])                       #30, 
ld([vPC])                       #31, < lbounds
st([vLR])                       #32, return to SYS call
ld([vPC+1])                     #33,
st([vLR+1])                     #34,
ld([0xBA])                      #35,
suba(2)                         #36,
st([vPC])                       #37, NEXT adds 2
ld([0xBB])                      #38,
st([vPC+1])                     #39, bounds call address
ld([0xBE])                      #40,
st([vAC])                       #41, vAC.lo = index
adda(1)                         #42,
st([0xBE])                      #43, index++
ld(hi('NEXTY'),Y)               #44,
jmp(Y,'NEXTY')                  #45,
ld(-48/2)                       #46,

label('.sys_cmpbytebounds_31')
bmi('.sys_cmpbytebounds_33')    #31,
suba([0xBD])                    #32,
bra('.sys_cmpbytebounds_35')    #33,
ora([0xBD])                     #34,
label('.sys_cmpbytebounds_33')
anda([0xBD])                    #33,
nop()                           #34,
label('.sys_cmpbytebounds_35')
bmi('.sys_cmpbytebounds_37')    #35,
ld([vPC])                       #36, >= ubounds
st([vLR])                       #37, return to SYS call
ld([vPC+1])                     #38,
st([vLR+1])                     #39,
ld([0xBA])                      #40,
suba(2)                         #41,
st([vPC])                       #42, NEXT adds 2
ld([0xBB])                      #43,
st([vPC+1])                     #44, bounds call address
ld([0xBE])                      #45,
st([vAC])                       #46, vAC.lo = index
adda(1)                         #47,
st([0xBE])                      #48, index++
ld(hi('REENTER'),Y)             #49,
jmp(Y,'REENTER')                #50,
ld(-54/2)                       #51,

label('.sys_cmpbytebounds_37')
suba(2)                         #37,
st([vPC])                       #38, restart
ld([0xBE])                      #39,
adda(1)                         #40,
st([0xBE])                      #41, index++
ld(hi('NEXTY'),Y)               #42,
jmp(Y,'NEXTY')                  #43,
ld(-46/2)                       #44,


# SYS_Reset_88 imolementation
label('sys_Reset_88')
st([romType])                   #18
ld(0)                           #19
st([vSP])                       #20 vSP
st([vSPH])                      #21 vSPH
ld(hi('videoTop_v5'),Y)         #22
st([Y,lo('videoTop_v5')])       #23 Show all 120 pixel lines
st([Y,vIRQ_v5])                 #24 Disable vIRQ dispatch
st([Y,vIRQ_v5+1])               #25
st([soundTimer])                #26 soundTimer
assert userCode&255 == 0
st([vLR])                       #27 vLR
ld(userCode>>8)                 #28
st([vLR+1])                     #29
ld('nopixels')                  #30 Video mode 3 (fast)
st([videoModeB])                #31
st([videoModeC])                #32
st([videoModeD])                #33
ld('SYS_Exec_88')               #34 SYS_Exec_88
st([sysFn])                     #35 High byte (remains) 0
ld('Reset')                     #36 Reset.gt1 from EPROM
st([sysArgs+0])                 #37
ld(hi('Reset'))                 #38
st([sysArgs+1])                 #39
ld([vPC])                       #40 Force second SYS call
suba(2)                         #41
st([vPC])                       #42
ctrl(0b01111111)                #43 Expansion board: (1) reset signal.
ctrl(0b01111100)                #44 (2) disable SPI slaves, enable RAM bank 1
ld(hi('REENTER'),Y)             #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47


#-----------------------------------------------------------------------
# sys_Divide_u168, x:u8 = x:u16 / y:u8, rem:u8 = x:u16 % y:u8
# sysArgs0,1=x, sysArgs2=y, sysArgs4=count, output : sysArgs0=res, sysArgs1=rem
label('sys_Divide_u168')
st([sysArgs+5])                 #18, save x.hi
ld([sysArgs+4])                 #19,
suba(1)                         #20, count--
bge('.sys_divide_u168_23')      #21,
st([sysArgs+4])                 #22,
ld(hi('REENTER'),Y)             #23,
jmp(Y,'REENTER')                #24,
ld(-28/2)                       #25,

label('.sys_divide_u168_23')
ld([sysArgs+0])                 #23, 
anda(128,X)                     #24,
adda([sysArgs+0])               #25,
st([sysArgs+0])                 #26,
ld([X])                         #27,
adda([sysArgs+1])               #28,
adda([sysArgs+1])               #29,
st([sysArgs+1])                 #30, x <<1
bmi('.sys_divide_u168_33')      #31, unsigned compare
suba([sysArgs+2])               #32, x.hi - y
st([vTmp])                      #33,
bra('.sys_divide_u168_36')      #34,
ora([sysArgs+2])                #35,
label('.sys_divide_u168_33')
st([vTmp])                      #33,
anda([sysArgs+2])               #34,
nop()                           #35,

label('.sys_divide_u168_36')
bmi('.sys_divide_u168_38')      #36,
ld([vTmp])                      #37, x.hi >= y
st([sysArgs+1])                 #38, x.hi = (x.hi - y) <<8
ld([sysArgs+0])                 #39,
adda(1)                         #40, x.lo++
bne('.sys_divide_u168_43')      #41,
st([sysArgs+0])                 #42,
ld([sysArgs+1])                 #43,
adda(1)                         #44, x.hi++
st([sysArgs+1])                 #45,
ld([vPC])                       #46,
suba(2)                         #47,
st([vPC])                       #48, restart
ld(hi('REENTER'),Y)             #49,
jmp(Y,'REENTER')                #50,
ld(-54/2)                       #51,
label('.sys_divide_u168_43')
ld([vPC])                       #43,
suba(2)                         #44,
st([vPC])                       #45, restart
ld(hi('NEXTY'),Y)               #46,
jmp(Y,'NEXTY')                  #47,
ld(-50/2)                       #48,

label('.sys_divide_u168_38')
ld([sysArgs+5])                 #38,
bpl('.sys_divide_u168_41')      #39, msb(x.hi) = 0
ld([vTmp])                      #40, x.hi >= y
st([sysArgs+1])                 #41, x.hi = (x.hi - y) <<8
ld([sysArgs+0])                 #42,
adda(1)                         #43, x.lo++
bne('.sys_divide_u168_46')      #44,
st([sysArgs+0])                 #45,
ld([sysArgs+1])                 #46,
adda(1)                         #47, x.hi++
st([sysArgs+1])                 #48,
ld([vPC])                       #49,
suba(2)                         #50,
st([vPC])                       #51, restart
ld(hi('NEXTY'),Y)               #52,
jmp(Y,'NEXTY')                  #53,
ld(-56/2)                       #54,
label('.sys_divide_u168_46')
ld([vPC])                       #46,
suba(2)                         #47,
st([vPC])                       #48, restart
ld(hi('REENTER'),Y)             #49,
jmp(Y,'REENTER')                #50,
ld(-54/2)                       #51,

label('.sys_divide_u168_41')
ld([vPC])                       #41,
suba(2)                         #42,
st([vPC])                       #43, restart
ld(hi('NEXTY'),Y)               #44,
jmp(Y,'NEXTY')                  #45,
ld(-48/2)                       #46,


fillers(until=0xc0)

# sys_ConvertVTableX, sysArgs0=dst, sysArgs2=offsetX, sysArgs3=accum, sysArgs4=count
label('SYS_ConvertVTableX_66')
ld(1,Y)                         #15, VTable.hi
ld([sysArgs+2],X)               #16, VTable.lo
ld([Y,X])                       #17, get VTableX
adda([sysArgs+3])               #18,
ld([sysArgs+1],Y)               #19, dst.hi
ld([sysArgs+0],X)               #20, dst.lo
st([Y,X])                       #21,
st([sysArgs+3])                 #22, accum

ld(1,Y)                         #23
ld([sysArgs+0])                 #24,
adda(1)                         #25,
st([sysArgs+0])                 #26, dst++
ld([sysArgs+2])                 #27,
adda(2)                         #28,
st([sysArgs+2],X)               #29, offsetX += 2
ld([Y,X])                       #30, get video table X
adda([sysArgs+3])               #31,
ld([sysArgs+1],Y)               #32, dst.hi
ld([sysArgs+0],X)               #33, dst.lo
st([Y,X])                       #34,
st([sysArgs+3])                 #35, accum

ld(1,Y)                         #36
ld([sysArgs+0])                 #37,
adda(1)                         #38,
st([sysArgs+0])                 #39, dst++
ld([sysArgs+2])                 #40,
adda(2)                         #41,
st([sysArgs+2],X)               #42, offsetX += 2
ld([Y,X])                       #43, get video table X
adda([sysArgs+3])               #44,
ld([sysArgs+1],Y)               #45, dst.hi
ld([sysArgs+0],X)               #46, dst.lo
st([Y,X])                       #47,
st([sysArgs+3])                 #48, accum

ld([sysArgs+4])                 #49,
suba(1)                         #50,
st([sysArgs+4])                 #51, count--
beq('.sys_convertvtablex_54')   #52,
ld([sysArgs+0])                 #53,
adda(1)                         #54,
st([sysArgs+0])                 #55, dst++
ld([sysArgs+2])                 #56,
adda(2)                         #57,
st([sysArgs+2])                 #58, offsetX += 2
ld([vPC])                       #59,
suba(2)                         #60,
st([vPC])                       #61, restart
ld(hi('NEXTY'),Y)               #62,
jmp(Y,'NEXTY')                  #63,
ld(-66/2)                       #64,
label('.sys_convertvtablex_54')
ld(hi('NEXTY'),Y)               #54,
jmp(Y,'NEXTY')                  #55,
ld(-58/2)                       #56,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x2000)
#-----------------------------------------------------------------------

# SYS_DrawSpriteH_vX_140
label('SYS_DrawSpriteH_vX_140')
ld([0x86])                      #15, spriteEnable
bmi('.sys_drawspriteh_18')      #16,
ld([0x83])                      #17, spriteY
adda([0x83])                    #18, spriteY
ld(AC,X)                        #19,
ld(1,Y)                         #20, 0x0100
ld([Y,X])                       #21, VTableY
st([0x85])                      #22,
ld([sysArgs+1],Y)               #23, VTableAbsX.hi
ld([sysArgs+0])                 #24, VTableAbsX.lo
adda([0x83])                    #25, spriteY
ld(AC,X)                        #26,
ld([0x82])                      #27, spriteX
adda([Y,X])                     #28, VTableX offset, (absolute)
st([0x84],X)                    #29,
ld([0x85],Y)                    #30,

ld([Y,X])                       #31, get back0
ld([0x87],Y)                    #32, spriteData
st([Y,0xA2])                    #33, save back0
ld([Y,0xA3])                    #34, load pixel0
ld([0x85],Y)                    #35, spriteY
bne(pc()+3)                     #36, colourkey
bra(pc()+3)                     #37,
ld([Y,X])                       #38,
nop()                           #38, (!)
st([Y,Xpp])                     #39, draw pixel0

ld([Y,X])                       #40, get back1
ld([0x87],Y)                    #41, spriteData
st([Y,0xA4])                    #42, save back1
ld([Y,0xA5])                    #43, load pixel1
ld([0x85],Y)                    #44, spriteY
bne(pc()+3)                     #45, colourkey
bra(pc()+3)                     #46,
ld([Y,X])                       #47,
nop()                           #47, (!)
st([Y,Xpp])                     #48, draw pixel1

ld([Y,X])                       #49, get back2
ld([0x87],Y)                    #50, spriteData
st([Y,0xA6])                    #51, save back2
ld([Y,0xA7])                    #52, load pixel2
ld([0x85],Y)                    #53, spriteY
bne(pc()+3)                     #54, colourkey
bra(pc()+3)                     #55,
ld([Y,X])                       #56,
nop()                           #56, (!)
st([Y,Xpp])                     #57, draw pixel2

ld([Y,X])                       #58, get back3
ld([0x87],Y)                    #59, spriteData
st([Y,0xA8])                    #60, save back3
ld([Y,0xA9])                    #61, load pixel3
ld([0x85],Y)                    #62, spriteY
bne(pc()+3)                     #63, colourkey
bra(pc()+3)                     #64,
ld([Y,X])                       #65,
nop()                           #65, (!)
st([Y,Xpp])                     #66, draw pixel3

ld([Y,X])                       #67, get back4
ld([0x87],Y)                    #68, spriteData
st([Y,0xAA])                    #69, save back4
ld([Y,0xAB])                    #70, load pixel4
ld([0x85],Y)                    #71, spriteY
bne(pc()+3)                     #72, colourkey
bra(pc()+3)                     #73,
ld([Y,X])                       #74,
nop()                           #74, (!)
st([Y,Xpp])                     #75, draw pixel4

ld([Y,X])                       #76, get back5
ld([0x87],Y)                    #77, spriteData
st([Y,0xAC])                    #78, save back5
ld([Y,0xAD])                    #79, load pixel5
ld([0x85],Y)                    #80, spriteY
bne(pc()+3)                     #81, colourkey
bra(pc()+3)                     #82,
ld([Y,X])                       #83,
nop()                           #83, (!)
st([Y,Xpp])                     #84, draw pixel5

ld([Y,X])                       #85, get back6
ld([0x87],Y)                    #86, spriteData
st([Y,0xAE])                    #87, save back6
ld([Y,0xAF])                    #88, load pixel6
ld([0x85],Y)                    #89, spriteY
bne(pc()+3)                     #90, colourkey
bra(pc()+3)                     #91,
ld([Y,X])                       #92,
nop()                           #92, (!)
st([Y,Xpp])                     #93, draw pixel6

ld([Y,X])                       #94, get back7
ld([0x87],Y)                    #95, spriteData
st([Y,0xB0])                    #96, save back7
ld([Y,0xB1])                    #97, load pixel7 
ld([0x85],Y)                    #98, spriteY
bne(pc()+3)                     #99, colourkey
bra(pc()+3)                     #100,
ld([Y,X])                       #101,
nop()                           #101, (!)
st([Y,Xpp])                     #102, draw pixel7

ld([Y,X])                       #103, get back8
ld([0x87],Y)                    #104, spriteData
st([Y,0xB2])                    #105, save back8
ld([Y,0xB3])                    #106, load pixel8
ld([0x85],Y)                    #107, spriteY
bne(pc()+3)                     #108, colourkey
bra(pc()+3)                     #109,
ld([Y,X])                       #110,
nop()                           #110, (!)
st([Y,Xpp])                     #111, draw pixel8

ld([0x87],Y)                    #112, spriteData
ld([0x84])                      #113, spriteX
st([Y,0xA0])                    #114, spriteOldX
ld([0x85])                      #115, spriteY
st([Y,0xA1])                    #116, spriteOldY
ld([0x86])                      #117,
suba(1)                         #118,
st([0x86])                      #119, scanLines--
beq('.sys_drawspriteh_122')     #120,
ld([0x83])                      #121, spriteY
adda(1)                         #122,
st([0x83])                      #123, spriteY++
ld([0x87])                      #124,
adda(1)                         #125,
st([0x87])                      #126, spriteData++
ld([vPC])                       #127,
suba(2)                         #128,
st([vPC])                       #129, restart
ld(hi('NEXTY'),Y)               #130,
jmp(Y,'NEXTY')                  #131,
ld(-134/2)                      #132,

label('.sys_drawspriteh_122')
ld([0xA0])                      #122,
adda(4)                         #123,
st([0xA0],X)                    #124, register15 += 4
ld([0xA1],Y)                    #125,
ld([Y,X])                       #126, peek(register15 + 0)
st([0x82])                      #127, spriteX
st([Y,Xpp])                     #128, X++
ld([Y,X])                       #129, peek(register15 + 1)
st([0x83])                      #130, spriteY
st([Y,Xpp])                     #131, X++
ld([Y,X])                       #132, peek(register15 + 2)
st([0x86])                      #133, spriteHeight
st([Y,Xpp])                     #134, X++
ld([Y,X])                       #135, peek(register15 + 3)
st([0x87])                      #136, spriteData
ld([0x88])                      #137,
suba(1)                         #138,
st([0x88])                      #139, register3--, spriteCount
beq('.sys_drawspriteh_142')     #140,
ld([vPC])                       #141,
suba(2)                         #142,
st([vPC])                       #143, restart
ld(hi('NEXTY'),Y)               #144,
jmp(Y,'NEXTY')                  #145, 
ld(-148/2)                      #146,
label('.sys_drawspriteh_142')
ld(hi('NEXTY'),Y)               #142,
jmp(Y,'NEXTY')                  #143,
ld(-146/2)                      #144,

label('.sys_drawspriteh_18')
ld([0xA0])                      #18,
adda(4)                         #19,
st([0xA0],X)                    #20, register15 += 4
ld([0xA1],Y)                    #21,
ld([Y,X])                       #22, peek(register15 + 0)
st([0x82])                      #23, spriteX
st([Y,Xpp])                     #24, X++
ld([Y,X])                       #25, peek(register15 + 1)
st([0x83])                      #26, spriteY
st([Y,Xpp])                     #27, X++
ld([Y,X])                       #28, peek(register15 + 2)
st([0x86])                      #29, spriteHeight
st([Y,Xpp])                     #30, X++
ld([Y,X])                       #31, peek(register15 + 3)
st([0x87])                      #32, spriteData
ld([0x88])                      #33,
suba(1)                         #34,
st([0x88])                      #35, register3--, spriteCount
beq('.sys_drawspriteh_38')      #36,
ld([vPC])                       #37,
suba(2)                         #38,
st([vPC])                       #39, restart
ld(hi('NEXTY'),Y)               #40,
jmp(Y,'NEXTY')                  #41, 
ld(-44/2)                       #42,
label('.sys_drawspriteh_38')
ld(hi('NEXTY'),Y)               #38,
jmp(Y,'NEXTY')                  #39,
ld(-42/2)                       #40,


fillers(until=0xc0)

# SYS_ScrollVTableY_vX_38, sysArgs0=scrollOffset, sysArgs1=scanCount, sysArgs2,3=videoTable
label('SYS_ScrollVTableY_vX_38')
ld([sysArgs+1])                 #15,
suba(1)                         #16,
st([sysArgs+1])                 #17, scanCount--
bge('.sys_scrollvty_20')        #18,
ld([sysArgs+3],Y)               #19, VTable.hi
ld(hi('NEXTY'),Y)               #20,
jmp(Y,'NEXTY')                  #21,
ld(-24/2)                       #22,
label('.sys_scrollvty_20')
ld([sysArgs+2],X)               #20, VTable.low
ld([Y,X])                       #21, scanline = peek [VTable]
adda([sysArgs+0])               #22, scanline += scrollOffset
suba(0x7f)                      #23, scanline -= 0x7f
ble('.sys_scrollvty_26')        #24,
adda(0x7F)                      #25, original scanline
adda(0x88)                      #26, overflow into the range 0x08 to 0x7f
st([Y,X])                       #27,
ld([sysArgs+2])                 #28,
adda(2)                         #29,
st([sysArgs+2])                 #30,
ld([vPC])                       #31,
suba(2)                         #32,
st([vPC])                       #33, restart
ld(hi('NEXTY'),Y)               #34,
jmp(Y,'NEXTY')                  #35,
ld(-38/2)                       #36,

label('.sys_scrollvty_26')
st([Y,X])                       #26,
ld([sysArgs+2])                 #27,
adda(2)                         #28,
st([sysArgs+2])                 #29,
ld([vPC])                       #30,
suba(2)                         #31,
st([vPC])                       #32, restart
ld(hi('REENTER'),Y)             #33,
jmp(Y,'REENTER')                #34,
ld(-38/2)                       #35,


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More sys implementations, (0x2100)
#-----------------------------------------------------------------------
# SYS_RestoreSprite_vX_126
label('SYS_RestoreSprite_vX_126')
ld([0x86])                      #15, spriteEnable
bmi('.sys_restoresprite_18')    #16,
ld([0x87],Y)                    #17, spriteData
ld([Y,0xA0])                    #18, spriteOldX
ld(AC,X)                        #19,
ld([Y,0xA1])                    #20, spriteOldY
st([vTmp])                      #21,

ld([Y,0xA2])                    #22, load back0
ld([vTmp],Y)                    #23, spriteOldY
st([Y,Xpp])                     #24, restore back0

ld([0x87],Y)                    #25, spriteData
ld([Y,0xA4])                    #26, load back1
ld([vTmp],Y)                    #27, spriteOldY
st([Y,Xpp])                     #28, restore back1

ld([0x87],Y)                    #29, spriteData
ld([Y,0xA6])                    #30, load back2
ld([vTmp],Y)                    #31, spriteOldY
st([Y,Xpp])                     #32, restore back2

ld([0x87],Y)                    #33, spriteData
ld([Y,0xA8])                    #34, load back3
ld([vTmp],Y)                    #35, spriteOldY
st([Y,Xpp])                     #36, restore back3

ld([0x87],Y)                    #37, spriteData
ld([Y,0xAA])                    #38, load back4
ld([vTmp],Y)                    #39, spriteOldY
st([Y,Xpp])                     #40, restore back4

ld([0x87],Y)                    #41, spriteData
ld([Y,0xAC])                    #42, load back5
ld([vTmp],Y)                    #43, spriteOldY
st([Y,Xpp])                     #44, restore back5

ld([0x87],Y)                    #45, spriteData
ld([Y,0xAE])                    #46, load back6
ld([vTmp],Y)                    #47, spriteOldY
st([Y,Xpp])                     #48, restore back6

ld([0x87],Y)                    #49, spriteData
ld([Y,0xB0])                    #50, load back7
ld([vTmp],Y)                    #51, spriteOldY
st([Y,Xpp])                     #52, restore back7

ld([0x87],Y)                    #53, spriteData
ld([Y,0xB2])                    #54, load back8
ld([vTmp],Y)                    #55, spriteOldY
st([Y,Xpp])                     #56, restore back8

ld([0x87])                      #57,
adda(1)                         #58,
st([0x87],Y)                    #59, spriteData++

ld([Y,0xA0])                    #60, spriteOldX
ld(AC,X)                        #61,
ld([Y,0xA1])                    #62, spriteOldY
st([vTmp])                      #63,

ld([Y,0xA2])                    #64, load back0
ld([vTmp],Y)                    #65, spriteOldY
st([Y,Xpp])                     #66, restore back0

ld([0x87],Y)                    #67, spriteData
ld([Y,0xA4])                    #68, load back1
ld([vTmp],Y)                    #69, spriteOldY
st([Y,Xpp])                     #70, restore back1

ld([0x87],Y)                    #71, spriteData
ld([Y,0xA6])                    #72, load back2
ld([vTmp],Y)                    #73, spriteOldY
st([Y,Xpp])                     #74, restore back2

ld([0x87],Y)                    #75, spriteData
ld([Y,0xA8])                    #76, load back3
ld([vTmp],Y)                    #77, spriteOldY
st([Y,Xpp])                     #78, restore back3

ld([0x87],Y)                    #79, spriteData
ld([Y,0xAA])                    #80, load back4
ld([vTmp],Y)                    #81, spriteOldY
st([Y,Xpp])                     #82, restore back4

ld([0x87],Y)                    #83, spriteData
ld([Y,0xAC])                    #84, load back5
ld([vTmp],Y)                    #85, spriteOldY
st([Y,Xpp])                     #86, restore back5

ld([0x87],Y)                    #87, spriteData
ld([Y,0xAE])                    #88, load back6
ld([vTmp],Y)                    #89, spriteOldY
st([Y,Xpp])                     #90, restore back6

ld([0x87],Y)                    #91, spriteData
ld([Y,0xB0])                    #92, load back7
ld([vTmp],Y)                    #93, spriteOldY
st([Y,Xpp])                     #94, restore back7

ld([0x87],Y)                    #95, spriteData
ld([Y,0xB2])                    #96, load back8
ld([vTmp],Y)                    #97, spriteOldY
st([Y,Xpp])                     #98, restore back8

ld([0x86])                      #99,
suba(2)                         #100,
st([0x86])                      #101, scanLines -= 2
beq('.sys_restoresprite_104')   #102,
ld([0x87])                      #103,
adda(1)                         #104,
st([0x87])                      #105, spriteData++
ld([vPC])                       #106,
suba(2)                         #107,
st([vPC])                       #108, restart
ld(hi('REENTER'),Y)             #109,
jmp(Y,'REENTER')                #110,
ld(-114/2)                      #111,
label('.sys_restoresprite_104')
ld([0xA0])                      #104,
suba(4)                         #105,
st([0xA0],X)                    #106, register15 -= 4
ld([0xA1],Y)                    #107,
ld([Y,X])                       #108, peek(register15 + 2)
st([0x86])                      #109, spriteHeight
st([Y,Xpp])                     #110, X++
ld([Y,X])                       #111, peek(register15 + 3)
st([0x87])                      #112, spriteData 
ld([0x88])                      #113,
suba(1)                         #114,
st([0x88])                      #115, numSprites--
beq('.sys_restoresprite_118')   #116,
ld([vPC])                       #117,
suba(2)                         #118,
st([vPC])                       #119, restart
ld(hi('NEXTY'),Y)               #120,
jmp(Y,'NEXTY')                  #121,
ld(-124/2)                      #122,
label('.sys_restoresprite_118')
ld(hi('NEXTY'),Y)               #118,
jmp(Y,'NEXTY')                  #119,
ld(-122/2)                      #120,

label('.sys_restoresprite_18')
ld([0xA0])                      #18,
suba(4)                         #19,
st([0xA0],X)                    #20, register15 -= 4
ld([0xA1],Y)                    #21,
ld([Y,X])                       #22, peek(register15 + 2)
st([0x86])                      #23, spriteHeight
st([Y,Xpp])                     #24, X++
ld([Y,X])                       #25, peek(register15 + 3)
st([0x87])                      #26, spriteData 
ld([0x88])                      #27,
suba(1)                         #28,
st([0x88])                      #29, numSprites--
beq('.sys_restoresprite_32')    #30,
ld([vPC])                       #31,
suba(2)                         #32,
st([vPC])                       #33, restart
ld(hi('NEXTY'),Y)               #34,
jmp(Y,'NEXTY')                  #35,
ld(-38/2)                       #36,
label('.sys_restoresprite_32')
ld(hi('NEXTY'),Y)               #32,
jmp(Y,'NEXTY')                  #33,
ld(-36/2)                       #34,


fillers(until=0xa0)

# SYS_ScrollRectVTableY_vX_44, sysArgs0=scrollOffset, sysArgs1=scanCount, sysArgs2,3=videoTable,
#                              sysArgs4,5=scrollStart,scrollEnd
label('SYS_ScrollRectVTableY_vX_44')
ld([sysArgs+1])                 #15,
suba(1)                         #16,
st([sysArgs+1])                 #17, scanCount--
bge('.sys_scrollrectvty_20')    #18,
ld([sysArgs+3],Y)               #19, VTable.hi
ld(hi('NEXTY'),Y)               #20,
jmp(Y,'NEXTY')                  #21,
ld(-24/2)                       #22,
label('.sys_scrollrectvty_20')
ld([sysArgs+2],X)               #20, VTable.low
ld([Y,X])                       #21, scanline = peek [VTable]
adda([sysArgs+0])               #22, scanline += scrollOffset
st([vTmp])                      #23,
suba([sysArgs+4])               #24,
bge('.sys_scrollrectvty_27')    #25, 
adda([sysArgs+5])               #26,
st([Y,X])                       #27,
ld([sysArgs+2])                 #28,
adda(2)                         #29,
st([sysArgs+2])                 #30,
ld([vPC])                       #31,
suba(2)                         #32,
st([vPC])                       #33, restart
ld(hi('NEXTY'),Y)               #34,
jmp(Y,'NEXTY')                  #35,
ld(-38/2)                       #36,

label('.sys_scrollrectvty_27')
ld([vTmp])                      #27,
suba([sysArgs+5])               #28,
blt('.sys_scrollrectvty_31')    #29, 
adda([sysArgs+4])               #30,
st([Y,X])                       #31,
ld([sysArgs+2])                 #32,
adda(2)                         #33,
st([sysArgs+2])                 #34,
ld([vPC])                       #35,
suba(2)                         #36,
st([vPC])                       #37, restart
ld(hi('NEXTY'),Y)               #38,
jmp(Y,'NEXTY')                  #39,
ld(-42/2)                       #40,

label('.sys_scrollrectvty_31')
ld([vTmp])                      #31,
st([Y,X])                       #32,
ld([sysArgs+2])                 #33,
adda(2)                         #34,
st([sysArgs+2])                 #35,
ld([vPC])                       #36,
suba(2)                         #37,
st([vPC])                       #38, restart
ld(hi('REENTER'),Y)             #39,
jmp(Y,'REENTER')                #40,
ld(-44/2)                       #41,


fillers(until=0xff)

#-----------------------------------------------------------------------
#       PREFX3 instruction page, (0x2200), PREFIX ARG1 OPCODE ARG0
#-----------------------------------------------------------------------
# Original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2099#p2099
#
label('PREFX3_PAGE')
bra('.next2')                   #0 Enter at '.next2' (so no startup overhead)
# --- Page boundary ---
align(0x100,size=0x100)
ld([vPC+1],Y)                   #1

# Fetch next instruction and execute it, but only if there are sufficient
# ticks left for the slowest instruction.
adda([vTicks])                  #0 Track elapsed ticks (actually counting down: AC<0)
blt('EXIT')                     #1 Escape near time out
st([vTicks])                    #2
ld([vPC],X)                     #3 PREFIX+ARG1 is 2 bytes, vPC has been incremented by 2
nop()                           #4
st(vCpuSelect,[vCpuSelect])     #5 Reset to default vCPU page
ld([Y,X])                       #6 Fetch opcode (actually a branch target)
st([Y,Xpp])                     #7 Just X++
bra(AC)                         #8 Dispatch
ld([Y,X])                       #9 Prefetch operand

# Resync with video driver and transfer control
adda(maxTicks)                  #3
bgt(pc()&255)                   #4 Resync
suba(1)                         #5
ld(hi('vBlankStart'),Y)         #6
jmp(Y,[vReturn])                #7 To video driver
ld(0)                           #8 AC should be 0 already. Still..
assert vCPU_overhead ==          9

# pc = 0x2211, Opcode = 0x11
# Instruction STB2: Store vAC.lo into 16bit immediate address, 22 + 20 cycles
# Original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2135#p2135
label('STB2')
ld(hi('stb2#13'),Y)             #10
jmp(Y,'stb2#13')                #11
ld(AC,X)                        #12

# pc = 0x2214, Opcode = 0x14
# Instruction STW2: Store vAC into 16bit immediate address, 22 + 22 cycles
# Original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2135#p2135
label('STW2')
ld(hi('stw2#13'),Y)             #10
jmp(Y,'stw2#13')                #11
ld(AC,X)                        #12 

# pc = 0x2217, Opcode = 0x17
# Instruction XCHGB: Exchange two zero byte variables, 22 + 28 cycles
label('XCHGB')
ld(hi('xchgb#13'),Y)            #10
jmp(Y,'xchgb#13')               #11
# dummy                         #12
#
# pc = 0x2219, Opcode = 0x19
# Instruction MOVW: Move 16bits from src zero page var to dst zero page var, 22 + 30 cycles
label('MOVW')
ld(hi('movw#13'),Y)             #10
jmp(Y,'movw#13')                #11
# dummy                         #12
#
# pc = 0x221b, Opcode = 0x1b
# Instruction ADDWI: vAC += immediate 16bit value, 22 + 28 cycles
label('ADDWI')
ld(hi('addwi#13'),Y)            #10 #12
jmp(Y,'addwi#13')               #11
# dummy                         #12
#
# pc = 0x221d, Opcode = 0x1d
# Instruction SUBWI: vAC -= immediate 16bit value, 22 + 28 cycles
label('SUBWI')
ld(hi('subwi#13'),Y)            #10 #12
jmp(Y,'subwi#13')               #11
# dummy                         #12
#
# pc = 0x221f, Opcode = 0x1f
# Instruction ANDWI: vAC &= immediate 16bit value, 22 + 22 cycles
label('ANDWI')
ld(hi('andwi#13'),Y)            #10 #12
jmp(Y,'andwi#13')               #11
# dummy                         #12
#
# pc = 0x2221, Opcode = 0x21
# Instruction ORWI: vAC |= immediate 16bit value, 22 + 22 cycles
label('ORWI')
ld(hi('orwi#13'),Y)             #10 #12
jmp(Y,'orwi#13')                #11
# dummy                         #12
#
# pc = 0x2223, Opcode = 0x23
# Instruction XORWI: vAC ^= immediate 16bit value, 22 + 22 cycles
label('XORWI')
ld(hi('xorwi#13'),Y)            #10 #12
jmp(Y,'xorwi#13')               #11
# dummy                         #12
#
# pc = 0x2225, Opcode = 0x25
# Instruction LDPX: Load Pixel, <address var>, <colour var>, 22 + 30 cycles
label('LDPX')
ld(hi('ldpx#13'),Y)             #10 #12
jmp(Y,'ldpx#13')                #11
st([vTmp])                      #12

# pc = 0x2228, Opcode = 0x28
# Instruction STPX: Store Pixel, <address var>, <colour var>, 22 + 30 cycles
label('STPX')
ld(hi('stpx#13'),Y)             #10
jmp(Y,'stpx#13')                #11
ld(AC,X)                        #12

# pc = 0x222b, Opcode = 0x2b
# Instruction CONDI: chooses immediate operand based on condition, (vAC == 0), 22 + 26 cycles
label('CONDI')
ld(hi('condi#13'),Y)            #10
jmp(Y,'condi#13')               #11
# dummy                         #12
#
# pc = 0x222d, Opcode = 0x2d
# Instruction CONDB: chooses zero page byte var based on condition, (vAC == 0), 22 + 26 cycles
label('CONDB')
ld(hi('condb#13'),Y)            #10 #12
jmp(Y,'condb#13')               #11
ld(AC,X)                        #12

# pc = 0x2230, Opcode = 0x30
# Instruction CONDIB: chooses between imm and zero page byte var based on condition, (vAC == 0), 22 + 26 cycles
label('CONDIB')
ld(hi('condib#13'),Y)           #10
jmp(Y,'condib#13')              #11
st([vTmp])                      #12

# pc = 0x2233, Opcode = 0x33
# Instruction CONDBI: chooses between zero page byte var and imm based on condition, (vAC == 0), 22 + 26 cycles
label('CONDBI')
ld(hi('condbi#13'),Y)           #10
jmp(Y,'condbi#13')              #11
# dummy                         #12
#
# pc = 0x2235, Opcode = 0x35
# Instruction XCHGW: Exchange two zero word variables, 22 + 46 cycles
label('XCHGW')
ld(hi('xchgw#13'),Y)            #10 #12
jmp(Y,'xchgw#13')               #11
st([sysArgs+6])                 #12 var1

# pc = 0x2238, Opcode = 0x38
# Instruction OSCPX:
label('OSCPX')
ld(hi('oscpx#13'),Y)            #10
jmp(Y,'oscpx#13')               #11
# dummy                         #12
# 
# pc = 0x223a, Opcode = 0x3a
# Instruction SWAPB: Swap two bytes in memory, 22 + 46 cycles
label('SWAPB')
ld(hi('swapb#13'),Y)            #10 #12
jmp(Y,'swapb#13')               #11
ld(AC,X)                        #12 var1
# 
# pc = 0x223d, Opcode = 0x3d
# Instruction SWAPW: Swap two words in memory, 22 + 58 cycles
label('SWAPW')
ld(hi('swapw#13'),Y)            #10
jmp(Y,'swapw#13')               #11
ld(AC,X)                        #12 var1

# pc = 0x2240, Opcode = 0x40
# Instruction NEEKA: Peek <n> bytes from [vAC] into [var], 22 + 34*n + 24 cycles
label('NEEKA')
ld(hi('neeka#13'),Y)            #10
jmp(Y,'neeka#13')               #11
st([sysArgs+6])                 #12 var

# pc = 0x2243, Opcode = 0x43
# Instruction NOKEA: Poke <n> bytes from [var] into [vAC], 22 + 34*n + 24 cycles
label('NOKEA')
ld(hi('nokea#13'),Y)            #10
jmp(Y,'nokea#13')               #11
st([sysArgs+6])                 #12 var

# pc = 0x2246, Opcode = 0x46
# Instruction ADDVL: Add two 32bit zero page vars, dst += src, 22 + 78 cycles
label('ADDVL')
ld(hi('addvl#13'),Y)            #10
jmp(Y,'addvl#13')               #11
st([sysArgs+6],X)               #12 src var

# pc = 0x2249, Opcode = 0x49
# Instruction SUBVL: Subtract two 32bit zero page vars, dst -= src, 22 + 74 cycles
label('SUBVL')
ld(hi('subvl#13'),Y)            #10
jmp(Y,'subvl#13')               #11
st([sysArgs+6],X)               #12 src var

# pc = 0x224c, Opcode = 0x4c
# Instruction ANDVL: And two 32bit zero page vars, dst &= src, 22 + 46 cycles
label('ANDVL')
ld(hi('andvl#13'),Y)            #10
jmp(Y,'andvl#13')               #11
ld(AC,X)                        #12 src var

# pc = 0x224f, Opcode = 0x4f
# Instruction ORVL: Or two 32bit zero page vars, dst |= src, 22 + 46 cycles
label('ORVL')
ld(hi('orvl#13'),Y)             #10
jmp(Y,'orvl#13')                #11
ld(AC,X)                        #12 src var

# pc = 0x2252, Opcode = 0x52
# Instruction XORVL: Xor two 32bit zero page vars, dst ^= src, 22 + 76 cycles
label('XORVL')
ld(hi('xorvl#13'),Y)            #10
jmp(Y,'xorvl#13')               #11
ld(AC,X)                        #12 src var

# pc = 0x2255, Opcode = 0x55
# Instruction JEQL: 22 + 40 cycles
label('JEQL')
ld(hi('jeql#13'),Y)             #10
jmp(Y,'jeql#13')                #11
st([sysArgs+6])                 #12 jump.hi

# pc = 0x2258, Opcode = 0x58
# Instruction JNEL: 22 + 40 cycles
label('JNEL')
ld(hi('jnel#13'),Y)             #10
jmp(Y,'jnel#13')                #11
st([sysArgs+6])                 #12 jump.hi

# pc = 0x225B, Opcode = 0x5B
# Instruction JLTL: 22 + 26 cycles
label('JLTL')
ld(hi('jltl#13'),Y)             #10
jmp(Y,'jltl#13')                #11
st([sysArgs+6])                 #12 jump.hi

# pc = 0x225E, Opcode = 0x5E
# Instruction JGTL: 22 + 42 cycles
label('JGTL')
ld(hi('jgtl#13'),Y)             #10
jmp(Y,'jgtl#13')                #11
st([sysArgs+6])                 #12 jump.hi

# pc = 0x2261, Opcode = 0x61
# Instruction JLEL: 22 + 42 cycles
label('JLEL')
ld(hi('jlel#13'),Y)             #10
jmp(Y,'jlel#13')                #11
st([sysArgs+6])                 #12 jump.hi

# pc = 0x2264, Opcode = 0x64
# Instruction JGEL: 22 + 26 cycles
label('JGEL')
ld(hi('jgel#13'),Y)             #10
jmp(Y,'jgel#13')                #11
st([sysArgs+6])                 #12 jump.hi

# pc = 0x2267, Opcode = 0x67
# Instruction ANDBI: And immediate byte with byte var, result in byte var, 22 + 20 cycles
label('ANDBI')
ld(hi('andbi#13'),Y)            #10
jmp(Y,'andbi#13')               #11
ld(AC,X)                        #12 address of var

# pc = 0x226A, Opcode = 0x6A
# Instruction ORBI: OR immediate byte with byte var, result in byte var, 22 + 20 cycles
label('ORBI')
ld(hi('orbi#13'),Y)             #10
jmp(Y,'orbi#13')                #11
ld(AC,X)                        #12 address of var

# pc = 0x226D, Opcode = 0x6D
# Instruction XORBI: var.lo ^= imm, 22 + 20 cycles
label('XORBI')
ld(hi('xorbi#13'),Y)            #10
jmp(Y,'xorbi#13')               #11
ld(AC,X)                        #12 address of var

# pc = 0x2270, Opcode = 0x70
# Instruction JMPI, (lb3361): Jump to 16bit address, preserve vLR, 22 + 20 cycles
label('JMPI')
ld(hi('jmpi#13'),Y)             #10
jmp(Y,'jmpi#13')                #11
suba(2)                         #12

# SYS calls and instruction implementations rely on these
fillers(until=0xca)
ld(-28/2)                       #25
bra('NEXT')                     #26 Return from SYS calls
ld([vPC+1],Y)                   #27

# pc = 0x22cd, Opcode = 0xcd
# Instruction MOVL (lb3361): Move long variable. 22+40 cycles
label('MOVL')
ld(hi('movl#13'),Y)             #10
jmp(Y,'movl#13')                #11
st([sysArgs+6])                 #12

# pc = 0x22d0, Opcode = 0xd0
# Instruction MOVF (lb3361): Move float variable. 22+46 cycles
label('MOVF')
ld(hi('movf#13'),Y)             #10
jmp(Y,'movf#13')                #11
st([sysArgs+6])                 #12

# pc = 0x22d3 Opcode = 0xd3
# Instruction NROL (lb3361): Left rotate n bytes. 22+18+n*18
# Shift vACsign<-bit(8n-1)<-...<-bit(0)<-vACsign. Destroys vAC
# Encoding: [0xc7,v,0xd3,v+n]
label('NROL')
ld(hi('nrol#13'),Y)             #10
jmp(Y,'nrol#13')                #11
st([sysArgs+6])                 #12

# pc = 0x22d6 Opcode = 0xd6
# Instruction NROR (lb3361): Right rotate n bytes. 22+26+n*32
# Shift vACsign->bit(8n-1)->...->bit(0)->vACsign. Destroys vAC
# Encoding: [0xc7,v+n,0xd6,v]
label('NROR')
ld(hi('nror#13'),Y)             #10
jmp(Y,'nror#13')                #11
st([sysArgs+6])                 #12



fillers(until=0xff)

#-----------------------------------------------------------------------
#       PREFX2 instruction page, (0x2300), PREFIX ARG0 OPCODE
#-----------------------------------------------------------------------
#
label('PREFX2_PAGE')
bra('.next2')                   #0 Enter at '.next2' (so no startup overhead)
# --- Page boundary ---
align(0x100,size=0x100)
ld([vPC+1],Y)                   #1

# Fetch next instruction and execute it, but only if there are sufficient
# ticks left for the slowest instruction.
adda([vTicks])                  #0 Track elapsed ticks (actually counting down: AC<0)
blt('EXIT')                     #1 Escape near time out
st([vTicks])                    #2
ld([vPC])                       #3 PREFIX+ARG0 is 2 bytes, vPC has been incremented by 1
adda(1,X)                       #4 point to opcode
st(vCpuSelect,[vCpuSelect])     #5 Reset to default vCPU page
ld([Y,X])                       #6 Fetch opcode (actually a branch target)
st([Y,Xpp])                     #7 Just X++
bra(AC)                         #8 Dispatch
ld([sysArgs+7])                 #9 Operand

# Resync with video driver and transfer control
adda(maxTicks)                  #3
bgt(pc()&255)                   #4 Resync
suba(1)                         #5
ld(hi('vBlankStart'),Y)         #6
jmp(Y,[vReturn])                #7 To video driver
ld(0)                           #8 AC should be 0 already. Still..
assert vCPU_overhead ==          9

# pc = 0x2311, Opcode = 0x11
# Instruction LSLN: Logical shift left vAC, (16bit), n times, 22 + 30*n + 20 cycles
label('LSLN')
ld(hi('lsln#13'),Y)             #10
jmp(Y,'lsln#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x2313, Opcode = 0x13
# Instruction SEXT: Sign extend vAC based on a variable mask, 22 + 24 cycles
label('SEXT')
ld(hi('sext#13'),Y)             #10 #12
jmp(Y,'sext#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x2315, Opcode = 0x15
# Instruction NOTW: Boolean invert var, 22 + 26 cycles
label('NOTW')
ld(hi('notw#13'),Y)             #10 #12
jmp(Y,'notw#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x2317, Opcode = 0x17
# Instruction NEGW: Arithmetic negate var, 22 + 28 cycles
label('NEGW')
ld(hi('negw#13'),Y)             #10 #12
jmp(Y,'negw#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x2319, Opcode = 0x19
# Instruction ANDBA: vAC &= var.lo, 22 + 22 cycles
label('ANDBA')
ld(hi('andba#13'),Y)            #10 #12
jmp(Y,'andba#13')               #11
ld(AC,X)                        #12 var

# pc = 0x231c, Opcode = 0x1c
# Instruction ORBA: vAC |= var.lo, 22 + 20 cycles
label('ORBA')
ld(hi('orba#13'),Y)             #10
jmp(Y,'orba#13')                #11
ld(AC,X)                        #12 var

# pc = 0x231f, Opcode = 0x1f
# Instruction XORBA: vAC ^= var.lo, 22 + 20 cycles
label('XORBA')
ld(hi('xorba#13'),Y)            #10
jmp(Y,'xorba#13')               #11
ld(AC,X)                        #12 var

# pc = 0x2322, Opcode = 0x22
# Instruction FREQM: [(((chan & 3) + 1) <<8) | 0x00FC] = vAC, chan var = [0..3], 22 + 26 cycles
label('FREQM')
ld(hi('freqm#13'),Y)            #10
jmp(Y,'freqm#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x2324, Opcode = 0x24
# Instruction FREQA: [((((chan - 1) & 3) + 1) <<8) | 0x00FC] = vAC, chan var = [1..4], 22 + 26 cycles
label('FREQA')
ld(hi('freqa#13'),Y)            #10 #12
jmp(Y,'freqa#13')               #11
ld(AC,X)                        #12 var

# pc = 0x2327, Opcode = 0x27
# Instruction FREQZI: [(((imm & 3) + 1) <<8) | 0x00FC] = 0, imm = [0..3], 22 + 22 cycles
label('FREQZI')
ld(hi('freqzi#13'),Y)           #10
jmp(Y,'freqzi#13')              #11
anda(3)                         #12 channel

# pc = 0x232a, Opcode = 0x2a
# Instruction VOLM: [(((chan & 3) + 1) <<8) | 0x00FA] = vAC.low, chan var = [0..3], 22 + 24 cycles
label('VOLM')
ld(hi('volm#13'),Y)             #10
jmp(Y,'volm#13')                #11
# dummy                         #12 Overlap
#
# pc = 0x232c, Opcode = 0x2c
# Instruction VOLA: [((((chan - 1) & 3) + 1) <<8) | 0x00FA] = 63 - vAC.low + 64, chan var = [1..4],  22 + 26 cycles
label('VOLA')
ld(hi('vola#13'),Y)             #10 #12
jmp(Y,'vola#13')                #11
ld(AC,X)                        #12 chan + 1

# pc = 0x232f, Opcode = 0x2f
# Instruction MODA: [((((chan - 1) & 3) + 1) <<8) | 0x00FB] = vAC.low, chan var = [1..4], 22 + 24 cycles
label('MODA')
ld(hi('moda#13'),Y)             #10
jmp(Y,'moda#13')                #11
ld(AC,X)                        #12 chan + 1

# pc = 0x2332, Opcode = 0x32
# Instruction MODZI: [(((imm & 3) + 1) <<8) | 0x00FA] = 0x0200, imm = [0..3], 22 + 24 cycles
label('MODZI')
ld(hi('modzi#13'),Y)            #10
jmp(Y,'modzi#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x2234, Opcode = 0x34
# Instruction SMPCPY:
label('SMPCPY')
ld(hi('smpcpy#13'),Y)           #10 #12
jmp(Y,'smpcpy#13')              #11
ld(AC,X)                        #12 dst var

# pc = 0x2237, Opcode = 0x37
# Instruction CMPWS:
label('CMPWS')
ld(hi('cmpws#13'),Y)            #10
jmp(Y,'cmpws#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x2239, Opcode = 0x39
# Instruction CMPWU:
label('CMPWU')
ld(hi('cmpwu#13'),Y)            #10 #12
jmp(Y,'cmpwu#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x223b, Opcode = 0x3b
# Instruction LEEKA:
label('LEEKA')
ld(hi('leeka#13'),Y)            #10 #12
jmp(Y,'leeka#13')               #11
# dummy                         #12 Overlap
#
# pc = 0x223d, Opcode = 0x3d
# Instruction LOKEA:
label('LOKEA')
ld(hi('lokea#13'),Y)            #10 #12
jmp(Y,'lokea#13')               #11
# dummy                         #12
#
# pc = 0x223f, Opcode = 0x3f
# Instruction FEEKA:
label('FEEKA')
ld(hi('feeka#13'),Y)            #10 #12
jmp(Y,'feeka#13')               #11
st([vTmp])                      #12 dst var

# pc = 0x2242, Opcode = 0x42
# Instruction FOKEA:
label('FOKEA')
ld(hi('fokea#13'),Y)            #10
jmp(Y,'fokea#13')               #11
# dummy                         #12
#
# pc = 0x2344, Opcode = 0x44
# Instruction MEEKA: Peek 8 bytes from [vAC] to [var], 22 + 64 cycles
label('MEEKA')
ld(hi('meeka#13'),Y)            #10 #12
jmp(Y,'meeka#13')               #11
# dummy                         #12
#
# pc = 0x2346, Opcode = 0x46
# Instruction MOKEA: Poke 8 bytes from [var] to [vAC], 22 + 64 cycles
label('MOKEA')
ld(hi('mokea#13'),Y)            #10 #12
jmp(Y,'mokea#13')               #11
# dummy                         #12
#
# pc = 0x2348, Opcode = 0x48
# Instruction LSRVL: Logical shift right var long, 22 + 104 cycles
label('LSRVL')
ld(hi('lsrvl#13'),Y)            #10 #12
jmp(Y,'lsrvl#13')               #11
ld(AC,X)                        #12 var

# pc = 0x234b, Opcode = 0x4b
# Instruction LSLVL: Logical shift left var long, 22 + 56 cycles
label('LSLVL')
ld(hi('lslvl#13'),Y)            #10 #12
jmp(Y,'lslvl#13')               #11
ld(AC,X)                        #12 var

# pc = 0x234e, Opcode = 0x4e
# Instruction INCL: Increment var long, 22 + 36 cycles
label('INCL')
ld(hi('incl#13'),Y)             #10 #12
jmp(Y,'incl#13')                #11
ld(AC,X)                        #12 var

# pc = 0x2351, Opcode = 0x51
# Instruction DECL: Decrement var long, 22 + 40 cycles
label('DECL')
ld(hi('decl#13'),Y)             #10 #12
jmp(Y,'decl#13')                #11
# dummy                         #12
#
# pc = 0x2353, Opcode = 0x53
# Instruction PEEKV+: Peek byte at address contained in var, var += 1, 22 + 38-42 cycles, (can cross page boundaries)
label('PEEKV+')
ld(hi('peekv+#13'),Y)           #10 #12
jmp(Y,'peekv+#13')              #11
# dummy                         #12
#
# SYS calls and instruction implementations rely on these
fillers(until=0xca)
ld(-28/2)                       #25
bra('NEXT')                     #26 Return from SYS calls
ld([vPC+1],Y)                   #27

# pc = 0x23cd, Opcode = 0xcd
# Instruction NCOPY (lb3361): copy n bytes from [vAC] to [vDST]. vAC+=n. vDST+=n
label('NCOPY')
ld(hi('ncopy#13'),Y)            #10
jmp(Y,'ncopy#13')               #11
ld([vTicks])                    #12


# pc = 0x23d0, Opcode = 0xd0
# Instruction STLU (lb3361): store zero extended vAC into long var. 22+26 cycles
label('STLU')
ld(hi('stlu#13'),Y)             #10
jmp(Y,'stlu#13')                #11
ld(AC,X)                        #12

# pc = 0x23d3, Opcode = 0xd3
# Instruction STLS (lb3361): store sign extended vAC into long var. 22+28 cycles
label('STLS')
ld(hi('stls#13'),Y)             #10
jmp(Y,'stls#13')                #11
ld(AC,X)                        #12

# pc = 0x23d6, Opcode = 0xd6
# Instruction NOTL (lb3361): complement long var. 22+30 cycles
label('NOTL')
ld(hi('notl#13'),Y)             #10
jmp(Y,'notl#13')                #11
ld(AC,X)                        #12

# pc = 0x23d9, Opcode = 0xd9
# Instruction NEGL (lb3361): negate long var. 22+58(max) cycles
label('NEGL')
ld(hi('negl#13'),Y)             #10
jmp(Y,'negl#13')                #11
ld(AC,X)                        #12


fillers(until=0xff)

#-----------------------------------------------------------------------
#       PREFX1 instruction page, (0x2400), PREFIX OPCODE
#-----------------------------------------------------------------------
#
label('PREFX1_PAGE')
bra('.next2')                   #0 Enter at '.next2' (so no startup overhead)
# --- Page boundary ---
align(0x100,size=0x100)
ld([vPC+1],Y)                   #1

# Fetch next instruction and execute it, but only if there are sufficient
# ticks left for the slowest instruction.
adda([vTicks])                  #0 Track elapsed ticks (actually counting down: AC<0)
blt('EXIT')                     #1 Escape near time out
st([vTicks])                    #2
ld([vPC])                       #3 PREFIX is 1 byte, vPC hasn't been incremented
adda(1,X)                       #4
st(vCpuSelect,[vCpuSelect])     #5 Reset to default vCPU page
ld([Y,X])                       #6 Fetch opcode (actually a branch target)
st([Y,Xpp])                     #7 Just X++
bra(AC)                         #8 Dispatch
ld([Y,X])                       #9 Prefetch operand

# Resync with video driver and transfer control
adda(maxTicks)                  #3
bgt(pc()&255)                   #4 Resync
suba(1)                         #5
ld(hi('vBlankStart'),Y)         #6
jmp(Y,[vReturn])                #7 To video driver
ld(0)                           #8 AC should be 0 already. Still..
assert vCPU_overhead ==          9

# pc = 0x2411, Opcode = 0x11
# Instruction NOTE: vAC = ROM:[NotesTable + vAC.lo*2], 18 + 28 cycles
label('NOTE')
ld(hi('note#13'),Y)             #10
jmp(Y,'note#13')                #11
ld('.note#28')                  #12 low byte of low note return address

# pc = 0x2414, Opcode = 0x14
# Instruction MIDI: vAC = ROM:[NotesTable + (vAC.lo - 11)*2], 18 + 30 cycles
label('MIDI')
ld(hi('midi#13'),Y)             #10
jmp(Y,'midi#13')                #11
ld('.midi#29')                  #12 low byte of low midi return address

# pc = 0x2417, Opcode = 0x17
# Instruction XLA, (lb3361): Exchange vLR with vAC, 18 + 28 cycles
label('XLA')
ld(hi('xla#13'),Y)              #10
jmp(Y,'xla#13')                 #11
ld([vAC])                       #12

# pc = 0x241a Opcode = 0x1a
# Instruction ADDLP (lb3361): vLAC += [vAC]. 22+66 cycles
label('ADDLP')
ld(hi('addlp#13'),Y)            #10
jmp(Y,'addlp#13')               #11
ld([vTicks])                    #12

# pc = 0x241d Opcode = 0x1d
# Instruction SUBLP (lb3361): vLAC -= [vAC]. 22+60 cycles
label('SUBLP')
ld(hi('sublp#13'),Y)            #10
jmp(Y,'sublp#13')               #11
ld([vTicks])                    #12

# pc = 0x241d Opcode = 0x20
# Instruction ANDLP (lb3361): vLAC &= [vAC]. 22+42 cycles
# On return vAC>0 (resp =0, <0) iff LAC>0 (resp =0, <0)
label('ANDLP')
ld(hi('andlp#13'),Y)            #10
jmp(Y,'andlp#13')               #11
ld([vTicks])                    #12

# pc = 0x2423 Opcode = 0x23
# Instruction ORLP (lb3361): vLAC |= [vAC]. 22+42 cycles
# On return vAC>0 (resp =0, <0) iff LAC>0 (resp =0, <0)
label('ORLP')
ld(hi('orlp#13'),Y)             #10
jmp(Y,'orlp#13')                #11
ld([vTicks])                    #12

# pc = 0x2426 Opcode = 0x26
# Instruction XORLP (lb3361): vLAC ^= [vAC]. 22+42 cycles
# On return vAC>0 (resp =0, <0) iff LAC>0 (resp =0, <0)
label('XORLP')
ld(hi('xorlp#13'),Y)            #10
jmp(Y,'xorlp#13')               #11
ld([vTicks])                    #12

# pc = 0x2429 Opcode = 0x29
# Instruction CMPLPU (lb3361): compare vLAC and [vAC] unsigned. 22+72(max) cycles
# On return vAC>0 (resp =0, <0) 
label('CMPLPU')
ld(hi('cmplpu#13'),Y)           #10
jmp(Y,'cmplpu#13')              #11
ld([vTicks])                    #12

# pc = 0x242c Opcode = 0x2c
# Instruction CMPLPS (lb3361): compare vLAC and [vAC] signed. 22+72(max) cycles
# On return vAC>0 (resp =0, <0) 
label('CMPLPS')
ld(hi('cmplps#13'),Y)           #10
jmp(Y,'cmplps#13')              #11
ld([vTicks])                    #12

# SYS calls and instruction implementations rely on these
fillers(until=0xca)
ld(-28/2)                       #25
bra('NEXT')                     #26 Return from SYS calls
ld([vPC+1],Y)                   #27


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       PREFX3 implementation page, (0x2500)
#-----------------------------------------------------------------------
# Original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2099#p2099
#
# STB2 implementation
# Original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2135#p2135
label('stb2#13')
ld([sysArgs+7],Y)               #13 Second operand
ld([vAC])                       #14
st([Y,X])                       #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# STW2 implementation
# Original idea by lb3361, see https://forum.gigatron.io/viewtopic.php?p=2135#p2135
label('stw2#13')
ld([sysArgs+7],Y)               #13 Second operand
ld([vAC])                       #14
st([Y,Xpp])                     #15
ld([vAC+1])                     #16
st([Y,X])                       #17
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# XCHGB implementation
label('xchgb#13')
st([sysArgs+6])                 #13 1st var
ld([sysArgs+7],X)               #14 2nd var
ld([X])                         #15
st([vTmp])                      #16
ld([sysArgs+6],X)               #17
ld([X])                         #18
ld([sysArgs+7],X)               #19
st([X])                         #20
ld([sysArgs+6],X)               #21
ld([vTmp])                      #22
st([X])                         #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# XCHGW implementation
label('xchgw#13')
ld(min(0,maxTicks-46/2))        #13
adda([vTicks])                  #14
blt('.xchgw#17')                #15 not enough time left, so retry
ld(0,Y)                         #16
ld([sysArgs+7],X)               #17
ld([X])                         #18
st([vTmp])                      #19 vTmp = var2.lo
ld([sysArgs+6],X)               #20
ld([X])                         #21 AC = var1.lo
ld([sysArgs+7],X)               #22
st([X])                         #23 var2.lo = AC
ld([sysArgs+6],X)               #24
ld([vTmp])                      #25
st([X])                         #26 var1.lo = vTmp
ld([sysArgs+7])                 #27
adda(1)                         #28
st([sysArgs+7],X)               #29 inc sysArgs+7
ld([X])                         #30
st([vTmp])                      #31 vTmp = var2.hi
ld([sysArgs+6])                 #32
adda(1,X)                       #33
ld([X])                         #34 AC = var1.hi
ld([sysArgs+7],X)               #35
st([X])                         #36 var2.hi = AC
ld([sysArgs+6])                 #37
adda(1,X)                       #38
ld([vTmp])                      #39
st([X])                         #40 var1.hi = vTmp
ld(hi('REENTER'),Y)             #41
jmp(Y,'REENTER')                #42
ld(-46/2)                       #43
label('.xchgw#17')
ld(hi('PREFX3_PAGE'))           #17 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #18 restore PREFX3 instruction page
adda(1,Y)                       #19 retry instruction
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

# MOVW implementation
label('movw#13')
ld(AC,X)                        #13
adda(1)                         #14
st([vTmp])                      #15 address of src.hi
ld([X])                         #16 src.lo
ld([sysArgs+7],X)               #17 address of dst.lo
st([X])                         #18 dst.lo = src.lo
ld([vTmp],X)                    #19
ld([X])                         #20 src.hi
st([vTmp])                      #21
ld([sysArgs+7])                 #22
adda(1,X)                       #23 address of dst.hi
ld([vTmp])                      #24
st([X])                         #25 dst.hi = src.hi
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# ADDWI implementation
label('addwi#13')
st([vTmp])                      #13 save imm.lo
adda([vAC])                     #14
st([vAC])                       #15 vAC.lo += imm.lo
bmi('.addwi#18')                #16 overflow
suba([vTmp])                    #17 restore vAC.lo
ora([vTmp])                     #18 imm.lo | vAC.lo
bmi('.addwi#21')                #19 if carry == 1
ld([sysArgs+7])                 #20 imm.hi
adda([vAC+1])                   #21 carry = 0
st([vAC+1])                     #22 vAC.hi += imm.hi
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.addwi#18')
anda([vTmp])                    #18 imm.lo & vAC.lo
bmi('.addwi#21')                #19 If carry == 1
ld([sysArgs+7])                 #20 imm.hi
adda([vAC+1])                   #21 carry = 0
st([vAC+1])                     #22 vAC.hi += imm.hi
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.addwi#21')
adda(1)                         #21 carry = 1
adda([vAC+1])                   #22
st([vAC+1])                     #23 vAC.hi = vAC.hi + carry + imm.hi
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# SUBWI implementation
label('subwi#13')
st([vTmp])                      #13 save imm.lo
ld([vAC])                       #14
bmi('.subwi#17')                #15 is -ve?
suba([vTmp])                    #16 
st([vAC])                       #17 vAC.lo -= imm.lo 
ora([vTmp])                     #18 borrow in bit 7
anda(0x80,X)                    #19 move borrow to bit 0
ld([vAC+1])                     #20 
suba([X])                       #21
suba([sysArgs+7])               #22
st([vAC+1])                     #23 vAC.hi = vAC.hi - borrow - imm.hi
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26
label('.subwi#17')
st([vAC])                       #17 vAC.lo -= imm.lo 
anda([vTmp])                    #18 borrow in bit 7
anda(0x80,X)                    #19 move borrow to bit 0
ld([vAC+1])                     #20 
suba([X])                       #21
suba([sysArgs+7])               #22
st([vAC+1])                     #23 vAC.hi = vAC.hi - borrow - imm.hi
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# ANDWI implementation
label('andwi#13')
anda([vAC])                     #13
st([vAC])                       #14 vAC.lo &= imm.lo 
ld([sysArgs+7])                 #15
anda([vAC+1])                   #16
st([vAC+1])                     #17 vAC.hi &= imm.hi
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# ORWI implementation
label('orwi#13')
ora([vAC])                      #13
st([vAC])                       #14 vAC.lo |= imm.lo 
ld([sysArgs+7])                 #15
ora([vAC+1])                    #16
st([vAC+1])                     #17 vAC.hi |= imm.hi
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# XORWI implementation
label('xorwi#13')
xora([vAC])                     #13
st([vAC])                       #14 vAC.lo ^= imm.lo 
ld([sysArgs+7])                 #15
xora([vAC+1])                   #16
st([vAC+1])                     #17 vAC.hi ^= imm.hi
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# LDPX implementation
label('ldpx#13')
adda(1,X)                       #13
ld([X])                         #14 y
adda([X])                       #15 y*2
ld(AC,X)                        #16
ld(1,Y)                         #17 Y,X = 0x0100 + 2*y, (0 >= y <= 127)
ld([Y,X])                       #18
ld(AC,Y)                        #19 Y = [Y,X]
ld([vTmp],X)                    #20
ld([X])                         #21
ld(AC,X)                        #22 X = x
ld([Y,X])                       #23 colour = [Y,X]
ld([sysArgs+7],X)               #24
st([X])                         #25 colour
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# CONDI implementation
label('condi#13')
st([vTmp])                      #13
ld([vAC])                       #14
ora([vAC+1])                    #15
bne(pc()+3)                     #16
bra(pc()+3)                     #17
ld([sysArgs+7])                 #18 get result cond == 0
ld([vTmp])                      #18 get result cond != 0
st([vAC])                       #19 result.lo
ld(0)                           #20
st([vAC+1])                     #21 result.hi = 0
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# CONDB implementation
label('condb#13')
ld([X])                         #13
st([vTmp])                      #14
ld([sysArgs+7],X)               #15 get result cond=0
ld([vAC])                       #16
ora([vAC+1])                    #17
bne(pc()+3)                     #18
bra(pc()+3)                     #19
ld([X])                         #20 get result cond == 0
ld([vTmp])                      #20 get result cond != 0
st([vAC])                       #21 result.lo
ld(0)                           #22
st([vAC+1])                     #23 result.hi = 0
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# CONDIB implementation
label('condib#13')
ld([sysArgs+7],X)               #13 get result cond=0
ld([vAC])                       #14
ora([vAC+1])                    #15
bne(pc()+3)                     #16
bra(pc()+3)                     #17
ld([X])                         #18 get result cond == 0
ld([vTmp])                      #18 get result cond != 0
st([vAC])                       #19 result.lo
ld(0)                           #20
st([vAC+1])                     #21 result.hi = 0
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# CONDBI implementation
label('condbi#13')
ld(AC,X)                        #13
ld([vAC])                       #14
ora([vAC+1])                    #15
bne(pc()+3)                     #16
bra(pc()+3)                     #17
ld([sysArgs+7])                 #18 get result cond == 0
ld([X])                         #18 get result cond != 0
st([vAC])                       #19 result.lo
ld(0)                           #20
st([vAC+1])                     #21 result.hi = 0
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       PREFX2 implementation page, (0x2600)
#-----------------------------------------------------------------------
#
# LSLN implementation
label('lsln#13')
suba(1)                         #13
bge('.lsln#16')                 #14
st([sysArgs+7])                 #15
ld(hi('NEXTY'),Y)               #16 exit PREFX2 instruction page
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18
label('.lsln#16')
ld([vAC])                       #16
anda(128,X)                     #17
adda([vAC])                     #18
st([vAC])                       #19
ld([X])                         #20
adda([vAC+1])                   #21
adda([vAC+1])                   #22
st([vAC+1])                     #23
ld(hi('PREFX2_PAGE'))           #24 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #25 restore PREFX2 instruction page
adda(1,Y)                       #26 restart instruction
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28

# NOTW implementation
label('notw#13')
ld([sysArgs+7],X)               #13
ld(0,Y)                         #14
ld([X])                         #15
xora(255)                       #16
st([Y,Xpp])                     #17
ld([X])                         #18
xora(255)                       #19
st([X])                         #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

# NEGW implementation
label('negw#13')
ld([sysArgs+7],X)               #13
ld(0,Y)                         #14
ld([X])                         #15
xora(255)                       #16
adda(1)                         #17
beq('.negw#20')                 #18
st([Y,Xpp])                     #19
ld([X])                         #20
xora(255)                       #21
st([X])                         #22                         
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25
label('.negw#20')
ld([X])                         #20
xora(255)                       #21
adda(1)                         #22
st([X])                         #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# SEXT implementation
label('sext#13')
anda([vAC+1])                   #13
bne('.sext#16')                 #14 check mask
ld([sysArgs+7])                 #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18
label('.sext#16')
xora(255)                       #16
adda(1)                         #17 mask = (~mask) + 1
ora([vAC+1])                    #18
st([vAC+1])                     #19 set sign bits
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# ANDBA implementation
label('andba#13')
ld([vAC])                       #13
anda([X])                       #14
st([vAC])                       #15 vAC.lo &= var.lo
ld(0)                           #16
st([vAC+1])                     #17 Store high result
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# ORBA implementation
label('orba#13')
ld([vAC])                       #13
ora([X])                        #14
st([vAC])                       #15 vAC.lo |= var.lo
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# XORBA implementation
label('xorba#13')
ld([vAC])                       #13
xora([X])                       #14
st([vAC])                       #15 vAC.lo ^= var.lo
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# FREQM implementation, (frequency control for static data in the correct format, i.e. MIDI)
label('freqm#13')
ld(AC,X)                        #13 chan var, (operand from PREFX2)
ld([X])                         #14
anda(3)                         #15
adda(1,Y)                       #16
ld(0xFC,X)                      #17
ld([vAC])                       #18
st([Y,Xpp])                     #19
ld([vAC+1])                     #20
st([Y,X])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# FREQA implementation
label('freqa#13')
ld([X])                         #13
suba(1)                         #14
anda(3)                         #15
adda(1,Y)                       #16
ld(0xFC,X)                      #17
ld([vAC])                       #18
st([Y,Xpp])                     #19
ld([vAC+1])                     #20
st([Y,X])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# FREQZI implementation
label('freqzi#13')
adda(1,Y)                       #13
ld(0xFC,X)                      #14
ld(0)                           #15
st([Y,Xpp])                     #16
st([Y,X])                       #17
ld(hi('NEXTY'),Y)               #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

# VOLM implementation, (volume control for static data in the correct format, i.e. MIDI)
label('volm#13')
ld(AC,X)                        #13 chan var, (operand from PREFX2)
ld([X])                         #14
anda(3)                         #15
adda(1,Y)                       #16
ld(0xFA,X)                      #17
ld([vAC])                       #18
st([Y,X])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# VOLA implementation
label('vola#13')
ld([X])                         #13
suba(1)                         #14
anda(3)                         #15
adda(1,Y)                       #16
ld(0xFA,X)                      #17
ld(63)                          #18
suba([vAC])                     #19
adda(64)                        #20
st([Y,X])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

# MODA implementation
label('moda#13')
ld([X])                         #13
suba(1)                         #14
anda(3)                         #15
adda(1,Y)                       #16 
ld(0xFB,X)                      #17
ld([vAC])                       #18
st([Y,X])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# MODZI implementation
label('modzi#13')
anda(3)                         #13
adda(1,Y)                       #14
ld(0xFA,X)                      #15
ld(0)                           #16
st([Y,Xpp])                     #17
ld(2)                           #18
st([Y,X])                       #19
ld(hi('NEXTY'),Y)               #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# SMPCPY implementation
label('smpcpy#13')
ld(0,Y)                         #13
ld(min(0,maxTicks-60/2))        #14
adda([vTicks])                  #15
blt('.smpcpy#18')               #16 not enough time left, so retry
ld([X])                         #17 dst address = [dst var]
st([Y,Xpp])                     #18
st([0xB0])                      #19
ld([X])                         #20
st([0xB1])                      #21
ld([vAC],X)                     #22 low byte of src address
ld([vAC+1],Y)                   #23 high byte of arc address
ld([Y,X])                       #24
st([vTmp])                      #25 packed sample, 4:4
ld(soundTable>>8,Y)             #26
anda(0xF0)                      #27 high nibble
ora(0x03,X)                     #28
ld([Y,X])                       #29 high nibble >>2
ld([0xB0],X)                    #30 low byte dst address
ld([0xB1],Y)                    #31 high byte dst address
st([Y,X])                       #32 [0x0702 + i] = 6bit sample
ld([0xB0])                      #33
adda(4,X)                       #34
adda(8)                         #35
st([0xB0])                      #36
ld([vTmp])                      #37 packed sample, 4:4
anda(0x0F)                      #38 low nibble
adda(AC)                        #39
adda(AC)                        #40 low nibble <<2
st([Y,X])                       #41 [0x0706 + i] = 6bit sample
ld([sysArgs+7],X)               #42
ld([0xB0])                      #43
st([X])                         #44
suba(0x02)                      #45
beq('.smpcpy#48')               #46 check for end sample
ld([vAC])                       #47
adda(1)                         #48
bne('.smpcpy#51')               #49
st([vAC])                       #50
ld([vAC+1])                     #51
adda(1)                         #52
st([vAC+1])                     #53
ld(hi('PREFX2_PAGE'))           #54 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #55 restore PREFX2 instruction page
adda(1,Y)                       #56 restart instruction
jmp(Y,'NEXTY')                  #57
ld(-60/2)                       #58

label('.smpcpy#51')
ld(hi('PREFX2_PAGE'))           #51 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #52 restore PREFX2 instruction page
adda(1,Y)                       #53 restart instruction
jmp(Y,'REENTER')                #54
ld(-58/2)                       #55

label('.smpcpy#48')
ld(hi('NEXTY'),Y)               #48
jmp(Y,'NEXTY')                  #49
ld(-52/2)                       #50

label('.smpcpy#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# CMPWS implementation, vAC = vAC CMPWS var
#           LDW   var0
#           XORW  var1
#           BGE   CMPWS_GE
#           LDW   var0
#           BRA   CMPWS_RT
#CMPWS_GE   LDW   var0
#           SUBW  var1
#CMPWS_RT   RET
#label('cmpws#13')
#ld(min(0,maxTicks-38/2))        #13
#adda([vTicks])                  #14
#blt('.cmpws#17')                #15 not enough time left, so retry
#ld(0,Y)                         #16
#ld([X])                         #17
#st([Y,Xpp])                     #18
#st([0xB0])                      #19 cmp var.lo
#ld([X])                         #20
#st([0xB1])                      #21 cmp var.hi
#xora([vAC+1])                   #22
#blt('.cmpws#25')                #23 if < 0
#ld([vAC])                       #24 Low byte
#bmi('.cmpws#27')                #25
#suba([0xB0])                    #26
#st([vAC])                       #27 Store low result
#ora([0xB0])                     #28 Carry in bit 7
#anda(0x80,X)                    #29 Move carry to bit 0
#ld([vAC+1])                     #30
#suba([X])                       #31
#suba([0xB1])                    #32 sub var.hi with carry
#st([vAC+1])                     #33
#ld(hi('NEXTY'),Y)               #34
#jmp(Y,'NEXTY')                  #35
#ld(-38/2)                       #36
#
#label('.cmpws#25')
#ld(hi('REENTER'),Y)             #25
#jmp(Y,'REENTER')                #26
#ld(-30/2)                       #27
#
#label('.cmpws#27')
#st([vAC])                       #27 Store low result
#anda([0xB0])                    #28 Carry in bit 7
#anda(0x80,X)                    #29 Move carry to bit 0
#ld([vAC+1])                     #30
#suba([X])                       #31
#suba([0xB1])                    #32 sub var.hi with carry
#st([vAC+1])                     #33
#ld(hi('NEXTY'),Y)               #34
#jmp(Y,'NEXTY')                  #35
#ld(-38/2)                       #36
#
#label('.cmpws#17')
#ld(hi('PREFX2_PAGE'))           #17 ENTER is at $(n-1)ff, where n = instruction page
#st([vCpuSelect])                #18 restore PREFX2 instruction page
#adda(1,Y)                       #19 retry instruction
#jmp(Y,'REENTER')                #20
#ld(-24/2)                       #21


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       PREFX1 implementation page, (0x2700)
#-----------------------------------------------------------------------
#
# NOTE implementation: vAC = ROM:[NotesTable + vAC.lo*2], 22 + 46 cycles
label('note#13')
st([vTmp])                      #13 low byte of low note return address
ld(min(0,maxTicks-46/2))        #14
adda([vTicks])                  #15
blt('.note#18')                 #16 not enough time left, so retry
ld(hi('noteTrampoline'),Y)      #17
ld([vAC])                       #18 vAC.lo = note index
adda(AC)                        #19 vAC.lo*2, (low note byte)
jmp(Y,'noteTrampoline')         #20
st([vAC+1])                     #21 vAC.hi = vAC.lo*2
label('.note#28')
st([vAC])                       #28 vAC.lo = note.lo
ld('.note#41')                  #29
st([vTmp])                      #30 low byte of high note return address
ld(hi('noteTrampoline'),Y)      #31
ld([vAC+1])                     #32
jmp(Y,'noteTrampoline')         #33
adda(1)                         #34 vAC.lo*2 + 1, (high note byte)
label('.note#41')
st([vAC+1])                     #41
ld(hi('NEXTY'),Y)               #42
jmp(Y,'NEXTY')                  #43
ld(-46/2)                       #44
label('.note#18')
ld(hi('PREFX1_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX1 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# MIDI implementation: vAC = ROM:[NotesTable + (vAC.lo - 11)*2], 22 + 48 cycles
label('midi#13')
st([vTmp])                      #13 low byte of low midi return address
ld(min(0,maxTicks-48/2))        #14
adda([vTicks])                  #15
blt('.midi#18')                 #16 not enough time left, so retry
ld(hi('noteTrampoline'),Y)      #17
ld([vAC])                       #18 vAC.lo = midi index
suba(11)                        #19 vAC.lo -= 11
adda(AC)                        #20 vAC.lo*2, (low midi byte)
jmp(Y,'noteTrampoline')         #21
st([vAC+1])                     #22 vAC.hi = vAC.lo*2
label('.midi#29')
st([vAC])                       #29 vAC.lo = midi.lo
ld('.midi#42')                  #30
st([vTmp])                      #31 low byte of high midi return address
ld(hi('noteTrampoline'),Y)      #32
ld([vAC+1])                     #33
jmp(Y,'noteTrampoline')         #34
adda(1)                         #35 vAC.lo*2 + 1, (high midi byte)
label('.midi#42')
st([vAC+1])                     #42
ld(hi('REENTER'),Y)             #43
jmp(Y,'REENTER')                #44
ld(-48/2)                       #45
label('.midi#18')
ld(hi('PREFX1_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX1 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

# XLA, implementation, (lb3361)
label('xla#13')
st([vTmp])                      #13
ld([vLR])                       #14
st([vAC])                       #15
ld([vTmp])                      #16
st([vLR])                       #17
ld([vAC+1])                     #18
st([vTmp])                      #19
ld([vLR+1])                     #20
st([vAC+1])                     #21
ld([vTmp])                      #22
st([vLR+1])                     #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

# JMPI implementation, (lb3361)
label('jmpi#13')
st([vPC])                       #13
ld([sysArgs+7])                 #14
st([vPC+1])                     #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x2800)
#-----------------------------------------------------------------------
#
# ADDVI implementation
label('addvi#13')
ld([vPC+1],Y)                   #13 vPC.hi
st([sysArgs+6])                 #14 immediate 8bit
st([Y,Xpp])                     #15 X++
ld(min(0,maxTicks-50/2))        #16
adda([vTicks])                  #17
blt('.addvi#20')                #18 not enough time left, so retry
ld([Y,X])                       #19
st([sysArgs+7],X)               #20 dst var addr
ld(0,Y)                         #21
ld([X])                         #22 dst low value
st([vAC])                       #23
st([Y,Xpp])                     #24 X++
ld([X])                         #25 dst hi value
st([vAC+1])                     #26
ld([sysArgs+6])                 #27
adda([vAC])                     #28
st([vAC])                       #29 Store low result
bmi('.addvi#32')                #30 Now figure out if there was a carry
suba([sysArgs+6])               #31 Gets back the initial value of vAC
ora([sysArgs+6])                #32 Carry in bit 7
anda(0x80,X)                    #33 Move carry to bit 0
ld([X])                         #34
adda([vAC+1])                   #35 Add the high bytes with carry
st([vAC+1])                     #36 Store high result
ld([sysArgs+7],X)               #37
ld([vAC])                       #38
st([Y,Xpp])                     #39
ld([vAC+1])                     #40
st([X])                         #41
ld([vPC])                       #42
adda(1)                         #43
st([vPC])                       #44
ld(hi('REENTER'),Y)             #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47

label('.addvi#32')
anda([sysArgs+6])               #32 Carry in bit 7
anda(0x80,X)                    #33 Move carry to bit 0
ld([X])                         #34
adda([vAC+1])                   #35 Add the high bytes with carry
st([vAC+1])                     #36 Store high result
ld([sysArgs+7],X)               #37
ld([vAC])                       #38
st([Y,Xpp])                     #39
ld([vAC+1])                     #40
st([X])                         #41
ld([vPC])                       #42
adda(1)                         #43
st([vPC])                       #44
ld(hi('REENTER'),Y)             #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47

label('.addvi#20')
ld([vPC])                       #20 retry instruction
suba(2)                         #21
st([vPC])                       #22
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25


# SUBVI implementation
label('subvi#13')
ld([vPC+1],Y)                   #13 vPC.hi
st([sysArgs+6])                 #14 immediate 8bit
st([Y,Xpp])                     #15 X++
ld(min(0,maxTicks-50/2))        #16
adda([vTicks])                  #17
blt('.subvi#20')                #18 not enough time left, so retry
ld([Y,X])                       #19
st([sysArgs+7],X)               #20 dst var addr
ld(0,Y)                         #21
ld([X])                         #22 dst low value
st([vAC])                       #23
st([Y,Xpp])                     #24 X++
ld([X])                         #25 dst hi value
st([vAC+1])                     #26
ld([sysArgs+7],X)               #27 dst var addr
ld([X])                         #28
bmi('.subvi#31')                #29
suba([sysArgs+6])               #30
st([vAC])                       #31 store low result
ora([sysArgs+6])                #32 carry in bit 7
anda(0x80,X)                    #33 move carry to bit 0
ld([vAC+1])                     #34
suba([X])                       #35
st([vAC+1])                     #36 store high result
ld([sysArgs+7],X)               #37
ld([vAC])                       #38
st([Y,Xpp])                     #39
ld([vAC+1])                     #40
st([X])                         #41
ld([vPC])                       #42
adda(1)                         #43
st([vPC])                       #44
ld(hi('REENTER'),Y)             #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47

label('.subvi#31')
st([vAC])                       #31 store low result
anda([sysArgs+6])               #32 carry in bit 7
anda(0x80,X)                    #33 move carry to bit 0
ld([vAC+1])                     #34
suba([X])                       #35
st([vAC+1])                     #36 store high result
ld([sysArgs+7],X)               #37
ld([vAC])                       #38
st([Y,Xpp])                     #39
ld([vAC+1])                     #40
st([X])                         #41
ld([vPC])                       #42
adda(1)                         #43
st([vPC])                       #44
ld(hi('REENTER'),Y)             #45
jmp(Y,'REENTER')                #46
ld(-50/2)                       #47

label('.subvi#20')
ld([vPC])                       #20 retry instruction
suba(2)                         #21
st([vPC])                       #22
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25


# OSCPX implementation
label('oscpx#13')
st([sysArgs+6],X)               #13 sample address var
ld(min(0,maxTicks-42/2))        #14
adda([vTicks])                  #15
blt('.oscpx#18')                #16 not enough time left, so retry
ld([X])                         #17 sample address low byte
ld([sysArgs+7],X)               #18 index var
adda([X])                       #19 sample address low byte + index
st([vTmp])                      #20
ld([sysArgs+6])                 #21
adda(1,X)                       #22
ld([X])                         #23
ld(AC,Y)                        #24 sample address high byte
ld([vTmp],X)                    #25
ld([Y,X])                       #26
anda(0xFC)                      #27 6bit sample in upper 8bits
ld(soundTable>>8,Y)             #28
ora(0x03,X)                     #29 
ld([Y,X])                       #30 6bit sample, (>>2)
adda([vAC+1],Y)                 #31 dest pixel address high byte
ld([vAC])                       #32 dest pixel address low byte
ld([sysArgs+7],X)               #33 index
adda([X])                       #34 sample address low byte + index
ld(AC,X)                        #35
ld([0xA3])                      #36 pixel colour
st([Y,X])                       #37
ld(hi('NEXTY'),Y)               #38
jmp(Y,'NEXTY')                  #39
ld(-42/2)                       #40

label('.oscpx#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# CMPWS implementation, vAC = vAC CMPWS var
#           LDW   var0
#           CMPHS var1.hi
#           SUBW  var1
label('cmpws#13')
ld(-46/2)                       #13
st([vTmp])                      #14
ld(min(0,maxTicks-46/2))        #15
adda([vTicks])                  #16
blt('.cmpws#19')                #17 not enough time left, so retry
ld(0,Y)                         #18
ld([X])                         #19
st([Y,Xpp])                     #20
st([0xB0])                      #21 cmp var.lo
ld([X])                         #22
st([0xB1])                      #23 cmp var.hi
xora([vAC+1])                   #24
bpl('.cmpws#27')                #25 skip if same sign
ld([vAC+1])                     #26
bmi(pc()+3)                     #27
bra(pc()+3)                     #28

label('.cmpws#29')
ld(+1)                          #29    vAC < variable
ld(-1)                          #29(!) vAC > variable

label('.cmpws#30')
adda([X])                       #30
st([vAC+1])                     #31

label('.cmpws#32')
ld([vAC])                       #32, #30 Low byte
bmi('.cmpws#35')                #33, #31
suba([0xB0])                    #34, #32
st([vAC])                       #35, #33 Store low result
ora([0xB0])                     #36, #34 Carry in bit 7
anda(0x80,X)                    #37, #35 Move carry to bit 0
ld([vAC+1])                     #38, #36
suba([X])                       #39, #37
suba([0xB1])                    #40, #38 sub var.hi with carry
st([vAC+1])                     #41, #39
ld(hi('NEXTY'),Y)               #42, #40
jmp(Y,'NEXTY')                  #43, #41
ld([vTmp])                      #44, #42

label('.cmpws#27')
ld(-44/2)                       #27
bra('.cmpws#32')                #28
st([vTmp])                      #29

label('.cmpws#35')
st([vAC])                       #35, #33 Store low result
anda([0xB0])                    #36, #34 Carry in bit 7
anda(0x80,X)                    #37, #35 Move carry to bit 0
ld([vAC+1])                     #38, #36
suba([X])                       #39, #37
suba([0xB1])                    #40, #38 sub var.hi with carry
st([vAC+1])                     #41, #39
ld(hi('NEXTY'),Y)               #42, #40
jmp(Y,'NEXTY')                  #43, #41
ld([vTmp])                      #44, #42

label('.cmpws#19')
ld(hi('PREFX2_PAGE'))           #19 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #20 restore PREFX2 instruction page
adda(1,Y)                       #21 retry instruction
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23


# CMPWU implementation, vAC = vAC CMPWU var
#           LDW   var0
#           CMPHU var1.hi
#           SUBW  var1
label('cmpwu#13')
ld(-46/2)                       #13
st([vTmp])                      #14
ld(min(0,maxTicks-46/2))        #15
adda([vTicks])                  #16
blt('.cmpws#19')                #17 not enough time left, so retry, (use .cmpws#19)
ld(0,Y)                         #18
ld([X])                         #19
st([Y,Xpp])                     #20
st([0xB0])                      #21 cmp var.lo
ld([X])                         #22
st([0xB1])                      #23 cmp var.hi
xora([vAC+1])                   #24
bpl('.cmpws#27')                #25 skip if same sign, (use .cmpws#27)
ld([vAC+1])                     #26
bmi('.cmpws#29')                #27 use .cmpws#29
bra('.cmpws#30')                #28 use .cmpws#30
ld(-1)                          #29 vAC > variable


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x2900)
#-----------------------------------------------------------------------
#
# LEEKA implementation
label('leeka#13')
st([vTmp])                      #13
ld(min(0,maxTicks-44/2))        #14
adda([vTicks])                  #15
blt('.leeka#18')                #16 not enough time left, so retry
ld([vAC+1],Y)                   #17
ld([vAC],X)                     #18
ld([Y,X])                       #19 peek [vAC]
st([Y,Xpp])                     #20
st([vTmpL+0])                   #21
ld([Y,X])                       #22 peek [vAC+1]
st([Y,Xpp])                     #23
st([vTmpL+1])                   #24
ld([Y,X])                       #25 peek [vAC+2]
st([Y,Xpp])                     #26
st([vTmpL+2])                   #27
ld([Y,X])                       #28 peek [vAC+3]
st([vTmpL+3])                   #29
ld(0,Y)                         #30
ld([vTmp],X)                    #31 dst var
ld([vTmpL+0])                   #32
st([Y,Xpp])                     #33 [var.0] = peek [vAC]
ld([vTmpL+1])                   #34
st([Y,Xpp])                     #35 [var.1] = peek [vAC+1]
ld([vTmpL+2])                   #36
st([Y,Xpp])                     #37 [var.2] = peek [vAC+2]
ld([vTmpL+3])                   #38
st([Y,X])                       #39 [var.3] = peek [vAC+3]
ld(hi('NEXTY'),Y)               #40
jmp(Y,'NEXTY')                  #41
ld(-44/2)                       #42

label('.leeka#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# LOKEA implementation
label('lokea#13')
ld(AC,X)                        #13
ld(min(0,maxTicks-44/2))        #14
adda([vTicks])                  #15
blt('.lokea#18')                #16 not enough time left, so retry
ld(0,Y)                         #17
ld([X])                         #18 var.0
st([Y,Xpp])                     #19
st([vTmpL+0])                   #20
ld([X])                         #21 var.1
st([Y,Xpp])                     #22
st([vTmpL+1])                   #23
ld([X])                         #24 var.2
st([Y,Xpp])                     #25
st([vTmpL+2])                   #26
ld([X])                         #27 var.3
st([vTmpL+3])                   #28
ld([vAC],X)                     #29
ld([vAC+1],Y)                   #30
ld([vTmpL+0])                   #31
st([Y,Xpp])                     #32 poke [vAC], var.0
ld([vTmpL+1])                   #33
st([Y,Xpp])                     #34 poke [vAC], var.1
ld([vTmpL+2])                   #35
st([Y,Xpp])                     #36 poke [vAC], var.2
ld([vTmpL+3])                   #37
st([Y,X])                       #38 poke [vAC], var.3
ld(hi('REENTER'),Y)             #39
jmp(Y,'REENTER')                #40
ld(-44/2)                       #41

label('.lokea#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# FEEKA implementation
label('feeka#13')
ld([vAC+1],Y)                   #13
ld(min(0,maxTicks-48/2))        #14
adda([vTicks])                  #15
blt('.feeka#18')                #16 not enough time left, so retry
ld([vAC],X)                     #17
ld([Y,X])                       #18 peek [vAC]
st([Y,Xpp])                     #19
st([0xB0])                      #20
ld([Y,X])                       #21 peek [vAC+1]
st([Y,Xpp])                     #22
st([0xB1])                      #23
ld([Y,X])                       #24 peek [vAC+2]
st([Y,Xpp])                     #25
st([0xB2])                      #26
ld([Y,X])                       #27 peek [vAC+3]
st([Y,Xpp])                     #28
st([0xB3])                      #29
ld([Y,X])                       #30 peek [vAC+4]
st([0xB4])                      #31
ld(0,Y)                         #32
ld([vTmp],X)                    #33 dst var
ld([0xB0])                      #34
st([Y,Xpp])                     #35 [var.0] = peek [vAC]
ld([0xB1])                      #36
st([Y,Xpp])                     #37 [var.1] = peek [vAC+1]
ld([0xB2])                      #38
st([Y,Xpp])                     #39 [var.2] = peek [vAC+2]
ld([0xB3])                      #40
st([Y,X])                       #41 [var.3] = peek [vAC+3]
ld([0xB4])                      #42
st([Y,X])                       #43 [var.4] = peek [vAC+4]
ld(hi('NEXTY'),Y)               #44
jmp(Y,'NEXTY')                  #45
ld(-48/2)                       #46

label('.feeka#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# FOKEA implementation
label('fokea#13')
ld(AC,X)                        #13 var.0
ld(min(0,maxTicks-48/2))        #14
adda([vTicks])                  #15
blt('.fokea#18')                #16 not enough time left, so retry
ld(0,Y)                         #17
ld([X])                         #18 [var.0]
st([Y,Xpp])                     #19
st([0xB0])                      #20
ld([X])                         #21 [var.1]
st([Y,Xpp])                     #22
st([0xB1])                      #23
ld([X])                         #24 [var.2]
st([Y,Xpp])                     #25
st([0xB2])                      #26
ld([X])                         #27 [var.3]
st([Y,Xpp])                     #28
st([0xB3])                      #29
ld([X])                         #30 [var.4]
st([0xB4])                      #31
ld([vAC],X)                     #32
ld([vAC+1],Y)                   #33
ld([0xB0])                      #34
st([Y,Xpp])                     #35 poke [vAC], [var.0]
ld([0xB1])                      #36
st([Y,Xpp])                     #37 poke [vAC], [var.1]
ld([0xB2])                      #38
st([Y,Xpp])                     #39 poke [vAC], [var.2]
ld([0xB3])                      #40
st([Y,X])                       #41 poke [vAC], [var.3]
ld([0xB4])                      #42
st([Y,X])                       #43 poke [vAC], [var.4]
ld(hi('NEXTY'),Y)               #44
jmp(Y,'NEXTY')                  #45
ld(-48/2)                       #46

label('.fokea#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# SWAPB implementation
label('swapb#13')
ld(min(0,maxTicks-46/2))        #13
adda([vTicks])                  #14
blt('.swapb#17')                #15 not enough time left, so retry
ld(0,Y)                         #16
ld([X])                         #17
st([Y,Xpp])                     #18
st([0xB2])                      #19 var1.lo
ld([X])                         #20
st([0xB3])                      #21 var1.hi
ld([sysArgs+7],X)               #22 var0
ld([X])                         #23
st([Y,Xpp])                     #24
st([0xB4])                      #25 var0.lo
ld([X])                         #26
st([0xB5])                      #27 var0.hi
ld([0xB2],X)                    #28
ld([0xB3],Y)                    #29
ld([Y,X])                       #30
st([0xB1])                      #31 tmp1 = [var1]
ld([0xB4],X)                    #32
ld([0xB5],Y)                    #33
ld([Y,X])                       #34
st([0xB0])                      #35 tmp0 = [var0]
ld([0xB1])                      #36
st([Y,X])                       #37 [var0] = tmp1
ld([0xB2],X)                    #38
ld([0xB3],Y)                    #39
ld([0xB0])                      #40
st([Y,X])                       #41 [var1] = tmp0
ld(hi('NEXTY'),Y)               #42
jmp(Y,'NEXTY')                  #43
ld(-46/2)                       #44

label('.swapb#17')
ld(hi('PREFX3_PAGE'))           #17 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #18 restore PREFX3 instruction page
adda(1,Y)                       #19 retry instruction
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21


# SWAPW implementation
label('swapw#13')
ld(min(0,maxTicks-58/2))        #13
adda([vTicks])                  #14
blt('.swapw#17')                #15 not enough time left, so retry
ld(0,Y)                         #16
ld([X])                         #17
st([Y,Xpp])                     #18
st([0xB2])                      #19 var1.lo
ld([X])                         #20
st([0xB3])                      #21 var1.hi
ld([sysArgs+7],X)               #22 var0
ld([X])                         #23
st([Y,Xpp])                     #24
st([0xB4])                      #25 var0.lo
ld([X])                         #26
st([0xB5])                      #27 var0.hi
ld([0xB2],X)                    #28
ld([0xB3],Y)                    #29
ld([Y,X])                       #30
st([Y,Xpp])                     #31
st([0xB0])                      #32 temp.lo = var1.lo
ld([Y,X])                       #33
st([0xB1])                      #34 temp.hi = var1.hi
ld([0xB4],X)                    #35
ld([0xB5],Y)                    #36
ld([Y,X])                       #37
st([Y,Xpp])                     #38
st([vAC])                       #39
ld([Y,X])                       #40
st([vAC+1])                     #41
ld([0xB2],X)                    #42
ld([0xB3],Y)                    #43
ld([vAC])                       #44
st([Y,Xpp])                     #45 var1.lo = var0.lo
ld([vAC+1])                     #46
st([Y,X])                       #47 var1.lo = var0.lo
ld([0xB4],X)                    #48
ld([0xB5],Y)                    #49
ld([0xB0])                      #50
st([Y,Xpp])                     #51 var0.lo = temp.lo
ld([0xB1])                      #52
st([Y,X])                       #53 var0.hi = temp.hi
ld(hi('NEXTY'),Y)               #54
jmp(Y,'NEXTY')                  #55
ld(-58/2)                       #56

label('.swapw#17')
ld(hi('PREFX3_PAGE'))           #17 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #18 restore PREFX3 instruction page
adda(1,Y)                       #19 retry instruction
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x2A00)
#-----------------------------------------------------------------------
#
# MEEKA implementation
label('meeka#13')
st([vTmp])                      #13
ld(min(0,maxTicks-64/2))        #14
adda([vTicks])                  #15
blt('.meeka#18')                #16 not enough time left, so retry
ld([vAC+1],Y)                   #17
ld([vAC],X)                     #18
ld([Y,X])                       #19 peek [vAC]
st([Y,Xpp])                     #20
st([0xB0])                      #21
ld([Y,X])                       #22 peek [vAC+1]
st([Y,Xpp])                     #23
st([0xB1])                      #24
ld([Y,X])                       #25 peek [vAC+2]
st([Y,Xpp])                     #26
st([0xB2])                      #27
ld([Y,X])                       #28 peek [vAC+3]
st([Y,Xpp])                     #29
st([0xB3])                      #30
ld([Y,X])                       #31 peek [vAC+4]
st([Y,Xpp])                     #32
st([0xB4])                      #33
ld([Y,X])                       #34 peek [vAC+5]
st([Y,Xpp])                     #35
st([0xB5])                      #36
ld([Y,X])                       #37 peek [vAC+6]
st([Y,Xpp])                     #38
st([0xB6])                      #39
ld([Y,X])                       #40 peek [vAC+7]
st([0xB7])                      #41
ld(0,Y)                         #42
ld([vTmp],X)                    #43 dst var
ld([0xB0])                      #44
st([Y,Xpp])                     #45 [var.0] = peek [vAC]
ld([0xB1])                      #46
st([Y,Xpp])                     #47 [var.1] = peek [vAC+1]
ld([0xB2])                      #48
st([Y,Xpp])                     #49 [var.2] = peek [vAC+2]
ld([0xB3])                      #50
st([Y,Xpp])                     #51 [var.3] = peek [vAC+3]
ld([0xB4])                      #52
st([Y,Xpp])                     #53 [var.0] = peek [vAC+4]
ld([0xB5])                      #54
st([Y,Xpp])                     #55 [var.1] = peek [vAC+5]
ld([0xB6])                      #56
st([Y,Xpp])                     #57 [var.2] = peek [vAC+6]
ld([0xB7])                      #58
st([Y,X])                       #59 [var.3] = peek [vAC+7]
ld(hi('NEXTY'),Y)               #60
jmp(Y,'NEXTY')                  #61
ld(-64/2)                       #62

label('.meeka#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# MOKEA
label('mokea#13')
ld(AC,X)                        #13 var
ld(min(0,maxTicks-64/2))        #14
adda([vTicks])                  #15
blt('.mokea#18')                #16 not enough time left, so retry
ld(0,Y)                         #17
ld([X])                         #18 var.0
st([Y,Xpp])                     #19
st([0xB0])                      #20
ld([X])                         #21 var.1
st([Y,Xpp])                     #22
st([0xB1])                      #23
ld([X])                         #24 var.2
st([Y,Xpp])                     #25
st([0xB2])                      #26
ld([X])                         #27 var.3
st([Y,Xpp])                     #28
st([0xB3])                      #29
ld([X])                         #30 var.4
st([Y,Xpp])                     #31
st([0xB4])                      #32
ld([X])                         #33 var.5
st([Y,Xpp])                     #34
st([0xB5])                      #35
ld([X])                         #36 var.6
st([Y,Xpp])                     #37
st([0xB6])                      #38
ld([X])                         #39 var.7
st([Y,Xpp])                     #40
st([0xB7])                      #41
ld([vAC],X)                     #42
ld([vAC+1],Y)                   #43
ld([0xB0])                      #44
st([Y,Xpp])                     #45 poke [vAC], var.0
ld([0xB1])                      #46
st([Y,Xpp])                     #47 poke [vAC], var.1
ld([0xB2])                      #48
st([Y,Xpp])                     #49 poke [vAC], var.2
ld([0xB3])                      #50
st([Y,Xpp])                     #51 poke [vAC], var.3
ld([0xB4])                      #52
st([Y,Xpp])                     #53 poke [vAC], var.4
ld([0xB5])                      #54
st([Y,Xpp])                     #55 poke [vAC], var.5
ld([0xB6])                      #56
st([Y,Xpp])                     #57 poke [vAC], var.6
ld([0xB7])                      #58
st([Y,Xpp])                     #59 poke [vAC], var.7
ld(hi('NEXTY'),Y)               #60
jmp(Y,'NEXTY')                  #61
ld(-64/2)                       #62

label('.mokea#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# NEEKA implementation
label('neeka#13')
ld(min(0,maxTicks-34/2))        #13
adda([vTicks])                  #14
blt('.neeka#17')                #15 not enough time left, so retry
ld([sysArgs+7])                 #16 number of bytes to peek
suba(1)                         #17
bge('.neeka#20')                #18
st([sysArgs+7])                 #19
ld(hi('NEXTY'),Y)               #20 done
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

label('.neeka#20')
ld([vAC+1],Y)                   #20
adda([vAC],X)                   #21
ld([Y,X])                       #22 peek [vAC + index]
st([vTmp])                      #23
ld([sysArgs+6])                 #24
adda([sysArgs+7],X)             #25 var + index
ld([vTmp])                      #26
st([X])                         #27 [var + index] = peek [vAC + index]
ld(hi('PREFX3_PAGE'))           #28 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #29 restore PREFX3 instruction page
adda(1,Y)                       #30 restart instruction
jmp(Y,'NEXTY')                  #31
ld(-34/2)                       #32

label('.neeka#17')
ld(hi('PREFX3_PAGE'))           #17 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #18 restore PREFX3 instruction page
adda(1,Y)                       #19 retry instruction
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21


# NOKEA implementation
label('nokea#13')
ld(min(0,maxTicks-34/2))        #13
adda([vTicks])                  #14
blt('.nokea#17')                #15 not enough time left, so retry
ld([sysArgs+7])                 #16 number of bytes to peek
suba(1)                         #17
bge('.nokea#20')                #18
st([sysArgs+7])                 #19
ld(hi('NEXTY'),Y)               #20 done
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

label('.nokea#20')
adda([sysArgs+6],X)             #20 var + index
ld([X])                         #21
st([vTmp])                      #22
ld([vAC+1],Y)                   #23
ld([sysArgs+7])                 #24
adda([vAC],X)                   #25 vAC + index
ld([vTmp])                      #26
st([Y,X])                       #27 [vAC + index] = peek [var + index]
ld(hi('PREFX3_PAGE'))           #28 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #29 restore PREFX3 instruction page
adda(1,Y)                       #30 restart instruction
jmp(Y,'NEXTY')                  #31
ld(-34/2)                       #32

label('.nokea#17')
ld(hi('PREFX3_PAGE'))           #17 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #18 restore PREFX3 instruction page
adda(1,Y)                       #19 retry instruction
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21
 

# DJNE implementation
label('djne#13')
st([Y,Xpp])                     #13 X++
st([vTmp])                      #14 var
ld(min(0,maxTicks-46/2))        #15
adda([vTicks])                  #16
blt('.djne#19')                 #17 not enough time left, so retry
ld([Y,X])                       #18
st([Y,Xpp])                     #19 X++
st([sysArgs+6])                 #20 jump addr.lo
ld([Y,X])                       #21
st([sysArgs+7])                 #22 jump addr.hi
ld(0,Y)                         #23
ld([vTmp],X)                    #24
ld([X])                         #25 count.lo
suba(1)                         #26
st([Y,Xpp])                     #27 count.lo-- X++
ora([X])                        #28 count.lo | count.hi
beq('.djne#31')                 #19 count = 0
ld([vTmp],X)                    #30
ld([X])                         #31 count.lo
st([Y,Xpp])                     #32 X++
xora(0xFF)                      #33 if low byte is 0xFF
bne('.djne#36')                 #34
ld([X])                         #35 count.hi
suba(1)                         #36
st([X])                         #37
ld([sysArgs+6])                 #38 jump
st([vPC])                       #39
ld([sysArgs+7])                 #40
st([vPC+1])                     #41
ld(hi('NEXTY'),Y)               #42
jmp(Y,'NEXTY')                  #43
ld(-46/2)                       #44

label('.djne#31')
ld([vPC])                       #31 done
adda(2)                         #32
st([vPC])                       #33
ld(hi('NEXTY'),Y)               #34
jmp(Y,'NEXTY')                  #35
ld(-38/2)                       #36

label('.djne#36')
ld([sysArgs+6])                 #36 jump
st([vPC])                       #37
ld([sysArgs+7])                 #38
st([vPC+1])                     #39
ld(hi('NEXTY'),Y)               #40
jmp(Y,'NEXTY')                  #41
ld(-44/2)                       #42

label('.djne#19')
ld([vPC])                       #19 retry
suba(2)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x2B00)
#-----------------------------------------------------------------------
#
# DJGE implementation
label('djge#13')
st([Y,Xpp])                     #13
st([vTmp])                      #14 var
ld(min(0,maxTicks-42/2))        #15
adda([vTicks])                  #16
blt('.djge#19')                 #17 not enough time left, so retry
ld([Y,X])                       #18
st([Y,Xpp])                     #19
st([sysArgs+6])                 #20 jump addr.lo
ld([Y,X])                       #21
st([sysArgs+7])                 #22 jump addr.hi
ld(0,Y)                         #23
ld([vTmp],X)                    #24
ld([X])                         #25 count.lo
suba(1)                         #26
st([Y,Xpp])                     #27 count.lo-- X++
xora(0xFF)                      #28 if low byte is 0xFF
bne('.djge#31')                 #29
ld([X])                         #30 count.hi
beq('.djge#33')                 #31
suba(1)                         #32
st([X])                         #33
ld([sysArgs+6])                 #34 jump
st([vPC])                       #35
ld([sysArgs+7])                 #36
st([vPC+1])                     #37
ld(hi('NEXTY'),Y)               #38
jmp(Y,'NEXTY')                  #39
ld(-42/2)                       #40

label('.djge#31')
ld([sysArgs+6])                 #31 jump
st([vPC])                       #32
ld([sysArgs+7])                 #33
st([vPC+1])                     #34
ld(hi('REENTER'),Y)             #35
jmp(Y,'REENTER')                #36
ld(-40/2)                       #37

label('.djge#33')
ld([vPC])                       #33 done
adda(2)                         #34
st([vPC])                       #35
ld(hi('NEXTY'),Y)               #36
jmp(Y,'NEXTY')                  #37
ld(-40/2)                       #38

label('.djge#19')
ld([vPC])                       #19 retry
suba(2)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


# ADDVL
label('addvl#13')
ld([X])                         #13
st([vTmp])                      #14 save src.0
ld([sysArgs+7],X)               #15 dst.0
ld(min(0,maxTicks-78/2))        #16
adda([vTicks])                  #17
blt('.addvl#20')                #18 not enough time left, so retry
ld([vTmp])                      #19 src.0
adda([X])                       #20
st([X])                         #21 dst.0 += src.0
bmi('.addvl#24')                #22 carry
suba([vTmp])                    #23 original dst.0
bra('.addvl#26')                #24
ora([vTmp])                     #25 carry in bit 7

label('.addvl#24')
anda([vTmp])                    #24 carry in bit 7
nop()                           #25

label('.addvl#26')
anda(0x80,X)                    #26 carry to bit 0
ld([X])                         #27
st([vTmp])                      #28 store carry
ld([sysArgs+6])                 #29
adda(1,X)                       #30 src.1
ld([vTmp])                      #31
adda([X])                       #32
st([vTmp])                      #33 src.1 += carry
ld([sysArgs+7])                 #34
adda(1,X)                       #35
ld([vTmp])                      #36
adda([X])                       #37
st([X])                         #38 dst.1 += src.1
bmi('.addvl#41')                #39 carry
suba([vTmp])                    #40 original dst.1
bra('.addvl#43')                #41
ora([vTmp])                     #42 carry in bit 7

label('.addvl#41')
anda([vTmp])                    #41 carry in bit 7
nop()                           #42

label('.addvl#43')
anda(0x80,X)                    #43 carry to bit 0
ld([X])                         #44
st([vTmp])                      #45 store carry
ld([sysArgs+6])                 #46
adda(2,X)                       #47 src.2
ld([vTmp])                      #48
adda([X])                       #49
st([vTmp])                      #50 src.2 += carry
ld([sysArgs+7])                 #51
adda(2,X)                       #52
ld([vTmp])                      #53
adda([X])                       #54
st([X])                         #55 dst.2 += src.2
bmi('.addvl#58')                #56 carry
suba([vTmp])                    #57 original dst.2
bra('.addvl#60')                #58
ora([vTmp])                     #59 carry in bit 7

label('.addvl#58')
anda([vTmp])                    #58 carry in bit 7
nop()                           #59

label('.addvl#60')
anda(0x80,X)                    #60 carry to bit 0
ld([X])                         #61
st([vTmp])                      #62 store carry
ld([sysArgs+6])                 #63
adda(3,X)                       #64 src.3
ld([vTmp])                      #65
adda([X])                       #66
st([vTmp])                      #67 src.3 += carry
ld([sysArgs+7])                 #68
adda(3,X)                       #69
ld([vTmp])                      #70
adda([X])                       #71
st([X])                         #72 dst.3 += src.3
ld(hi('REENTER'),Y)             #73
jmp(Y,'REENTER')                #74
ld(-78/2)                       #75

label('.addvl#20')
ld(hi('PREFX3_PAGE'))           #20 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #21 restore PREFX3 instruction page
adda(1,Y)                       #22 retry instruction
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


# SUBVL
label('subvl#13')
ld([X])                         #13
st([vTmp])                      #14 save src.0
ld([sysArgs+7],X)               #15 dst.0
ld(min(0,maxTicks-74/2))        #16
adda([vTicks])                  #17
blt('.subvl#20')                #18 not enough time left, so retry
ld([X])                         #19 dst.0
bmi('.subvl#22')                #20
suba([vTmp])                    #21
st([X])                         #22 dst.0 -= src.0
bra('.subvl#25')                #23
ora([vTmp])                     #24 borrow in bit 7

label('.subvl#22')
st([X])                         #22 dst.0 -= src.0
anda([vTmp])                    #23 borrow in bit 7
nop()                           #24

label('.subvl#25')
anda(0x80,X)                    #25 borrow to bit 0
ld([X])                         #26
st([0xB0])                      #27 store borrow
ld([sysArgs+6])                 #28
adda(1,X)                       #29 
ld([X])                         #30
st([vTmp])                      #31 src.1
ld([sysArgs+7])                 #32
adda(1,X)                       #33
ld([X])                         #34 dst.1 
bmi('.subvl#37')                #35
suba([vTmp])                    #36
suba([0xB0])                    #37
st([X])                         #38 dst.1 = dst.1 - src.1 - borrow
bra('.subvl#41')                #39
ora([vTmp])                     #40 borrow in bit 7

label('.subvl#37')
suba([0xB0])                    #37
st([X])                         #38 dst.1 = dst.1 - src.1 - borrow
anda([vTmp])                    #39 borrow in bit 7
nop()                           #40

label('.subvl#41')
anda(0x80,X)                    #41 borrow to bit 0
ld([X])                         #42
st([0xB0])                      #43 store borrow
ld([sysArgs+6])                 #44
adda(2,X)                       #45
ld([X])                         #46
st([vTmp])                      #47 src.2
ld([sysArgs+7])                 #48
adda(2,X)                       #49
ld([X])                         #50 dst.2
bmi('.subvl#53')                #51
suba([vTmp])                    #52
suba([0xB0])                    #53
st([X])                         #54 dst.2 = dst.2 - src.2 - borrow
bra('.subvl#57')                #55
ora([vTmp])                     #56 borrow in bit 7

label('.subvl#53')
suba([0xB0])                    #53
st([X])                         #54 dst.2 = dst.2 - src.2 - borrow
anda([vTmp])                    #55 borrow in bit 7
nop()                           #56

label('.subvl#57')
anda(0x80,X)                    #57 borrow to bit 0
ld([X])                         #58
st([0xB0])                      #59 store borrow
ld([sysArgs+6])                 #60
adda(3,X)                       #61
ld([X])                         #62
st([vTmp])                      #63 src.3
ld([sysArgs+7])                 #64
adda(3,X)                       #65 dst.3
ld([X])                         #66
suba([vTmp])                    #67
suba([0xB0])                    #68
st([X])                         #69 dst.3 = dst.3 - src.3 - borrow
ld(hi('NEXTY'),Y)               #70
jmp(Y,'NEXTY')                  #71
ld(-74/2)                       #72

label('.subvl#20')
ld(hi('PREFX3_PAGE'))           #20 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #21 restore PREFX3 instruction page
adda(1,Y)                       #22 retry instruction
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


# ANDVL
label('andvl#13')
ld(0,Y)                         #13
ld(min(0,maxTicks-46/2))        #14
adda([vTicks])                  #15
blt('.andvl#18')                #16 not enough time left, so retry
ld([Y,X])                       #17 src.0
st([Y,Xpp])                     #18
st([0xB0])                      #19
ld([Y,X])                       #20 src.1
st([Y,Xpp])                     #21
st([0xB1])                      #22
ld([Y,X])                       #23 src.2
st([Y,Xpp])                     #24
st([0xB2])                      #25
ld([X])                         #26 src.3
st([0xB3])                      #27
ld([sysArgs+7],X)               #28
ld([X])                         #29 dst.0
anda([0xB0])                    #30
st([Y,Xpp])                     #31 dst.0 &= src.0
ld([X])                         #32 dst.1
anda([0xB1])                    #33
st([Y,Xpp])                     #34 dst.1 &= src.1
ld([X])                         #35 dst.2
anda([0xB2])                    #36
st([Y,Xpp])                     #37 dst.2 &= src.2
ld([X])                         #38 dst.3
anda([0xB3])                    #39
st([Y,Xpp])                     #40 dst.3 &= src.3
ld(hi('REENTER'),Y)             #41
jmp(Y,'REENTER')                #42
ld(-46/2)                       #43

label('.andvl#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x2C00)
#-----------------------------------------------------------------------
#
# ORVL
label('orvl#13')
ld(0,Y)                         #13
ld(min(0,maxTicks-46/2))        #14
adda([vTicks])                  #15
blt('.orvl#18')                 #16 not enough time left, so retry
ld([Y,X])                       #17 src.0
st([Y,Xpp])                     #18
st([0xB0])                      #19
ld([Y,X])                       #20 src.1
st([Y,Xpp])                     #21
st([0xB1])                      #22
ld([Y,X])                       #23 src.2
st([Y,Xpp])                     #24
st([0xB2])                      #25
ld([X])                         #26 src.3
st([0xB3])                      #27
ld([sysArgs+7],X)               #28
ld([X])                         #29 dst.0
ora([0xB0])                     #30
st([Y,Xpp])                     #31 dst.0 |= src.0
ld([X])                         #32 dst.1
ora([0xB1])                     #33
st([Y,Xpp])                     #34 dst.1 |= src.1
ld([X])                         #35 dst.2
ora([0xB2])                     #36
st([Y,Xpp])                     #37 dst.2 |= src.2
ld([X])                         #38 dst.3
ora([0xB3])                     #39
st([Y,Xpp])                     #40 dst.3 |= src.3
ld(hi('REENTER'),Y)             #41
jmp(Y,'REENTER')                #42
ld(-46/2)                       #43

label('.orvl#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# XORVL
label('xorvl#13')
ld(0,Y)                         #13
ld(min(0,maxTicks-46/2))        #14
adda([vTicks])                  #15
blt('.xorvl#18')                #16 not enough time left, so retry
ld([Y,X])                       #17 src.0
st([Y,Xpp])                     #18
st([0xB0])                      #19
ld([Y,X])                       #20 src.1
st([Y,Xpp])                     #21
st([0xB1])                      #22
ld([Y,X])                       #23 src.2
st([Y,Xpp])                     #24
st([0xB2])                      #25
ld([X])                         #26 src.3
st([0xB3])                      #27
ld([sysArgs+7],X)               #28
ld([X])                         #29 dst.0
xora([0xB0])                    #30
st([Y,Xpp])                     #31 dst.0 ^= src.0
ld([X])                         #32 dst.1
xora([0xB1])                    #33
st([Y,Xpp])                     #34 dst.1 ^= src.1
ld([X])                         #35 dst.2
xora([0xB2])                    #36
st([Y,Xpp])                     #37 dst.2 ^= src.2
ld([X])                         #38 dst.3
xora([0xB3])                    #39
st([Y,Xpp])                     #40 dst.3 ^= src.3
ld(hi('REENTER'),Y)             #41
jmp(Y,'REENTER')                #42
ld(-46/2)                       #43

label('.xorvl#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# LSRV implementation
label('lsrv#13')
st([sysArgs+7],X)               #13 var
ld(min(0,maxTicks-56/2))        #14
adda([vTicks])                  #15
blt('.lsrv#18')                 #16 not enough time left, so retry
ld(hi('shiftTable'),Y)          #17 (ac >> 1) table
ld('.lsrv#27')                  #18 continuation address
st([vTmp])                      #19
ld([X])                         #20 load byte from var
anda(0xfe)                      #21
jmp(Y,AC)                       #22
bra(255)                        #23 bra shiftTable+255
# continues in page 0x0600 at label('.lsrv#27') fetching shifted byte from 0x0500

label('.lsrv#30')
ld([sysArgs+7])                 #30 low byte address
adda(1)                         #31
st([sysArgs+6],X)               #32 high byte address
ld([X])                         #33 bit 0 of high byte
anda(1)                         #34
adda(127)                       #35
anda(128)                       #36 
ld([sysArgs+7],X)               #37
ora([X])                        #38
st([X])                         #39 transfer to bit 7 of low byte
ld(hi('shiftTable'),Y)          #40 (ac >> 1) table
ld('.lsrv#51')                  #41 continuation address
st([vTmp])                      #42
ld([sysArgs+6],X)               #43 
ld([X])                         #44
anda(0b11111110)                #45
jmp(Y,AC)                       #46
bra(255)                        #47 bra shiftTable+255
# continues in page 0x0600 at label('.lsrv#51') fetching shifted byte from 0x0500

label('.lsrv#18')
ld([vPC])                       #18 retry instruction
suba(2)                         #19
st([vPC])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23


# LSRVL implementation
label('lsrvl#13')
ld(hi('shiftTable'),Y)          #13 (ac >> 1) table
ld(min(0,maxTicks-104/2))       #14
adda([vTicks])                  #15
blt('.lsrvl#18')                #16 not enough time left, so retry
ld('.lsrvl#26')                 #17 continuation address
st([vTmp])                      #18
ld([X])                         #19 load byte 0 from var
anda(0xfe)                      #20
jmp(Y,AC)                       #21
bra(255)                        #22 bra shiftTable+255
# continues in page 0x0600 at label('.lsrvl#26') fetching shifted byte from 0x0500

label('.lsrvl#29')
ld([sysArgs+7])                 #29 byte 0 address
adda(1)                         #30
st([0xB0],X)                    #31 byte 1 address
adda(1)                         #32
st([0xB1])                      #33 byte 2 address
adda(1)                         #34
st([0xB2])                      #35 byte 3 address
ld([X])                         #36 bit 0 of byte 1
anda(1)                         #37
adda(127)                       #38
anda(128)                       #39 
ld([sysArgs+7],X)               #40 byte 0 address
ora([X])                        #41
st([X])                         #42 transfer to bit 7 of byte 0
ld(hi('shiftTable'),Y)          #43 (ac >> 1) table
ld('.lsrvl#54')                 #44 continuation address
st([vTmp])                      #45
ld([0xB0],X)                    #46 byte 1 address
ld([X])                         #47
anda(0b11111110)                #48
jmp(Y,AC)                       #49
bra(255)                        #50 bra shiftTable+255
# continues in page 0x0600 at label('.lsrvl#54') fetching shifted byte from 0x0500

label('.lsrvl#57')
ld([0xB1],X)                    #57 byte 2 address
ld([X])                         #58 bit 0 of byte 2
anda(1)                         #59
adda(127)                       #60
anda(128)                       #61 
ld([0xB0],X)                    #62 byte 1 address
ora([X])                        #63
st([X])                         #64 transfer to bit 7 of byte 1
ld(hi('shiftTable'),Y)          #65 (ac >> 1) table
ld('.lsrvl#76')                 #66 continuation address
st([vTmp])                      #67
ld([0xB1],X)                    #68 byte 2 address 
ld([X])                         #69
anda(0b11111110)                #70
jmp(Y,AC)                       #71
bra(255)                        #72 bra shiftTable+255
# continues in page 0x0600 at label('.lsrvl#76') fetching shifted byte from 0x0500

label('.lsrvl#79')
ld([0xB2],X)                    #79 byte 3 address
ld([X])                         #80 bit 0 of byte 3
anda(1)                         #81
adda(127)                       #82
anda(128)                       #83 
ld([0xB1],X)                    #84 byte 2 address
ora([X])                        #85
st([X])                         #86 transfer to bit 7 of byte 2
ld(hi('shiftTable'),Y)          #87 (ac >> 1) table
ld('.lsrvl#98')                 #88 continuation address
st([vTmp])                      #89
ld([0xB2],X)                    #90 byte 3 address 
ld([X])                         #91
anda(0b11111110)                #92
jmp(Y,AC)                       #93
bra(255)                        #94 bra shiftTable+255
# continues in page 0x0600 at label('.lsrvl#98') fetching shifted byte from 0x0500

label('.lsrvl#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# LSLVL implementation
label('lslvl#13')
ld(0,Y)                         #13
ld(min(0,maxTicks-56/2))        #14
adda([vTicks])                  #15
blt('.lslvl#18')                #16 not enough time left, so retry
ld([X])                         #17
bge(pc()+3)                     #18
bra(pc()+3)                     #19
ld(1)                           #20
ld(0)                           #20
st([0xB0])                      #21 (var.0 & 0x80) >>7
ld([X])                         #22
adda([X])                       #23
st([Y,Xpp])                     #24 var.0 <<1
ld([X])                         #25
bge(pc()+3)                     #26
bra(pc()+3)                     #27
ld(1)                           #28
ld(0)                           #28
st([0xB1])                      #29 (var.1 & 0x80) >>7
ld([X])                         #30
adda([X])                       #31
ora([0xB0])                     #32
st([Y,Xpp])                     #33 (var.1 <<1) | [0xB0]
ld([X])                         #34
bge(pc()+3)                     #35
bra(pc()+3)                     #36
ld(1)                           #37
ld(0)                           #37
st([0xB0])                      #38 (var.2 & 0x80) >>7
ld([X])                         #39
adda([X])                       #40
ora([0xB1])                     #41
st([Y,Xpp])                     #42 (var.2 <<1) | [0xB1]
ld([X])                         #43
bge(pc()+3)                     #44
bra(pc()+3)                     #45
ld(1)                           #46
ld(0)                           #46
st([0xB1])                      #47 (var.3 & 0x80) >>7
ld([X])                         #48
adda([X])                       #49
ora([0xB0])                     #50
st([Y,Xpp])                     #51 (var.3 <<1) | [0xB0]
ld(hi('NEXTY'),Y)               #52
jmp(Y,'NEXTY')                  #53
ld(-56/2)                       #54

label('.lslvl#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x2D00)
#-----------------------------------------------------------------------
#
# JEQL implementation
label('jeql#13')
ld([vAC+1],Y)                   #13
ld(min(0,maxTicks-40/2))        #14
adda([vTicks])                  #15
blt('.jeql#18')                 #16 not enough time left, so retry
ld([vAC],X)                     #17
ld([Y,X])                       #18
st([Y,Xpp])                     #19
st([0xB0])                      #20 [vAC].0
ld([Y,X])                       #21
st([Y,Xpp])                     #22
st([0xB1])                      #23 [vAC].1
ld([Y,X])                       #24
st([Y,Xpp])                     #25
st([0xB2])                      #26 [vAC].2
ld([Y,X])                       #27 [vAC].3
ora([0xB2])                     #28
ora([0xB1])                     #29
ora([0xB0])                     #30
bne('.jeql#33')                 #31 if (vAC.3 OR vAC.2 OR vAC.1 OR vAC.0) != 0
ld([sysArgs+7])                 #32 address of branch destination
st([vPC])                       #33
ld([sysArgs+6])                 #34
st([vPC+1])                     #35
ld(hi('NEXTY'),Y)               #36
jmp(Y,'NEXTY')                  #37
ld(-40/2)                       #38

label('.jeql#33')
ld(hi('REENTER'),Y)             #33
jmp(Y,'REENTER')                #34
ld(-38/2)                       #35

label('.jeql#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# JNEL implementation
label('jnel#13')
ld([vAC+1],Y)                   #13
ld(min(0,maxTicks-40/2))        #14
adda([vTicks])                  #15
blt('.jnel#18')                 #16 not enough time left, so retry
ld([vAC],X)                     #17
ld([Y,X])                       #18
st([Y,Xpp])                     #19
st([0xB0])                      #20 [vAC].0
ld([Y,X])                       #21
st([Y,Xpp])                     #22
st([0xB1])                      #23 [vAC].1
ld([Y,X])                       #24
st([Y,Xpp])                     #25
st([0xB2])                      #26 [vAC].2
ld([Y,X])                       #27 [vAC].3
ora([0xB2])                     #28
ora([0xB1])                     #29
ora([0xB0])                     #30
beq('.jnel#33')                 #31 if (vAC.3 OR vAC.2 OR vAC.1 OR vAC.0) == 0
ld([sysArgs+7])                 #32 address of branch destination
st([vPC])                       #33
ld([sysArgs+6])                 #34
st([vPC+1])                     #35
ld(hi('NEXTY'),Y)               #36
jmp(Y,'NEXTY')                  #37
ld(-40/2)                       #38

label('.jnel#33')
ld(hi('REENTER'),Y)             #33
jmp(Y,'REENTER')                #34
ld(-38/2)                       #35

label('.jnel#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# JLTL implementation 
label('jltl#13')
ld([vAC+1],Y)                   #13
ld([vAC])                       #14
adda(3,X)                       #15
ld([Y,X])                       #16
blt('.jltl#19')                 #17 if [vAC.3] < 0
ld([sysArgs+7])                 #18 address of branch destination
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21

label('.jltl#19')
st([vPC])                       #19
ld([sysArgs+6])                 #20
st([vPC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


# JGTL implementation
label('jgtl#13')
ld([vAC+1],Y)                   #13
ld(min(0,maxTicks-42/2))        #14
adda([vTicks])                  #15
blt('.jgtl#18')                 #16 not enough time left, so retry
ld([vAC],X)                     #17
ld([Y,X])                       #18
st([Y,Xpp])                     #19
st([0xB0])                      #20 [vAC].0
ld([Y,X])                       #21
st([Y,Xpp])                     #22
st([0xB1])                      #23 [vAC].1
ld([Y,X])                       #24
st([Y,Xpp])                     #25
st([0xB2])                      #26 [vAC].2
ld([Y,X])                       #27
st([0xB3])                      #28 [vAC].3
blt('.jgtl#31')                 #29 if [vAC.3] < 0
ora([0xB2])                     #30
ora([0xB1])                     #31
ora([0xB0])                     #32
bne('.jgtl#35')                 #33 if (vAC.3 OR vAC.2 OR vAC.1 OR vAC.0) != 0
ld([sysArgs+7])                 #34 address of branch destination
ld(hi('REENTER'),Y)             #35
jmp(Y,'REENTER')                #36
ld(-40/2)                       #37

label('.jgtl#31')
ld(hi('REENTER'),Y)             #31
jmp(Y,'REENTER')                #32
ld(-36/2)                       #33

label('.jgtl#35')
st([vPC])                       #35
ld([sysArgs+6])                 #36
st([vPC+1])                     #37
ld(hi('NEXTY'),Y)               #38
jmp(Y,'NEXTY')                  #39
ld(-42/2)                       #40

label('.jgtl#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# JLEL implementation
label('jlel#13')
ld([vAC+1],Y)                   #13
ld(min(0,maxTicks-42/2))        #14
adda([vTicks])                  #15
blt('.jlel#18')                 #16 not enough time left, so retry
ld([vAC],X)                     #17
ld([Y,X])                       #18
st([Y,Xpp])                     #19
st([0xB0])                      #20 [vAC].0
ld([Y,X])                       #21
st([Y,Xpp])                     #22
st([0xB1])                      #23 [vAC].1
ld([Y,X])                       #24
st([Y,Xpp])                     #25
st([0xB2])                      #26 [vAC].2
ld([Y,X])                       #27
st([0xB3])                      #28 [vAC].3
blt('.jlel#31')                 #29 if [vAC.3] < 0
ora([0xB2])                     #30
ora([0xB1])                     #31
ora([0xB0])                     #32
beq('.jlel#35')                 #33 if (vAC.3 OR vAC.2 OR vAC.1 OR vAC.0) == 0
ld([sysArgs+7])                 #34 address of branch destination
ld(hi('REENTER'),Y)             #35
jmp(Y,'REENTER')                #36
ld(-40/2)                       #37

label('.jlel#31')
ld([sysArgs+7])                 #31 address of branch destination
st([vPC])                       #32
ld([sysArgs+6])                 #33
st([vPC+1])                     #34
ld(hi('REENTER'),Y)             #35
jmp(Y,'REENTER')                #36
ld(-40/2)                       #37

label('.jlel#35')
st([vPC])                       #35
ld([sysArgs+6])                 #36
st([vPC+1])                     #37
ld(hi('NEXTY'),Y)               #38
jmp(Y,'NEXTY')                  #39
ld(-42/2)                       #40

label('.jlel#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# JGEL implementation
label('jgel#13')
ld([vAC+1],Y)                   #13
ld([vAC])                       #14
adda(3,X)                       #15
ld([Y,X])                       #16
blt('.jgel#19')                 #17 if [vAC.3] < 0
ld([sysArgs+7])                 #18 address of branch destination
st([vPC])                       #19
ld([sysArgs+6])                 #20
st([vPC+1])                     #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

label('.jgel#19')
ld(hi('REENTER'),Y)             #19
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21


# LOKEQI implementation
label('lokeqi#13')
ld([vPC+1],Y)                   #13
st([Y,Xpp])                     #14 X++
ld(min(0,maxTicks-34/2))        #15
adda([vTicks])                  #16
blt('.lokeqi#19')               #17 not enough time left, so retry
ld([Y,X])                       #18 imm.0
ld([vAC+1],Y)                   #19
ld([vAC],X)                     #20
st([Y,Xpp])                     #21 [vAC.0] = imm.0, X++
ld([vTmp])                      #22
st([Y,Xpp])                     #23 [vAC.1] = imm.1, X++
ld(0)                           #24
st([Y,Xpp])                     #25 [vAC.2] = 0, X++
st([Y,X])                       #26 [vAC.3] = 0
ld([vPC])                       #27 fix vPC for 3 byte instruction
adda(1)                         #28
st([vPC])                       #29
ld(hi('NEXTY'),Y)               #30
jmp(Y,'NEXTY')                  #31
ld(-34/2)                       #32

label('.lokeqi#19')
ld([vPC])                       #19 retry
suba(2)                         #20
st([vPC])                       #21
ld(hi('NEXTY'),Y)               #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24


# LOKEI implementation
label('lokei#13')
ld([vPC+1],Y)                   #13
st([0xB0])                      #14 imm.3
st([Y,Xpp])                     #15 X++
ld(min(0,maxTicks-42/2))        #16
adda([vTicks])                  #17
blt('.lokei#20')                #18 not enough time left, so retry
ld([Y,X])                       #19
st([Y,Xpp])                     #20 X++
st([0xB1])                      #21 imm.2
ld([Y,X])                       #22
st([Y,Xpp])                     #23 X++
st([0xB2])                      #24 imm.1
ld([Y,X])                       #25 imm.0
ld([vAC+1],Y)                   #26
ld([vAC],X)                     #27
st([Y,Xpp])                     #28 [vAC.0] = imm.0, X++
ld([0xB2])                      #29
st([Y,Xpp])                     #30 [vAC.1] = imm.1, X++
ld([0xB1])                      #31
st([Y,Xpp])                     #32 [vAC.2] = imm.2, X++
ld([0xB0])                      #33
st([Y,X])                       #34 [vAC.3] = imm.3
ld([vPC])                       #35 fix vPC for 5 byte instruction
adda(3)                         #36
st([vPC])                       #37
ld(hi('NEXTY'),Y)               #38
jmp(Y,'NEXTY')                  #39
ld(-42/2)                       #40

label('.lokei#20')
ld([vPC])                       #20 retry
suba(2)                         #21
st([vPC])                       #22
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x2E00)
#-----------------------------------------------------------------------
#
# ANDBI implementation
label('andbi#13')
ld([X])                         #13
anda([sysArgs+7])               #14 immediate value 
st([X])                         #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# ORBI implementation
label('orbi#13')
ld([X])                         #13
ora([sysArgs+7])                #14 immediate value 
st([X])                         #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18

# XORBI implementation
label('xorbi#13')
ld([X])                         #13
xora([sysArgs+7])               #14 immediate value 
st([X])                         #15
ld(hi('NEXTY'),Y)               #16
jmp(Y,'NEXTY')                  #17
ld(-20/2)                       #18


# INCL implementation
label('incl#13')
ld(min(0,maxTicks-36/2))        #13
adda([vTicks])                  #14
blt('.incl#17')                 #15 not enough time left, so retry
ld(0,Y)                         #16
ld([X])                         #17
label('incl#18')
adda(1)                         #18
beq('.incl#21')                 #19 if 0 byte is 0x00
st([Y,Xpp])                     #20 inc 0 byte
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23

label('.incl#21')
ld([X])                         #21
adda(1)                         #22
beq('.incl#25')                 #23 if 1 byte is 0x00
st([Y,Xpp])                     #24 inc 1 byte
ld(hi('REENTER'),Y)             #25
jmp(Y,'REENTER')                #26
ld(-30/2)                       #27

label('.incl#25')
ld([X])                         #25
adda(1)                         #26
beq('.incl#29')                 #27 if 2 byte is 0x00
st([Y,Xpp])                     #28 inc 2 byte
ld(hi('REENTER'),Y)             #29
jmp(Y,'REENTER')                #30
ld(-34/2)                       #31

label('.incl#29')
ld([X])                         #29
adda(1)                         #30
st([X])                         #31 inc high byte
ld(hi('NEXTY'),Y)               #32
jmp(Y,'NEXTY')                  #33
ld(-36/2)                       #34

label('.incl#17')
ld(hi('PREFX2_PAGE'))           #17 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #18 restore PREFX2 instruction page
adda(1,Y)                       #19 retry instruction
jmp(Y,'REENTER')                #20
ld(-24/2)                       #21


# DECL implementation
label('decl#13')
ld(AC,X)                        #13 var
ld(min(0,maxTicks-40/2))        #14
adda([vTicks])                  #15
blt('.decl#18')                 #16 not enough time left, so retry
ld(0,Y)                         #17
ld([X])                         #18
suba(1)                         #19
st([Y,Xpp])                     #20 dec 0 byte
xora(0xff)                      #21 if 0 byte is 0xff
beq('.decl#24')                 #22
ld([X])                         #23
ld(hi('NEXTY'),Y)               #24
jmp(Y,'NEXTY')                  #25
ld(-28/2)                       #26

label('.decl#24')
suba(1)                         #24
st([Y,Xpp])                     #25 dec 1 byte
xora(0xff)                      #26 if 1 byte is 0xff
beq('.decl#29')                 #27
ld([X])                         #28
ld(hi('REENTER'),Y)             #29
jmp(Y,'REENTER')                #30
ld(-34/2)                       #31

label('.decl#29')
suba(1)                         #29
st([Y,Xpp])                     #30 dec 2 byte
xora(0xff)                      #31 if 2 byte is 0xff
beq('.decl#34')                 #32
ld([X])                         #33
ld(hi('NEXTY'),Y)               #34
jmp(Y,'NEXTY')                  #35
ld(-38/2)                       #36

label('.decl#34')
suba(1)                         #34
st([X])                         #35 dec 3 byte
ld(hi('NEXTY'),Y)               #36
jmp(Y,'NEXTY')                  #37
ld(-40/2)                       #38

label('.decl#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# NEGL implementation, (lb3361)
# Complement then jumps into INCL
label('negl#13')
ld(0,Y)                         #13
ld(min(0,maxTicks-(40+18)/2))   #14
adda([vTicks])                  #15
blt('.decl#18')                 #16 restart
ld([Y,X])                       #17
xora(0xff)                      #18
st([Y,Xpp])                     #19
ld([Y,X])                       #20
xora(0xff)                      #21
st([Y,Xpp])                     #22
ld([Y,X])                       #23
xora(0xff)                      #24
st([Y,Xpp])                     #25
ld([Y,X])                       #26
xora(0xff)                      #27
st([Y,Xpp])                     #28
nop()                           #29
ld([vTicks])                    #30
adda(-18/2)                     #13=31-18
st([vTicks])                    #14
ld([sysArgs+7],X)               #15
bra('incl#18')                  #16
ld([X])                         #17



fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  Implementation of SYS_CopyMemory[Ext], (0x2F00)
#-----------------------------------------------------------------------
#
# SYS_CopyMemory_vX_80 implementation, (lb3361)
label('sys_CopyMemory')
ble('.sysCm#20')                     #18   goto burst6
suba(6)                              #19
bge('.sysCm#22')                     #20   goto burst6
ld([sysArgs+3],Y)                    #21
adda(3)                              #22
bge('.sysCm#25')                     #23   goto burst3
ld([sysArgs+2],X)                    #24

adda(2)                              #25   single
st([vAC])                            #26
ld([Y,X])                            #27
ld([sysArgs+1],Y)                    #28
ld([sysArgs+0],X)                    #29
st([Y,X])                            #30
ld([sysArgs+0])                      #31
adda(1)                              #32
st([sysArgs+0])                      #33
ld([sysArgs+2])                      #34
adda(1)                              #35
st([sysArgs+2])                      #36
ld([vAC])                            #37
beq(pc()+3)                          #38
bra(pc()+3)                          #39
ld(-2)                               #40
ld(0)                                #40!
adda([vPC])                          #41
st([vPC])                            #42
ld(hi('REENTER'),Y)                  #43
jmp(Y,'REENTER')                     #44
ld(-48/2)                            #45

label('.sysCm#25')
st([vAC])                            #25   burst3
for i in range(3):
  ld([Y,X])                            #26+3*i
  st([sysArgs+4+i])                    #27+3*i
  st([Y,Xpp]) if i<2 else None         #28+3*i
ld([sysArgs+1],Y)                    #34
ld([sysArgs+0],X)                    #35
for i in range(3):
  ld([sysArgs+4+i])                    #36+2*i
  st([Y,Xpp])                          #37+2*i
ld([sysArgs+0])                      #42
adda(3)                              #43
st([sysArgs+0])                      #44
ld([sysArgs+2])                      #45
adda(3)                              #46
st([sysArgs+2])                      #47
ld([vAC])                            #48
beq(pc()+3)                          #49
bra(pc()+3)                          #50
ld(-2)                               #51
ld(0)                                #51!
adda([vPC])                          #52
st([vPC])                            #53
ld(hi('NEXTY'),Y)                    #54
jmp(Y,'NEXTY')                       #55
ld(-58/2)                            #56

label('.sysCm#20')
nop()                                #20   burst6
ld([sysArgs+3],Y)                    #21
label('.sysCm#22')
st([vAC])                            #22   burst6
ld([sysArgs+2],X)                    #23
for i in range(6):
  ld([Y,X])                            #24+i*3
  st([vLR+i if i<2 else sysArgs+2+i])  #25+i*3
  st([Y,Xpp]) if i<5 else None         #26+i*3 if i<5
ld([sysArgs+1],Y)                    #41
ld([sysArgs+0],X)                    #42
for i in range(6):
  ld([vLR+i if i<2 else sysArgs+2+i])  #43+i*2
  st([Y,Xpp])                          #44+i*2
ld([sysArgs+0])                      #55
adda(6)                              #56
st([sysArgs+0])                      #57
ld([sysArgs+2])                      #58
adda(6)                              #59
st([sysArgs+2])                      #60

ld([vAC])                            #61
bne('.sysCm#64')                     #62
ld(hi('REENTER'),Y)                  #63
jmp(Y,'REENTER')                     #64
ld(-68/2)                            #65

label('.sysCm#64')
ld(-52/2)                            #64
adda([vTicks])                       #13 = 65 - 52
st([vTicks])                         #14
adda(min(0,maxTicks-(26+52)/2))      #15   could probably be min(0,maxTicks-(26+52)/2)
bge('sys_CopyMemory')                #16
ld([vAC])                            #17
ld(-2)                               #18   notime
adda([vPC])                          #19
st([vPC])                            #20
ld(hi('REENTER'),Y)                  #21
jmp(Y,'REENTER')                     #22
ld(-26/2)                            #23


# SYS_CopyMemoryExt_vX_100 implementation, (lb3361)
label('sys_CopyMemoryExt')
adda(AC)                             #18
adda(AC)                             #19
ora(0x3c)                            #20
st([vTmp])                           #21 [vTmp] = src ctrl value
ld([vAC+1])                          #22
anda(0xfc)                           #23
ora(0x3c)                            #24
st([vLR])                            #25 [vLR] = dest ctrl value

label('.sysCme#26')
ld([vAC])                            #26
ble('.sysCme#29')                    #27   goto burst5
suba(5)                              #28
bge('.sysCme#31')                    #29   goto burst5
ld([sysArgs+3],Y)                    #30
adda(4)                              #31

st([vAC])                            #32   single
ld([vTmp],X)                         #33
ctrl(X)                              #34
ld([sysArgs+2],X)                    #35
ld([Y,X])                            #36
ld([vLR],X)                          #37
ctrl(X)                              #38
ld([sysArgs+1],Y)                    #39
ld([sysArgs+0],X)                    #40
st([Y,X])                            #41
ld(hi(ctrlBits), Y)                  #42
ld([Y,ctrlBits])                     #43
ld(AC,X)                             #44
ctrl(X)                              #45
ld([sysArgs+0])                      #46
adda(1)                              #47
st([sysArgs+0])                      #48
ld([sysArgs+2])                      #49
adda(1)                              #50
st([sysArgs+2])                      #51
ld([vAC])                            #52  done?
beq(pc()+3)                          #53
bra(pc()+3)                          #54
ld(-2)                               #55  restart
ld(0)                                #55! finished
adda([vPC])                          #56
st([vPC])                            #57
ld(hi('NEXTY'),Y)                    #58
jmp(Y,'NEXTY')                       #59
ld(-62/2)                            #60

label('.sysCme#29')
nop()                                #29   burst5
ld([sysArgs+3],Y)                    #30
label('.sysCme#31')
st([vAC])                            #31   burst5
ld([vTmp],X)                         #32
ctrl(X)                              #33
ld([sysArgs+2],X)                    #34
for i in range(5):
  ld([Y,X])                            #35+i*3
  st([vLR+1 if i<1 else sysArgs+3+i])  #36+i*3
  st([Y,Xpp]) if i<4 else None         #37+i*3 if i<4
ld([vLR],X)                          #49
ctrl(X)                              #50
ld([sysArgs+1],Y)                    #51
ld([sysArgs+0],X)                    #52
for i in range(5):
  ld([vLR+1 if i<1 else sysArgs+3+i])  #53+i*2
  st([Y,Xpp])                          #54+i*2
ld([sysArgs+0])                      #63
adda(5)                              #64
st([sysArgs+0])                      #65
ld([sysArgs+2])                      #66
adda(5)                              #67
st([sysArgs+2])                      #68

ld([vAC])                            #69
bne('.sysCme#72')                    #70
ld(hi(ctrlBits), Y)                  #71  we're done!
ld([Y,ctrlBits])                     #72
anda(0xfc,X)                         #73
ctrl(X)                              #74
ld([vTmp])                           #75  always read after ctrl
ld(hi('NEXTY'),Y)                    #76
jmp(Y,'NEXTY')                       #77
ld(-80/2)                            #78

label('.sysCme#72')
ld(-52/2)                            #72
adda([vTicks])                       #21 = 72 - 52
st([vTicks])                         #22
adda(min(0,maxTicks-(40+52)/2))      #23
bge('.sysCme#26')                    #24  enough time for another loop
ld(-2)                               #25
adda([vPC])                          #26  restart
st([vPC])                            #27
ld(hi(ctrlBits), Y)                  #28
ld([Y,ctrlBits])                     #29
anda(0xfc,X)                         #30
ctrl(X)                              #31
ld([vTmp])                           #32 always read after ctrl
ld(hi('REENTER'),Y)                  #33
jmp(Y,'REENTER')                     #34
ld(-38/2)                            #35 max: 38 + 52 = 90 cycles


fillers(until=0xff)
align(0x100, size=0x100)
 
#-----------------------------------------------------------------------
#  Implementation of SYS_ScanMemory[Ext], (0x3000)
#-----------------------------------------------------------------------
#
# SYS_ScanMemory_vX_50 implementation, (lb3361)
label('sys_ScanMemory')
ld([sysArgs+0],X)                    #18
ld([Y,X])                            #19
label('.sysSme#20')
xora([sysArgs+2])                    #20
beq('.sysSme#23')                    #21
ld([Y,X])                            #22
xora([sysArgs+3])                    #23
beq('.sysSme#26')                    #24
ld([sysArgs+0])                      #25
adda(1);                             #26
st([sysArgs+0],X)                    #27
ld([vAC])                            #28
suba(1)                              #29
beq('.sysSme#32')                    #30 return zero
st([vAC])                            #31
ld(-18/2)                            #14 = 32 - 18
adda([vTicks])                       #15
st([vTicks])                         #16
adda(min(0,maxTicks -(28+18)/2))     #17
bge('.sysSme#20')                    #18
ld([Y,X])                            #19
ld(-2)                               #20 restart
adda([vPC])                          #21
st([vPC])                            #22
ld(hi('REENTER'),Y)                  #23
jmp(Y,'REENTER')                     #24
ld(-28/2)                            #25

label('.sysSme#32')
st([vAC+1])                          #32 return zero
ld(hi('REENTER'),Y)                  #33
jmp(Y,'REENTER')                     #34
ld(-38/2)                            #35

label('.sysSme#23')
nop()                                #23 success
nop()                                #24
ld([sysArgs+0])                      #25
label('.sysSme#26')
st([vAC])                            #26 success
ld([sysArgs+1])                      #27
st([vAC+1])                          #28
ld(hi('REENTER'),Y)                  #29
jmp(Y,'REENTER')                     #30
ld(-34/2)                            #31


# SYS_ScanMemoryExt_vX_50 implementation, (lb3361)
label('sys_ScanMemoryExt')
ora(0x3c,X)                          #18
ctrl(X)                              #19
ld([sysArgs+1],Y)                    #20
ld([sysArgs+0],X)                    #21
ld([Y,X])                            #22
nop()                                #23
label('.sysSmx#24')
xora([sysArgs+2])                    #24
beq('.sysSmx#27')                    #25
ld([Y,X])                            #26
xora([sysArgs+3])                    #27
beq('.sysSmx#30')                    #28
ld([sysArgs+0])                      #29
adda(1);                             #30
st([sysArgs+0],X)                    #31
ld([vAC])                            #32
suba(1)                              #33
beq('.sysSmx#36')                    #34 return zero
st([vAC])                            #35
ld(-18/2)                            #18 = 36 - 18
adda([vTicks])                       #19
st([vTicks])                         #20
adda(min(0,maxTicks -(30+18)/2))     #21
bge('.sysSmx#24')                    #22
ld([Y,X])                            #23
ld([vPC])                            #24
suba(2)                              #25 restart
st([vPC])                            #26
ld(hi(ctrlBits),Y)                   #27 restore and return
ld([Y,ctrlBits])                     #28
anda(0xfc,X)                         #29
ctrl(X)                              #30
ld([vTmp])                           #31
ld(hi('NEXTY'),Y)                    #32
jmp(Y,'NEXTY')                       #33
ld(-36/2)                            #34

label('.sysSmx#27')
nop()                                #27 success
nop()                                #28
ld([sysArgs+0])                      #29
label('.sysSmx#30')
st([vAC])                            #30 success
ld([sysArgs+1])                      #31
nop()                                #32
nop()                                #33
nop()                                #34
nop()                                #35
label('.sysSmx#36')
st([vAC+1])                          #36
ld(hi(ctrlBits),Y)                   #37 restore and return
ld([Y,ctrlBits])                     #38
anda(0xfc,X)                         #39
ctrl(X)                              #40
ld([vTmp])                           #41
ld(hi('NEXTY'),Y)                    #42
jmp(Y,'NEXTY')                       #43
ld(-46/2)                            #44


fillers(until=0x80)

# SYS_LoaderSerialIN
# sysArgs[0]   (in), zero page address of odd videoY waits and first even videoY wait, 207 219 235 251 2
# sysArgs[1]   (in), sysArgs2 address, buffer that receives protocol, packet length and packet address
# sysArgs[2]   (out), protocol, filled in automatically by the Sys call
# sysArgs[3]   (out), packet length, filled in automatically by the Sys call
# sysArgs[4:5] (out), packet address, filled in automatically by the Sys call
# No error checks are performed, (apart from checking protocol)
# No checksums are performed
label('SYS_LoadSerialIn_vX_58') 
ld([sysArgs+0],X)                   #15
ld([videoY])                        #16
xora([X])                           #17
bne('.sysLsi#20')                   #18 wrong videoY so restart
ld([X])                             #19
anda(1)                             #20
beq('.sysLsi#23')                   #21 even videoY
ld([sysArgs+0])                     #22
adda(1)                             #23 odd videoY
st([sysArgs+0])                     #24
ld([sysArgs+1],X)                   #25 sysArgs address
st(IN,[X])                          #26
ld([sysArgs+1])                     #27
adda(1)                             #28
st([sysArgs+1])                     #29
ld([vPC])                           #30 restart
suba(2)                             #31
st([vPC])                           #32
ld(hi('REENTER'),Y)                 #33
jmp(Y,'REENTER')                    #34
ld(-38/2)                           #35

# wrong videoY 
label('.sysLsi#20')
ld([vPC])                           #20 restart
suba(2)                             #21
st([vPC])                           #22
ld(hi('REENTER'),Y)                 #23
jmp(Y,'REENTER')                    #24
ld(-28/2)                           #25

# even videoY
label('.sysLsi#23')
ld([X])                             #23
adda(4)                             #24
st([X])                             #25
ld([sysArgs+2])                     #26 protocol
xora(ord('L'))                      #27 check valid load
bne('.sysLsi#30')                   #28 invalid packet
ld([sysArgs+3])                     #29 check length
beq('.sysLsi#32')                   #30 execute
suba(1)                             #31
st([sysArgs+3])                     #32 length--
ld([sysArgs+4],X)                   #33
ld([sysArgs+5],Y)                   #34
st(IN,[Y,X])                        #35
ld([sysArgs+3])                     #36 check length
beq('.sysLsi#39')                   #37 new packet
ld([sysArgs+4])                     #38
adda(1)                             #39
st([sysArgs+4])                     #40
ld([vPC])                           #41 restart
suba(2)                             #42
st([vPC])                           #43
ld(hi('NEXTY'),Y)                   #44
jmp(Y,'NEXTY')                      #45
ld(-48/2)                           #46

# invalid packet
label('.sysLsi#30')
ld(2)                               #30
st([0xBC])                          #31 reset even videoY
ld(0xB8)                            #32
st([sysArgs+0])                     #33 reset videoY address
ld(0x26)                            #34
st([sysArgs+1])                     #35 reset sys args address
ld([vPC])                           #36 restart
suba(2)                             #37
st([vPC])                           #38
ld(0)                               #39
ld(0x01,Y)                          #40
st([Y,0x01])                        #41 reset screen vibration
ld(hi('NEXTY'),Y)                   #42
jmp(Y,'NEXTY')                      #43
ld(-46/2)                           #44

# new packet
label('.sysLsi#39')
ld(2)                               #39
st([0xBC])                          #40 reset even videoY
ld(0xB8)                            #41
st([sysArgs+0])                     #42 reset videoY address
ld(0x26)                            #43
st([sysArgs+1])                     #44 reset sys args address
ld([vPC])                           #45 restart
suba(2)                             #46
st([vPC])                           #47
ld([sysArgs+5])                     #48 hi address
anda(1)                             #49
ld(0x01,Y)                          #50
xora([Y,0x01])                      #51
st([Y,0x01])                        #52 vibrate screen to show loading progress
ld(hi('REENTER'),Y)                 #53
jmp(Y,'REENTER')                    #54
ld(-58/2)                           #55

# execute
label('.sysLsi#32')
ld([sysArgs+4])                     #32 low execute address
st([vLR])                           #33
suba(2)                             #34
st([vPC])                           #35
ld([sysArgs+5])                     #36 high execute address
st([vPC+1])                         #37
st([vLR+1])                         #38
ld(0)                               #39
ld(0x01,Y)                          #40
st([Y,0x01])                        #41 reset screen vibration
ld(hi('NEXTY'),Y)                   #42
jmp(Y,'NEXTY')                      #43
ld(-46/2)                           #44

 
fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#       More vCPU instruction implementations, (0x3100)
#-----------------------------------------------------------------------
#
# PEEKV+ implementation
label('peekv+#13')
ld(AC,X)                        #13 var
ld(min(0,maxTicks-42/2))        #14
adda([vTicks])                  #15
blt('.peekv+#18')               #16 not enough time left, so retry
ld(0,Y)                         #17
ld([X])                         #18 low byte peek address
st([Y,Xpp])                     #19 X++
st([sysArgs+6])                 #20
ld([X])                         #21 high byte peek address
st([sysArgs+7])                 #22
ld(AC,Y)                        #23
ld([sysArgs+6],X)               #24
ld([Y,X])                       #25 peek byte
st([vAC])                       #26
ld(0)                           #27
st([vAC+1])                     #28
ld([sysArgs+6],X)               #29 address of low byte to be added
ld([X])                         #30
adda(1)                         #31
beq('.peekv+#34')               #32 if low byte is 0x00
st([X])                         #33 inc low byte
ld(hi('NEXTY'),Y)               #34
jmp(Y,'NEXTY')                  #35
ld(-38/2)                       #36

label('.peekv+#34')
ld([sysArgs+7],X)               #34 address of high byte to be added
ld([X])                         #35
adda(1)                         #36
st([X])                         #37 inc high byte
ld(hi('NEXTY'),Y)               #38
jmp(Y,'NEXTY')                  #39
ld(-42/2)                       #40

label('.peekv+#18')
ld(hi('PREFX2_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX2 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3200)
#-----------------------------------------------------------------------
#
# MOVL implementation, (lb3361)
label('movl#13')
adda(1, X)                      #13
ld(min(0,maxTicks-40/2))        #14
adda([vTicks])                  #15
blt('movl#18')                  #16
ld(0,Y)                         #17
ld([Y,X])                       #18
st([Y,Xpp])                     #19
st([vTmpL])                     #20
ld([Y,X])                       #21
st([Y,Xpp])                     #22
st([vTmpL+1])                   #23
ld([Y,X])                       #24
st([vTmpL+2])                   #25
ld([sysArgs+6],X)               #26
ld([Y,X])                       #27
ld([sysArgs+7],X)               #28
st([Y,Xpp])                     #29
ld([vTmpL])                     #30
st([Y,Xpp])                     #31
ld([vTmpL+1])                   #32
st([Y,Xpp])                     #33
ld([vTmpL+2])                   #34
st([Y,Xpp])                     #35
ld(hi('NEXTY'),Y)               #36
jmp(Y,'NEXTY')                  #37
ld(-40/2)                       #38
label('movl#18')
ld(hi('PREFX3_PAGE'))           #18 ENTER is at $(n-1)ff, where n = instruction page
st([vCpuSelect])                #19 restore PREFX3 instruction page
adda(1,Y)                       #20 retry instruction
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22


# MOVF implementation, (lb3361)
label('movf#13')
adda(1, X)                      #13
ld(min(0,maxTicks-46/2))        #14
adda([vTicks])                  #15
blt('movl#18')                  #16
ld(0,Y)                         #17
ld([Y,X])                       #18
st([Y,Xpp])                     #19
st([vTmpL])                     #20
ld([Y,X])                       #21
st([Y,Xpp])                     #22
st([vTmpL+1])                   #23
ld([Y,X])                       #24
st([Y,Xpp])                     #25
st([vTmpL+2])                   #26
ld([Y,X])                       #27
st([vTmpL+3])                   #28
ld([sysArgs+6],X)               #29
ld([Y,X])                       #30
ld([sysArgs+7],X)               #31
st([Y,Xpp])                     #32
ld([vTmpL])                     #33
st([Y,Xpp])                     #34
ld([vTmpL+1])                   #35
st([Y,Xpp])                     #36
ld([vTmpL+2])                   #37
st([Y,Xpp])                     #38
ld([vTmpL+3])                   #39
st([Y,Xpp])                     #40
nop()                           #41
ld(hi('NEXTY'),Y)               #42
jmp(Y,'NEXTY')                  #43
ld(-46/2)                       #44


# ADDLP implementation, (lb3361)
label('addlp#13')
adda(min(0,maxTicks-66/2))      #13
blt('addlp#16')                 #14
ld([vAC+1],Y)                   #15
ld([vAC],X)                     #16
ld([vLAC])                      #17 lac0
adda([Y,X])                     #18 +arg0
st([vLAC])                      #19
bmi('addlp#22')                 #20
suba([Y,X])                     #21 reconstruct lac0
bra('addlp#24')                 #22
ora([Y,X])                      #23
label('addlp#22')
anda([Y,X])                     #22
nop()                           #23
label('addlp#24')
anda(0x80,X)                    #24
ld([X])                         #25
st([vTmp])                      #26 save carry
ld([vAC])                       #27
adda(1,X)                       #28
ld([vLAC+1])                    #29 lac1
adda([vTmp])                    #30 +carry
adda([Y,X])                     #31 +arg1
st([vLAC+1])                    #32
bmi('addlp#35')                 #33
suba([Y,X])                     #34
suba([vTmp])                    #35 reconstruct lac1
bra('addlp#38')                 #36
ora([Y,X])                      #37
label('addlp#35')
suba([vTmp])                    #35
anda([Y,X])                     #36
nop()                           #37
label('addlp#38')
anda(0x80,X)                    #38
ld([X])                         #39
st([vTmp])                      #40
ld([vAC])                       #41
adda(2,X)                       #42
ld([vLAC+2])                    #43 lac2
adda([vTmp])                    #44 +carry
adda([Y,X])                     #45 +arg2
st([vLAC+2])                    #46
bmi('addlp#49')                 #47
suba([Y,X])                     #48
suba([vTmp])                    #49 reconstruct lac2
bra('addlp#52')                 #50
ora([Y,X])                      #51
label('addlp#49')
suba([vTmp])                    #49
anda([Y,X])                     #50
nop()                           #51
label('addlp#52')
anda(0x80,X)                    #52
ld([X])                         #53
st([vTmp])                      #54
ld([vAC])                       #55
adda(3,X)                       #56
ld([vLAC+3])                    #57 lac3
adda([vTmp])                    #58 +carry
adda([Y,X])                     #59 +arg3
st([vLAC+3])                    #60
ld(hi('REENTER'),Y)             #61
jmp(Y,'REENTER')                #62
ld(-66/2)                       #63
label('addlp#16')
ld(hi('PREFX1_PAGE'))           #16 restart
st([vCpuSelect])                #17
adda(1,Y)                       #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20


# SUBLP implementation, (lb3361)
label('sublp#13')
adda(min(0,maxTicks-60/2))      #13
blt('addlp#16')                 #14
ld([vAC+1],Y)                   #15
ld([vAC],X)                     #16
ld([vLAC])                      #17 lac0
bmi('sublp#20')                 #18
suba([Y,X])                     #19 -arg0
st([vLAC])                      #20
bra('sublp#23')                 #21
ora([Y,X])                      #22
label('sublp#20')
st([vLAC])                      #20
anda([Y,X])                     #21
nop()                           #22
label('sublp#23')
anda(0x80,X)                    #23
ld([X])                         #24
st([vTmp])                      #25 borrow
ld([vAC])                       #26
adda(1,X)                       #27
ld([vLAC+1])                    #28 lac1
bmi('sublp#31')                 #29
suba([Y,X])                     #30 -arg1
suba([vTmp])                    #31 -borrow
st([vLAC+1])                    #32
bra('sublp#35')                 #33
ora([Y,X])                      #34
label('sublp#31')
suba([vTmp])                    #31
st([vLAC+1])                    #32
anda([Y,X])                     #33
nop()                           #34
label('sublp#35')
anda(0x80,X)                    #35
ld([X])                         #36
st([vTmp])                      #37
ld([vAC])                       #38
adda(2,X)                       #39
ld([vLAC+2])                    #40 lac2
bmi('sublp#43')                 #41
suba([Y,X])                     #42 -arg2
suba([vTmp])                    #43 -borrow
st([vLAC+2])                    #44
bra('sublp#47')                 #45
ora([Y,X])                      #46
label('sublp#43')
suba([vTmp])                    #43
st([vLAC+2])                    #44
anda([Y,X])                     #45
nop()                           #46
label('sublp#47')
anda(0x80,X)                    #47
ld([X])                         #48
st([vTmp])                      #49
ld([vAC])                       #50
adda(3,X)                       #51
ld([vLAC+3])                    #52 lac3
suba([Y,X])                     #53 -arg3
suba([vTmp])                    #54 -borrow
st([vLAC+3])                    #55
ld(hi('NEXTY'),Y)               #56
jmp(Y,'NEXTY')                  #57
ld(-60/2)                       #58


# ANDLP implementation, (lb3361)
label('andlp#13')
adda(min(0,maxTicks-42/2))      #13
blt('addlp#16')                 #14
ld([vAC+1],Y)                   #15
ld([vAC],X)                     #16
ld([Y,X])                       #17
st([Y,Xpp])                     #18
anda([vLAC+0])                  #19
st([vLAC+0])                    #20
ld([Y,X])                       #21
st([Y,Xpp])                     #22
anda([vLAC+1])                  #23
st([vLAC+1])                    #24
ld([Y,X])                       #25
st([Y,Xpp])                     #26
anda([vLAC+2])                  #27
st([vLAC+2])                    #28
ld([Y,X])                       #29
anda([vLAC+3])                  #30
nop()                           #31
label('andlp#32')
st([vLAC+3])                    #32
st([vAC+1])                     #33
ld([vLAC+0])                    #34
ora([vLAC+1])                   #35
ora([vLAC+2])                   #36
st([vAC])                       #37
ld(hi('NEXTY'),Y)               #38
jmp(Y,'NEXTY')                  #39
ld(-42/2)                       #40


# ORLP implementation, (lb3361)
label('orlp#13')
adda(min(0,maxTicks-42/2))      #13
blt('addlp#16')                 #14
ld([vAC+1],Y)                   #15
ld([vAC],X)                     #16
ld([Y,X])                       #17
st([Y,Xpp])                     #18
ora([vLAC+0])                   #19
st([vLAC+0])                    #20
ld([Y,X])                       #21
st([Y,Xpp])                     #22
ora([vLAC+1])                   #23
st([vLAC+1])                    #24
ld([Y,X])                       #25
st([Y,Xpp])                     #26
ora([vLAC+2])                   #27
st([vLAC+2])                    #28
ld([Y,X])                       #29
bra('andlp#32')                 #30
ora([vLAC+3])                   #31


# XORLP implementation, (lb3361)
label('xorlp#13')
adda(min(0,maxTicks-42/2))      #13
blt('addlp#16')                 #14
ld([vAC+1],Y)                   #15
ld([vAC],X)                     #16
ld([Y,X])                       #17
st([Y,Xpp])                     #18
xora([vLAC+0])                  #19
st([vLAC+0])                    #20
ld([Y,X])                       #21
st([Y,Xpp])                     #22
xora([vLAC+1])                  #23
st([vLAC+1])                    #24
ld([Y,X])                       #25
st([Y,Xpp])                     #26
xora([vLAC+2])                  #27
st([vLAC+2])                    #28
ld([Y,X])                       #29
bra('andlp#32')                 #30
xora([vLAC+3])                  #31


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3300)
#-----------------------------------------------------------------------
#
# NCOPY implementation, (lb3361)
label('ncopy#16')
ld(hi('PREFX2_PAGE'))           #16 restart
st([vCpuSelect])                #17
adda(1,Y)                       #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

label('ncopy#11')
adda([vTicks])                  #11
st([vTicks])                    #12
label('ncopy#13')               #-- entry point
adda(min(0,maxTicks-(22+50)/2)) #13 time for longest path (72 cycles)
blt('ncopy#16')                 #14 > restart
ld([vAC])                       #15
anda(0xfc)                      #16
xora(0xfc)                      #17
beq('ncopy#20')                 #18 > slow because src crosses page boundary
ld([vDST])                      #19
anda(0xfc)                      #20
xora(0xfc)                      #21
beq('ncopy#24')                 #22 > slow because dst crosses page boundnary
ld([sysArgs+7])                 #23
anda(0xfc)                      #24
beq('ncopy#27')                 #25 > slow because n<4 but no page crossings
ld([vAC+1],Y)                   #26
ld([vAC],X)                     #27 four bytes burst
ld([Y,X])                       #28
st([Y,Xpp])                     #29
st([vTmpL])                     #30
ld([Y,X])                       #31
st([Y,Xpp])                     #32
st([vTmpL+1])                   #33
ld([Y,X])                       #34
st([Y,Xpp])                     #35
st([vTmpL+2])                   #36
ld([Y,X])                       #37
st([Y,Xpp])                     #38
st([vTmpL+3])                   #39
ld([vDST+1],Y)                  #40
ld([vDST],X)                    #41
ld([vTmpL])                     #42
st([Y,Xpp])                     #43
ld([vTmpL+1])                   #44
st([Y,Xpp])                     #45
ld([vTmpL+2])                   #46
st([Y,Xpp])                     #47
ld([vTmpL+3])                   #48
st([Y,Xpp])                     #40
ld(4)                           #50 increment (no page crossing)
adda([vAC])                     #51
st([vAC])                       #52
ld(4)                           #53
adda([vDST])                    #54
st([vDST])                      #55
ld([sysArgs+7])                 #56
suba(4)                         #57
st([sysArgs+7])                 #58
bne('ncopy#11')                 #59-50=9
ld(-50/2)                       #60-50=10
ld(hi('REENTER'),Y)             #61
jmp(Y,'REENTER')                #62
ld(-66/2)                       #63

label('ncopy#27')
ld([vAC],X)                     #27 one byte, no page crossings
ld([Y,X])                       #28
ld([vDST+1],Y)                  #29
ld([vDST],X)                    #30
st([Y,X])                       #31
ld(1)                           #32 increment (no page crossing)
adda([vAC])                     #33
st([vAC])                       #34
ld(1)                           #35
adda([vDST])                    #36
st([vDST])                      #37
ld([sysArgs+7])                 #38
suba(1)                         #39
st([sysArgs+7])                 #40
bne('ncopy#11')                 #41-32=9
ld(-32/2)                       #42-32=10
nop()                           #43
ld(hi('NEXTY'),Y)               #44
jmp(Y,'NEXTY')                  #45
ld(-48/2)                       #46

label('ncopy#20')
nop()                           #20
nop()                           #21
nop()                           #22
nop()                           #23
label('ncopy#24')
ld([vAC+1],Y)                   #24
ld([vAC],X)                     #25 one byte, possible page crossings
ld([Y,X])                       #26
ld([vDST+1],Y)                  #27
ld([vDST],X)                    #28
st([Y,X])                       #29
ld(1)                           #30 increment vAC
adda([vAC])                     #31
st([vAC])                       #32
beq(pc()+3)                     #33
bra(pc()+3)                     #34
ld(0)                           #35
ld(1)                           #35!
adda([vAC+1])                   #36
st([vAC+1])                     #37
ld(1)                           #38 increment vDST
adda([vDST])                    #39
st([vDST])                      #40
beq(pc()+3)                     #41
bra(pc()+3)                     #42
ld(0)                           #43
ld(1)                           #43!
adda([vDST+1])                  #44
st([vDST+1])                    #45
nop()                           #46
ld([sysArgs+7])                 #47 decrement sysArgs7
suba(1)                         #48
st([sysArgs+7])                 #49
ld(hi('NEXTY'),Y)               #50
bne('ncopy#11')                 #51-42=9
ld(-42/2)                       #52-42=10
jmp(Y,'NEXTY')                  #53
ld(-56/2)                       #54


# STLU implementation, (lb3361)
label('stlu#13')
ld(0,Y)                         #13
ld([vAC])                       #14
st([Y,Xpp])                     #15
ld([vAC+1])                     #16
st([Y,Xpp])                     #17
ld(0)                           #18
st([Y,Xpp])                     #19
st([Y,X])                       #20
ld(hi('REENTER'),Y)             #21
jmp(Y,'REENTER')                #22
ld(-26/2)                       #23


# STLS implementation, (lb3361)
label('stls#13')
ld(0,Y)                         #13
ld([vAC])                       #14
st([Y,Xpp])                     #15
ld([vAC+1])                     #16
st([Y,Xpp])                     #17
bmi(pc()+3)                     #18
bra(pc()+3)                     #19
ld(0)                           #20
ld(0xff)                        #20
st([Y,Xpp])                     #21
st([Y,X])                       #22
ld(hi('REENTER'),Y)             #23
jmp(Y,'REENTER')                #24
ld(-28/2)                       #25


# CMPLPU/CMPLPS implementation, (lb3361)
label('cmplp#16')
ld(hi('PREFX1_PAGE'))           #16 restart
st([vCpuSelect])                #17
adda(1,Y)                       #18
jmp(Y,'NEXTY')                  #19
ld(-22/2)                       #20

label('cmplp#lt')
ld(0xff)                        #vTmp-7
st([vAC])                       #vTmp-6
st([vAC+1])                     #vTmp-5
ld(hi('NEXTY'),Y)               #vTmp-4
jmp(Y,'NEXTY')                  #vTmp-3
ld([vTmp])                      #vTmp-2

label('cmplp#gt')
ld(1)                           #vTmp-8
st([vAC])                       #vTmp-7
ld(0)                           #vTmp-6
st([vAC+1])                     #vTmp-5
ld(hi('NEXTY'),Y)               #vTmp-4
jmp(Y,'NEXTY')                  #vTmp-3
ld([vTmp])                      #vTmp-2

label('cmplpu#13')
adda(min(0,maxTicks-72/2))      #13
blt('cmplp#16')                 #14
ld([vAC+1],Y)                   #15
# byte3
ld([vAC])                       #16
adda(3,X)                       #17
ld(-36/2)                       #18
st([vTmp])                      #19
ld([vLAC+3])                    #20
xora([Y,X])                     #21
blt(pc()+4)                     #22
ld([vLAC+3])                    #23
bra('cmplp#26')                 #24
suba([Y,X])                     #25
xora(0x80)                      #24
ora(1)                          #25
label('cmplp#26')
bgt('cmplp#gt')                 #26
blt('cmplp#lt')                 #27
# byte2
ld([vAC])                       #28
adda(2,X)                       #29
ld(-48/2)                       #30
st([vTmp])                      #31
ld([vLAC+2])                    #32
xora([Y,X])                     #33
blt(pc()+4)                     #34
ld([vLAC+2])                    #35
bra(pc()+4)                     #36
suba([Y,X])                     #37
xora(0x80)                      #36
ora(1)                          #37
bgt('cmplp#gt')                 #38
blt('cmplp#lt')                 #39
# byte1
ld([vAC])                       #40
adda(1,X)                       #41
ld(-60/2)                       #42
st([vTmp])                      #43
ld([vLAC+1])                    #44
xora([Y,X])                     #45
blt(pc()+4)                     #46
ld([vLAC+1])                    #47
bra(pc()+4)                     #48
suba([Y,X])                     #49
xora(0x80)                      #48
ora(1)                          #49
bgt('cmplp#gt')                 #50
blt('cmplp#lt')                 #51
# byte0
ld([vAC])                       #52
adda(0,X)                       #53
ld(-72/2)                       #54
st([vTmp])                      #55
ld([vLAC])                      #56
xora([Y,X])                     #57
blt(pc()+4)                     #58
ld([vLAC])                      #59
bra(pc()+4)                     #60
suba([Y,X])                     #61
xora(0x80)                      #60
ora(1)                          #61
bgt('cmplp#gt')                 #62
blt('cmplp#lt')                 #63
st([vAC+1])                     #64
st([vAC])                       #65
ld(hi('NEXTY'),Y)               #66
jmp(Y,'NEXTY')                  #67
ld(-70/2)                       #68

label('cmplps#13')
adda(min(0,maxTicks-72/2))      #13
blt('cmplp#16')                 #14
ld([vAC+1],Y)                   #15
#byte3
ld([vAC])                       #16
adda(3,X)                       #17
ld(-36/2)                       #18
st([vTmp])                      #19
ld([vLAC+3])                    #20
xora([Y,X])                     #21
blt(pc()+4)                     #22
ld([vLAC+3])                    #23
bra('cmplp#26')                 #24
suba([Y,X])                     #25
nop()                           #24
ora(1)                          #25
bgt('cmplp#gt')                 #26
blt('cmplp#lt')                 #27
#dummy                          #28


# NOTL implementation, (lb3361)
label('notl#13')
ld(0,Y)                         #13
ld([Y,X])                       #14
xora(0xff)                      #15
st([Y,Xpp])                     #16
ld([Y,X])                       #17
xora(0xff)                      #18
st([Y,Xpp])                     #19
ld([Y,X])                       #20
xora(0xff)                      #21
st([Y,Xpp])                     #22
ld([Y,X])                       #23
xora(0xff)                      #24
st([Y,Xpp])                     #25
ld(hi('NEXTY'),Y)               #26
jmp(Y,'NEXTY')                  #27
ld(-30/2)                       #28


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3400)
#-----------------------------------------------------------------------
#
# NROL implementation, (lb3361)
label('nrol#13')
ld([sysArgs+7],X)               #13
bra('nrol#16')                  #14
ld([vTicks])                    #15

label('nrol#14')
adda([vTicks])                  #14
st([vTicks])                    #15
label('nrol#16')
adda(min(0,maxTicks-(24+18)/2)) #16
bge('nrol#19')                  #17 -> enough time
ld(hi('PREFX3_PAGE'))           #18 restart
st([vCpuSelect])                #19
adda(1,Y)                       #20
jmp(Y,'NEXTY')                  #21
ld(-24/2)                       #22

label('nrol#19')
ld([vAC+1])                     #19 enough time
blt('nrol#22')                  #20
ld([X])                         #21
st([vAC+1])                     #22
bra('nrol#25')                  #23
adda(AC)                        #24 
label('nrol#22')
st([vAC+1])                     #22
adda(AC)                        #23
adda(1)                         #24
label('nrol#25')
st([X])                         #25
ld(1)                           #26
adda([sysArgs+7])               #27
st([sysArgs+7],X)               #28
xora([sysArgs+6])               #29
bne('nrol#14')                  #30-18=12
ld(-18/2)                       #31-18=13
ld(hi('NEXTY'),Y)               #32
jmp(Y,'NEXTY')                  #33
ld(-36/2)                       #34


# NROR implementation, (lb3361)
label('nror#20')
ld(hi('PREFX3_PAGE'))           #20 restart
st([vCpuSelect])                #21
adda(1,Y)                       #22
jmp(Y,'NEXTY')                  #23
ld(-26/2)                       #24

label('nror#13')
ld('nror#35')                   #13
st([vTmp])                      #14
ld([vTicks])                    #15
label('nror#16')
ld(hi('shiftTable'),Y)          #16
adda(min(0,maxTicks-(26+30)/2)) #17
blt('nror#20')                  #18
ld([sysArgs+7])                 #19
suba(1)                         #20
st([sysArgs+7],X)               #21
ld([vAC+1])                     #22
anda(0x80)                      #23
st([vAC])                       #24
ld([X])                         #25
anda(0xfe)                      #26
suba([X])                       #27
st([vAC+1])                     #28
adda([X])                       #29
jmp(Y,AC)                       #30  
bra(255)                        #31 continues in page 0x600


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3500)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3600)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3700)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3800)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3900)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3A00)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3B00)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3C00)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3D00)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3E00)
#-----------------------------------------------------------------------
#


fillers(until=0xff)
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#  More vCPU instruction implementations, (0x3F00)
#-----------------------------------------------------------------------
#


fillers(until=0xff) 
align(0x100, size=0x100)

#-----------------------------------------------------------------------
#
#  End of core
#
#-----------------------------------------------------------------------
disableListing()

#-----------------------------------------------------------------------
#
#  Start of storage area
#
#-----------------------------------------------------------------------

# Export some zero page variables to GCL
# These constants were already loaded from interface.json.
# We're redefining them here to get a consistency check.
define('memSize',    memSize)
for i in range(3):
  define('entropy%d' % i, entropy+i)
define('videoY',     videoY)
define('frameCount', frameCount)
define('serialRaw',  serialRaw)
define('buttonState',buttonState)
define('xoutMask',   xoutMask)
define('vPC',        vPC)
define('vAC',        vAC)
define('vACH',       vAC+1)
define('vLR',        vLR)
define('vSP',        vSP)
define('vTmp',       vTmp)      # Not in interface.json
define('romType',    romType)
define('sysFn',      sysFn)
for i in range(8):
  define('sysArgs%d' % i, sysArgs+i)
define('soundTimer', soundTimer)
define('ledTimer',   ledTimer)
define('ledState_v2',ledState_v2)
define('ledTempo',   ledTempo)
define('userVars',   userVars)
define('videoTable', videoTable)
define('vIRQ_v5',    vIRQ_v5)
define('videoTop_v5',videoTop_v5)
define('userCode',   userCode)
define('soundTable', soundTable)
define('screenMemory',screenMemory)
define('vReset',     vReset)
define('wavA',       wavA)
define('wavX',       wavX)
define('keyL',       keyL)
define('keyH',       keyH)
define('oscL',       oscL)
define('oscH',       oscH)
define('maxTicks',   maxTicks)
define('v6502_PC',   v6502_PC)
define('v6502_PCL',  v6502_PCL)
define('v6502_PCH',  v6502_PCH)
define('v6502_A',    v6502_A)
define('v6502_X',    v6502_X)
define('v6502_Y',    v6502_Y)
define('qqVgaWidth', qqVgaWidth)
define('qqVgaHeight',qqVgaHeight)
define('buttonRight',buttonRight)
define('buttonLeft', buttonLeft)
define('buttonDown', buttonDown)
define('buttonUp',   buttonUp)
define('buttonStart',buttonStart)
define('buttonSelect',buttonSelect)
define('buttonB',    buttonB)
define('buttonA',    buttonA)

# XXX This is a hack (trampoline() is probably in the wrong module):
define('vPC+1',      vPC+1)

#-----------------------------------------------------------------------
#       Embedded programs -- import and convert programs and data
#-----------------------------------------------------------------------

def basicLine(address, number, text):
  """Helper to encode lines for TinyBASIC"""
  head = [] if number is None else [number&255, number>>8]
  body = [] if text is None else [ord(c) for c in text] + [0]
  s = head + body
  assert len(s) > 0
  for i, byte in enumerate([address>>8, address&255, len(s)]+s):
    comment = repr(chr(byte)) if i >= 3+len(head) else None
    program.putInRomTable(byte, comment=comment)

#-----------------------------------------------------------------------

lastRomFile = ''

def insertRomDir(name):
  global lastRomFile
  if name[0] != '_':                    # Mechanism for hiding files
    if pc()&255 >= 251-14:              # Prevent page crossing
      trampoline()
    s = lastRomFile[0:8].ljust(8,'\0')  # Cropping and padding
    if len(lastRomFile) == 0:
      lastRomFile = 0
    for i in range(8):
      st(ord(s[i]), [Y,Xpp])            #25-32
      C(repr(s[i]))
    ld(lo(lastRomFile))                 #33
    st([vAC])                           #34
    ld(hi(lastRomFile))                 #35
    ld(hi('.sysDir#39'),Y)              #36
    jmp(Y,'.sysDir#39')                 #37
    st([vAC+1])                         #38
    lastRomFile = name

#-----------------------------------------------------------------------
#       Embedded programs must be given on the command line
#-----------------------------------------------------------------------

if pc()&255 >= 251:                     # Don't start in a trampoline region
  align(0x100)

for application in argv[1:]:
  print()

  # Determine label
  if '=' in application:
    # Explicit label given as 'label=filename'
    name, application = application.split('=', 1)
  else:
    # Label derived from filename itself
    name = application.rsplit('.', 1)[0] # Remove extension
    name = name.rsplit('/', 1)[-1]       # Remove path
  print('Processing file %s label %s' % (application, name))

  C('+-----------------------------------+')
  C('| %-33s |' % application)
  C('+-----------------------------------+')

  # Pre-compiled GT1 files
  if application.endswith(('.gt1', '.gt1x')):
    print('Load type .gt1 at $%04x' % pc())
    with open(application, 'rb') as f:
      raw = bytearray(f.read())
    insertRomDir(name) 
    label(name)
    raw = raw[:-2] # Drop start address
    if raw[0] == 0 and raw[1] + raw[2] > 0xc0:
      highlight('Warning: zero-page conflict with ROM loader (SYS_Exec_88)')
    program = gcl.Program(None)
    for byte in raw:
      program.putInRomTable(byte)
    program.end()

  # GCL files
  #----------------------------------------------------------------
  #  !!! GCL programs using *labels* "_L=xx" must be cautious !!!
  # Those labels end up in the same symbol table as the ROM build,
  # and name clashes cause havoc. It's safer to precompile such
  # applications into .gt1/.gt1x files. (This warning doesn't apply
  # to ordinary GCL variable names "xx A=".)
  #----------------------------------------------------------------
  elif application.endswith('.gcl'):
    print('Compile type .gcl at $%04x' % pc())
    insertRomDir(name)
    label(name)
    program = gcl.Program(name)
    program.org(userCode)
    zpReset(userVars)
    for line in open(application).readlines():
      program.line(line)
    program.end()

  # Application-specific SYS extensions
  elif application.endswith('.py'):
    print('Include type .py at $%04x' % pc())
    label(name)
    importlib.import_module(name)

  # GTB files
  elif application.endswith('.gtb'):
    print('Link type .gtb at $%04x' % pc())
    zpReset(userVars)
    label(name)
    program = gcl.Program(name)
    # BasicProgram comes from TinyBASIC.gcl
    address = symbol('BasicProgram')
    if not has(address):
      highlight('Error: TinyBASIC must be compiled-in first')
    program.org(address)
    i = 0
    for line in open(application):
      i += 1
      line = line.rstrip()[0:25]
      number, text = '', ''
      for c in line:
        if c.isdigit() and len(text) == 0:
          number += c
        else:
          text += c
      basicLine(address, int(number), text)
      address += 32
      if address & 255 == 0:
        address += 160
    basicLine(address+2, None, 'RUN')           # Startup command
    # Buffer comes from TinyBASIC.gcl
    basicLine(symbol('Buffer'), address, None)  # End of program
    program.putInRomTable(0)
    program.end()
    print(' Lines', i)

  # Simple sequential RGB file (for Racer horizon image)
  elif application.endswith('-256x16.rgb'):
    width, height = 256, 16
    print('Convert type .rgb/sequential at $%04x' % pc())
    f = open(application, 'rb')
    raw = bytearray(f.read())
    f.close()
    insertRomDir(name)
    label(name)
    packed, quartet = [], []
    for i in range(0, len(raw), 3):
      R, G, B = raw[i+0], raw[i+1], raw[i+2]
      quartet.append((R//85) + 4*(G//85) + 16*(B//85))
      if len(quartet) == 4:
        # Pack 4 pixels in 3 bytes
        packed.append( ((quartet[0]&0b111111)>>0) + ((quartet[1]&0b000011)<<6) )
        packed.append( ((quartet[1]&0b111100)>>2) + ((quartet[2]&0b001111)<<4) )
        packed.append( ((quartet[2]&0b110000)>>4) + ((quartet[3]&0b111111)<<2) )
        quartet = []
    for i in range(len(packed)):
      ld(packed[i])
      if pc()&255 == 251:
        trampoline()
    print(' Pixels %dx%d' % (width, height))

  # Random access RGB files (for Pictures application)
  elif application.endswith('-160x120.rgb'):
    if pc()&255 > 0:
      trampoline()
    print('Convert type .rgb/parallel at $%04x' % pc())
    f = open(application, 'rb')
    raw = f.read()
    f.close()
    label(name)
    for y in range(0, qqVgaHeight, 2):
      for j in range(2):
        comment = 'Pixels for %s line %s' % (name, y+j)
        for x in range(0, qqVgaWidth, 4):
          bytes = []
          for i in range(4):
            R = raw[3 * ((y + j) * qqVgaWidth + x + i) + 0]
            G = raw[3 * ((y + j) * qqVgaWidth + x + i) + 1]
            B = raw[3 * ((y + j) * qqVgaWidth + x + i) + 2]
            bytes.append( (R//85) + 4*(G//85) + 16*(B//85) )
          # Pack 4 pixels in 3 bytes
          ld( ((bytes[0]&0b111111)>>0) + ((bytes[1]&0b000011)<<6) ); comment = C(comment)
          ld( ((bytes[1]&0b111100)>>2) + ((bytes[2]&0b001111)<<4) )
          ld( ((bytes[2]&0b110000)>>4) + ((bytes[3]&0b111111)<<2) )
        if j==0:
          trampoline3a()
        else:
          trampoline3b()
    print(' Pixels %dx%d' % (width, height))

  # XXX Provisionally bring ROMv1 egg back as placeholder for Pictures
  elif application.endswith(('/gigatron.rgb', '/packedPictures.rgb')):
    print(('Convert type gigatron.rgb at $%04x' % pc()))
    f = open(application, 'rb')
    raw = bytearray(f.read())
    f.close()
    label(name)
    for i in range(len(raw)):
      if i&255 < 251:
        ld(raw[i])
      elif pc()&255 == 251:
        trampoline()

  else:
    assert False

  C('End of %s, size %d' % (application, pc() - symbol(name)))
  print(' Size %s' % (pc() - symbol(name)))

#-----------------------------------------------------------------------
# ROM directory
#-----------------------------------------------------------------------

# SYS_ReadRomDir implementation

if pc()&255 > 251 - 28:         # Prevent page crossing
  trampoline()
label('sys_ReadRomDir')
beq('.sysDir#20')               #18
ld(lo(sysArgs),X)               #19
ld(AC,Y)                        #20 Follow chain to next entry
ld([vAC])                       #21
suba(14)                        #22
jmp(Y,AC)                       #23
#ld(hi(sysArgs),Y)              #24 Overlap
#
label('.sysDir#20')
ld(hi(sysArgs),Y)               #20,24 Dummy
ld(lo('.sysDir#25'))            #21 Go to first entry in chain
ld(hi('.sysDir#25'),Y)          #22
jmp(Y,AC)                       #23
ld(hi(sysArgs),Y)               #24
label('.sysDir#25')
insertRomDir(lastRomFile)       #25-38 Start of chain
label('.sysDir#39')
ld(hi('REENTER'),Y)             #39 Return
jmp(Y,'REENTER')                #40
ld(-44/2)                       #41

print()

#-----------------------------------------------------------------------
# End of embedded applications
#-----------------------------------------------------------------------

if pc()&255 > 0:
  trampoline()

#-----------------------------------------------------------------------
# Finish assembly
#-----------------------------------------------------------------------
end()
writeRomFiles(argv[0])
