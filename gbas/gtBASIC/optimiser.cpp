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
    enum OptimiseTypes {StStw=0, StwLdwPair, StwLdPair, StwStHigh, ExtraLdw, ExtraLd, LdwPair, LdwStwLdwStw, StwLdiAddw, StwLdAddw, StwLdAddwVar, StwLdwAddw, StwLdwAddwVar, StwLdiAndw, StwLdAndw, StwLdAndwVar, StwLdwAndw,
                        StwLdwAndwVar, StwLdiXorw, StwLdXorw, StwLdXorwVar, StwLdwXorw, StwLdwXorwVar, StwLdiOrw, StwLdOrw, StwLdOrwVar, StwLdwOrw, StwLdwOrwVar, LdPxSt, LdStPx, LdwStPx, StwLdwiAddw, StwLdwiSubw, StwLdwiAndw,
                        StwLdwiXorw, StwLdwiOrw, LdwBeqStwTmp, LdwBeqStwVar, LdBeqStTmp, LdBeqStVar, Inc4Var, Inc3Var, Inc2Var, Dec4Var, Dec3Var, Dec2Var, LdwAddiStw, LdwAddwStw, LdAddiSt, LdwAddiSt, LdwSubiStw, LdwSubwStw,
                        IncwLdw, DecwLdw, LdStwLdSubw, LdStwLdwSubw, LdSub1St, LdwSub1Stw, LdSubiSt, LdwSubiSt, LdAndiSt, LdwAndiSt, LdOriSt, LdwOriSt, LdwOriStw, LdXoriSt, LdwXoriSt, LdwXoriStw, LdAndi, LdwAndi, LdOri, LdXori,
                        LdwLslwStw, LdwLsrvStw, LdwLsrv2Stw, LdLsrb1St, LdLsrb2St, LdLsrb3St, LdLsrb4St, PokeVar, DokeVar, LdwPeekInc, PeekvInc, PokeInc, LdwDeekIncInc, DeekvIncInc, DokeIncInc, LdSubiBcc, LdwSubiBcc, LdXoriBcc,
                        LdwXoriBcc, LdwNotwStw, LdwNegwStw, LdNotwSt, LdwNotwSt, LdwiAddwStwLdwiAddw, LdwiAddw2StwLdwiAddw2, LdwiStwLdiPoke, LdwiStwLdiDoke, LdwiStwLdwiDoke, StwLdPoke, StwLdwPoke, StwLdwDoke, StwLdwDokeReg,
                        StwLdwIncPoke, LdStwLdwPokea, LdStwLdwiPokea, LdStwLdwiAddiPokea, NegwArrayB, NegwArray, AddwArrayB, AddwArray, SubwArrayB, SubwArray, AndwArrayB, AndwArray, OrwArrayB, OrwArray, XorwArrayB, XorwArray,
                        LdStwMovqbLdwStw, LdwStwMovqbLdwStw, LdwiStwMovqbLdwStw, LdStwMovqwLdwStw, LdwStwMovqwLdwStw, LdwiStwMovqwLdwStw, MovbSt, PeekSt, PeekVar, DeekStw, DeekVar, LdwDeekAddbi, DeekvAddbi, DokeAddbi, LdSt, LdwSt,
                        StwPair, StwPairReg, ExtraStw, PeekArrayB, PeekArray, DeekArray, PokeArray, DokeArray, PokeVarArrayB, PokeVarArray, DokeVarArray, PokeTmpArrayB, PokeTmpArray, DokeTmpArray, PokeaVarArrayB, PokeaVarArray,
                        DokeaVarArray, PokeaTmpArrayB, PokeaTmpArray, DokeaTmpArray, MovqwLdwiAddiPokea, StwMovb, StwPokeArray, StwDokeArray, LdiSt, LdiStw, LdSubLoHi, LdiSubLoHi, LdwSubLoHi, LdiAddi, LdiSubi, LdiAndi, LdiOri,
                        LdiXori, AddiPair, TeqJump, TneJump, TltJump, TgtJump, TleJump, TgeJump, AddiZero, SubiZero, LdwiNeg, NumOptimiseTypes};

    struct MatchSequence
    {
        int _firstIndex;
        int _secondIndex;
        std::vector<std::string> _sequence;
    };

    std::vector<MatchSequence> matchSequences = 
    {
        // StStw
        {0, 0, {Expression::createPaddedString("ST",  OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_" }},

        // StwLdwPair
        {0, 1, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x" }},

        // StwLdPair
        {0, 1, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "0x" }},

        // StwStHigh
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + "_" }},

        // ExtraLdw
        {0, 1, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_"  }},

        // ExtraLd
        {0, 1, {Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_"  }},

        // LdwPair
        {0, 1, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x"  }},

        // LdwStwLdwStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // StwLdiAddw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAddw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAddwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAddw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAddwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdiAndw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAndw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdAndwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAndw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwAndwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdiXorw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdXorw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdXorwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwXorw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwXorwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdiOrw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdOrw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdOrwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwOrw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwOrwVar
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdPxSt
        {0, 0, {Expression::createPaddedString("LDPX", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // LdStPx,
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STPX", OPCODE_TRUNC_SIZE, ' ') + "" }},

        // LdwStPx,
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STPX", OPCODE_TRUNC_SIZE, ' ') + "" }},

        // StwLdwiAddw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiSubw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiAndw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiXorw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwLdwiOrw
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdwBeqStwTmp
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdwBeqStwVar
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // LdBeqStTmp
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // LdBeqStVar
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("BEQ", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + "_"}},

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
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAddwStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdAddiSt
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAddiSt
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubiStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubwStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // IncwLdw
        {0, 1, {Expression::createPaddedString("INCW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DecwLdw
        {0, 1, {Expression::createPaddedString("DECW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdStwLdSubw
        {1, 3, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdStwLdwSubw
        {1, 3, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_", 
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSub1St
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSub1Stw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSubiSt
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubiSt
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdAndiSt
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAndiSt
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdOriSt
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwOriSt
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwOriStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdXoriSt
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwXoriSt
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwXoriStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdAndi
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwAndi
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdOri
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdXori
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwLslwStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("LSLW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwLsrvStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwLsrv2Stw
        {0, 3, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb1St
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb2St
        {0, 3, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb3St
        {0, 4, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdLsrb4St
        {0, 5, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LSRB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PokeVar
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DokeVar
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwPeekInc
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PeekvInc
        {0, 1, {Expression::createPaddedString("PEEKV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PokeInc
        {0, 1, {Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwDeekIncInc
        {0, 3, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekvIncInc
        {0, 2, {Expression::createPaddedString("DEEKV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DokeIncInc
        {0, 2, {Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSubiBcc
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSubiBcc
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdXoriBcc
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwXoriBcc
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwNotwStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NOTW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwNegwStw
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NEGW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdNotwSt
        {0, 2, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NOTW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwNotwSt
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NOTW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddwStwLdwiAddw
        {0, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiAddw2StwLdwiAddw2
        {0, 4, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiStwLdiPoke
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdwiStwLdiDoke
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdwiStwLdwiDoke
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // StwLdPoke
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwLdwPoke
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwLdwDoke
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwLdwDokeReg
        {0, 2, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // StwLdwIncPoke
        {0, 3, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("INC", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwLdwPokea
        {1, 3, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwLdwiPokea
        {1, 3, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwLdwiAddiPokea
        {1, 4, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // NegwArrayB
        {0, 5, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NEGW", OPCODE_TRUNC_SIZE, ' ') + "giga_vAC",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // NegwArray
        {0, 5, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("NEGW", OPCODE_TRUNC_SIZE, ' ') + "giga_vAC",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AddwArrayB
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AddwArray
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // SubwArrayB
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // SubwArray
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AndwArrayB
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // AndwArray
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // OrwArrayB
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // OrwArray
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // XorwArrayB
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // XorwArray
        {0, 9, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdStwMovqbLdwStw
        {1, 3, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwStwMovqbLdwStw
        {1, 3, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiStwMovqbLdwStw
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQB", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdStwMovqwLdwStw
        {1, 3, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwStwMovqwLdwStw
        {1, 3, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwiStwMovqwLdwStw
        {1, 3, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        /******************************************************************************************/
        /* Operands are not matched from here on in
        /******************************************************************************************/

        // MovbSt
        {0, 0, {Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "giga_vAC + 1, giga_vAC",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PeekSt
        {0, 0, {Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // PeekVar
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekStw
        {0, 0, {Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "_"}},

        // DeekVar
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwDeekAddbi
        {0, 2, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekvAddbi
        {0, 1, {Expression::createPaddedString("DEEKV", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DokeAddbi
        {0, 1, {Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSt
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdwSt
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

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
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PeekArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("PEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // DeekArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("DEEK", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // PokeArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x", 
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeVarArrayB
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeVarArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeVarArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeTmpArrayB
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeTmpArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeTmpArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaVarArrayB
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaVarArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeaVarArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaTmpArrayB
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // PokeaTmpArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // DokeaTmpArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("ADDW", OPCODE_TRUNC_SIZE, ' ') + "mem",
                Expression::createPaddedString("DOKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // MovqwLdwiAddiPokea
        {0, 0, {Expression::createPaddedString("MOVQW", OPCODE_TRUNC_SIZE, ' ') + "mem", 
                Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "", 
                Expression::createPaddedString("POKEA", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwMovb
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("MOVB", OPCODE_TRUNC_SIZE, ' ') + "0x"}},

        // StwPokeArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("POKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // StwDokeArray
        {0, 0, {Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "0x",
                Expression::createPaddedString("DOKE", OPCODE_TRUNC_SIZE, ' ') + "mem"}},

        // LdiSt
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ST", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiStw
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdSubLoHi
        {0, 0, {Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdiSubLoHi
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdwSubLoHi
        {0, 0, {Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LD", OPCODE_TRUNC_SIZE, ' ') + "_",
                Expression::createPaddedString("STW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("LDW", OPCODE_TRUNC_SIZE, ' ') + "reg",
                Expression::createPaddedString("SUBW", OPCODE_TRUNC_SIZE, ' ') + "reg"}},

        // LdiAddi
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiSubi
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiAndi
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ANDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiOri
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // LdiXori
        {0, 0, {Expression::createPaddedString("LDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("XORI", OPCODE_TRUNC_SIZE, ' ') + ""}},

        // AddiPair
        {0, 0, {Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + "",
                Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' ') + ""}},

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

        /******************************************************************************************/
        /* Opcodes are manually matched here on in
        /******************************************************************************************/

        // AddiZero
        {0, 0, {Expression::createPaddedString("ADDI", OPCODE_TRUNC_SIZE, ' '), ""}},

        // SubiZero
        {0, 0, {Expression::createPaddedString("SUBI", OPCODE_TRUNC_SIZE, ' '), ""}},

        // LdwiNeg
        {0, 0, {Expression::createPaddedString("LDWI", OPCODE_TRUNC_SIZE, ' '), ""}},
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

    // Migrate internal label for an instruction that has been deleted, (use this function before the instruction is deleted)
    bool migrateInternalLabel(int index, int oldLine, int newLine)
    {
        // If a label exists, move it to next available vasm line
        if(Compiler::getCodeLines()[index]._vasm[oldLine]._internalLabel.size())
        {
            // Next available vasm line is part of a new BASIC line, so can't optimise
            if(int(Compiler::getCodeLines()[index]._vasm.size()) <= newLine) return false;
            Compiler::getCodeLines()[index]._vasm[newLine]._internalLabel = Compiler::getCodeLines()[index]._vasm[oldLine]._internalLabel;
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
        if(vlIdx >= int(Compiler::getCodeLines()[codeLineIndex]._vasm.size()))
        {
            vlIdx = 0;
            if(++clIdx >= int(Compiler::getCodeLines().size())) return;
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

    // Assumes there is either 1 operand or 2 operands seperated by a comma
    std::string getOperand(const std::string& operand, int index=0)
    {
        std::string opr = operand;
        Expression::stripWhitespace(opr);
        std::size_t comma = operand.find(',');
        if(comma == std::string::npos) return opr;
        if(index  &&  comma + 1 < opr.size()) return opr.substr(comma + 1);
        return opr.substr(0, comma);
    }

    bool optimiseCode(void)
    {
        bool optimised;
        int numOptimiserPasses = 0;

        do
        {
            optimised = false;

            for(int i=0; i<int(Compiler::getCodeLines().size()); i++)
            {
                // Skip code lines when optimiser is disabled
                if(!Compiler::getCodeLines()[i]._optimiserEnabled) continue;

                for(int j=0; j<int(matchSequences.size()); j++)
                {
                    for(auto itVasm=Compiler::getCodeLines()[i]._vasm.begin(); itVasm!=Compiler::getCodeLines()[i]._vasm.end();)
                    {
                        bool linesDeleted = false;
                        int vasmIndex = int(itVasm - Compiler::getCodeLines()[i]._vasm.begin());

                        // Can only optimise within a BASIC code line, (use multi-statements to optimise across lines)
                        int vasmIndexMax = vasmIndex + int(matchSequences[j]._sequence.size()) - 1;
                        if(vasmIndexMax >= int(Compiler::getCodeLines()[i]._vasm.size()))
                        {
                            ++itVasm;
                            continue;
                        }

                        // Find opcode match
                        bool foundOpcodeMatch = true;
                        for(int k=vasmIndex; k<=vasmIndexMax; k++)
                        {
                            if(Compiler::getCodeLines()[i]._vasm[k]._code.find(matchSequences[j]._sequence[k - vasmIndex]) == std::string::npos)
                            {
                                foundOpcodeMatch = false;
                                break;
                            }
                        }

                        if(foundOpcodeMatch)
                        {
                            // First operand
                            int firstIndex = matchSequences[j]._firstIndex;
                            int firstLine = vasmIndex + firstIndex;
                            std::string firstOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;

                            // Second operand
                            int secondIndex = matchSequences[j]._secondIndex;
                            int secondLine = vasmIndex + secondIndex;
                            std::string secondOperand = Compiler::getCodeLines()[i]._vasm[secondLine]._operand;

    /*************************************************************************************************************************************************************/
    /* Opcode matches required, operand matches required                                                                                                         */
    /*************************************************************************************************************************************************************/

                            // Find operand match, (temporary variables are a minimum of 4 chars, i.e. '0xd0')
                            // Don't use this, as sequences such as 'ST blah + 1' will match with 'ST blah'
                            //if(firstOperand.substr(0, 4) == secondOperand.substr(0, 4))
                            if(firstOperand == secondOperand)
                            {
                                switch(j)
                                {
                                    // Match ST, STW, replace ST's operand with STW's operand, delete STW
                                    case StStw:
                                    {
                                        // Migrate internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 1)) break;

                                        // Replace ST's operand with STW's operand
                                        std::string stwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ST", stwOperand);

                                        // Delete STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW"});
                                    }
                                    break;

                                    // Match STW LDW, delete STW LDW
                                    //case StwLdPair:
                                    case StwLdwPair:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW ST, delete STW
                                    case StwStHigh:
                                    {
                                        // Assume neither of these instructions can have a label
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW"});
                                    }
                                    break;

                                    // Match STW/ST LDW/LD, delete LDW/LD
                                    case ExtraLdw:
                                    case ExtraLd:
                                    {
                                        // If the LDW has an internal label, then it probably can't be optimised away
                                        if(!Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size())
                                        {
                                            // Migrate internal label to next available instruction, (if it exists)
                                            if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                            // Delete LDW
                                            linesDeleted = true;
                                            itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                            adjustLabelAndVasmAddresses(i, firstLine + 1, {"LDW"});
                                        }
                                    }
                                    break;

                                    // Match LDW LDW, delete first LDW
                                    case LdwPair:
                                    {
                                        // Migrate internal label from first LDW to second LDW, (if it exists)
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 1)) break;

                                        // Delete first LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"LDW"});
                                    }
                                    break;

                                    // Match LDW STW LDW STW, delete second LDW
                                    case LdwStwLdwStw:
                                    {
                                        // if second LDW has a label then bail
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Delete second LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 2);
                                        adjustLabelAndVasmAddresses(i, firstLine + 2, {"LDW"});
                                    }
                                    break;

                                    // Match STW LDI ADDW, copy LDI operand to ADDW operand, change ADDW to ADDI, delete STW LDI
                                    case StwLdiAddw:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ADDW with ADDI and replace ADDW's operand with LDI's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ADDI", ldOperand);

                                        // Delete STW and LDI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDI"});
                                    }
                                    break;

                                    // Match STW LD ADDW, replace ADDW with ADDBA and LD's operand, delete STW LD
                                    case StwLdAddw:
                                    case StwLdAddwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ADDW with ADDBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ADDBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW ADDW, copy LDW operand to ADDW operand, delete STW LDW
                                    case StwLdwAddw:
                                    case StwLdwAddwVar:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ADDW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ADDW", ldwOperand);

                                        // Delete STW and LD/LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDI ANDW, copy LDI operand to ANDW operand, change ANDW to ANDI, delete STW LDW
                                    case StwLdiAndw:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ANDW with ANDI and replace ANDW's operand with LDI's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ANDI", ldOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LD ANDW, replace ANDW with ANDBA and LD's operand, delete STW LD
                                    case StwLdAndw:
                                    case StwLdAndwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ANDW with ANDBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ANDBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW ANDW, copy LDW operand to ANDW operand, delete STW LDW
                                    case StwLdwAndw:
                                    case StwLdwAndwVar:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ANDW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ANDW", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LDI XORW, copy LDI operand to XORW operand, change XORW to XORI, delete STW LDW
                                    case StwLdiXorw:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace XORW with XORI and replace XORW's operand with LDI's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "XORI", ldOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LD XORW, replace XORW with XORBA and LD's operand, delete STW LD
                                    case StwLdXorw:
                                    case StwLdXorwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace XORW with XORBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "XORBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW XORW, copy LDW operand to XORW operand, delete STW LDW
                                    case StwLdwXorw:
                                    case StwLdwXorwVar:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace XORW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "XORW", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LDI ORW, copy LDI operand to ORW operand, change ORW to ORI, delete STW LDW
                                    case StwLdiOrw:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ORW with ORI and replace ORW's operand with LDI's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ORI", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LD ORW, replace ORW with ORBA and LD's operand, delete STW LD
                                    case StwLdOrw:
                                    case StwLdOrwVar:
                                    {
                                        // Bail if wrong ROM version or if one of these has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ORW with ORBA and LD's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ORBA", ldOperand);

                                        // Delete STW and LD
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW ORW, copy LDW operand to ORW operand, delete STW LDW
                                    case StwLdwOrw:
                                    case StwLdwOrwVar:
                                    {
                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ORW's operand with LD/LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ORW", ldwOperand);

                                        // Delete STW and LD/LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LD"});
                                    }
                                    break;

                                    // Match LDPX ST, copy ST operand to LDPX, delete ST
                                    case LdPxSt:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Migrate ST's label, (if it exists)
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        std::string ldpxOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine]._operand);
                                        std::string stOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "LDPX", ldpxOperand + ", " + stOperand);

                                        // Delete ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"ST"});
                                    }
                                    break;

                                    // Match LD/LDW STPX, copy LD/LDW operand to STPX, delete LD/LDW
                                    case LdStPx:
                                    case LdwStPx:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Migrate LD/LDW's label, (if it exists)
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 1)) break;

                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string stpxOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand);
                                        updateVasm(i, firstLine + 1, "STPX", stpxOperand + ", " + ldOperand);

                                        // Delete LD/LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"LD"});
                                    }
                                    break;

                                    // Match STW LDWI ADDW, copy LDWI operand to ADDW operand, change ADDW to ADDWI, delete STW LDWI
                                    case StwLdwiAddw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ADDW with ADDWI and replace ADDWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ADDWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI SUBW, copy LDWI operand to SUBW operand, change SUBW to SUBWI, delete STW LDWI
                                    case StwLdwiSubw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace SUBW with SUBWI and replace SUBWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "SUBWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI ANDW, copy LDWI operand to ANDW operand, change ANDW to ANDWI, delete STW LDWI
                                    case StwLdwiAndw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ANDW with ANDWI and replace ANDWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ANDWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI XORW, copy LDWI operand to XORW operand, change XORW to XORWI, delete STW LDWI
                                    case StwLdwiXorw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace XORW with XORWI and replace XORWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "XORWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match STW LDWI ORW, copy LDWI operand to ORW operand, change ORW to ORWI, delete STW LDWI
                                    case StwLdwiOrw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Only one of these can have an internal label
                                        if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                        // Replace ORW with ORWI and replace ORWI's operand with LDWI's operand
                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ORWI", ldwiOperand);

                                        // Delete STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDWI"});
                                    }
                                    break;

                                    // Match LDW/LD BEQ STW/ST, delete STW/ST
                                    case LdwBeqStwTmp:
                                    case LdwBeqStwVar:
                                    case LdBeqStTmp:
                                    case LdBeqStVar:
                                    {
                                        // STW/ST can have an internal label
                                        if(!migrateInternalLabel(i, firstLine + 2, firstLine + 3)) break;

                                        // Delete STW/ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 2);
                                        adjustLabelAndVasmAddresses(i, firstLine + 2, {"STW"});
                                    }
                                    break;

                                    // Match LDW POKE/DOKE LDW, delete second LDW if it matches with first LDW
                                    case PokeVar:
                                    case DokeVar:
                                    {
                                        // Migrate second LDW's label, (if it exists)
                                        if(!migrateInternalLabel(i, firstLine + 2, firstLine + 3)) break;

                                        // Delete second LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 2);
                                        adjustLabelAndVasmAddresses(i, firstLine + 2, {"LDW"});
                                    }
                                    break;

                                    // Match LDW PEEK INC
                                    case LdwPeekInc:
                                    {
                                        // Bail if wrong ROM version or if PEEK or INC have an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Replace LDW with PEEK+ and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "PEEK+", ldwOperand);

                                        // Delete PEEK, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"PEEK", "INC"});
                                    }
                                    break;

                                    // Match PEEKV INC
                                    case PeekvInc:
                                    {
                                        // Bail if wrong ROM version or if INC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                        // Replace PEEKV with PEEK+ and new operand
                                        std::string peekOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "PEEK+", peekOperand);

                                        // Delete INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"INC"});
                                    }
                                    break;

                                    // Match POKE INC
                                    case PokeInc:
                                    {
                                        // Bail if wrong ROM version or if INC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                        // Replace POKE with POKE+ and new operand
                                        std::string pokeOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "POKE+", pokeOperand);

                                        // Delete INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"INC"});
                                    }
                                    break;

                                    // Match LDW DEEK INC INC
                                    case LdwDeekIncInc:
                                    {
                                        // Bail if wrong ROM version or if DEEK, INC or INC have an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._internalLabel.size()) break;

                                        // Replace LDW with DEEK+ and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "DEEK+", ldwOperand);

                                        // Delete DEEK, INC, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"DEEK", "INC", "INC"});
                                    }
                                    break;

                                    // Match DEEKV INC INC
                                    case DeekvIncInc:
                                    {
                                        // Bail if wrong ROM version or if INC or INC have an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Replace DEEKV with DEEK+
                                        std::string deekvOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "DEEK+", deekvOperand);

                                        // Delete INC, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"INC", "INC"});
                                    }
                                    break;

                                    // Match DOKE INC INC
                                    case DokeIncInc:
                                    {
                                        // Bail if wrong ROM version or if INC or INC have an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Replace DOKE with DOKE+
                                        std::string dokeOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "DOKE+", dokeOperand);

                                        // Delete INC, INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"INC", "INC"});
                                    }
                                    break;

                                    // TODO: CMPI can fail with LDW SUBI/XORI as it is an 8bit compare
                                    // Match LD/LDW SUBI/XORI BCC
                                    case LdSubiBcc:
                                    case LdXoriBcc:
                                    //case LdwSubiBcc:
                                    //case LdwXoriBcc:
                                    {
                                        // Bail if wrong ROM version or if SUBI/XORI has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                        // Bail if next instruction is not a BCC, JCC or JumpXX macro
                                        std::string bccOpcode;
                                        if(firstLine + 2 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            bccOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 2]._opcode.substr(0, 5);
                                            if(bccOpcode != "BEQ"  &&  bccOpcode != "BNE"  &&  bccOpcode != "BLE"  &&  bccOpcode != "BGE"  &&  bccOpcode != "BLT"  &&  bccOpcode != "BGT"  &&
                                               bccOpcode != "JEQ"  &&  bccOpcode != "JNE"  &&  bccOpcode != "JLE"  &&  bccOpcode != "JGE"  &&  bccOpcode != "JLT"  &&  bccOpcode != "JGT"  &&
                                               bccOpcode != "%Jump")
                                            {
                                                break;
                                            }
                                        }

                                        // Bail if next instruction is a write instruction, (this can fail for delayed write's in hand written code, but won't fail for compiled code; the optimiser is only ever run on compiled code)
                                        // This can still fail if this next instruction relies on the correct results of a subtraction, enable LdSubiBcc/LdXoriBcc with caution.
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW"  ||  stOpcode == "POKE"  ||  stOpcode == "DOKE"  ||  stOpcode == "POKE+"  ||  stOpcode == "DOKE+") break;
                                        }

                                        // Bail if SUBI/XORI has an internal label
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                        // Bail if SUBI/XORI operand is 0 and BCC/JCC is one of (BLE, BGE, BLT, BGT), as CMPI is an 8bit compare
                                        std::string subiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        if(subiOperand == "0" || subiOperand == "0x00")
                                        {
                                            if(bccOpcode == "BLE"  ||  bccOpcode == "BGE"  ||  bccOpcode == "BLT"  ||  bccOpcode == "BGT") break;
                                            if(bccOpcode == "JLE"  ||  bccOpcode == "JGE"  ||  bccOpcode == "JLT"  ||  bccOpcode == "JGT") break;
                                        }

                                        // Replace LD/LDW with CMPI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "CMPI", ldOperand + ", " + subiOperand);

                                        // Delete SUBI/XORI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"SUBI"});
                                    }
                                    break;

                                    // Match LDW NOTW STW, replace with NOTW
                                    case LdwNotwStw:
                                    {
                                        // Bail if wrong ROM version or if NOTW or STW have an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with NOTW and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "NOTW", ldwOperand);

                                        // Delete NOTW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"NOTW", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LDW NEGW STW
                                    case LdwNegwStw:
                                    {
                                        // Bail if wrong ROM version or if NEGW or STW have an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with NEGW and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "NEGW", ldwOperand);

                                        // Delete NEGW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"NEGW", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW NOTW ST, replace with NOTB
                                    case LdNotwSt:
                                    case LdwNotwSt:
                                    {
                                        // Bail if wrong ROM version or if NOTW or ST have an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LD/LDW with NOTB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "NOTB", ldOperand);

                                        // Delete NOTW and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"NOTW", "ST"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldOperand);
                                    }
                                    break;

                                    // Match INC INC, replace both with ADDBI 2
                                    case Inc2Var:
                                    {
                                        // Bail if wrong ROM version or if second INC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                
                                        // Replace first INC with ADDBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "ADDBI", incOperand + ", 2");

                                        // Delete second INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"INC"});
                                    }
                                    break;

                                    // Match INC INC INC, replace all three with ADDBI 3
                                    case Inc3Var:
                                    {
                                        // Bail if wrong ROM version or if second or third INC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                
                                        // Replace first INC with ADDBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "ADDBI", incOperand + ", 3");

                                        // Delete second and third INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"INC", "INC"});
                                    }
                                    break;

                                    // Match INC INC INC INC, replace all four with ADDBI 4
                                    case Inc4Var:
                                    {
                                        // Bail if wrong ROM version or if second or third or fourth INC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._internalLabel.size()) break;
                                
                                        // Replace first INC with ADDBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "ADDBI", incOperand + ", 4");

                                        // Delete second, third and fourth INC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"INC", "INC", "INC"});
                                    }
                                    break;

                                    // Match DEC DEC, replace both with SUBBI 2
                                    case Dec2Var:
                                    {
                                        // Bail if wrong ROM version or if second DEC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                
                                        // Replace first DEC with SUBBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "SUBBI", incOperand + ", 2");

                                        // Delete second DEC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"DEC"});
                                    }
                                    break;

                                    // Match DEC DEC DEC, replace all three with SUBBI 3
                                    case Dec3Var:
                                    {
                                        // Bail if wrong ROM version or if second or third DEC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                
                                        // Replace first DEC with SUBBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "SUBBI", incOperand + ", 3");

                                        // Delete second and third DEC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"DEC", "DEC"});
                                    }
                                    break;

                                    // Match DEC DEC DEC DEC, replace all four with SUBBI 4
                                    case Dec4Var:
                                    {
                                        // Bail if wrong ROM version or if second or third or fourth DEC has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._internalLabel.size()) break;
                                
                                        // Replace first DEC with SUBBI and new operands
                                        std::string incOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "SUBBI", incOperand + ", 4");

                                        // Delete second, third and fourth DEC
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"DEC", "DEC", "DEC"});
                                    }
                                    break;

                                    // Match LDW ADDI STW, replace all with ADDVI
                                    case LdwAddiStw:
                                    {
                                        // Bail if wrong ROM version or if ADDI or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Replace LDW with ADDVI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string addiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ADDVI", ldwOperand + ", " + addiOperand);

                                        // Delete ADDI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ADDI", "STW"});
                                    }
                                    break;

                                    // Match LDW ADDW STW, replace all with ADDVW
                                    case LdwAddwStw:
                                    {
                                        // Bail if wrong ROM version or if ADDW or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Replace LDW with ADDVW and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string addwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ADDVW", addwOperand + ", " + ldwOperand);

                                        // Delete ADDW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ADDW", "STW"});
                                    }
                                    break;

                                    // Match LD/LDW ADDI ST, replace all with ADDBI
                                    case LdAddiSt:
                                    case LdwAddiSt:
                                    {
                                        // Bail if wrong ROM version or if ADDI or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with ADDBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string addOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ADDBI", ldOperand + ", " + addOperand);

                                        // Delete ADDI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ADDI", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW SUBI STW, replace all with SUBVI
                                    case LdwSubiStw:
                                    {
                                        // Bail if wrong ROM version or if SUBI or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Replace LDW with SUBVI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string addiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "SUBVI", ldwOperand + ", " + addiOperand);

                                        // Delete SUBI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"SUBI", "STW"});
                                    }
                                    break;

                                    // Match LDW SUBW STW, replace all with SUBVW
                                    case LdwSubwStw:
                                    {
                                        // Bail if wrong ROM version or if SUBW or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // Replace LDW with SUBVW and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string subwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "SUBVW", subwOperand + ", " + ldwOperand);

                                        // Delete SUBW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"SUBW", "STW"});
                                    }
                                    break;

                                    // Match INCW LDW, replace INCW with INCWA, del LDW
                                    case IncwLdw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Migrate LDW's internal label if it exists
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine)) break;

                                        // Replace INCW with INCWA
                                        std::string incwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "INCWA", incwOperand);

                                        // Delete LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"LDW"});
                                    }
                                    break;

                                    // Match DECW LDW, replace DECW with DECWA, del LDW
                                    case DecwLdw:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Migrate LDW's internal label if it exists
                                        if(!migrateInternalLabel(i, firstLine + 1, firstLine)) break;

                                        // Replace DECW with DECWA
                                        std::string decwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "DECWA", decwOperand);

                                        // Delete LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"LDW"});
                                    }
                                    break;

                                    // Match LD STW LD/LDW SUBW, del LD STW, replace SUBW with SUBBA, (matches on STW and SUBW, that's why first LD is at firstLine - 1)
                                    case LdStwLdSubw:
                                    case LdStwLdwSubw:
                                    {
                                        // Bail if wrong ROM version or if STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine]._internalLabel.size()) break;

                                        // Migrate internal label to next available instruction, (if it exists)
                                        if(!migrateInternalLabel(i, firstLine - 1, firstLine + 1)) break;

                                        // Replace SUBW with SUBBA and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine - 1]._operand;
                                        updateVasm(i, firstLine + 2, "SUBBA", ldOperand);

                                        // Delete LD and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine - 1, {"LD", "STW"});
                                    }
                                    break;

                                    // Match LD SUB1 ST, replace all with DEC
                                    case LdSub1St:
                                    {
                                        // Bail if wrong ROM version or if SUBI or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD with DEC and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string subOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        if(subOperand == "1"  ||  subOperand == "0x01"  ||  subOperand == "0x0001")
                                        {
                                            updateVasm(i, firstLine, "DEC", ldOperand);
                                        }
                                        // Not a decrement so bail
                                        else break;

                                        // Delete SUBI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"SUBI", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW SUB1 STW, replace all with DECW
                                    case LdwSub1Stw:
                                    {
                                        // Bail if wrong ROM version or if SUBI or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with DECW and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string subOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        if(subOperand == "1"  ||  subOperand == "0x01"  ||  subOperand == "0x0001")
                                        {
                                            updateVasm(i, firstLine, "DECW", ldwOperand);
                                        }
                                        // Not a decrement so bail
                                        else break;

                                        // Delete SUBI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"SUBI", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW SUBI ST, replace all with SUBBI
                                    case LdSubiSt:
                                    case LdwSubiSt:
                                    {
                                        // Bail if wrong ROM version or if SUBI or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with SUBBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string subOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "SUBBI", ldOperand + ", " + subOperand);

                                        // Delete SUBI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"SUBI", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD/LDW ANDI ST, replace all with ANDBI
                                    case LdAndiSt:
                                    case LdwAndiSt:
                                    {
                                        // Bail if wrong ROM version or if ANDI or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with ANDBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string andOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ANDBI", ldOperand + ", " + andOperand);

                                        // Delete ANDI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ANDI", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD/LDW ORI ST, replace all with ORBI
                                    case LdOriSt:
                                    case LdwOriSt:
                                    {
                                        // Bail if wrong ROM version or if ORI or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with ORBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string orOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ORBI", ldOperand + ", " + orOperand);

                                        // Delete ORI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ORI", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW ORI STW, replace all with ORBI
                                    case LdwOriStw:
                                    {
                                        // Bail if wrong ROM version or if ORI or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with ORBI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string orOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ORBI", ldwOperand + ", " + orOperand);

                                        // Delete ORI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ORI", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW XORI ST, replace all with XORBI
                                    case LdXoriSt:
                                    case LdwXoriSt:
                                    {
                                        // Bail if wrong ROM version or if XORI or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD/LDW with XORBI and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string xorOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "XORBI", ldOperand + ", " + xorOperand);

                                        // Delete XORI and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"XORI", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDW XORI STW, replace all with XORBI
                                    case LdwXoriStw:
                                    {
                                        // Bail if wrong ROM version or if XORI or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with XORBI and new operands
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string xorOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "XORBI", ldwOperand + ", " + xorOperand);

                                        // Delete XORI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"XORI", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD/LDW ANDI, replace all with ANDBK
                                    case LdAndi:
                                    case LdwAndi:
                                    {
                                        // Bail if wrong ROM version or if ANDI has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                        // Replace LD/LDW with ANDBK and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string andOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ANDBK", ldOperand + ", " + andOperand);

                                        // Delete ANDI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ANDI"});
                                    }
                                    break;

                                    // Match LD ORI, replace all with ORBK
                                    case LdOri:
                                    {
                                        // Bail if wrong ROM version or if ORI has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                        // Replace LD with ORBK and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string orOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "ORBK", ldOperand + ", " + orOperand);

                                        // Delete ORI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"ORI"});
                                    }
                                    break;

                                    // Match LD XORI, replace all with XORBK
                                    case LdXori:
                                    {
                                        // Bail if wrong ROM version or if XORI has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                        // Replace LD with XORBK and new operands
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        std::string xorOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "XORBK", ldOperand + ", " + xorOperand);

                                        // Delete XORI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"XORI"});
                                    }
                                    break;

                                    // Match LDW LSLW STW, replace all with LSLV
                                    case LdwLslwStw:
                                    {
                                        // Bail if wrong ROM version or if LSLW or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with LSLV and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "LSLV", ldwOperand);

                                        // Delete LSLW and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"LSLW", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LDW LSRV STW, replace all with LSRV
                                    case LdwLsrvStw:
                                    {
                                        // Bail if wrong ROM version or if LSRV or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with LSRV and new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "LSRV", ldwOperand);

                                        // Delete LSRV and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"LSRV", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 1, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LDW LSRV LSRV STW, replace all with LSRV LSRV
                                    case LdwLsrv2Stw:
                                    {
                                        // Bail if wrong ROM version or if LSRV or LSRV or STW has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LDW, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLdw = false;
                                        if(firstLine + 4 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 4]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLdw = true;
                                        }

                                        // Replace LDW with LSRV and new operand, replace first LSRV's operand with new operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine,     "LSRV", ldwOperand);
                                        updateVasm(i, firstLine + 1, "LSRV", ldwOperand);

                                        // Delete second LSRV and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 2);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 2, {"LSRV", "STW"});

                                        if(insertLdw) insertVasm(i, firstLine, firstLine + 2, "LDW", ldwOperand);
                                    }
                                    break;

                                    // Match LD LSRB ST, replace all with LSRB
                                    case LdLsrb1St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 3]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD with LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 1, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD LSRB LSRB ST, replace all with LSRB LSRB
                                    case LdLsrb2St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or LSRB or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 4 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 4]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD LSRB with LSRB LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine,     "LSRB", ldOperand);
                                        updateVasm(i, firstLine + 1, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 2);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 2, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 2, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD LSRB LSRB LSRB ST, replace all with LSRB LSRB LSRB
                                    case LdLsrb3St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or LSRB or LSRB or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 4]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 5 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 5]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD LSRB LSRB with LSRB LSRB LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine,     "LSRB", ldOperand);
                                        updateVasm(i, firstLine + 1, "LSRB", ldOperand);
                                        updateVasm(i, firstLine + 2, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 3);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 3, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 3, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LD LSRB LSRB LSRB LSRB ST, replace all with LSRB LSRB LSRB LSRB
                                    case LdLsrb4St:
                                    {
                                        // Bail if wrong ROM version or if LSRB or LSRB or LSRB or LSRB or ST has an internal label
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 4]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 5]._internalLabel.size()) break;

                                        // If next instruction is a ST/STW then insert an LD, (TODO: fails for many instructions like POKE/DOKE/BXX/JXX/etc)
                                        bool insertLd = false;
                                        if(firstLine + 6 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 6]._opcode;
                                            if(stOpcode == "ST"  ||  stOpcode == "STW") insertLd = true;
                                        }

                                        // Replace LD LSRB LSRB LSRB with LSRB LSRB LSRB LSRB and new operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                        updateVasm(i, firstLine,     "LSRB", ldOperand);
                                        updateVasm(i, firstLine + 1, "LSRB", ldOperand);
                                        updateVasm(i, firstLine + 2, "LSRB", ldOperand);
                                        updateVasm(i, firstLine + 3, "LSRB", ldOperand);

                                        // Delete LSRB and ST
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 4);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 4, {"LSRB", "ST"});

                                        if(insertLd) insertVasm(i, firstLine, firstLine + 4, "LD", ldOperand);
                                    }
                                    break;

                                    // Match LDWI ADDW STW LDWI ADDW, delete 2nd LDWI ADDW
                                    case LdwiAddwStwLdwiAddw:
                                    {
                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 4]._operand) break;

                                        // Delete 2nd LDWI and ADDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 3);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 3, {"LDWI", "ADDW"});
                                    }
                                    break;

                                    // Match LDWI ADDW ADDW STW LDWI ADDW ADDW, delete 2nd LDWI ADDW ADDW
                                    case LdwiAddw2StwLdwiAddw2:
                                    {
                                        // Bail if all ADDW operand's don't match
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 2]._operand) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 5]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 6]._operand) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 5]._operand) break;

                                        // Delete 2nd LDWI ADDW and ADDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 4);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 4, {"LDWI", "ADDW", "ADDW"});
                                    }
                                    break;

                                    // Match LDWI STW LDI POKE, replace with LDWI POKEI
                                    case LdwiStwLdiPoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace POKE with POKEI and LDI's operand, (matches on STW and POKE, that's why LDWI is at firstLine - 1)
                                        std::string ldiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "POKEI", ldiOperand);
                                
                                        // Delete STW and LDI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDI"});
                                    }
                                    break;

                                    // Match LDWI STW LDI DOKE, replace with LDWI DOKEI
                                    case LdwiStwLdiDoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Replace STW with DOKEI and LDI's operand, (matches on STW and DOKE, that's why first LDWI is at firstLine - 1)
                                        std::string ldiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "DOKEI", ldiOperand);

                                        // Delete LDI and DOKE
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"LDI", "DOKE"});
                                    }
                                    break;

                                    // Match LDWI STW LDWI DOKE, replace with LDWI DOKEI
                                    case LdwiStwLdwiDoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace STW with DOKEI and second LDWI's operand, (matches on STW and DOKE, that's why first LDWI is at firstLine - 1)
                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine, "DOKEI", ldwiOperand);

                                        // Delete LDWI and DOKE
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 1, {"LDWI", "DOKE"});
                                    }
                                    break;

                                    // Match STW LD/LDW POKE, replace with POKEA
                                    case StwLdPoke:
                                    case StwLdwPoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace POKE with POKEA and LD/LDW's operand
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "POKEA", ldOperand);

                                        // Delete STW and LD/LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LD"});
                                    }
                                    break;

                                    // Match STW LDW DOKE, replace with DOKEA
                                    case StwLdwDoke:
                                    case StwLdwDokeReg:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // If next instruction's operand equals LDW's operand, then bail, (this will fail if operand is referenced after next instruction)
                                        if(firstLine + 3 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                        {
                                            std::string stwOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                            std::string operand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._operand);
                                            if(operand == stwOperand) break;
                                        }

                                        // Replace DOKE with DOKEA and LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "DOKEA", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match STW LDW INC POKE, replace with ADDI POKEA
                                    case StwLdwIncPoke:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace INC with ADDI, POKE with POKEA and LDW's operand
                                        std::string ldwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        updateVasm(i, firstLine + 2, "ADDI", "1");
                                        updateVasm(i, firstLine + 3, "POKEA", ldwOperand);

                                        // Delete STW and LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"STW", "LDW"});
                                    }
                                    break;

                                    // Match LD STW LDW/LDWI POKEA, replace with LD POKEA
                                    case LdStwLdwPokea:
                                    case LdStwLdwiPokea:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Replace POKEA's operand with LD's operand, (matches on STW and POKEA, that's why LD is at firstLine - 1)
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine - 1]._operand;
                                        updateVasm(i, firstLine + 2, "POKEA", ldOperand);

                                        // Delete LD and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine - 1, {"LD", "STW"});
                                    }
                                    break;

                                    // Match LD STW LDWI ADDI POKEA, replace with LD POKEA
                                    case LdStwLdwiAddiPokea:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                
                                        // Add ADDI's operand to LDWI's operand, update POKEA with LD's operand, (matches on STW and POKEA, that's why LD is at firstLine - 1)
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine - 1]._operand;
                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                        std::string addiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 2]._operand;
                                        updateVasm(i, firstLine + 2, "LDWI", ldwiOperand + " + " + addiOperand);
                                        updateVasm(i, firstLine + 3, "POKEA", ldOperand);

                                        // Delete LD, STW and LDWI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine, {"LD", "STW", "LDWI"});
                                    }
                                    break;

                                    // Match LDWI ADDW PEEK NEGW STW LDWI ADDW POKEA
                                    case NegwArrayB:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 6]._operand) break;

                                        updateVasm(i, firstLine + 2, "STW", "memAddr");
                                        updateVasm(i, firstLine + 3, "PEEK", "");
                                        updateVasm(i, firstLine + 4, "NEGW", "giga_vAC");
                                        updateVasm(i, firstLine + 5, "POKE", "memAddr");

                                        // Delete ADDW and POKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 6);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 6, {"ADDW", "POKEA"});
                                    }
                                    break;

                                    // Match LDWI ADDW DEEK NEGW STW LDWI ADDW DOKEA
                                    case NegwArray:
                                    {
                                        // Bail if wrong ROM version
                                        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                        // Bail if ADDW operand's don't match
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 6]._operand) break;

                                        updateVasm(i, firstLine + 2, "STW", "memAddr");
                                        updateVasm(i, firstLine + 3, "DEEK", "");
                                        updateVasm(i, firstLine + 4, "NEGW", "giga_vAC");
                                        updateVasm(i, firstLine + 5, "DOKE", "memAddr");

                                        // Delete ADDW and DOKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 6);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 6, {"ADDW", "DOKEA"});
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
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 10]._operand) break;

                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 4]._operand;
                                        std::string addwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 5]._operand;
                                        std::string mathOpcode  = Compiler::getCodeLines()[i]._vasm[firstLine + 7]._opcode;

                                        updateVasm(i, firstLine + 2, "STW", "memAddr");
                                        updateVasm(i, firstLine + 3, "LDWI", ldwiOperand);
                                        updateVasm(i, firstLine + 4, "ADDW", addwOperand);
                                        updateVasm(i, firstLine + 5, "PEEKA", "memValue");
                                        updateVasm(i, firstLine + 6, "PEEKV", "memAddr");
                                        updateVasm(i, firstLine + 7, mathOpcode, "memValue");
                                        updateVasm(i, firstLine + 8, "POKE", "memAddr");

                                        // Delete LDWI ADDW and POKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 9);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 9, {"LDWI", "ADDW", "POKEA"});
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
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand != Compiler::getCodeLines()[i]._vasm[firstLine + 10]._operand) break;

                                        std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 4]._operand;
                                        std::string addwOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 5]._operand;
                                        std::string mathOpcode  = Compiler::getCodeLines()[i]._vasm[firstLine + 7]._opcode;

                                        updateVasm(i, firstLine + 2, "STW", "memAddr");
                                        updateVasm(i, firstLine + 3, "LDWI", ldwiOperand);
                                        updateVasm(i, firstLine + 4, "ADDW", addwOperand);
                                        updateVasm(i, firstLine + 5, "DEEKA", "memValue");
                                        updateVasm(i, firstLine + 6, "DEEKV", "memAddr");
                                        updateVasm(i, firstLine + 7, mathOpcode, "memValue");
                                        updateVasm(i, firstLine + 8, "DOKE", "memAddr");

                                        // Delete LDWI ADDW and DOKEA
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 9);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine + 9, {"LDWI", "ADDW", "DOKEA"});
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
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine - 1]._internalLabel.size()) break;
                                        if(Compiler::getCodeLines()[i]._vasm[firstLine + 0]._internalLabel.size()) break;

                                        // Replace LDW with LD/LDW/LDWI and operand
                                        std::string ldOpcode  = Compiler::getCodeLines()[i]._vasm[firstLine -1]._opcode;
                                        std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine -1]._operand;
                                        updateVasm(i, firstLine + 2, ldOpcode, ldOperand);

                                        // Delete LD/LDW/LDWI and STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine - 1, {ldOpcode, "STW"});
                                    }
                                    break;

                                    default: break;
                                }
                            }

    /*************************************************************************************************************************************************************/
    /* Opcode matches required, operand matches NOT required                                                                                                     */
    /*************************************************************************************************************************************************************/
                            switch(j)
                            {
                                // Match MOVB ST, replace with MOVB
                                case MovbSt:
                                {
                                    // Bail if wrong ROM version or if ST has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Replace MOVB with MOVB and ST's operand
                                    std::string stOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    updateVasm(i, firstLine, "MOVB", "giga_vAC + 1, " + stOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ST"});
                                }
                                break;

                                // Match PEEK ST, replace with PEEKA
                                case PeekSt:
                                {
                                    // Bail if wrong ROM version or if ST has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Replace PEEK with PEEKA and ST's operand
                                    std::string stOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    updateVasm(i, firstLine, "PEEKA", stOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ST"});
                                }
                                break;

                                // Match LDW PEEK, replace with PEEKV
                                case PeekVar:
                                {
                                    // Bail if wrong ROM version or if PEEK has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                            
                                    // Replace LDW with PEEKV and new operand
                                    std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                    updateVasm(i, firstLine, "PEEKV", ldOperand);
                            
                                    // Delete PEEK
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"PEEK"});
                                }
                                break;

                                // Match DEEK STW, replace with DEEKA
                                case DeekStw:
                                {
                                    // Bail if wrong ROM version or if STW has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Bail if next instruction is a ST/STW/PEEK/DEEK/POKE/DOKE
                                    if(firstLine + 2 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                    {
                                        std::string bailOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 2]._opcode;
                                        if(bailOpcode.find("ST") != std::string::npos  ||  bailOpcode.find("EEK") != std::string::npos  ||  bailOpcode.find("OKE") != std::string::npos) break;
                                    }

                                    // Replace DEEK with DEEKA and STW's operand
                                    std::string stOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    updateVasm(i, firstLine, "DEEKA", stOperand);

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"STW"});
                                }
                                break;

                                // Match LDW DEEK, replace with DEEKV
                                case DeekVar:
                                {
                                    // Bail if wrong ROM version or if DEEK has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                            
                                    // Replace LDW with DEEKV and new operand
                                    std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                    updateVasm(i, firstLine, "DEEKV", ldOperand);
                            
                                    // Delete DEEK
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"DEEK"});
                                }
                                break;

                                // Match LDW DEEK ADDBI
                                case LdwDeekAddbi:
                                {
                                    // Bail if wrong ROM version or if DEEK or ADDBI have an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._internalLabel.size()) break;

                                    // Check that LDW and ADDBI operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string ldwOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine]._operand);
                                    std::string addbiOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._operand);
                                    if(ldwOperand != addbiOperand) break;

                                    // Bail if ADDBI imm operand != 2
                                    uint8_t immValue = 0;
                                    std::string immOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 2]._operand, 1);
                                    if(!Expression::stringToU8(immOperand, immValue)) break;
                                    if(immValue != 2) break;

                                    // Replace LDW with DEEK+ and new operand
                                    updateVasm(i, firstLine, "DEEK+", ldwOperand);

                                    // Delete DEEK, ADDBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"DEEK", "ADDBI"});
                                }
                                break;

                                // Match DEEKV ADDBI
                                case DeekvAddbi:
                                {
                                    // Bail if wrong ROM version or if ADDBI has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Check that DEEKV and ADDBI operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string deekvOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine]._operand);
                                    std::string addbiOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand);
                                    if(deekvOperand != addbiOperand) break;

                                    // Bail if ADDBI imm operand != 2
                                    uint8_t immValue = 0;
                                    std::string immOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand, 1);
                                    if(!Expression::stringToU8(immOperand, immValue)) break;
                                    if(immValue != 2) break;

                                    // Replace DEEKV with DEEK+
                                    updateVasm(i, firstLine, "DEEK+", deekvOperand);

                                    // Delete ADDBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ADDBI"});
                                }
                                break;

                                // Match DOKE ADDBI
                                case DokeAddbi:
                                {
                                    // Bail if wrong ROM version or if ADDBI has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Check that DOKE and ADDBI operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string dokeOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine]._operand);
                                    std::string addbiOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand);
                                    if(dokeOperand != addbiOperand) break;

                                    // Bail if ADDBI imm operand != 2
                                    uint8_t immValue = 0;
                                    std::string immOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand, 1);
                                    if(!Expression::stringToU8(immOperand, immValue)) break;
                                    if(immValue != 2) break;

                                    // Replace DOKE with DOKE+
                                    updateVasm(i, firstLine, "DOKE+", dokeOperand);

                                    // Delete ADDBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ADDBI"});
                                }
                                break;

                                // Match LD/LDW ST, replace with MOVB
                                case LdSt:
                                case LdwSt:
                                {
                                    // Bail if wrong ROM version or if ST has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Bail if next instruction is a store
                                    if(firstLine + 2 < int(Compiler::getCodeLines()[i]._vasm.size()))
                                    {
                                        std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 2]._opcode;
                                        if(stOpcode == "ST"  ||  stOpcode == "STW") break;
                                    }

                                    // Replace LD/LDW with MOVB and new operands
                                    std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                    std::string stOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    updateVasm(i, firstLine, "MOVB", ldOperand + ", " + stOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ST"});
                                }
                                break;

                                // Extra STW, (doesn't require an operand match)
                                case StwPair:
                                case StwPairReg:
                                case ExtraStw:
                                {
                                    // Migrate internal label to next available instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine, firstLine + 1)) break;

                                    // Delete first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"STW"});
                                }
                                break;

                                // Match LDW STW LDWI ADDW PEEK end up with LDWI ADDW PEEK
                                case PeekArrayB:
                                {
                                    // Save previous line LDW, if opcode is not LDW then can't optimise
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLDW._opcode != "LDW") break;

                                    // Migrate it's label, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine - 1, firstLine + 1)) break;

                                    // Delete previous line LDW and first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);

                                    // Replace operand of ADDW with LDW's operand
                                    updateVasm(i, firstLine, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(i, firstLine - 1, {"LDW", "STW"});
                                }
                                break;

                                // Match LDW STW LDWI ADDW ADDW PEEK/DEEK end up with LDWI ADDW ADDW PEEK/DEEK
                                case PeekArray:
                                case DeekArray:
                                {
                                    // Save previous line LDW, if opcode is not LDW then can't optimise
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLDW._opcode != "LDW") break;

                                    // Migrate it's label, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine - 1, firstLine + 1)) break;

                                    // Delete previous line LDW and first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's with LDW's operand
                                    updateVasm(i, firstLine, "ADDW", savedLDW._operand);
                                    updateVasm(i, firstLine + 1, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(i, firstLine - 1, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDWI STW LDW POKE/DOKE end up with LDWI STW LD<X> POKE/DOKE
                                case PokeArray:
                                case DokeArray:
                                {
                                    uint16_t offset = 9;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLD._opcode.find("LD") == std::string::npos) break;
                                    if(savedLD._opcode.find("LDWI") != std::string::npos) offset += 1;

                                    // Discard it's label, (it's no longer needed), and adjust it's address
                                    if(!migrateInternalLabel(i, firstLine - 1, firstLine + 1)) break;
                                    savedLD._internalLabel = "";
                                    savedLD._address += offset; // LD<X> is moved 9bytes, LDWI is moved 10 bytes

                                    // Delete previous line LD<X>, first STW and LDW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm + 2);

                                    // Replace LDW with saved LD<X> and operand
                                    if(offset == 9)
                                    {
                                        itVasm = Compiler::getCodeLines()[i]._vasm.insert(itVasm, savedLD);
                                        adjustLabelAddresses(i, firstLine - 1, -4);
                                        adjustVasmAddresses(i, firstLine - 1, -4);
                                    }
                                    // LDW is replaced with LDWI so push everything forward starting at the POKE/DOKE by 1 more byte
                                    else
                                    {
                                        itVasm = Compiler::getCodeLines()[i]._vasm.insert(itVasm, savedLD);
                                        adjustLabelAddresses(i, firstLine - 1, -5);
                                        adjustVasmAddresses(i, firstLine - 1, -5);
                                        adjustLabelAddresses(i, firstLine + 2, 1);
                                        adjustVasmAddresses(i, firstLine + 2, 1);
                                    }
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW STW LDW POKE
                                case PokeVarArrayB:
                                case PokeTmpArrayB:
                                {
                                    uint16_t offset = 15;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        if(savedLD._opcode.find("LDWI") != std::string::npos) offset += 1;

                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migrateInternalLabel(i, firstLine - 1, firstLine + 3)) break;
                                        savedLD._internalLabel = "";
                                        savedLD._address += offset; // LD<X> is moved 15bytes, LDWI is moved 16 bytes

                                        // Delete previous line LD<X>, first STW and last LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm + 5); //points to last LDW

                                        // Replace LDW with saved LD<X> and operand
                                        if(offset == 15)
                                        {
                                            itVasm = Compiler::getCodeLines()[i]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(i, firstLine - 1, -4);
                                            adjustVasmAddresses(i, firstLine - 1, -4);
                                        }
                                        // LDW is replaced with LDWI so push everything forward starting at the last LDW by 1 more byte
                                        else
                                        {
                                            itVasm = Compiler::getCodeLines()[i]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(i, firstLine - 1, -5);
                                            adjustVasmAddresses(i, firstLine - 1, -5);
                                            adjustLabelAddresses(i, firstLine + 5, 1);
                                            adjustVasmAddresses(i, firstLine + 5, 1);
                                        }

                                        firstLine = firstLine - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstLine = firstLine + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[i]._vasm[firstLine];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);

                                    // Replace operand of ADDW with LDW's operand
                                    updateVasm(i, firstLine + 1, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"LDW", "STW"});
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
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        if(savedLD._opcode.find("LDWI") != std::string::npos) offset += 1;

                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migrateInternalLabel(i, firstLine - 1, firstLine + 3)) break;
                                        savedLD._internalLabel = "";
                                        savedLD._address += offset; // LD<X> is moved 17bytes, LDWI is moved 18 bytes

                                        // Delete previous line LD<X>, first STW and last LDW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm + 6); //points to last LDW, after LD<X> and STW were deleted

                                        // Replace LDW with saved LD<X> and operand
                                        if(offset == 17)
                                        {
                                            itVasm = Compiler::getCodeLines()[i]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(i, firstLine - 1, -4);
                                            adjustVasmAddresses(i, firstLine - 1, -4);
                                        }
                                        // LDW is replaced with LDWI so push everything forward starting at the last LDW by 1 more byte
                                        else
                                        {
                                            itVasm = Compiler::getCodeLines()[i]._vasm.insert(itVasm, savedLD);
                                            adjustLabelAddresses(i, firstLine - 1, -5);
                                            adjustVasmAddresses(i, firstLine - 1, -5);
                                            adjustLabelAddresses(i, firstLine + 6, 1);
                                            adjustVasmAddresses(i, firstLine + 6, 1);
                                        }

                                        firstLine = firstLine - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstLine = firstLine + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[i]._vasm[firstLine];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's
                                    updateVasm(i, firstLine + 1, "ADDW", savedLDW._operand);
                                    updateVasm(i, firstLine + 2, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW POKEA
                                case PokeaVarArrayB:
                                case PokeaTmpArrayB:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migrateInternalLabel(i, firstLine - 1, firstLine + 3)) break;
                                        savedLD._internalLabel = "";

                                        // Operand is not a variable, so can't use POKEA, replace with POKEI
                                        if(savedLD._opcode == "LDI"  ||  savedLD._opcode == "LDWI")
                                        {
                                            updateVasm(i, firstLine+5, "POKEI", savedLD._operand);
                                        }
                                        // Replace POKEA's operand
                                        else
                                        {
                                            updateVasm(i, firstLine+5, "POKEA", savedLD._operand);
                                        }

                                        // Delete previous line LD<X> and first STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine - 1, {savedLD._opcode, "STW"});

                                        firstLine = firstLine - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstLine = firstLine + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[i]._vasm[firstLine];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);

                                    // Replace operand of ADDW with LDW's operand
                                    updateVasm(i, firstLine + 1, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW ADDW POKEA
                                case PokeaVarArray:
                                case PokeaTmpArray:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migrateInternalLabel(i, firstLine - 1, firstLine + 3)) break;
                                        savedLD._internalLabel = "";

                                        // Operand is not a variable, so can't use POKEA, replace with POKEI
                                        if(savedLD._opcode == "LDI"  ||  savedLD._opcode == "LDWI")
                                        {
                                            updateVasm(i, firstLine+6, "POKEI", savedLD._operand);
                                        }
                                        // Replace POKEA's operand
                                        else
                                        {
                                            updateVasm(i, firstLine+6, "POKEA", savedLD._operand);
                                        }

                                        // Delete previous line LD<X> and first STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine - 1, {savedLD._opcode, "STW"});

                                        firstLine = firstLine - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstLine = firstLine + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[i]._vasm[firstLine];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's
                                    updateVasm(i, firstLine + 1, "ADDW", savedLDW._operand);
                                    updateVasm(i, firstLine + 2, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"LDW", "STW"});
                                }
                                break;

                                // Match LD<X> STW LDW STW LDWI ADDW ADDW DOKEA
                                case DokeaVarArray:
                                case DokeaTmpArray:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Save previous line LD<X>, if opcode is not some sort of LD then can't optimise first phase
                                    if(firstLine - 1 < 0) break;
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[i]._vasm[firstLine - 1];
                                    if(savedLD._opcode.find("LD") != std::string::npos)
                                    {
                                        // Discard it's label, (it's no longer needed), and adjust it's address
                                        if(!migrateInternalLabel(i, firstLine - 1, firstLine + 3)) break;
                                        savedLD._internalLabel = "";

                                        // Operand is not a variable, so can't use DOKEA, replace with DOKEI
                                        if(savedLD._opcode == "LDI"  ||  savedLD._opcode == "LDWI")
                                        {
                                            updateVasm(i, firstLine+6, "DOKEI", savedLD._operand);
                                        }
                                        // Replace DOKEA's operand
                                        else
                                        {
                                            updateVasm(i, firstLine+6, "DOKEA", savedLD._operand);
                                        }

                                        // Delete previous line LD<X> and first STW
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine - 1);
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                        adjustLabelAndVasmAddresses(i, firstLine - 1, {savedLD._opcode, "STW"});

                                        firstLine = firstLine - 1;  // points to new first LDW
                                    }
                                    else
                                    {
                                        firstLine = firstLine + 1; // points to first LDW
                                    }

                                    // Now optimise the first LDW and second STW for second phase
                                    Compiler::VasmLine savedLDW = Compiler::getCodeLines()[i]._vasm[firstLine];

                                    // Delete first LDW and second STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);

                                    // Replace operand of both ADDW's
                                    updateVasm(i, firstLine + 1, "ADDW", savedLDW._operand);
                                    updateVasm(i, firstLine + 2, "ADDW", savedLDW._operand);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"LDW", "STW"});
                                }
                                break;

                                // Match MOVQW LDWI ADDI POKEA
                                case MovqwLdwiAddiPokea:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // TODO: maybe add a flag to allow the optimiser to choose from different matching algorithms
                                    // Check that MOVQW and POKEA operands are the same, (don't use optimiser's matching system as it can't handle comma seperated operands)
                                    std::string movqwOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine]._operand);
                                    std::string pokeaOperand = getOperand(Compiler::getCodeLines()[i]._vasm[firstLine + 3]._operand);
                                    if(movqwOperand != pokeaOperand) break;

                                    // Add ADDI's operand to LDWI's operand, update POKEI with MOVQW's immediate operand
                                    std::string ldwiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    std::string addiOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 2]._operand;
                                    std::string immOperand = getOperand(movqwOperand, 1);
                                    updateVasm(i, firstLine + 2, "LDWI", ldwiOperand + " + " + addiOperand);
                                    updateVasm(i, firstLine + 3, "POKEI", immOperand);

                                    // Delete MOVQW and LDWI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"MOVQW", "LDWI"});
                                }
                                break;

                                // Match STW MOVB, (generated by LSL 8)
                                case StwMovb:
                                {
                                    // Bail if wrong ROM version
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                    // Migrate internal label to next available instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine, firstLine + 1)) break;

                                    // Replace MOVB operands
                                    updateVasm(i, firstLine + 1, "MOVB", "giga_vAC, giga_vAC + 1");

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"STW"});
                                }
                                break;

                                case StwPokeArray:
                                case StwDokeArray:
                                {
                                    // Migrate internal label to next available instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine, firstLine + 1)) break;

                                    // Delete first STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"STW"});
                                }
                                break;

                                // LdiSt, (replace with MOVQB in ROMvX0)
                                case LdiSt:
                                {
                                    // Bail if wrong ROM version or if ST has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Bail if next instruction is a ST or STW
                                    if(int(Compiler::getCodeLines()[i]._vasm.size()) > firstLine + 2)
                                    {
                                        std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 2]._opcode;
                                        if(stOpcode == "ST"  ||  stOpcode == "STW") break;
                                    }

                                    // Replace LDI with MOVQB and new operands
                                    std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                    std::string stOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    if(stOperand == "romUser") break; // RomCheck code cannot use new instructions
                                    updateVasm(i, firstLine, "MOVQB", stOperand + ", " + ldOperand);

                                    // Delete ST
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ST"});
                                }
                                break;

                                // LdiStw, (replace with MOVQW in ROMvX0)
                                case LdiStw:
                                {
                                    // Bail if wrong ROM version or if STW has an internal label
                                    if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._internalLabel.size()) break;

                                    // Bail if next instruction is a ST or STW
                                    if(int(Compiler::getCodeLines()[i]._vasm.size()) > firstLine + 2)
                                    {
                                        std::string stOpcode = Compiler::getCodeLines()[i]._vasm[firstLine + 2]._opcode;
                                        if(stOpcode == "ST"  ||  stOpcode == "STW") break;
                                    }

                                    // Replace LDI with MOVQW and new operands
                                    std::string ldOperand = Compiler::getCodeLines()[i]._vasm[firstLine]._operand;
                                    std::string stOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    if(stOperand == "romUser") break; // RomCheck code cannot use new instructions
                                    updateVasm(i, firstLine, "MOVQW", stOperand + ", " + ldOperand);

                                    // Delete STW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"STW"});
                                }
                                break;

                                // Match LD/LDW STW LD STW LDW SUBW
                                case LdSubLoHi:
                                case LdiSubLoHi:
                                case LdwSubLoHi:
                                {
                                    // Save LD<X>
                                    Compiler::VasmLine savedLD = Compiler::getCodeLines()[i]._vasm[firstLine];

                                    // Migrate LD<X>'s label to LD
                                    if(!migrateInternalLabel(i, firstLine, firstLine + 2)) break;
                                    savedLD._internalLabel = "";
                                    savedLD._address += 8; // LD<X> is moved 8 bytes

                                    // Delete LD<X>, first STW and last LDW
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm);
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(itVasm + 2); //points to last LDW, after LD<X> and STW were deleted

                                    // Replace LDW with saved LD<X> and operand
                                    itVasm = Compiler::getCodeLines()[i]._vasm.insert(itVasm, savedLD);
                                    adjustLabelAndVasmAddresses(i, firstLine, {"LD", "STW"});
                                }
                                break;

                                // Match LDI ADDI
                                case LdiAddi:
                                {
                                    uint8_t ldi, addi;

                                    // Migrate ADDI's label to next instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand, addi);

                                    // Result is either an LDI or LDWI
                                    int16_t result = ldi + addi;
                                    if(result >= 0  &&  result <= 255)
                                    {
                                        updateVasm(i, firstLine, "LDI", std::to_string(uint8_t(result)));
                                    }
                                    else
                                    {
                                        updateVasm(i, firstLine, "LDWI", std::to_string(result));
                                    }

                                    // Delete ADDI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ADDI"});
                                }
                                break;

                                // Match LDI SUBI
                                case LdiSubi:
                                {
                                    uint8_t ldi, subi;

                                    // Migrate SUBI's label to next instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand, subi);

                                    // Result is either an LDI or LDWI
                                    int16_t result = ldi - subi;
                                    if(result >= 0  &&  result <= 255)
                                    {
                                        updateVasm(i, firstLine, "LDI", std::to_string(uint8_t(result)));
                                    }
                                    else
                                    {
                                        updateVasm(i, firstLine, "LDWI", std::to_string(result));
                                    }

                                    // Delete SUBI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"SUBI"});
                                }
                                break;

                                // Match LDI ANDI
                                case LdiAndi:
                                {
                                    uint8_t ldi, andi;

                                    // Migrate ANDI's label to next instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand, andi);

                                    // Result is an LDI
                                    uint8_t result = ldi & andi;
                                    updateVasm(i, firstLine, "LDI", Expression::byteToHexString(result));

                                    // Delete ANDI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ANDI"});
                                }
                                break;

                                // Match LDI ORI
                                case LdiOri:
                                {
                                    uint8_t ldi, ori;

                                    // Migrate ORI's label to next instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand, ori);

                                    // Result is an LDI
                                    uint8_t result = ldi | ori;
                                    updateVasm(i, firstLine, "LDI", Expression::byteToHexString(result));

                                    // Delete ORI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ORI"});
                                }
                                break;

                                // Match LDI XORI
                                case LdiXori:
                                {
                                    uint8_t ldi, xori;

                                    // Migrate XORI's label to next instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                    // Operands
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine]._operand, ldi);
                                    Expression::stringToU8(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand, xori);

                                    // Result is an LDI
                                    uint8_t result = ldi ^ xori;
                                    updateVasm(i, firstLine, "LDI", Expression::byteToHexString(result));

                                    // Delete XORI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"XORI"});
                                }
                                break;

                                // Match ADDI ADDI
                                case AddiPair:
                                {
                                    uint8_t addi0, addi1;

                                    // Migrate second ADDI's label to next instruction, (if it exists)
                                    if(!migrateInternalLabel(i, firstLine + 1, firstLine + 2)) break;

                                    // Add operands together, replace first operand, delete second opcode and operand
                                    Compiler::VasmLine vasm = Compiler::getCodeLines()[i]._vasm[firstLine];
                                    std::string operand = vasm._operand;
                                    Expression::stringToU8(operand, addi0);
                                    vasm = Compiler::getCodeLines()[i]._vasm[firstLine + 1];
                                    operand = vasm._operand;
                                    Expression::stringToU8(operand, addi1);

                                    // Result can't fit in an ADDI operand so exit, (ADDI can't be -ve so result can't be -ve)
                                    uint16_t result = addi0 + addi1;
                                    if(result > 255) break;

                                    // Delete second ADDI
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);

                                    // Replace first ADDI's operand with sum of both ADDI's operands
                                    updateVasm(i, firstLine, "ADDI", std::to_string(uint8_t(result)));
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"ADDI"});
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
                                    if(firstLine + 1 >= int(Compiler::getCodeLines()[i]._vasm.size())) break;

                                    // Bail if next instruction is not a %Jump macro
                                    if(Compiler::getCodeLines()[i]._vasm[firstLine + 1]._opcode.substr(0, 10) != "%JumpFalse") break;

                                    // Replace %JumpFalse with JCC and label
                                    std::string tccOpcode = Compiler::getCodeLines()[i]._vasm[firstLine]._opcode;
                                    std::string labOperand = Compiler::getCodeLines()[i]._vasm[firstLine + 1]._operand;
                                    if(!invertCC(tccOpcode)) break;
                                    updateVasm(i, firstLine, "J" + tccOpcode.substr(1), labOperand);

                                    // Delete %JumpFalse
                                    linesDeleted = true;
                                    itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + firstLine + 1);
                                    adjustLabelAndVasmAddresses(i, firstLine + 1, {"%JumpFalse"});
                                }
                                break;

                                default: break;
                            }
                        }

                        // Perform their own opcode and/or operand matching
                        switch(j)
                        {
                            case AddiZero:
                            case SubiZero:
                            {
                                std::string operand;
                                size_t pos = itVasm->_code.find(matchSequences[j]._sequence[0]);
                                if(pos != std::string::npos)
                                {
                                    operand = itVasm->_code.substr(pos + matchSequences[j]._sequence[0].size());
                                    if(operand == "0" || operand == "0x00")
                                    {
                                        // Migrate internal label to next available instruction, (if it exists)
                                        if(!migrateInternalLabel(i, vasmIndex, vasmIndex + 1)) break;

                                        // Delete ADDI/SUBI
                                        linesDeleted = true;
                                        itVasm = Compiler::getCodeLines()[i]._vasm.erase(Compiler::getCodeLines()[i]._vasm.begin() + vasmIndex);
                                        adjustLabelAndVasmAddresses(i, vasmIndex, {"ADDI"});
                                    }
                                }
                            }
                            break;

                            case LdwiNeg:
                            {
                                // Bail if wrong ROM version
                                if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) break;

                                // Operand
                                int16_t ldwi;
                                std::string operand;
                                size_t pos = itVasm->_code.find(matchSequences[j]._sequence[0]);
                                if(pos != std::string::npos)
                                {
                                    operand = itVasm->_code.substr(pos + matchSequences[j]._sequence[0].size());
                                    if(Expression::stringToI16(operand, ldwi))
                                    {
                                        if(ldwi > -256  &&  ldwi < 0)
                                        {
                                            updateVasm(i, vasmIndex, "LDNI", std::to_string(uint8_t(abs(ldwi))));
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