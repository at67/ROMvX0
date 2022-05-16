#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <algorithm>

#include "memory.h"
#include "cpu.h"
#include "assembler.h"
#include "compiler.h"
#include "optimiser.h"


namespace Optimiser
{
    enum OptimiseTypes
    {
        // Operands are matched
        StStw=0, StwLdwPair, StwLdPair, StwStHigh, ExtraLdw, ExtraLd, LdwPair, LdwStwLdwStw, StwLdiAddw, StwLdAddw, StwLdAddwVar, StwLdwAddw, StwLdwAddwVar, StwLdiAndw, StwLdAndw, StwLdAndwVar, StwLdwAndw,
        StwLdwAndwVar, StwLdiXorw, StwLdXorw, StwLdXorwVar, StwLdwXorw, StwLdwXorwVar, StwLdiOrw, StwLdOrw, StwLdOrwVar, StwLdwOrw, StwLdwOrwVar, Stpx0, Stpx1, Stpx2, Stpx3, StwLdwiAddw, StwLdwiSubw,
        StwLdwiAndw, StwLdwiXorw, StwLdwiOrw, LdwBeqStwTmp, LdwBeqStwVar, LdBeqStTmp, LdBeqStVar, Inc4Var, Inc3Var, Inc2Var, Dec4Var, Dec3Var, Dec2Var, LdwAddiStw, LdwAddwStw, LdAddiSt, LdwAddiSt, LdwSubiStw,
        LdwSubwStw, IncwLdw, DecwLdw, LdStwLdSubw, LdStwLdwSubw, LdSub1St, LdwSub1Stw, LdSubiSt, LdwSubiSt, LdAndiSt, LdwAndiSt, LdOriSt, LdwOriSt, LdwOriStw, LdXoriSt, LdwXoriSt, LdwXoriStw, LdAndi, LdwAndi,
        LdOri, LdXori, LdwLslwStw, LdwLsrvStw, LdwLsrv2Stw, LdLsrb1St, LdLsrb2St, LdLsrb3St, LdLsrb4St, PokeVar, DokeVar, LdwPeekInc, PeekvInc, PokeInc, LdwDeekIncInc, DeekvIncInc, DokeIncInc, LdSubiBcc,
        LdwSubiBcc, LdXoriBcc, LdwXoriBcc, LdwNotwStw, LdwNegwStw, LdNotwSt, LdwNotwSt, LdwiAddwStwLdwiAddw, LdwiAddw2StwLdwiAddw2, LdwiStwLdiPoke, LdwiStwLdiDoke, LdwiStwLdwiDoke, StwLdPoke, StwLdwPoke,
        StwLdwDoke, StwLdwDokeReg, StwLdwIncPoke, LdStwLdwPokea, LdStwLdwiPokea, LdStwLdwiAddiPokea, NegwArrayB, NegwArray, AddwArrayB, AddwArray, SubwArrayB, SubwArray, AndwArrayB, AndwArray, OrwArrayB,
        OrwArray, XorwArrayB, XorwArray, LdStwMovqbLdwStw, LdwStwMovqbLdwStw, LdwiStwMovqbLdwStw, LdStwMovqwLdwStw, LdwStwMovqwLdwStw, LdwiStwMovqwLdwStw, StwLdwiAddwAddw, LdwiAddwAddw, LdwArrw, StwStwArrvwDokea,
        StwArrvwDokea, ArrvwDeek, AssignArray, StarrwLdarrw, StwLdiPokea, AddwAddwDeek,

        // Operands are NOT matched
        MovbSt, PeekSt, PeekVar, DeekStw, DeekVar, LdwiDeek, LdwDeekAddbi, DeekvAddbi, DokeAddbi, LdSt, LdwSt, StwPair, StwPairReg, ExtraStw, PeekArrayB, PeekArray, DeekArray, PokeArray,
        DokeArray, PokeVarArrayB, PokeVarArray, DokeVarArray, PokeTmpArrayB, PokeTmpArray, DokeTmpArray, PokeaVarArrayB, PokeaVarArray, DokeaVarArray, PokeaTmpArrayB, PokeaTmpArray, DokeaTmpArray, MovwaLdwiPokea,
        MovwaLdwiAddwPeeka, MovqwLdwiAddiPokea, LdwiAddwPeek, LdwiAddwPeeka, MovwaLdarrbSt, StwLdwiAddwPokea, LdwiAddwPokea, LdwiAddwPokei, StwMovb, StwPokeArray, StwDokeArray, LdiSt, LdiStw, LdSubLoHi, LdiSubLoHi,
        LdwSubLoHi, LdiAddi, LdiSubi, LdiAndi, LdiOri, LdiXori, AddiPair, LdwStw, TeqStw, TneStw, TltStw, TgtStw, TleStw, TgeStw, TeqJump, TneJump, TltJump, TgtJump, TleJump, TgeJump, TeqCondii, TeqCondib, TeqCondbi,
        TeqCondbb, MovbMovb0, MovbMovb1, MovbMovb2, PackvwLdw,

        // Opcodes AND operands are manually matched
        MovwaStarrb, AddiZero, SubiZero, LdwiNeg, LdwiSml, NumOptimiseTypes
    };

    struct MatchSequence
    {
        int _firstIndex;
        int _secondIndex;
        std::vector<std::string> _sequence;
        bool _doMatch = true;
    };

    std::vector<MatchSequence> matchSequences = 
    {
        // StStw
        {0, 0, {Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // StwLdwPair
        {0, 1, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdPair
        {0, 1, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",  OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwStHigh
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // ExtraLdw
        {0, 1, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // ExtraLd
        {0, 1, {Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // LdwPair
        {0, 1, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdwStwLdwStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwLdiAddw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAddw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAddwVar
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAddw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAddwVar
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdiAndw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAndw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAndwVar
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAndw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAndwVar
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdiXorw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",     
                Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdXorw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdXorwVar
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwXorw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwXorwVar
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdiOrw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdOrw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdOrwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD",  OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwOrw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwOrwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // Stpx0
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STPX", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Stpx1
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STPX", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Stpx2
        {0, 0, {Expression::createPaddedString("MOVB",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVB",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVQB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STPX",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Stpx3
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STPX", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwLdwiAddw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiSubw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiAndw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiXorw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiOrw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW",  OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdwBeqStwTmp
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdwBeqStwVar
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // LdBeqStTmp
        {0, 2, {Expression::createPaddedString("LD",  OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdBeqStVar
        {0, 2, {Expression::createPaddedString("LD",  OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // Inc4Var, this will fail with malicious code, (as it only matches 0 and 3)
        {0, 3, {Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Inc3Var, this will fail with malicious code, (as it only matches 0 and 2)
        {0, 2, {Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Inc2Var
        {0, 1, {Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Dec4Var, this will fail with malicious code, (as it only matches 0 and 3)
        {0, 3, {Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Dec3Var, this will fail with malicious code, (as it only matches 0 and 2)
        {0, 2, {Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // Dec2Var
        {0, 1, {Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("DEC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAddiStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAddwStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdAddiSt
        {0, 2, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAddiSt
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubiStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubwStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // IncwLdw
        {0, 1, {Expression::createPaddedString("INCW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DecwLdw
        {0, 1, {Expression::createPaddedString("DECW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdStwLdSubw
        {1, 3, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdStwLdwSubw
        {1, 3, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSub1St
        {0, 2, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSub1Stw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSubiSt
        {0, 2, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubiSt
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdAndiSt
        {0, 2, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAndiSt
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdOriSt
        {0, 2, {Expression::createPaddedString("LD",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwOriSt
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwOriStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdXoriSt
        {0, 2, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwXoriSt
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwXoriStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdAndi
        {0, 0, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAndi
        {0, 0, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdOri
        {0, 0, {Expression::createPaddedString("LD",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdXori
        {0, 0, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwLslwStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("LSLW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwLsrvStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwLsrv2Stw
        {0, 3, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb1St
        {0, 2, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb2St
        {0, 3, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb3St
        {0, 4, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb4St
        {0, 5, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PokeVar
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DokeVar
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwPeekInc
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PeekvInc
        {0, 1, {Expression::createPaddedString("PEEKV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PokeInc
        {0, 1, {Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwDeekIncInc
        {0, 3, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekvIncInc
        {0, 2, {Expression::createPaddedString("DEEKV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DokeIncInc
        {0, 2, {Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSubiBcc
        {0, 0, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubiBcc
        {0, 0, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdXoriBcc
        {0, 0, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwXoriBcc
        {0, 0, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwNotwStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NOTW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwNegwStw
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NEGW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdNotwSt
        {0, 2, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NOTW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwNotwSt
        {0, 2, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NOTW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddwStwLdwiAddw
        {0, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddw2StwLdwiAddw2
        {0, 4, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiStwLdiPoke
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdwiStwLdiDoke
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdwiStwLdwiDoke
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // StwLdPoke
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwLdwPoke
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwLdwDoke
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwLdwDokeReg
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // StwLdwIncPoke
        {0, 3, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwLdwPokea
        {1, 3, {Expression::createPaddedString("LD",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwLdwiPokea
        {1, 3, {Expression::createPaddedString("LD",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwLdwiAddiPokea
        {1, 4, {Expression::createPaddedString("LD",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // NegwArrayB
        {0, 5, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NEGW",  OPCODE_TRUNC_SIZE, ' ') + "giga_vAC",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // NegwArray
        {0, 5, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NEGW",  OPCODE_TRUNC_SIZE, ' ') + "giga_vAC",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AddwArrayB
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AddwArray
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // SubwArrayB
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // SubwArray
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AndwArrayB
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AndwArray
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // OrwArrayB
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // OrwArray
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // XorwArrayB
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // XorwArray
        {0, 9, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwMovqbLdwStw
        {1, 3, {Expression::createPaddedString("LD",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwStwMovqbLdwStw
        {1, 3, {Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiStwMovqbLdwStw
        {1, 3, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdStwMovqwLdwStw
        {1, 3, {Expression::createPaddedString("LD",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwStwMovqwLdwStw
        {1, 3, {Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiStwMovqwLdwStw
        {1, 3, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwLdwiAddwAddw
        {0, 2, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddwAddw
        {1, 2, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwArrw
        {0, 0, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ARRW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwStwArrvwDokea
        {1, 3, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ARRVW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwArrvwDokea
        {0, 2, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ARRVW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // ArrvwDeek
        {0, 0, {Expression::createPaddedString("ARRVW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // AssignArray
        {1, 3, {Expression::createPaddedString("ARRVW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEKA", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ARRVW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StarrwLdarrw
        {0, 1, {Expression::createPaddedString("STARRW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDARRW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwLdiPokea
        {0, 2, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDI",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // AddwAddwDeek
        {0, 1, {Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        /******************************************************************************************/
        /* Operands are not matched from here on in
        /******************************************************************************************/

        // MovbSt
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "giga_vAC + 1, giga_vAC",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PeekSt
        {0, 0, {Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",   OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // PeekVar
        {0, 0, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekStw
        {0, 0, {Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekVar
        {0, 0, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiDeek
        {0, 0, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwDeekAddbi
        {0, 2, {Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekvAddbi
        {0, 1, {Expression::createPaddedString("DEEKV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DokeAddbi
        {0, 1, {Expression::createPaddedString("DOKE",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSt
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSt
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwPair
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x" }},

        // StwPairReg
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // ExtraStw
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_"  }},

        // PeekArrayB
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PeekArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PokeArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeVarArrayB
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeVarArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeVarArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeTmpArrayB
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeTmpArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeTmpArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaVarArrayB
        {0, 0, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaVarArray
        {0, 0, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeaVarArray
        {0, 0, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaTmpArrayB
        {0, 0, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaTmpArray
        {0, 0, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeaTmpArray
        {0, 0, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW",   OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // MovwaLdwiPokea
        {0, 0, {Expression::createPaddedString("MOVWA", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // MovwaLdwiAddwPeeka
        {0, 0, {Expression::createPaddedString("MOVWA", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEKA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // MovqwLdwiAddiPokea
        {0, 0, {Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdwiAddwPeek
        {0, 0, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddwPeeka
        {0, 0, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("PEEKA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // MovwaLdarrbSt
        {0, 0, {Expression::createPaddedString("MOVWA",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("LDARRB", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ST",     OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwLdwiAddwPokea
        {0, 0, {Expression::createPaddedString("STW",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddwPokea
        {0, 0, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddwPokei
        {0, 0, {Expression::createPaddedString("LDWI",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDW",  OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("POKEI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwMovb
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwPokeArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwDokeArray
        {0, 0, {Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdiSt
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiStw
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSubLoHi
        {0, 0, {Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdiSubLoHi
        {0, 0, {Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdwSubLoHi
        {0, 0, {Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LD",   OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW",  OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdiAddi
        {0, 0, {Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiSubi
        {0, 0, {Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiAndi
        {0, 0, {Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiOri
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiXori
        {0, 0, {Expression::createPaddedString("LDI",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // AddiPair
        {0, 0, {Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwStw
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TeqStw
        {0, 0, {Expression::createPaddedString("TEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TneStw
        {0, 0, {Expression::createPaddedString("TNE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TltStw
        {0, 0, {Expression::createPaddedString("TLT", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TgtStw
        {0, 0, {Expression::createPaddedString("TGT", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TleStw
        {0, 0, {Expression::createPaddedString("TLE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TgeStw
        {0, 0, {Expression::createPaddedString("TGE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TeqJump
        {0, 0, {Expression::createPaddedString("TEQ", OPCODE_TRUNC_SIZE, ' '), ""}},

        // TneJump
        {0, 0, {Expression::createPaddedString("TNE", OPCODE_TRUNC_SIZE, ' '), ""}},

        // TltJump
        {0, 0, {Expression::createPaddedString("TLT", OPCODE_TRUNC_SIZE, ' '), ""}},

        // TgtJump
        {0, 0, {Expression::createPaddedString("TGT", OPCODE_TRUNC_SIZE, ' '), ""}},

        // TleJump
        {0, 0, {Expression::createPaddedString("TLE", OPCODE_TRUNC_SIZE, ' '), ""}},

        // TgeJump
        {0, 0, {Expression::createPaddedString("TGE", OPCODE_TRUNC_SIZE, ' '), ""}},

        // TeqCondii
        {0, 0, {Expression::createPaddedString("TEQ",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("CONDII", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TeqCondib
        {0, 0, {Expression::createPaddedString("TEQ",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("CONDIB", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TeqCondbi
        {0, 0, {Expression::createPaddedString("TEQ",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("CONDBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // TeqCondbb
        {0, 0, {Expression::createPaddedString("TEQ",    OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("CONDBB", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // MovbMovb0
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // MovbMovb1
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // MovbMovb2
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PackvwLdw
        {0, 0, {Expression::createPaddedString("PACKVW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW",    OPCODE_TRUNC_SIZE, ' ') + ""}},

        /******************************************************************************************/
        /* Opcodes are manually matched here on in
        /******************************************************************************************/

        // MovwaStarrb
        {0, 0, {Expression::createPaddedString("MOVWA",  OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STARRB", OPCODE_TRUNC_SIZE, ' ') + ""}, false},

        // AddiZero
        {0, 0, {Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' '), ""}, false},

        // SubiZero
        {0, 0, {Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' '), ""}, false},

        // LdwiNeg
        {0, 0, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' '), ""}, false},

        // LdwiSml
        {0, 0, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' '), ""}, false},
    };


    bool initialise(void)
    {
        return true;
    }

    
    bool invertCC(std::string& cc)
    {
        if(cc.length() != 3) return false;

        if(cc.substr(1) == "EQ") {cc.replace(1, 2, "NE"); return true;}
        if(cc.substr(1) == "NE") {cc.replace(1, 2, "EQ"); return true;}
        if(cc.substr(1) == "LT") {cc.replace(1, 2, "GE"); return true;}
        if(cc.substr(1) == "GT") {cc.replace(1, 2, "LE"); return true;}
        if(cc.substr(1) == "LE") {cc.replace(1, 2, "GT"); return true;}
        if(cc.substr(1) == "GE") {cc.replace(1, 2, "LT"); return true;}

        return false;
    }

    // Migrate label for an instruction that has been deleted, (use this function before the instruction is deleted)
    bool migratelLabel(int index, int oldLine, int newLine)
    {
        // If a label exists, move it to next available vasm line
        if(Compiler::getCodeLines()[index]._vasm[oldLine]._labelIndex > -1)
        {
            // Next available vasm line is part of a new BASIC line, so can't optimise
            if(int(Compiler::getCodeLines()[index]._vasm.size()) <= newLine) return false;
            Compiler::getCodeLines()[index]._vasm[newLine]._labelIndex = Compiler::getCodeLines()[index]._vasm[oldLine]._labelIndex;
        }
    
        return true;
    }

    // Adjust label addresses for any labels with addresses higher than optimised vasm instruction address
    void adjustLabelAddresses(int codeLineIndex, int vasmLineIndex, int16_t offset)
    {
        // Loop through commented out code
        do
        {
            if(vasmLineIndex >= int(Compiler::getCodeLines()[codeLineIndex]._vasm.size()))
            {
                if(++codeLineIndex >= int(Compiler::getCodeLines().size())) return;
                vasmLineIndex = 0;
            }
        }
        while(Compiler::getCodeLines()[codeLineIndex]._vasm.size() == 0);

        uint16_t optimisedAddress = Compiler::getCodeLines()[codeLineIndex]._vasm[vasmLineIndex]._address;

        for(int i=0; i<int(Compiler::getLabels().size()); i++)
        {
            if(Compiler::getLabels()[i]._address >= optimisedAddress)
            {
                Compiler::getLabels()[i]._address += offset;
            }
        }
    }

    // Adjust vasm code addresses
    void adjustVasmAddresses(int codeLineIndex, int vasmLineIndex, int16_t offset)
    {
        // Loop through commented out code
        do
        {
            if(vasmLineIndex >= int(Compiler::getCodeLines()[codeLineIndex]._vasm.size()))
            {
                if(++codeLineIndex >= int(Compiler::getCodeLines().size())) return;
                vasmLineIndex = 0;
            }
        }
        while(Compiler::getCodeLines()[codeLineIndex]._vasm.size() == 0);

        for(int i=codeLineIndex; i<int(Compiler::getCodeLines().size()); i++)
        {
            int start = (i == codeLineIndex) ? vasmLineIndex : 0;
            for(int j=start; j<int(Compiler::getCodeLines()[i]._vasm.size()); j++)
            {
                Compiler::getCodeLines()[i]._vasm[j]._address += offset;
            }
        }
    }

    void adjustLabelAndVasmAddresses(int codeLineIndex, int vasmLineIndex, std::vector<std::string> instructions)
    {
        if(instructions.size() == 0) return;

        int16_t offset = 0;
        for(int i=0; i<int(instructions.size()); i++)
        {
            std::string opcode = (instructions[i][0] == '%'  &&  instructions[i].size() > 1) ? instructions[i].substr(1) : instructions[i];
            int macroSize = int16_t(Compiler::getMacroSize(opcode));
            if(macroSize)
            {
                offset += int16_t(macroSize);
            }
            else
            {
                offset += int16_t(Assembler::getAsmOpcodeSize(instructions[i]));
            }
        }

        adjustLabelAddresses(codeLineIndex, vasmLineIndex, -offset);
        adjustVasmAddresses(codeLineIndex, vasmLineIndex, -offset);
    }

    void updateVasm(int codeLineIndex, int vasmLineIndex, const std::string& opcode, const std::string& operand)
    {
        int opcodeSize = Assembler::getAsmOpcodeSize(opcode);
        int opcodeOffset = opcodeSize - Compiler::getCodeLines()[codeLineIndex]._vasm[vasmLineIndex]._vasmSize;

        Compiler::getCodeLines()[codeLineIndex]._vasm[vasmLineIndex]._opcode = opcode;
        Compiler::getCodeLines()[codeLineIndex]._vasm[vasmLineIndex]._operand = operand;
        Compiler::getCodeLines()[codeLineIndex]._vasm[vasmLineIndex]._code = Expression::createPaddedString(opcode,  OPCODE_TRUNC_SIZE, ' ') + operand;
        Compiler::getCodeLines()[codeLineIndex]._vasm[vasmLineIndex]._vasmSize = opcodeSize;

        // if new opcode is a different size to old opcode, then adjust label and vasm addresses accordingly
        int clIdx = codeLineIndex;
        int vlIdx = vasmLineIndex + 1;
        if(vlIdx >= Compiler::getCodeLines()[codeLineIndex]._vasm.size())
        {
            vlIdx = 0;
            if(++clIdx >= Compiler::getCodeLines().size()) return;
        }

        adjustLabelAddresses(clIdx, vlIdx, int16_t(opcodeOffset));
        adjustVasmAddresses(clIdx, vlIdx, int16_t(opcodeOffset));
    }

    void insertVasm(int codeLineIndex, int vasmSrcLineIndex, int vasmDstLineIndex, const std::string& opcode, const std::string& operand)
    {
        Compiler::VasmLine vasm = Compiler::getCodeLines()[codeLineIndex]._vasm[vasmSrcLineIndex];
        int vasmSizeOld = vasm._vasmSize;
        vasm._address += uint16_t(vasmSizeOld);
        Compiler::getCodeLines()[codeLineIndex]._vasm.insert(Compiler::getCodeLines()[codeLineIndex]._vasm.begin() + vasmDstLineIndex, vasm);
        adjustLabelAddresses(codeLineIndex, vasmDstLineIndex + 1, int16_t(vasmSizeOld));
        adjustVasmAddresses(codeLineIndex, vasmDstLineIndex + 1, int16_t(vasmSizeOld));
        updateVasm(codeLineIndex, vasmDstLineIndex, opcode, operand);
    }


    bool optimiseCode(void)
    {
        bool optimised;
        int numOptimiserPasses = 0;

        do
        {
            optimised = false;

            for(int codeLine=0; codeLine<int(Compiler::getCodeLines().size()); codeLine++)
            {
                // Skip code lines when optimiser is disabled
                if(!Compiler::getCodeLines()[codeLine]._optimiserEnabled) continue;

                for(int matchIndex=0; matchIndex<int(matchSequences.size()); matchIndex++)
                {
                    for(auto itVasm=Compiler::getCodeLines()[codeLine]._vasm.begin(); itVasm!=Compiler::getCodeLines()[codeLine]._vasm.end();)
                    {
                        bool linesDeleted = false;
                        int vasmIndex = int(itVasm - Compiler::getCodeLines()[codeLine]._vasm.begin());

                        // Match opcodes
                        bool foundOpcodeMatch = false;
                        if(matchSequences[matchIndex]._doMatch)
                        {
                            // Can only optimise within a BASIC code line, (use multi-statements to optimise across lines)
                            int vasmIndexMax = vasmIndex + int(matchSequences[matchIndex]._sequence.size()) - 1;
                            if(vasmIndexMax >= int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                            {
                                ++itVasm;
                                continue;
                            }

                            // Find opcode match
                            foundOpcodeMatch = true;
                            for(int k=vasmIndex; k<=vasmIndexMax; k++)
                            {
                                if(Compiler::getCodeLines()[codeLine]._vasm[k]._code.find(matchSequences[matchIndex]._sequence[k - vasmIndex]) == std::string::npos)
                                {
                                    foundOpcodeMatch = false;
                                    break;
                                }
                            }
                        }

                        if(foundOpcodeMatch)
                        {
                            // First operand
                            int firstIndex = matchSequences[matchIndex]._firstIndex;
                            int firstMatch = vasmIndex + firstIndex;
                            std::string firstOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;

                            // Second operand
                            int secondIndex = matchSequences[matchIndex]._secondIndex;
                            int secondMatch = vasmIndex + secondIndex;
                            std::string secondOperand = Compiler::getCodeLines()[codeLine]._vasm[secondMatch]._operand;

    /*************************************************************************************************************************************************************/
    /* Opcode matches required, operand matches required                                                                                                         */
    /*************************************************************************************************************************************************************/

                            // Find operand match, (temporary variables are a minimum of 4 chars, i.e. '0xd0')
                            // Don't use this, as sequences such as 'ST blah + 1' will match with 'ST blah'
                            //if(firstOperand.substr(0, 4) == secondOperand.substr(0, 4))
                            if(firstOperand == secondOperand)
                            {
                                switch(matchIndex)
                                {
                                    // Match ST, STW, replace ST's operand with STW's operand, delete STW
                                    case StStw:
                                    {
                                        // Migrate label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 1)) break;

                                        // Replace ST's operand with STW's operand
                                        std::string stwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ST", stwOperand);

                                        // Delete STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW"});
                                    }
                                    break;

                                    // Match STW LDW, delete STW LDW
                                    //case StwLdPair:
                                    case StwLdwPair:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW ST, delete STW
                                    case StwStHigh:
                                    {
                                        // Assume neither of these instructions can have a label
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW"});
                                    }
                                    break;

                                    // Match STW/ST LDW/LD, delete LDW/LD
                                    case ExtraLdw:
                                    case ExtraLd:
                                    {
                                        // If the LDW has a label, then it probably can't be optimised away
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex == -1)
                                        {
                                            // Migratel label to next available instruction, (if it exists)
                                            if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                            // Delete LDW
                                            linesDeleted = true;
                                            itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                            adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDW"});
                                        }
                                    }
                                    break;

                                    // Match LDW LDW, delete first LDW
                                    case LdwPair:
                                    {
                                        // Migratel label from first LDW to second LDW, (if it exists)
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 1)) break;

                                        // Delete first LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LDW"});
                                    }
                                    break;

                                    // Match LDW STW LDW STW, delete second LDW
                                    case LdwStwLdwStw:
                                    {
                                        // if second LDW has a label then bail
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Delete second LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"LDW"});
                                    }
                                    break;

                                    // Match STW LDI ADDW, copy LDI operand to ADDW operand, change ADDW to ADDI, delete STW LDI
                                    case StwLdiAddw:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ADDW with ADDI and replace ADDW's operand with LDI's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ADDI", ldOperand);

                                        // Delete STW and LDI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDI"});
                                    }
                                    break;

                                    // Match STW LD ADDW, replace ADDW with ADDBA and LD's operand, delete STW LD
                                    case StwLdAddw:
                                    case StwLdAddwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ADDW with ADDBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ADDBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW ADDW, copy LDW operand to ADDW operand, delete STW LDW
                                    case StwLdwAddw:
                                    case StwLdwAddwVar:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ADDW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ADDW", ldwOperand);

                                        // Delete STW and LD/LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDI ANDW, copy LDI operand to ANDW operand, change ANDW to ANDI, delete STW LDW
                                    case StwLdiAndw:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ANDW with ANDI and replace ANDW's operand with LDI's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ANDI", ldOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LD ANDW, replace ANDW with ANDBA and LD's operand, delete STW LD
                                    case StwLdAndw:
                                    case StwLdAndwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ANDW with ANDBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ANDBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW ANDW, copy LDW operand to ANDW operand, delete STW LDW
                                    case StwLdwAndw:
                                    case StwLdwAndwVar:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ANDW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ANDW", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LDI XORW, copy LDI operand to XORW operand, change XORW to XORI, delete STW LDW
                                    case StwLdiXorw:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace XORW with XORI and replace XORW's operand with LDI's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "XORI", ldOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LD XORW, replace XORW with XORBA and LD's operand, delete STW LD
                                    case StwLdXorw:
                                    case StwLdXorwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace XORW with XORBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "XORBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW XORW, copy LDW operand to XORW operand, delete STW LDW
                                    case StwLdwXorw:
                                    case StwLdwXorwVar:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace XORW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "XORW", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LDI ORW, copy LDI operand to ORW operand, change ORW to ORI, delete STW LDW
                                    case StwLdiOrw:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ORW with ORI and replace ORW's operand with LDI's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ORI", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LD ORW, replace ORW with ORBA and LD's operand, delete STW LD
                                    case StwLdOrw:
                                    case StwLdOrwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ORW with ORBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ORBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW ORW, copy LDW operand to ORW operand, delete STW LDW
                                    case StwLdwOrw:
                                    case StwLdwOrwVar:
                                    {
                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ORW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ORW", ldwOperand);

                                        // Delete STW and LD/LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LD"});
                                    }
                                    break;

                                    // Match MOVB MOVB LDW STPX, delete LDW and update both MOVB operands
                                    case Stpx0:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail for labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;

                                        // Match operands
                                        std::string movb00Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                        std::string movb10Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                        std::string movb01Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 1);
                                        std::string movb11Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                        std::string ldwOperand   = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand);
                                        std::string stpxOperand  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._operand);
                                        if(movb01Operand != ldwOperand) break;
                                        if(movb11Operand.find(ldwOperand) == std::string::npos) break;

                                        // Update MOVB's
                                        updateVasm(codeLine, firstMatch + 0, "MOVB", movb00Operand + ", giga_vAC");
                                        updateVasm(codeLine, firstMatch + 1, "MOVB", movb01Operand + ", giga_vAC + 1");

                                        // Delete LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"LDW"});
                                    }
                                    break;

                                    // Match MOVB MOVB MOVB LDW STPX, copy third MOVB's operand to STPX, update first and second MOVB operands, delete third MOVB and LDW
                                    case Stpx1:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail for labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._labelIndex >= 0) break;

                                        // Match operands
                                        std::string movb00Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                        std::string movb10Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                        std::string movb20Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand, 0);
                                        std::string movb01Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 1);
                                        std::string movb11Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                        std::string movb21Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand, 1);
                                        std::string ldwOperand   = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._operand);
                                        std::string stpxOperand  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._operand);
                                        if(movb01Operand != ldwOperand) break;
                                        if(movb11Operand.find(ldwOperand) == std::string::npos) break;
                                        bool match = false;
                                        if((movb21Operand == "fgbgColour+1"  ||  movb21Operand == "fgbgColour + 1") &&
                                           (stpxOperand == "fgbgColour+1"  ||  stpxOperand == "fgbgColour + 1")) match = true;
                                        if(movb21Operand == stpxOperand) match = true;
                                        if(!match) break;

                                        // Update MOVB's
                                        updateVasm(codeLine, firstMatch + 0, "MOVB", movb00Operand + ", giga_vAC");
                                        updateVasm(codeLine, firstMatch + 1, "MOVB", movb10Operand + ", giga_vAC + 1");

                                        // Update STPX
                                        updateVasm(codeLine, firstMatch + 4, "STPX", movb20Operand);

                                        // Delete third MOVB and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"MOVB", "LDW"});
                                    }
                                    break;

                                    // Match MOVB MOVB MOVQB LDW STPX, update first and second MOVB operands, delete LDW
                                    case Stpx2:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail for labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._labelIndex >= 0) break;

                                        // Match operands
                                        std::string movb00Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                        std::string movb10Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                        std::string movb01Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 1);
                                        std::string movb11Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                        std::string ldwOperand   = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._operand);
                                        std::string stpxOperand  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._operand);
                                        if(movb01Operand != ldwOperand) break;
                                        if(movb11Operand.find(ldwOperand) == std::string::npos) break;

                                        // Update MOVB's
                                        updateVasm(codeLine, firstMatch + 0, "MOVB", movb00Operand + ", giga_vAC");
                                        updateVasm(codeLine, firstMatch + 1, "MOVB", movb10Operand + ", giga_vAC + 1");

                                        // Delete LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 3);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 3, {"LDW"});
                                    }
                                    break;

                                    // Match MOVB LDW STPX, copy MOVB's operand to STPX and delete MOVB
                                    case Stpx3:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail for labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Match operands
                                        std::string movb0Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                        std::string movb1Operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 1);
                                        std::string ldwOperand   = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand);
                                        std::string stpxOperand  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand);
                                        bool match = false;
                                        if((movb1Operand == "fgbgColour+1"  ||  movb1Operand == "fgbgColour + 1") &&
                                           (stpxOperand == "fgbgColour+1"  ||  stpxOperand == "fgbgColour + 1")) match = true;
                                        if(movb1Operand == stpxOperand) match = true;
                                        if(!match) break;

                                        // Update STPX
                                        updateVasm(codeLine, firstMatch + 2, "STPX", movb0Operand);

                                        // Delete MOVB
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 0);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 0, {"MOVB"});
                                    }
                                    break;

                                    // Match STW LDWI ADDW, copy LDWI operand to ADDW operand, change ADDW to ADDWI, delete STW LDWI
                                    case StwLdwiAddw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ADDW with ADDWI and replace ADDWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ADDWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI SUBW, copy LDWI operand to SUBW operand, change SUBW to SUBIW, delete STW LDWI
                                    case StwLdwiSubw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace SUBW with SUBIW and replace SUBIW's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "SUBIW", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI ANDW, copy LDWI operand to ANDW operand, change ANDW to ANDWI, delete STW LDWI
                                    case StwLdwiAndw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ANDW with ANDWI and replace ANDWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ANDWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI XORW, copy LDWI operand to XORW operand, change XORW to XORWI, delete STW LDWI
                                    case StwLdwiXorw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace XORW with XORWI and replace XORWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "XORWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI ORW, copy LDWI operand to ORW operand, change ORW to ORWI, delete STW LDWI
                                    case StwLdwiOrw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have a label
                                        if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                        // Replace ORW with ORWI and replace ORWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ORWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match LDW/LD BEQ STW/ST, delete STW/ST
                                    case LdwBeqStwTmp:
                                    case LdwBeqStwVar:
                                    case LdBeqStTmp:
                                    case LdBeqStVar:
                                    {
                                        // STW/ST can have a label
                                        if(!migratelLabel(codeLine, firstMatch + 2, firstMatch + 3)) break;

                                        // Delete STW/ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"STW"});
                                    }
                                    break;

                                    // Match LDW POKE/DOKE LDW, delete second LDW if it matches with first LDW
                                    case PokeVar:
                                    case DokeVar:
                                    {
                                        // Migrate second LDW's label, (if it exists)
                                        if(!migratelLabel(codeLine, firstMatch + 2, firstMatch + 3)) break;

                                        // Delete second LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"LDW"});
                                    }
                                    break;

                                    // Match LDW PEEK INC
                                    case LdwPeekInc:
                                    {
                                        // Bail if wrong ROM version or if PEEK or INC have a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Replace LDW with PEEKV+ and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "PEEKV+", ldwOperand);

                                        // Delete PEEK, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"PEEK", "INC"});
                                    }
                                    break;

                                    // Match PEEKV INC
                                    case PeekvInc:
                                    {
                                        // Bail if wrong ROM version or if INC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Replace PEEKV with PEEKV+ and new operand
                                        std::string peekOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "PEEKV+", peekOperand);

                                        // Delete INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"INC"});
                                    }
                                    break;

                                    // Match POKE INC
                                    case PokeInc:
                                    {
                                        // Bail if wrong ROM version or if INC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Replace POKE with POKEV+ and new operand
                                        std::string pokeOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "POKEV+", pokeOperand);

                                        // Delete INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"INC"});
                                    }
                                    break;

                                    // Match LDW DEEK INC INC
                                    case LdwDeekIncInc:
                                    {
                                        // Bail if wrong ROM version or if DEEK, INC or INC have a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;

                                        // Replace LDW with DEEKV+ and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "DEEKV+", ldwOperand);

                                        // Delete DEEK, INC, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEEK", "INC", "INC"});
                                    }
                                    break;

                                    // Match DEEKV INC INC
                                    case DeekvIncInc:
                                    {
                                        // Bail if wrong ROM version or if INC or INC have a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Replace DEEKV with DEEKV+
                                        std::string deekvOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "DEEKV+", deekvOperand);

                                        // Delete INC, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"INC", "INC"});
                                    }
                                    break;

                                    // Match DOKE INC INC
                                    case DokeIncInc:
                                    {
                                        // Bail if wrong ROM version or if INC or INC have a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Replace DOKE with DOKEV+
                                        std::string dokeOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "DOKEV+", dokeOperand);

                                        // Delete INC, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"INC", "INC"});
                                    }
                                    break;

                                    // Match LD/LDW SUBI/XORI BCC
                                    case LdSubiBcc:
                                    //case LdXoriBcc:
                                    //case LdwSubiBcc:
                                    case LdwXoriBcc:
                                    {
                                        // Bail if wrong ROM version or if SUBI/XORI has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Bail if next instruction is not a BCC, JCC or JumpXX macro
                                        std::string bccOpcode;
                                        if(firstMatch + 2 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            bccOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._opcode.substr(0, 5);
                                            if(bccOpcode != "BEQ"  &&  bccOpcode != "BNE"  &&  bccOpcode != "BLE"  &&  bccOpcode != "BGE"  &&  bccOpcode != "BLT"  &&  bccOpcode != "BGT"  &&
                                               bccOpcode != "JEQ"  &&  bccOpcode != "JNE"  &&  bccOpcode != "JLE"  &&  bccOpcode != "JGE"  &&  bccOpcode != "JLT"  &&  bccOpcode != "JGT"  &&
                                               bccOpcode != "%Jump")
                                            {
                                                break;
                                            }
                                        }

                                        // Bail if next instruction is a write instruction, (this can fail for delayed write's in hand written code, but won't fail for compiled code; the optimiser is only ever run on compiled code)
                                        // This can still fail if this next instruction relies on the correct results of a subtraction, enable LdSubiBcc/LdXoriBcc with caution.
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode.find("ST") != std::string::npos  ||  stOpcode.find("OKE") != std::string::npos) break;
                                        }

                                        // Bail if SUBI/XORI has a label
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Replace LD/LDW with CMPI and new operands
                                        std::string subiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "CMPI", ldOperand + ", " + subiOperand);

                                        // Delete SUBI/XORI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"SUBI"});
                                    }
                                    break;

                                    // Match LDW NOTW STW, replace with NOTW
                                    case LdwNotwStw:
                                    {
                                        // Bail if wrong ROM version or if NOTW or STW have a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with NOTW and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "NOTW", ldwOperand);

                                        // Delete NOTW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"NOTW", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LDW NEGW STW
                                    case LdwNegwStw:
                                    {
                                        // Bail if wrong ROM version or if NEGW or STW have a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with NEGW and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "NEGW", ldwOperand);

                                        // Delete NEGW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"NEGW", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW NOTW ST, replace with NOTB
                                    case LdNotwSt:
                                    case LdwNotwSt:
                                    {
                                        // Bail if wrong ROM version or if NOTW or ST have a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LD/LDW with NOTB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "NOTB", ldOperand);

                                        // Delete NOTW and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"NOTW", "ST"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldOperand);
                                    }
                                    break;

                                    // Match INC INC, replace both with ADDBI 2
                                    case Inc2Var:
                                    {
                                        // Bail if wrong ROM version or if second INC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                
                                        // Replace first INC with ADDBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "ADDBI", incOperand + ", 2");

                                        // Delete second INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"INC"});
                                    }
                                    break;

                                    // Match INC INC INC, replace all three with ADDBI 3
                                    case Inc3Var:
                                    {
                                        // Bail if wrong ROM version or if second or third INC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                
                                        // Replace first INC with ADDBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "ADDBI", incOperand + ", 3");

                                        // Delete second and third INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"INC", "INC"});
                                    }
                                    break;

                                    // Match INC INC INC INC, replace all four with ADDBI 4
                                    case Inc4Var:
                                    {
                                        // Bail if wrong ROM version or if second or third or fourth INC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;
                                
                                        // Replace first INC with ADDBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "ADDBI", incOperand + ", 4");

                                        // Delete second, third and fourth INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"INC", "INC", "INC"});
                                    }
                                    break;

                                    // Match DEC DEC, replace both with SUBBI 2
                                    case Dec2Var:
                                    {
                                        // Bail if wrong ROM version or if second DEC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                
                                        // Replace first DEC with SUBBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "SUBBI", incOperand + ", 2");

                                        // Delete second DEC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEC"});
                                    }
                                    break;

                                    // Match DEC DEC DEC, replace all three with SUBBI 3
                                    case Dec3Var:
                                    {
                                        // Bail if wrong ROM version or if second or third DEC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                
                                        // Replace first DEC with SUBBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "SUBBI", incOperand + ", 3");

                                        // Delete second and third DEC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEC", "DEC"});
                                    }
                                    break;

                                    // Match DEC DEC DEC DEC, replace all four with SUBBI 4
                                    case Dec4Var:
                                    {
                                        // Bail if wrong ROM version or if second or third or fourth DEC has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;
                                
                                        // Replace first DEC with SUBBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "SUBBI", incOperand + ", 4");

                                        // Delete second, third and fourth DEC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEC", "DEC", "DEC"});
                                    }
                                    break;

                                    // Match LDW ADDI STW, replace all with ADDVI
                                    case LdwAddiStw:
                                    {
                                        // Bail if wrong ROM version or if ADDI or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Replace LDW with ADDVI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string addiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ADDVI", ldwOperand + ", " + addiOperand);

                                        // Delete ADDI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDI", "STW"});
                                    }
                                    break;

                                    // Match LDW ADDW STW, replace all with ADDVW
                                    case LdwAddwStw:
                                    {
                                        // Bail if wrong ROM version or if ADDW or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Replace LDW with ADDVW and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string addwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ADDVW", addwOperand + ", " + ldwOperand);

                                        // Delete ADDW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDW", "STW"});
                                    }
                                    break;

                                    // Match LD/LDW ADDI ST, replace all with ADDBI
                                    case LdAddiSt:
                                    case LdwAddiSt:
                                    {
                                        // Bail if wrong ROM version or if ADDI or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with ADDBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string addOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ADDBI", ldOperand + ", " + addOperand);

                                        // Delete ADDI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDI", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW SUBI STW, replace all with SUBVI
                                    case LdwSubiStw:
                                    {
                                        // Bail if wrong ROM version or if SUBI or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Replace LDW with SUBVI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string addiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "SUBVI", ldwOperand + ", " + addiOperand);

                                        // Delete SUBI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"SUBI", "STW"});
                                    }
                                    break;

                                    // Match LDW SUBW STW, replace all with SUBVW
                                    case LdwSubwStw:
                                    {
                                        // Bail if wrong ROM version or if SUBW or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Replace LDW with SUBVW and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string subwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "SUBVW", subwOperand + ", " + ldwOperand);

                                        // Delete SUBW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"SUBW", "STW"});
                                    }
                                    break;

                                    // Match INCW LDW, replace INCW with INCWA, del LDW
                                    case IncwLdw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Migrate LDW's label if it exists
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch)) break;

                                        // Replace INCW with INCWA
                                        std::string incwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "INCWA", incwOperand);

                                        // Delete LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDW"});
                                    }
                                    break;

                                    // Match DECW LDW, replace DECW with DECWA, del LDW
                                    case DecwLdw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Migrate LDW's label if it exists
                                        if(!migratelLabel(codeLine, firstMatch + 1, firstMatch)) break;

                                        // Replace DECW with DECWA
                                        std::string decwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "DECWA", decwOperand);

                                        // Delete LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDW"});
                                    }
                                    break;

                                    // Match LD STW LD/LDW SUBW, del LD STW, replace SUBW with SUBBA, (matches on STW and SUBW, that's why first LD is at firstLine - 1)
                                    case LdStwLdSubw:
                                    case LdStwLdwSubw:
                                    {
                                        // Bail if wrong ROM version or if STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._labelIndex >= 0) break;

                                        // Migratel label to next available instruction, (if it exists)
                                        if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 1)) break;

                                        // Replace SUBW with SUBBA and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "SUBBA", ldOperand);

                                        // Delete LD and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {"LD", "STW"});
                                    }
                                    break;

                                    // Match LD SUB1 ST, replace all with DEC
                                    case LdSub1St:
                                    {
                                        // Bail if wrong ROM version or if SUBI or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD with DEC and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string subOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        if(subOperand == "1"  ||  subOperand == "0x01"  ||  subOperand == "0x0001")
                                        {
                                            updateVasm(codeLine, firstMatch, "DEC", ldOperand);
                                        }
                                        // Not a decrement so bail
                                        else break;

                                        // Delete SUBI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"SUBI", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW SUB1 STW, replace all with DECW
                                    case LdwSub1Stw:
                                    {
                                        // Bail if wrong ROM version or if SUBI or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with DECW and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string subOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        if(subOperand == "1"  ||  subOperand == "0x01"  ||  subOperand == "0x0001")
                                        {
                                            updateVasm(codeLine, firstMatch, "DECW", ldwOperand);
                                        }
                                        // Not a decrement so bail
                                        else break;

                                        // Delete SUBI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"SUBI", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW SUBI ST, replace all with SUBBI
                                    case LdSubiSt:
                                    case LdwSubiSt:
                                    {
                                        // Bail if wrong ROM version or if SUBI or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with SUBBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string subOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "SUBBI", ldOperand + ", " + subOperand);

                                        // Delete SUBI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"SUBI", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD/LDW ANDI ST, replace all with ANDBI
                                    case LdAndiSt:
                                    case LdwAndiSt:
                                    {
                                        // Bail if wrong ROM version or if ANDI or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with ANDBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string andOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ANDBI", ldOperand + ", " + andOperand);

                                        // Delete ANDI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ANDI", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD/LDW ORI ST, replace all with ORBI
                                    case LdOriSt:
                                    case LdwOriSt:
                                    {
                                        // Bail if wrong ROM version or if ORI or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with ORBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string orOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ORBI", ldOperand + ", " + orOperand);

                                        // Delete ORI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ORI", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW ORI STW, replace all with ORBI
                                    case LdwOriStw:
                                    {
                                        // Bail if wrong ROM version or if ORI or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with ORBI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string orOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ORBI", ldwOperand + ", " + orOperand);

                                        // Delete ORI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ORI", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW XORI ST, replace all with XORBI
                                    case LdXoriSt:
                                    case LdwXoriSt:
                                    {
                                        // Bail if wrong ROM version or if XORI or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with XORBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string xorOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "XORBI", ldOperand + ", " + xorOperand);

                                        // Delete XORI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"XORI", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW XORI STW, replace all with XORBI
                                    case LdwXoriStw:
                                    {
                                        // Bail if wrong ROM version or if XORI or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with XORBI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string xorOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "XORBI", ldwOperand + ", " + xorOperand);

                                        // Delete XORI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"XORI", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW ANDI, replace all with ANDBK
                                    case LdAndi:
                                    case LdwAndi:
                                    {
                                        // Bail if wrong ROM version or if ANDI has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Replace LD/LDW with ANDBK and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string andOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ANDBK", ldOperand + ", " + andOperand);

                                        // Delete ANDI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ANDI"});
                                    }
                                    break;

                                    // Match LD ORI, replace all with ORBK
                                    case LdOri:
                                    {
                                        // Bail if wrong ROM version or if ORI has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Replace LD with ORBK and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string orOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ORBK", ldOperand + ", " + orOperand);

                                        // Delete ORI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ORI"});
                                    }
                                    break;

                                    // Match LD XORI, replace all with XORBK
                                    case LdXori:
                                    {
                                        // Bail if wrong ROM version or if XORI has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Replace LD with XORBK and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        std::string xorOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "XORBK", ldOperand + ", " + xorOperand);

                                        // Delete XORI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"XORI"});
                                    }
                                    break;

                                    // Match LDW LSLW STW, replace all with LSLV
                                    case LdwLslwStw:
                                    {
                                        // Bail if wrong ROM version or if LSLW or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with LSLV and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "LSLV", ldwOperand);

                                        // Delete LSLW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LSLW", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LDW LSRV STW, replace all with LSRV
                                    case LdwLsrvStw:
                                    {
                                        // Bail if wrong ROM version or if LSRV or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with LSRV and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "LSRV", ldwOperand);

                                        // Delete LSRV and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LSRV", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LDW LSRV LSRV STW, replace all with LSRV LSRV
                                    case LdwLsrv2Stw:
                                    {
                                        // Bail if wrong ROM version or if LSRV or LSRV or STW has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstMatch + 4 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with LSRV and new operand, replace first LSRV's operand with new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch,     "LSRV", ldwOperand);
                                        updateVasm(codeLine, firstMatch + 1, "LSRV", ldwOperand);

                                        // Delete second LSRV and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"LSRV", "STW"});

                                        if(insertLdw) insertVasm(codeLine, firstMatch, firstMatch + 2, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD LSRB ST, replace all with LSRB
                                    case LdLsrb1St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD with LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD LSRB LSRB ST, replace all with LSRB LSRB
                                    case LdLsrb2St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or LSRB or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 4 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD LSRB with LSRB LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch,     "LSRB", ldOperand);
                                        updateVasm(codeLine, firstMatch + 1, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 2, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD LSRB LSRB LSRB ST, replace all with LSRB LSRB LSRB
                                    case LdLsrb3St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or LSRB or LSRB or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 5 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 5]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD LSRB LSRB with LSRB LSRB LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch,     "LSRB", ldOperand);
                                        updateVasm(codeLine, firstMatch + 1, "LSRB", ldOperand);
                                        updateVasm(codeLine, firstMatch + 2, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 3);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 3, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 3, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD LSRB LSRB LSRB LSRB ST, replace all with LSRB LSRB LSRB LSRB
                                    case LdLsrb4St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or LSRB or LSRB or LSRB or ST has a label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 5]._labelIndex >= 0) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstMatch + 6 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 6]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD LSRB LSRB LSRB with LSRB LSRB LSRB LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch,     "LSRB", ldOperand);
                                        updateVasm(codeLine, firstMatch + 1, "LSRB", ldOperand);
                                        updateVasm(codeLine, firstMatch + 2, "LSRB", ldOperand);
                                        updateVasm(codeLine, firstMatch + 3, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 4);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 4, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(codeLine, firstMatch, firstMatch + 4, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDWI ADDW STW LDWI ADDW, delete 2nd LDWI ADDW
                                    case LdwiAddwStwLdwiAddw:
                                    {
                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._operand) break;

                                        // Delete 2nd LDWI and ADDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 3);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 3, {"LDWI", "ADDW"});
                                    }
                                    break;

                                    // Match LDWI ADDW ADDW STW LDWI ADDW ADDW, delete 2nd LDWI ADDW ADDW
                                    case LdwiAddw2StwLdwiAddw2:
                                    {
                                        // Bail if all ADDW operand's don't match
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 5]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 6]._operand) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 5]._operand) break;

                                        // Delete 2nd LDWI ADDW and ADDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 4);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 4, {"LDWI", "ADDW", "ADDW"});
                                    }
                                    break;

                                    // Match LDWI STW LDI POKE, replace with LDWI POKEI
                                    case LdwiStwLdiPoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace POKE with POKEI and LDI's operand, (matches on STW and POKE, that's why LDWI is at firstLine - 1)
                                        std::string ldiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "POKEI", ldiOperand);
                                
                                        // Delete STW and LDI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDI"});
                                    }
                                    break;

                                    // Match LDWI STW LDI DOKE, replace with LDWI DOKEI
                                    case LdwiStwLdiDoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Replace STW with DOKEI and LDI's operand, (matches on STW and DOKE, that's why first LDWI is at firstLine - 1)
                                        std::string ldiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "DOKEI", ldiOperand);

                                        // Delete LDI and DOKE
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDI", "DOKE"});
                                    }
                                    break;

                                    // Match LDWI STW LDWI DOKE, replace with LDWI DOKEI
                                    case LdwiStwLdwiDoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace STW with DOKEI and second LDWI's operand, (matches on STW and DOKE, that's why first LDWI is at firstLine - 1)
                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "DOKEI", ldwiOperand);

                                        // Delete LDWI and DOKE
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDWI", "DOKE"});
                                    }
                                    break;

                                    // Match STW LD/LDW POKE, replace with POKEA
                                    case StwLdPoke:
                                    case StwLdwPoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace POKE with POKEA and LD/LDW's operand
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "POKEA", ldOperand);

                                        // Delete STW and LD/LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW DOKE, replace with DOKEA
                                    case StwLdwDoke:
                                    case StwLdwDokeReg:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // If next instruction's operand equals LDW's operand, then bail, (this will fail if operand is referenced after next instruction)
                                        if(firstMatch + 3 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                        {
                                            std::string stwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                            std::string operand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._operand);
                                            if(operand == stwOperand) break;
                                        }

                                        // Replace DOKE with DOKEA and LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "DOKEA", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LDW INC POKE, replace with ADDI POKEA
                                    case StwLdwIncPoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace INC with ADDI, POKE with POKEA and LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "ADDI", "1");
                                        updateVasm(codeLine, firstMatch + 3, "POKEA", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match LD STW LDW/LDWI POKEA, replace with LD POKEA
                                    case LdStwLdwPokea:
                                    case LdStwLdwiPokea:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace POKEA's operand with LD's operand, (matches on STW and POKEA, that's why LD is at firstLine - 1)
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "POKEA", ldOperand);

                                        // Delete LD and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {"LD", "STW"});
                                    }
                                    break;

                                    // Match LD STW LDWI ADDI POKEA, replace with LD POKEA
                                    case LdStwLdwiAddiPokea:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Add ADDI's operand to LDWI's operand, update POKEA with LD's operand, (matches on STW and POKEA, that's why LD is at firstLine - 1)
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1]._operand;
                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        std::string addiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "LDWI", ldwiOperand + " + " + addiOperand);
                                        updateVasm(codeLine, firstMatch + 3, "POKEA", ldOperand);

                                        // Delete LD, STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LD", "STW", "LDWI"});
                                    }
                                    break;

                                    // Match LDWI ADDW PEEK NEGW STW LDWI ADDW POKEA
                                    case NegwArrayB:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 6]._operand) break;

                                        updateVasm(codeLine, firstMatch + 2, "STW", "memAddr");
                                        updateVasm(codeLine, firstMatch + 3, "PEEK", "");
                                        updateVasm(codeLine, firstMatch + 4, "NEGW", "giga_vAC");
                                        updateVasm(codeLine, firstMatch + 5, "POKE", "memAddr");

                                        // Delete ADDW and POKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 6);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 6, {"ADDW", "POKEA"});
                                    }
                                    break;

                                    // Match LDWI ADDW DEEK NEGW STW LDWI ADDW DOKEA
                                    case NegwArray:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 6]._operand) break;

                                        updateVasm(codeLine, firstMatch + 2, "STW", "memAddr");
                                        updateVasm(codeLine, firstMatch + 3, "DEEK", "");
                                        updateVasm(codeLine, firstMatch + 4, "NEGW", "giga_vAC");
                                        updateVasm(codeLine, firstMatch + 5, "DOKE", "memAddr");

                                        // Delete ADDW and DOKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 6);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 6, {"ADDW", "DOKEA"});
                                    }
                                    break;

                                    // MathArrayB, where Math = Addw, Subw, Andw, Orw, Xorw
                                    // Match LDWI ADDW PEEK STW LDWI ADDW PEEK MATH STW LDWI ADDW POKEA
                                    case AddwArrayB:
                                    case SubwArrayB:
                                    case AndwArrayB:
                                    case OrwArrayB:
                                    case XorwArrayB:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 10]._operand) break;

                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._operand;
                                        std::string addwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 5]._operand;
                                        std::string mathOpcode  = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 7]._opcode;

                                        updateVasm(codeLine, firstMatch + 2, "STW", "memAddr");
                                        updateVasm(codeLine, firstMatch + 3, "LDWI", ldwiOperand);
                                        updateVasm(codeLine, firstMatch + 4, "ADDW", addwOperand);
                                        updateVasm(codeLine, firstMatch + 5, "PEEKA", "memValue");
                                        updateVasm(codeLine, firstMatch + 6, "PEEKV", "memAddr");
                                        updateVasm(codeLine, firstMatch + 7, mathOpcode, "memValue");
                                        updateVasm(codeLine, firstMatch + 8, "POKE", "memAddr");

                                        // Delete LDWI ADDW and POKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 9);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 9, {"LDWI", "ADDW", "POKEA"});
                                    }
                                    break;

                                    // MathArray, where Math = Addw, Subw, Andw, Orw, Xorw
                                    // Match LDWI ADDW DEEK STW LDWI ADDW DEEK MATH STW LDWI ADDW DOKEA
                                    case AddwArray:
                                    case SubwArray:
                                    case AndwArray:
                                    case OrwArray:
                                    case XorwArray:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand != Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 10]._operand) break;

                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 4]._operand;
                                        std::string addwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 5]._operand;
                                        std::string mathOpcode  = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 7]._opcode;

                                        updateVasm(codeLine, firstMatch + 2, "STW", "memAddr");
                                        updateVasm(codeLine, firstMatch + 3, "LDWI", ldwiOperand);
                                        updateVasm(codeLine, firstMatch + 4, "ADDW", addwOperand);
                                        updateVasm(codeLine, firstMatch + 5, "DEEKA", "memValue");
                                        updateVasm(codeLine, firstMatch + 6, "DEEKV", "memAddr");
                                        updateVasm(codeLine, firstMatch + 7, mathOpcode, "memValue");
                                        updateVasm(codeLine, firstMatch + 8, "DOKE", "memAddr");

                                        // Delete LDWI ADDW and DOKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 9);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 9, {"LDWI", "ADDW", "DOKEA"});
                                    }
                                    break;

                                    // Match LD/LDW/LDWI STW MOVQB/MOVQW LDW STW, replace with MOVQB/MOVQW LD/LDW/LDWI STW
                                    case LdStwMovqbLdwStw:
                                    case LdwStwMovqbLdwStw:
                                    case LdwiStwMovqbLdwStw:
                                    case LdStwMovqwLdwStw:
                                    case LdwStwMovqwLdwStw:
                                    case LdwiStwMovqwLdwStw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if LD/LDW/LDWI or STW have labels, (starts at -1 as first match was on 1)
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._labelIndex >= 0) break;

                                        // Replace LDW with LD/LDW/LDWI and operand
                                        std::string ldOpcode  = Compiler::getCodeLines()[codeLine]._vasm[firstMatch -1]._opcode;
                                        std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch -1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, ldOpcode, ldOperand);

                                        // Delete LD/LDW/LDWI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {ldOpcode, "STW"});
                                    }
                                    break;

                                    // Match STW LDWI ADDW ADDW, replace with ARRW
                                    case StwLdwiAddwAddw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if LDWI, ADDW or ADDW have labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;

                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "ARRW", ldwiOperand);

                                        // Delete LDWI, ADDW, ADDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDWI", "ADDW", "ADDW"});
                                    }
                                    break;

                                    // Match LDWI ADDW ADDW, replace with ARRVW, (matches on ADDW and ADDW, that's why first LDWI is at firstLine - 1)
                                    case LdwiAddwAddw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ADDW or ADDW have labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1]._operand;
                                        std::string addwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand;
                                        updateVasm(codeLine, firstMatch - 1, "ARRVW", addwOperand + ", " + ldwiOperand);

                                        // Delete ADDW, ADDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 0);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 0, {"ADDW", "ADDW"});
                                    }
                                    break;

                                    // Match LDW ARRW, replace with ARRVW
                                    case LdwArrw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ARRW has a label
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand;
                                        std::string arrwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 0, "ARRVW", ldwOperand + ", " + arrwOperand);

                                        // Delete ARRW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ARRW"});
                                    }
                                    break;

                                    // Match STW STW ARRVW DOKEA, delete second STW, (matches on second STW and DOKEA, that's why first STW is at firstLine - 1)
                                    case StwStwArrvwDokea:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if second STW, ARRVW or DOKEA have labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        std::string stwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1]._operand;
                                        updateVasm(codeLine, firstMatch + 2, "DOKEA", stwOperand);

                                        // Delete second STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 0);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 0, {"STW"});
                                    }
                                    break;

                                    // Match STW ARRVW DOKEA, replace with STARRW, delete ARRVW DOKEA
                                    case StwArrvwDokea:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ARRVW or DOKEA have labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Update STW to STARRW
                                        std::string arrvwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch, "STARRW", arrvwOperand);

                                        // Delete ARRVW DOKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ARRVW", "DOKEA"});
                                    }
                                    break;

                                    // Match ARRVW DEEK, replace with LDARRW, delete DEEK
                                    case ArrvwDeek:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if DEEK has a label
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Update ARRVW to LDARRW
                                        std::string arrvwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand;
                                        updateVasm(codeLine, firstMatch, "LDARRW", arrvwOperand);

                                        // Delete DEEK
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEEK"});
                                    }
                                    break;

                                    // Match ARRVW DEEKA ARRVW DOKEA, replace with LDARRW STARRW, delete ARRVW and DOKEA, (matches on DEEKA and DOKEA, that's why first ARRW is at firstLine - 1)
                                    case AssignArray:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if DEEKA, second ARRVW or DOKEA have labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Update to LDARRW STARRW
                                        std::string arrvwOperand0 = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1]._operand;
                                        std::string arrvwOperand1 = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch - 1, "LDARRW", arrvwOperand0);
                                        updateVasm(codeLine, firstMatch + 0, "STARRW", arrvwOperand1);

                                        // Delete second ARRVW and DOKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ARRVW", "DOKEA"});
                                    }
                                    break;

                                    // Match STARRW LDARRW, delete LDARRW
                                    case StarrwLdarrw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if LDARRW has a label
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                        // Delete LDARRW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDARRW"});
                                    }
                                    break;

                                    // Match STW LDI POKEA, replace with ST using LDI's operand
                                    case StwLdiPokea:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if LDI or POKEA have labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                        // Update STW to POKE
                                        std::string ldiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                        updateVasm(codeLine, firstMatch + 0, "ST", ldiOperand);

                                        // Delete LDI POKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDI", "POKEA"});
                                    }
                                    break;

                                    // Match ADDW ADDW DEEK, replace with DEEKR
                                    case AddwAddwDeek:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Bail if second ADDW or DEEK have labels
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                        if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                
                                        // Replace first ADDW with DEEKR and operand
                                        std::string addwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                        updateVasm(codeLine, firstMatch, "DEEKR", addwOperand);
                                
                                        // Delete second ADDW and DEEK
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDW", "DEEK"});
                                    }
                                    break;

                                    default: break;
                                }
                            }

    /*************************************************************************************************************************************************************/
    /* Opcode matches required, operand matches NOT required                                                                                                     */
    /*************************************************************************************************************************************************************/
                            switch(matchIndex)
                            {
                                // Match MOVB ST, replace with MOVB
                                case MovbSt:
                                {
                                    // Bail if wrong ROM version or if ST has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Replace MOVB with MOVB and ST's operand
                                    std::string stOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch, "MOVB", "giga_vAC + 1, " + stOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ST"});
                                }
                                break;

                                // Match PEEK ST, replace with PEEKA
                                case PeekSt:
                                {
                                    // Bail if wrong ROM version or if ST has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Replace PEEK with PEEKA and ST's operand
                                    std::string stOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch, "PEEKA", stOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ST"});
                                }
                                break;

                                // Match LDW PEEK, replace with PEEKV
                                case PeekVar:
                                {
                                    // Bail if wrong ROM version or if PEEK has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                            
                                    // Replace LDW with PEEKV and new operand
                                    std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                    updateVasm(codeLine, firstMatch, "PEEKV", ldOperand);
                            
                                    // Delete PEEK
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"PEEK"});
                                }
                                break;

                                // Match DEEK STW, replace with DEEKA
                                case DeekStw:
                                {
                                    // Bail if wrong ROM version or if STW has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if next instruction is a ST/STW/PEEK/DEEK/POKE/DOKE/etc
                                    if(firstMatch + 2 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                    {
                                        std::string bailOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._opcode;
                                        if(bailOpcode.find("ST") != std::string::npos  ||  bailOpcode.find("EEK") != std::string::npos  ||  bailOpcode.find("OKE") != std::string::npos) break;
                                    }

                                    // Replace DEEK with DEEKA and STW's operand
                                    std::string stOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch, "DEEKA", stOperand);

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"STW"});
                                }
                                break;

                                // Match LDW DEEK, replace with DEEKV
                                case DeekVar:
                                {
                                    // Bail if wrong ROM version or if DEEK has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                            
                                    // Replace LDW with DEEKV and new operand
                                    std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                    updateVasm(codeLine, firstMatch, "DEEKV", ldOperand);
                            
                                    // Delete DEEK
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEEK"});
                                }
                                break;

                                // Match LDWI DEEK, replace with LDWM
                                case LdwiDeek:
                                {
                                    // Bail if wrong ROM version or if DEEK has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                            
                                    // Replace LDWI with LDWM and new operand
                                    std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                    updateVasm(codeLine, firstMatch, "LDWM", ldwiOperand);
                            
                                    // Delete DEEK
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEEK"});
                                }
                                break;

                                // Match LDW DEEK ADDBI
                                case LdwDeekAddbi:
                                {
                                    // Bail if wrong ROM version or if DEEK or ADDBI have a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                    // Check that LDW and ADDBI operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string ldwOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand);
                                    std::string addbiOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand);
                                    if(ldwOperand != addbiOperand) break;

                                    // Bail if ADDBI imm operand != 2
                                    uint8_t immValue = 0;
                                    std::string immOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand, 1);
                                    if(!Expression::stringToU8(immOperand, immValue)) break;
                                    if(immValue != 2) break;

                                    // Replace LDW with DEEKV+ and new operand
                                    updateVasm(codeLine, firstMatch, "DEEKV+", ldwOperand);

                                    // Delete DEEK, ADDBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"DEEK", "ADDBI"});
                                }
                                break;

                                // Match DEEKV ADDBI
                                case DeekvAddbi:
                                {
                                    // Bail if wrong ROM version or if ADDBI has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Check that DEEKV and ADDBI operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string deekvOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand);
                                    std::string addbiOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand);
                                    if(deekvOperand != addbiOperand) break;

                                    // Bail if ADDBI imm operand != 2
                                    uint8_t immValue = 0;
                                    std::string immOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    if(!Expression::stringToU8(immOperand, immValue)) break;
                                    if(immValue != 2) break;

                                    // Replace DEEKV with DEEKV+
                                    updateVasm(codeLine, firstMatch, "DEEKV+", deekvOperand);

                                    // Delete ADDBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDBI"});
                                }
                                break;

                                // Match DOKE ADDBI
                                case DokeAddbi:
                                {
                                    // Bail if wrong ROM version or if ADDBI has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Check that DOKE and ADDBI operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string dokeOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand);
                                    std::string addbiOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand);
                                    if(dokeOperand != addbiOperand) break;

                                    // Bail if ADDBI imm operand != 2
                                    uint8_t immValue = 0;
                                    std::string immOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    if(!Expression::stringToU8(immOperand, immValue)) break;
                                    if(immValue != 2) break;

                                    // Replace DOKE with DOKEV+
                                    updateVasm(codeLine, firstMatch, "DOKEV+", dokeOperand);

                                    // Delete ADDBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDBI"});
                                }
                                break;

                                // Match LD/LDW ST, replace with MOVB
                                case LdSt:
                                case LdwSt:
                                {
                                    // Bail if wrong ROM version or if ST has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if next instruction is a write
                                    if(firstMatch + 2 < int(Compiler::getCodeLines()[codeLine]._vasm.size()))
                                    {
                                        std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._opcode;
                                        if(stOpcode.find("ST") != std::string::npos  ||  stOpcode.find("OKE") != std::string::npos) break;
                                    }

                                    // Replace LD/LDW with MOVB and new operands
                                    std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                    std::string stOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch, "MOVB", ldOperand + ", " + stOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ST"});
                                }
                                break;

                                // Extra STW, (doesn't require an operand match)
                                case StwPair:
                                case StwPairReg:
                                case ExtraStw:
                                {
                                    // Migratel label to next available instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch, firstMatch + 1)) break;

                                    // Delete first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW"});
                                }
                                break;

                                // Match LDW STW LDWI ADDW PEEK end up with LDWI ADDW PEEK
                                case PeekArrayB:
                                {
                                    // Save previous line LDW, if opcode is not LDW then can't optimise
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLDW._opcode != "LDW") break;

                                    // Migrate it's label, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 1)) break;

                                    // Delete previous line LDW and first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);

                                    // Replace operand of ADDW with LDW's operand
                                    updateVasm(codeLine, firstMatch, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {"LDW", "STW"});
                                }
                                break;

                                // Match LDW STW LDWI ADDW ADDW PEEK/DEEK end up with LDWI ADDW ADDW PEEK/DEEK
                                case PeekArray:
                                case DeekArray:
                                {
                                    // Save previous line LDW, if opcode is not LDW then can't optimise
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLDW._opcode != "LDW") break;

                                    // Migrate it's label, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 1)) break;

                                    // Delete previous line LDW and first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's with LDW's operand
                                    updateVasm(codeLine, firstMatch, "ADDW", savedLDW._operand);
                                    updateVasm(codeLine, firstMatch + 1, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDWI STW LDW POKE/DOKE end up with LDWI STW LD<X> POKE/DOKE
                                case PokeArray:
                                case DokeArray:
                                {
                                    uint16_t offset = 9;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLD._opcode.find("LD") == std::string::npos) break;
                                    if(savedLD._opcode.find("LDWI") != std::string::npos) offset += 1;

                                    // Discard it's label, (it's no longer needed), and adjust it's address
                                    if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 1)) break;
                                    savedLD._labelIndex = -1;
                                    savedLD._address += offset; // LD<X> is moved 9bytes, LDWI is moved 10 bytes

                                    // Delete previous line LD<X>, first STW and LDW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm + 2);

                                    // Replace LDW with saved LD<X> and operand
                                    if(offset == 9)
                                    {
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.insert(itVasm, savedLD);
                                        adjustLabelAddresses(codeLine, firstMatch - 1, -4);
                                        adjustVasmAddresses(codeLine, firstMatch - 1, -4);
                                    }
                                    // LDW is replaced with LDWI so push everything forward starting at the POKE/DOKE by 1 more byte
                                    else
                                    {
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.insert(itVasm, savedLD);
                                        adjustLabelAddresses(codeLine, firstMatch - 1, -5);
                                        adjustVasmAddresses(codeLine, firstMatch - 1, -5);
                                        adjustLabelAddresses(codeLine, firstMatch + 2, 1);
                                        adjustVasmAddresses(codeLine, firstMatch + 2, 1);
                                    }
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW STW LDW POKE
                                case PokeVarArrayB:
                                case PokeTmpArrayB:
                                {
                                    uint16_t offset = 15;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        if(savedLD._opcode.find("LDWI") != std::string::npos) offset += 1;

                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 3)) break;
                                        savedLD._labelIndex = -1;
                                        savedLD._address += offset; // LD<X> is moved 15bytes, LDWI is moved 16 bytes

                                        // Delete previous line LD<X>, first STW and last LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm + 5); //points to last LDW

                                        // Replace LDW with saved LD<X> and operand
                                        if(offset == 15)
                                        {
                                            itVasm = Compiler::getCodeLines()[codeLine]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(codeLine, firstMatch - 1, -4);
                                            adjustVasmAddresses(codeLine, firstMatch - 1, -4);
                                        }
                                        // LDW is replaced with LDWI so push everything forward starting at the last LDW by 1 more byte
                                        else
                                        {
                                            itVasm = Compiler::getCodeLines()[codeLine]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(codeLine, firstMatch - 1, -5);
                                            adjustVasmAddresses(codeLine, firstMatch - 1, -5);
                                            adjustLabelAddresses(codeLine, firstMatch + 5, 1);
                                            adjustVasmAddresses(codeLine, firstMatch + 5, 1);
                                        }

                                        firstMatch = firstMatch - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstMatch = firstMatch + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[codeLine]._vasm[firstMatch];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);

                                    // Replace operand of ADDW with LDW's operand
                                    updateVasm(codeLine, firstMatch + 1, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LDW", "STW"});
                                }
                                break;

                                // Match STW LDW STW LDWI ADDW ADDW STW LDW POKE/DOKE
                                case PokeVarArray:
                                case PokeTmpArray:
                                case DokeVarArray:
                                case DokeTmpArray:
                                {
                                    uint16_t offset = 17;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        if(savedLD._opcode.find("LDWI") != std::string::npos) offset += 1;

                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 3)) break;
                                        savedLD._labelIndex = -1;
                                        savedLD._address += offset; // LD<X> is moved 17bytes, LDWI is moved 18 bytes

                                        // Delete previous line LD<X>, first STW and last LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm + 6); //points to last LDW, after LD<X> and STW were deleted

                                        // Replace LDW with saved LD<X> and operand
                                        if(offset == 17)
                                        {
                                            itVasm = Compiler::getCodeLines()[codeLine]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(codeLine, firstMatch - 1, -4);
                                            adjustVasmAddresses(codeLine, firstMatch - 1, -4);
                                        }
                                        // LDW is replaced with LDWI so push everything forward starting at the last LDW by 1 more byte
                                        else
                                        {
                                            itVasm = Compiler::getCodeLines()[codeLine]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(codeLine, firstMatch - 1, -5);
                                            adjustVasmAddresses(codeLine, firstMatch - 1, -5);
                                            adjustLabelAddresses(codeLine, firstMatch + 6, 1);
                                            adjustVasmAddresses(codeLine, firstMatch + 6, 1);
                                        }

                                        firstMatch = firstMatch - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstMatch = firstMatch + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[codeLine]._vasm[firstMatch];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's
                                    updateVasm(codeLine, firstMatch + 1, "ADDW", savedLDW._operand);
                                    updateVasm(codeLine, firstMatch + 2, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW POKEA
                                case PokeaVarArrayB:
                                case PokeaTmpArrayB:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 3)) break;
                                        savedLD._labelIndex = -1;

                                        // Operand is not a variable, so can't use POKEA, replace with POKEI
                                        if(savedLD._opcode == "LDI"  ||  savedLD._opcode == "LDWI")
                                        {
                                            updateVasm(codeLine, firstMatch+5, "POKEI", savedLD._operand);
                                        }
                                        // Replace POKEA's operand
                                        else
                                        {
                                            updateVasm(codeLine, firstMatch+5, "POKEA", savedLD._operand);
                                        }

                                        // Delete previous line LD<X> and first STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {savedLD._opcode, "STW"});

                                        firstMatch = firstMatch - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstMatch = firstMatch + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[codeLine]._vasm[firstMatch];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);

                                    // Replace operand of ADDW with LDW's operand
                                    updateVasm(codeLine, firstMatch + 1, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW ADDW POKEA
                                case PokeaVarArray:
                                case PokeaTmpArray:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 3)) break;
                                        savedLD._labelIndex = -1;

                                        // Operand is not a variable, so can't use POKEA, replace with POKEI
                                        if(savedLD._opcode == "LDI"  ||  savedLD._opcode == "LDWI")
                                        {
                                            updateVasm(codeLine, firstMatch+6, "POKEI", savedLD._operand);
                                        }
                                        // Replace POKEA's operand
                                        else
                                        {
                                            updateVasm(codeLine, firstMatch+6, "POKEA", savedLD._operand);
                                        }

                                        // Delete previous line LD<X> and first STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {savedLD._opcode, "STW"});

                                        firstMatch = firstMatch - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstMatch = firstMatch + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[codeLine]._vasm[firstMatch];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's
                                    updateVasm(codeLine, firstMatch + 1, "ADDW", savedLDW._operand);
                                    updateVasm(codeLine, firstMatch + 2, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW ADDW DOKEA
                                case DokeaVarArray:
                                case DokeaTmpArray:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstMatch - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[codeLine]._vasm[firstMatch - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migratelLabel(codeLine, firstMatch - 1, firstMatch + 3)) break;
                                        savedLD._labelIndex = -1;

                                        // Operand is not a variable, so can't use DOKEA, replace with DOKEI
                                        if(savedLD._opcode == "LDI"  ||  savedLD._opcode == "LDWI")
                                        {
                                            updateVasm(codeLine, firstMatch+6, "DOKEI", savedLD._operand);
                                        }
                                        // Replace DOKEA's operand
                                        else
                                        {
                                            updateVasm(codeLine, firstMatch+6, "DOKEA", savedLD._operand);
                                        }

                                        // Delete previous line LD<X> and first STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch - 1);
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(codeLine, firstMatch - 1, {savedLD._opcode, "STW"});

                                        firstMatch = firstMatch - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstMatch = firstMatch + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[codeLine]._vasm[firstMatch];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's
                                    updateVasm(codeLine, firstMatch + 1, "ADDW", savedLDW._operand);
                                    updateVasm(codeLine, firstMatch + 2, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LDW", "STW"});
                                }
                                break;

                                // Match MOVWA LDWI POKEA, update POKEA and delete MOVWA
                                case MovwaLdwiPokea:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail for labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                    // Migrate MOVWA's label if it has one
                                    if(!migratelLabel(codeLine, firstMatch + 0, firstMatch + 1)) break;

                                    // Check that MOVWA and POKEA operands are the same
                                    std::string movwaOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                    std::string movwaOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 1);
                                    std::string pokeaOperand  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand);
                                    if(movwaOperand1 != pokeaOperand) break;

                                    // Update POKEA
                                    updateVasm(codeLine, firstMatch + 2, "POKEA", movwaOperand0);

                                    // Delete MOVWA
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 0);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 0, {"MOVWA"});
                                }
                                break;

                                // Match MOVWA LDWI ADDW PEEKA, update ADDW and delete MOVWA
                                case MovwaLdwiAddwPeeka:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail for labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;

                                    // Migrate MOVWA's label if it has one
                                    if(!migratelLabel(codeLine, firstMatch + 0, firstMatch + 1)) break;

                                    // Check that MOVWA and ADDW operands are the same
                                    std::string movwaOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                    std::string movwaOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 1);
                                    std::string addwOperand   = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand);
                                    if(movwaOperand1 != addwOperand) break;

                                    // Update ADDW
                                    updateVasm(codeLine, firstMatch + 2, "ADDW", movwaOperand0);

                                    // Delete MOVWA
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 0);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 0, {"MOVWA"});
                                }
                                break;

                                // Match MOVWA LDARRB ST, update LDARRB with MOVA operand, delete MOVWA
                                case MovwaLdarrbSt:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail for labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                    // Migrate MOVWA's label if it has one
                                    if(!migratelLabel(codeLine, firstMatch + 0, firstMatch + 1)) break;

                                    // Check that MOVWA and LDARRB operands are the same
                                    std::string movwaOperand0  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                    std::string movwaOperand1  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 1);
                                    std::string ldarrbOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string ldarrbOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    if(movwaOperand1 != ldarrbOperand0) break;

                                    // Update LDARRB
                                    updateVasm(codeLine, firstMatch + 1, "LDARRB", movwaOperand0 + ", " + ldarrbOperand1);

                                    // Delete MOVWA
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 0);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 0, {"MOVWA"});
                                }
                                break;

                                // Match MOVQW LDWI ADDI POKEA
                                case MovqwLdwiAddiPokea:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // TODO: maybe add a flag to allow the optimiser to choose from different matching algorithms
                                    // Check that MOVQW and POKEA operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string movqwOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand);
                                    std::string pokeaOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._operand);
                                    if(movqwOperand != pokeaOperand) break;

                                    // Add ADDI's operand to LDWI's operand, update POKEI with MOVQW's immediate operand
                                    std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    std::string addiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand;
                                    std::string immOperand = Expression::getVasmOperand(movqwOperand, 1);
                                    updateVasm(codeLine, firstMatch + 2, "LDWI", ldwiOperand + " + " + addiOperand);
                                    updateVasm(codeLine, firstMatch + 3, "POKEI", immOperand);

                                    // Delete MOVQW and LDWI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"MOVQW", "LDWI"});
                                }
                                break;

                                // Match LDWI ADDW PEEK, update LDWI to LDARRB using LDWI and ADDW operands, delete ADDW and PEEK
                                case LdwiAddwPeek:
                                {
                                    // Bail if wrong ROM version, (shouldn't ever get here, but let's check anyway)
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail if ADDW or PEEK have labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                    // Update LDWI
                                    std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand;
                                    std::string addwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch + 0, "LDARRB", addwOperand + ", " + ldwiOperand);

                                    // Delete ADDW PEEK
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDW", "PEEK"});
                                }
                                break;

                                // Match LDWI ADDW PEEKA, update LDWI to LDARRB using LDWI and ADDW operands, update ADDW to ST using PEEKA operand, delete PEEKA
                                case LdwiAddwPeeka:
                                {
                                    // Bail if wrong ROM version, (shouldn't ever get here, but let's check anyway)
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail if ADDW or PEEKA have labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                    // Update LDWI
                                    std::string ldwiOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand;
                                    std::string addwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch + 0, "LDARRB", addwOperand + ", " + ldwiOperand);

                                    // Update ADDW
                                    std::string peekaOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand;
                                    updateVasm(codeLine, firstMatch + 1, "ST", peekaOperand);

                                    // Delete PEEKA
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"PEEKA"});
                                }
                                break;

                                // Match STW LDWI ADDW POKEA, update STW to STARRB using LDWI and ADDW operands, delete LDWI ADDW and POKEA
                                case StwLdwiAddwPokea:
                                {
                                    // Bail if wrong ROM version, (shouldn't ever get here, but let's check anyway)
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail if LDWI ADDW or POKEA have labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._labelIndex >= 0) break;

                                    // Check STW and POKEA operands match
                                    std::string stwOperand   = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                    std::string pokeaOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 3]._operand, 0);
                                    if(stwOperand != pokeaOperand) break;

                                    // Update STW
                                    std::string ldwiOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string addwOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand, 0);
                                    updateVasm(codeLine, firstMatch + 0, "STARRB", addwOperand + ", " + ldwiOperand);

                                    // Delete LDWI ADDW POKEA
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDWI", "ADDW", "POKEA"});
                                }
                                break;

                                // Match LDWI ADDW POKEA, update LDWI to LDW using POKEA operand, update ADDW to STARRB using LDWI and ADDW operands, delete POKEA
                                case LdwiAddwPokea:
                                {
                                    // Bail if wrong ROM version, (shouldn't ever get here, but let's check anyway)
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail if ADDW or POKEA have labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                    // Update LDWI and ADDW
                                    std::string ldwiOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                    std::string addwOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string pokeaOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand, 0);
                                    updateVasm(codeLine, firstMatch + 0, "LDW", pokeaOperand);
                                    updateVasm(codeLine, firstMatch + 1, "STARRB", addwOperand + ", " + ldwiOperand);

                                    // Delete POKEA
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 2);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 2, {"POKEA"});
                                }
                                break;

                                // Match LDWI ADDW POKEI, update LDWI to STARRI using LDWI, ADDW and POKEI operands, delete ADDW and POKEI
                                case LdwiAddwPokei:
                                {
                                    // Bail if wrong ROM version, (shouldn't ever get here, but let's check anyway)
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail if ADDW or POKEI have labels
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._labelIndex >= 0) break;

                                    // Update LDWI
                                    std::string ldwiOperand  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand, 0);
                                    std::string addwOperand  = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string pokeiOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._operand, 0);
                                    updateVasm(codeLine, firstMatch + 0, "STARRI", addwOperand + ", " + ldwiOperand + ", " + pokeiOperand);

                                    // Delete ADDW POKEI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDW", "POKEI"});
                                }
                                break;

                                // Match STW MOVB, (generated by LSL 8)
                                case StwMovb:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Migratel label to next available instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch, firstMatch + 1)) break;

                                    // Replace MOVB operands
                                    updateVasm(codeLine, firstMatch + 1, "MOVB", "giga_vAC, giga_vAC + 1");

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW"});
                                }
                                break;

                                case StwPokeArray:
                                case StwDokeArray:
                                {
                                    // Migratel label to next available instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch, firstMatch + 1)) break;

                                    // Delete first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"STW"});
                                }
                                break;

                                // LdiSt, (replace with MOVQB in ROMvX0)
                                case LdiSt:
                                {
                                    // Bail if wrong ROM version or if ST has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if next instruction is a write
                                    if(Compiler::getCodeLines()[codeLine]._vasm.size() > firstMatch + 2)
                                    {
                                        std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._opcode;
                                        if(stOpcode.find("ST") != std::string::npos  ||  stOpcode.find("OKE") != std::string::npos) break;
                                    }

                                    // Replace LDI with MOVQB and new operands
                                    std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                    std::string stOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    if(stOperand == "romUser") break; // RomCheck code cannot use new instructions
                                    updateVasm(codeLine, firstMatch, "MOVQB", stOperand + ", " + ldOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ST"});
                                }
                                break;

                                // LdiStw, (replace with MOVQW in ROMvX0)
                                case LdiStw:
                                {
                                    // Bail if wrong ROM version or if STW has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if next instruction is a write
                                    if(Compiler::getCodeLines()[codeLine]._vasm.size() > firstMatch + 2)
                                    {
                                        std::string stOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 2]._opcode;
                                        if(stOpcode.find("ST") != std::string::npos  ||  stOpcode.find("OKE") != std::string::npos) break;
                                    }

                                    // Replace LDI with MOVQW and new operands
                                    std::string ldOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand;
                                    std::string stOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    if(stOperand == "romUser") break; // RomCheck code cannot use new instructions
                                    updateVasm(codeLine, firstMatch, "MOVQW", stOperand + ", " + ldOperand);

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"STW"});
                                }
                                break;

                                // Match LD/LDW STW LD STW LDW SUBW
                                case LdSubLoHi:
                                case LdiSubLoHi:
                                case LdwSubLoHi:
                                {
                                    // Save LD<X>
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[codeLine]._vasm[firstMatch];

                                    // Migrate LD<X>'s label to LD
                                    if(!migratelLabel(codeLine, firstMatch, firstMatch + 2)) break;
                                    savedLD._labelIndex = -1;
                                    savedLD._address += 8; // LD<X> is moved 8 bytes

                                    // Delete LD<X>, first STW and last LDW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm);
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(itVasm + 2); // points to last LDW, after LD<X> and STW were deleted

                                    // Replace LDW with saved LD<X> and operand
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.insert(itVasm, savedLD);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"LD", "STW"});
                                }
                                break;

                                // Match LDI ADDI
                                case LdiAddi:
                                {
                                    uint8_t ldi, addi;

                                    // Migrate ADDI's label to next instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, addi);

                                    // Result is either an LDI or LDWI
                                    int16_t result = ldi + addi;
                                    if(result >= 0  &&  result <= 255)
                                    {
                                        updateVasm(codeLine, firstMatch, "LDI", std::to_string(uint8_t(result)));
                                    }
                                    else
                                    {
                                        updateVasm(codeLine, firstMatch, "LDWI", std::to_string(result));
                                    }

                                    // Delete ADDI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDI"});
                                }
                                break;

                                // Match LDI SUBI
                                case LdiSubi:
                                {
                                    uint8_t ldi, subi;

                                    // Migrate SUBI's label to next instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, subi);

                                    // Result is either an LDI or LDWI
                                    int16_t result = ldi - subi;
                                    if(result >= 0  &&  result <= 255)
                                    {
                                        updateVasm(codeLine, firstMatch, "LDI", std::to_string(uint8_t(result)));
                                    }
                                    else
                                    {
                                        updateVasm(codeLine, firstMatch, "LDWI", std::to_string(result));
                                    }

                                    // Delete SUBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"SUBI"});
                                }
                                break;

                                // Match LDI ANDI
                                case LdiAndi:
                                {
                                    uint8_t ldi, andi;

                                    // Migrate ANDI's label to next instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, andi);

                                    // Result is an LDI
                                    uint8_t result = ldi & andi;
                                    updateVasm(codeLine, firstMatch, "LDI", Expression::byteToHexString(result));

                                    // Delete ANDI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ANDI"});
                                }
                                break;

                                // Match LDI ORI
                                case LdiOri:
                                {
                                    uint8_t ldi, ori;

                                    // Migrate ORI's label to next instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, ori);

                                    // Result is an LDI
                                    uint8_t result = ldi | ori;
                                    updateVasm(codeLine, firstMatch, "LDI", Expression::byteToHexString(result));

                                    // Delete ORI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ORI"});
                                }
                                break;

                                // Match LDI XORI
                                case LdiXori:
                                {
                                    uint8_t ldi, xori;

                                    // Migrate XORI's label to next instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, xori);

                                    // Result is an LDI
                                    uint8_t result = ldi ^ xori;
                                    updateVasm(codeLine, firstMatch, "LDI", Expression::byteToHexString(result));

                                    // Delete XORI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"XORI"});
                                }
                                break;

                                // Match ADDI ADDI
                                case AddiPair:
                                {
                                    uint8_t addi0, addi1;

                                    // Migrate second ADDI's label to next instruction, (if it exists)
                                    if(!migratelLabel(codeLine, firstMatch + 1, firstMatch + 2)) break;

                                    // Add operands together, replace first operand, delete second opcode and operand
                                    Compiler::VasmLine vasm = Compiler::getCodeLines()[codeLine]._vasm[firstMatch];
                                    std::string operand = vasm._operand;
                                    Expression::stringToU8(operand, addi0);
                                    vasm = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1];
                                    operand = vasm._operand;
                                    Expression::stringToU8(operand, addi1);

                                    // Result can't fit in an ADDI operand so exit, (ADDI can't be -ve so result can't be -ve)
                                    uint16_t result = addi0 + addi1;
                                    if(result > 255) break;

                                    // Delete second ADDI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);

                                    // Replace first ADDI's operand with sum of both ADDI's operands
                                    updateVasm(codeLine, firstMatch, "ADDI", std::to_string(uint8_t(result)));
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"ADDI"});
                                }
                                break;

                                // Match LDW STW, replace with MOVWA
                                case LdwStw:
                                {
                                    // Bail if wrong ROM version or if STW has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Replace LDW with MOVWA and operand
                                    std::string ldwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._operand;
                                    std::string stwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch + 0, "MOVWA", ldwOperand + ", " + stwOperand);

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"STW"});
                                }
                                break;

                                // Match TCC STW, replace TCC operand with STW operand, delete STW
                                case TeqStw:
                                case TneStw:
                                case TltStw:
                                case TgtStw:
                                case TleStw:
                                case TgeStw:
                                {
                                    // Bail if wrong ROM version, (shouldn't ever get here, but let's check anyway)
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail if STW has a label
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Update TCC
                                    std::string tccOpcode  = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 0]._opcode;
                                    std::string stwOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    updateVasm(codeLine, firstMatch + 0, "T" + tccOpcode.substr(1), stwOperand);

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"STW"});
                                }
                                break;

                                // Match TCC %JumpFalse
                                case TeqJump:
                                case TneJump:
                                case TltJump:
                                case TgtJump:
                                case TleJump:
                                case TgeJump:
                                {
                                    // Bail if wrong ROM version, (shouldn't ever get here, but let's check anyway)
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Bail if next instruction doesn't exist
                                    if(firstMatch + 1 >= int(Compiler::getCodeLines()[codeLine]._vasm.size())) break;

                                    // Bail if next instruction is not a %Jump macro
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._opcode.substr(0, 10) != "%JumpFalse") break;

                                    // Replace %JumpFalse with JCC and label
                                    std::string tccOpcode = Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._opcode;
                                    std::string labOperand = Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand;
                                    if(!invertCC(tccOpcode)) break;
                                    updateVasm(codeLine, firstMatch, "J" + tccOpcode.substr(1), labOperand);

                                    // Delete %JumpFalse
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"%JumpFalse"});
                                }
                                break;

                                // Match TEQ CONDII, swap CONDII operands, delete TEQ
                                case TeqCondii:
                                {
                                    // Bail if wrong ROM version or if TEQ has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._labelIndex >= 0) break;

                                    // Bail if TEQ operand is not giga_vAC
                                    std::string teqOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    if(teqOperand != "giga_vAC") break;

                                    // Update CONDII
                                    std::string condOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string condOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    updateVasm(codeLine, firstMatch + 1, "CONDII", condOperand1 + ", " + condOperand0);

                                    // Delete TEQ
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"TEQ"});
                                }
                                break;

                                // Match TEQ CONDIB, swap CONDIB with CONDBI and swap it's operands, delete TEQ
                                case TeqCondib:
                                {
                                    // Bail if wrong ROM version or if TEQ has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._labelIndex >= 0) break;

                                    // Bail if TEQ operand is not giga_vAC
                                    std::string teqOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    if(teqOperand != "giga_vAC") break;

                                    // Update CONDIB
                                    std::string condOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string condOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    updateVasm(codeLine, firstMatch + 1, "CONDBI", condOperand1 + ", " + condOperand0);

                                    // Delete TEQ
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"TEQ"});
                                }
                                break;

                                // Match TEQ CONDBI, swap CONDBI with CONDIB and swap it's operands, delete TEQ
                                case TeqCondbi:
                                {
                                    // Bail if wrong ROM version or if TEQ has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._labelIndex >= 0) break;

                                    // Bail if TEQ operand is not giga_vAC
                                    std::string teqOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    if(teqOperand != "giga_vAC") break;

                                    // Update CONDBI
                                    std::string condOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string condOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    updateVasm(codeLine, firstMatch + 1, "CONDIB", condOperand1 + ", " + condOperand0);

                                    // Delete TEQ
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"TEQ"});
                                }
                                break;

                                // Match TEQ CONDBB, swap CONDBB operands, delete TEQ
                                case TeqCondbb:
                                {
                                    // Bail if wrong ROM version or if TEQ has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._labelIndex >= 0) break;

                                    // Bail if TEQ operand is not giga_vAC
                                    std::string teqOperand = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    if(teqOperand != "giga_vAC") break;

                                    // Update CONDBB
                                    std::string condOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string condOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    updateVasm(codeLine, firstMatch + 1, "CONDBB", condOperand1 + ", " + condOperand0);

                                    // Delete TEQ
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch, {"TEQ"});
                                }
                                break;

                                // Match MOVB MOVB, replace with MOVWA, (TODO: this can fail if vAC must remain valid, compiler should not produce code that requires vAC to be valid, fix is to use MOVW instead)
                                case MovbMovb0:
                                {
                                    // Bail if wrong ROM version or if second MOVB has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if operands don't match corectly
                                    std::string movbOperand00 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    std::string movbOperand01 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 1);
                                    std::string movbOperand10 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string movbOperand11 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    if(movbOperand10.find(movbOperand00) == std::string::npos) break;
                                    if(movbOperand11.find(movbOperand01) == std::string::npos) break;
                                    if(movbOperand10.find("+1") == std::string::npos  &&  movbOperand10.find("+ 1") == std::string::npos) break;
                                    if(movbOperand11.find("+1") == std::string::npos  &&  movbOperand11.find("+ 1") == std::string::npos) break;

                                    // Replace first MOVB with MOVWA
                                    updateVasm(codeLine, firstMatch, "MOVWA", movbOperand00 + ", " + movbOperand01);

                                    // Delete second MOVB
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"MOVB"});
                                }
                                break;

                                // Match MOVB MOVB, replace with PACKAW
                                case MovbMovb1:
                                {
                                    // Bail if wrong ROM version or if second MOVB has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if operands don't match corectly
                                    std::string movbOperand00 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    std::string movbOperand01 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 1);
                                    std::string movbOperand10 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string movbOperand11 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    if(movbOperand01 != "giga_vAC"  ||  (movbOperand11 != "giga_vAC+1"  &&  movbOperand11 != "giga_vAC + 1")) break;

                                    // Replace first MOVB with PACKAW
                                    updateVasm(codeLine, firstMatch, "PACKAW", movbOperand00 + ", " + movbOperand10);

                                    // Delete second MOVB
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"MOVB"});
                                }
                                break;

                                // Match MOVB MOVB, replace with PACKVW
                                case MovbMovb2:
                                {
                                    // Bail if wrong ROM version or if second MOVB has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if operands don't match corectly
                                    std::string movbOperand00 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    std::string movbOperand01 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 1);
                                    std::string movbOperand10 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    std::string movbOperand11 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 1);
                                    if(movbOperand01 == "giga_vAC"  ||  movbOperand11.find(movbOperand01) == std::string::npos  ||  
                                       (movbOperand11.find("+1") == std::string::npos  &&  movbOperand11.find(" + 1") == std::string::npos)) break;

                                    // Replace first MOVB with PACKVW
                                    updateVasm(codeLine, firstMatch, "PACKVW", movbOperand00 + ", " + movbOperand10 + ", " + movbOperand01);

                                    // Delete second MOVB
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"MOVB"});
                                }
                                break;

                                // Match PACKVW LDW, replace with PACKAW and delete LDW
                                case PackvwLdw:
                                {
                                    // Bail if wrong ROM version or if second MOVB has a label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._labelIndex >= 0) break;

                                    // Bail if operands don't match corectly
                                    std::string pckOperand00 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 0);
                                    std::string pckOperand01 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 1);
                                    std::string pckOperand02 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch]._operand, 2);
                                    std::string ldwOperand   = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[firstMatch + 1]._operand, 0);
                                    if(pckOperand02 != ldwOperand) break;

                                    // Replace PACKVW with PACKAW
                                    updateVasm(codeLine, firstMatch, "PACKAW", pckOperand00 + ", " + pckOperand01);

                                    // Delete LDW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + firstMatch + 1);
                                    adjustLabelAndVasmAddresses(codeLine, firstMatch + 1, {"LDW"});
                                }
                                break;

                                default: break;
                            }
                        }

                        // Match MOVWA LDW STARRB in any vasm sequence, MOVWA and LDW operands must match, STARRB must directly follow LDW, update LDW operand with MOVWA operand, delete MOVWA
                        if(matchIndex == MovwaStarrb)
                        {
                            enum MovwaStarrbStates {MovwaSearch, StarrbSearch};
                            static MovwaStarrbStates movwaStarrbState = MovwaSearch;
                            static int prevCodeLine = -1;
                            static int movwaIndex = -1;

                            // Bail if wrong ROM version
                            if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                            // Restart for each new line of BASIC code
                            if(codeLine != prevCodeLine)
                            {
                                movwaStarrbState = MovwaSearch;
                                prevCodeLine = codeLine;
                            }

                            switch(movwaStarrbState)
                            {
                                case MovwaSearch:
                                {
                                    movwaIndex = -1;

                                    // Match MOVWA
                                    size_t pos = itVasm->_code.find(matchSequences[matchIndex]._sequence[0]);
                                    if(pos != std::string::npos)
                                    {
                                        movwaStarrbState = StarrbSearch;
                                        movwaIndex = vasmIndex;
                                    }
                                }
                                break;

                                case StarrbSearch:
                                {
                                    // Match Starrb
                                    size_t pos = itVasm->_code.find(matchSequences[matchIndex]._sequence[1]);
                                    if(pos != std::string::npos  &&  movwaIndex >= 0)
                                    {
                                        // Previous VASM instruction must be LDW
                                        int starrbIndex = vasmIndex;
                                        if(starrbIndex > 0)
                                        {
                                            int ldwIndex = starrbIndex - 1;
                                            if(Compiler::getCodeLines()[codeLine]._vasm[ldwIndex]._opcode == "LDW")
                                            {
                                                // Operands must match
                                                std::string movwaOperand0 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[movwaIndex]._operand, 0);
                                                std::string movwaOperand1 = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[movwaIndex]._operand, 1);
                                                std::string ldw_operand    = Expression::getVasmOperand(Compiler::getCodeLines()[codeLine]._vasm[ldwIndex]._operand, 0);
                                                if(movwaOperand1 == ldw_operand)
                                                {
                                                    // Update LDW with MOVWA operand
                                                    updateVasm(codeLine, ldwIndex, "LDW", movwaOperand0);

                                                    // Delete MOVWA
                                                    linesDeleted = true;
                                                    itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + movwaIndex);
                                                    adjustLabelAndVasmAddresses(codeLine, movwaIndex, {"MOVWA"});
                                                }
                                            }
                                        }                                        
                                    }
                                    // Continuing searching for STARRB on current code line
                                    else
                                    {
                                        break;
                                    }

                                    // Reset search
                                    movwaStarrbState = MovwaSearch;
                                }
                                break;

                                default: break;
                            }
                        }

                        // Perform their own opcode and/or operand matching
                        switch(matchIndex)
                        {
                            case AddiZero:
                            case SubiZero:
                            {
                                // Match ADDI/SUBI
                                size_t pos = itVasm->_code.find(matchSequences[matchIndex]._sequence[0]);
                                if(pos != std::string::npos)
                                {
                                    std::string operand = itVasm->_code.substr(pos + matchSequences[matchIndex]._sequence[0].size());
                                    if(operand == "0" || operand == "0x00")
                                    {
                                        // Migratel label to next available instruction, (if it exists)
                                        if(!migratelLabel(codeLine, vasmIndex, vasmIndex + 1)) break;

                                        // Delete ADDI/SUBI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[codeLine]._vasm.erase(Compiler::getCodeLines()[codeLine]._vasm.begin() + vasmIndex);
                                        adjustLabelAndVasmAddresses(codeLine, vasmIndex, {"ADDI"});
                                    }
                                }
                            }
                            break;

                            case LdwiNeg:
                            {
                                // Bail if wrong ROM version
                                if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                // Match LDWI
                                size_t pos = itVasm->_code.find(matchSequences[matchIndex]._sequence[0]);
                                if(pos != std::string::npos)
                                {
                                    int16_t ldwi;
                                    std::string operand = itVasm->_code.substr(pos + matchSequences[matchIndex]._sequence[0].size());
                                    if(Expression::stringToI16(operand, ldwi))
                                    {
                                        if(ldwi > -256  &&  ldwi < 0)
                                        {
                                            updateVasm(codeLine, vasmIndex, "LDNI", std::to_string(uint8_t(abs(ldwi))));
                                        }
                                    }
                                }
                            }
                            break;

                            case LdwiSml:
                            {
                                // Match LDWI
                                size_t pos = itVasm->_code.find(matchSequences[matchIndex]._sequence[0]);
                                if(pos != std::string::npos)
                                {
                                    int16_t ldwi;
                                    std::string operand = itVasm->_code.substr(pos + matchSequences[matchIndex]._sequence[0].size());
                                    if(Expression::stringToI16(operand, ldwi))
                                    {
                                        if(ldwi >= 0  &&  ldwi <= 255)
                                        {
                                            updateVasm(codeLine, vasmIndex, "LDI", std::to_string(uint8_t(ldwi)));
                                        }
                                    }
                                }
                            }
                            break;

                            default: break;
                        }

                        if(!linesDeleted)
                        {
                            // Only increment iterator if it has not been invalidated
                            ++itVasm;
                        }
                        else
                        {
                            optimised = true;
                        }
                    }
                }
            }

            numOptimiserPasses++;
        }
        // Successful optimisation can cause new optimising opportunities to present themselves, so restart
        while(optimised);

        Cpu::reportError(Cpu::NoError, stderr, "\n*******************************************************\n");
        Cpu::reportError(Cpu::NoError, stderr, "* Optimiser performed %d passes\n", numOptimiserPasses);
        Cpu::reportError(Cpu::NoError, stderr, "*******************************************************\n");

        return true;
    }
}