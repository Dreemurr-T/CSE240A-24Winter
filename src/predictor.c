//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Haoyuan Tian";
const char *studentID = "A59024246";
const char *email = "h2tian@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;
int gsBits; // Custom G-share bits

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// G-share
uint16_t gHistoryReg;   // global history register
uint8_t *pHistoryTable; // pattern history table

// Tournament
uint8_t *gPredictionTable; // global prediction table
uint16_t *lHistoryTable;   // local history table
uint8_t *lPredictionTable; // local prediction table
uint8_t *cPredictionTable; // choice prediction table

// Custom
uint8_t *gsTable;
uint16_t gsHistoryReg;
uint8_t **sCounters;
int predictorNum = 3;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void init_predictor()
{
  //
  // TODO: Initialize Branch Predictor Data Structures
  //
  switch (bpType)
  {
  case STATIC:
    return;
  case GSHARE:
  {
    gHistoryReg = 0;
    int phtSize = 1 << ghistoryBits;
    pHistoryTable = (uint8_t *)malloc(phtSize * sizeof(2));
    for (int i = 0; i < phtSize; i++)
    {
      pHistoryTable[i] = WN;
    }
    break;
  }
  case TOURNAMENT:
  {
    gHistoryReg = 0;
    int gSize = 1 << ghistoryBits;
    int lSize = 1 << lhistoryBits;
    int pcSize = 1 << pcIndexBits;

    gPredictionTable = (uint8_t *)malloc(gSize * sizeof(uint8_t));
    cPredictionTable = (uint8_t *)malloc(gSize * sizeof(uint8_t));
    lHistoryTable = (uint16_t *)malloc(pcSize * sizeof(uint16_t));
    lPredictionTable = (uint8_t *)malloc(lSize * sizeof(uint8_t));

    for (int i = 0; i < gSize; i++)
    {
      gPredictionTable[i] = WN;
      cPredictionTable[i] = 1;
    }

    for (int i = 0; i < pcSize; i++)
    {
      lHistoryTable[i] = 0;
    }

    for (int i = 0; i < lSize; i++)
    {
      lPredictionTable[i] = WN;
    }
    break;
  }
  case CUSTOM:
  {
    gHistoryReg = 0;  // history for global prediction table
    gsHistoryReg = 0; // history for g-share prediction table
    int gSize = 1 << ghistoryBits;
    int lSize = 1 << lhistoryBits;
    int pcSize = 1 << pcIndexBits;
    int gsSize = 1 << gsBits;

    gPredictionTable = (uint8_t *)malloc(gSize * sizeof(uint8_t));
    // cPredictionTable = (uint8_t *)malloc(gSize * sizeof(uint8_t));
    lHistoryTable = (uint16_t *)malloc(pcSize * sizeof(uint16_t));
    lPredictionTable = (uint8_t *)malloc(lSize * sizeof(uint8_t));
    gsTable = (uint8_t *)malloc(gsSize * sizeof(uint8_t));
    sCounters = (uint8_t **)malloc(pcSize * sizeof(uint8_t *));

    for (int i = 0; i < gSize; i++)
    {
      gPredictionTable[i] = WN;
    }

    for (int i = 0; i < pcSize; i++)
    {
      lHistoryTable[i] = 0;
      sCounters[i] = (uint8_t *)malloc(predictorNum * sizeof(uint8_t));
      for (int j = 0; j < predictorNum; j++)
        sCounters[i][j] = 3;
    }

    for (int i = 0; i < lSize; i++)
    {
      lPredictionTable[i] = WN;
    }

    for (int i = 0; i < gsSize; i++)
    {
      gsTable[i] = WN;
    }

    break;
  }
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  // TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
  {
    uint32_t phtIndex = (pc & ((1 << ghistoryBits) - 1)) ^ gHistoryReg;
    uint8_t prediction = pHistoryTable[phtIndex];
    return (prediction >> 1); // Return the MSB for prediction (TAKEN or NOTTAKEN)
  }
  case TOURNAMENT:
  {
    uint8_t choice = cPredictionTable[gHistoryReg];
    if (choice <= 1)
    {
      return (gPredictionTable[gHistoryReg] >> 1);
    }
    else
    {
      uint16_t lHisIndex = pc & ((1 << pcIndexBits) - 1);
      uint16_t lPreIndex = lHistoryTable[lHisIndex];
      return (lPredictionTable[lPreIndex] >> 1);
    }
    break;
  }
  case CUSTOM:
  {
    uint16_t lHisIndex = pc & ((1 << pcIndexBits) - 1);
    uint16_t lPreIndex = lHistoryTable[lHisIndex];

    uint16_t gsIndex = (pc ^ gsHistoryReg) & ((1 << gsBits) - 1);

    uint8_t lPrediction = lPredictionTable[lPreIndex] >> 1;
    uint8_t gPrediction = gPredictionTable[gHistoryReg] >> 1;
    uint8_t gsPrediction = gsTable[gsIndex] >> 1;
    
    uint8_t choice = 0;

    for (int i=0; i<predictorNum; i++) {
      if (sCounters[lHisIndex][i] == 3) {
        choice = i;
        break;
      }
    }

    if (choice == 0)
      return gPrediction;
    else if (choice == 1)
      return gsPrediction;
    else if (choice == 2) 
      return lPrediction;
    else
      return NOTTAKEN;

    break;
  }
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  // TODO: Implement Predictor training
  //
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
  {
    uint32_t phtIndex = (pc & ((1 << ghistoryBits) - 1)) ^ gHistoryReg;
    uint8_t pattern = pHistoryTable[phtIndex];

    // Update the PHT based on the outcome
    if (outcome == TAKEN && pattern < ST)
    {
      pHistoryTable[phtIndex]++;
    }
    else if (outcome == NOTTAKEN && pattern > SN)
    {
      pHistoryTable[phtIndex]--;
    }

    // Update the global history register
    gHistoryReg = ((gHistoryReg << 1) | outcome) & ((1 << ghistoryBits) - 1);
    break;
  }
  case TOURNAMENT:
  {
    uint16_t lHisIndex = pc & ((1 << pcIndexBits) - 1);
    uint16_t lPreIndex = lHistoryTable[lHisIndex];

    uint8_t lPrediction = lPredictionTable[lPreIndex] >> 1;
    uint8_t gPrediction = gPredictionTable[gHistoryReg] >> 1;

    if (outcome == TAKEN)
    {
      if (lPredictionTable[lPreIndex] < ST)
      {
        lPredictionTable[lPreIndex]++;
      }
      if (gPredictionTable[gHistoryReg] < ST)
      {
        gPredictionTable[gHistoryReg]++;
      }
    }
    else if (outcome == NOTTAKEN)
    {
      if (lPredictionTable[lPreIndex] > SN)
      {
        lPredictionTable[lPreIndex]--;
      }
      if (gPredictionTable[gHistoryReg] > SN)
      {
        gPredictionTable[gHistoryReg]--;
      }
    }

    if (lPrediction == outcome && gPrediction != outcome)
    {
      if (cPredictionTable[gHistoryReg] < 3)
        cPredictionTable[gHistoryReg]++;
    }
    else if (lPrediction != outcome && gPrediction == outcome)
    {
      if (cPredictionTable[gHistoryReg] > 0)
        cPredictionTable[gHistoryReg]--;
    }

    lHistoryTable[lHisIndex] = ((lPreIndex << 1) | outcome) & ((1 << lhistoryBits) - 1);
    gHistoryReg = ((gHistoryReg << 1) | outcome) & ((1 << ghistoryBits) - 1);

    break;
  }
  case CUSTOM:
  {
    uint16_t lHisIndex = pc & ((1 << pcIndexBits) - 1);
    uint16_t lPreIndex = lHistoryTable[lHisIndex];
    uint16_t gsIndex = (pc ^ gsHistoryReg) & ((1 << gsBits) - 1);

    uint8_t gPrediction = gPredictionTable[gHistoryReg] >> 1;
    uint8_t lPrediction = lPredictionTable[lPreIndex] >> 1;
    uint8_t gsPrediction = gsTable[gsIndex] >> 1;

    if (outcome == TAKEN)
    {
      if (lPredictionTable[lPreIndex] < ST)
      {
        lPredictionTable[lPreIndex]++;
      }
      if (gPredictionTable[gHistoryReg] < ST)
      {
        gPredictionTable[gHistoryReg]++;
      }
      if (gsTable[gsIndex] < ST)
      {
        gsTable[gsIndex]++;
      }
    }
    else if (outcome == NOTTAKEN)
    {
      if (lPredictionTable[lPreIndex] > SN)
      {
        lPredictionTable[lPreIndex]--;
      }
      if (gPredictionTable[gHistoryReg] > SN)
      {
        gPredictionTable[gHistoryReg]--;
      }
      if (gsTable[gsIndex] > SN)
      {
        gsTable[gsIndex]--;
      }
    }

    uint8_t gCounter = sCounters[lHisIndex][0];
    uint8_t gsCounter = sCounters[lHisIndex][1];
    uint8_t lCounter = sCounters[lHisIndex][2];

    if ((gPrediction == outcome && gCounter == 3) || (gsPrediction == outcome && gsCounter == 3) || (lPrediction == outcome && lCounter == 3)) {
      if (gPrediction != outcome && gCounter > 0) sCounters[lHisIndex][0]--;
      if (gsPrediction != outcome && gsCounter > 0) sCounters[lHisIndex][1]--;
      if (lPrediction != outcome && lCounter > 0) sCounters[lHisIndex][2]--; 
    }
    else {
      if (gPrediction == outcome) sCounters[lHisIndex][0]++;
      if (gsPrediction == outcome) sCounters[lHisIndex][1]++;
      if (lPrediction == outcome) sCounters[lHisIndex][2]++; 
    }

    lHistoryTable[lHisIndex] = ((lPreIndex << 1) | outcome) & ((1 << lhistoryBits) - 1);
    gHistoryReg = ((gHistoryReg << 1) | outcome) & ((1 << ghistoryBits) - 1);
    gsHistoryReg = ((gsHistoryReg << 1) | outcome) & ((1 << gsBits) - 1);

    break;
  }
  default:
    break;
  }
}
