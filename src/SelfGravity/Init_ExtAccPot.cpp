#include "GAMER.h"

#ifdef GRAVITY


// these function pointers must be set by a test problem initializer
void (*Init_ExtAcc_Ptr)( const bool ) = NULL;
void (*Init_ExtPot_Ptr)()             = NULL;




//-------------------------------------------------------------------------------------------------------
// Function    :  Init_ExtAccPot
// Description :  Initialize external acceleration and potential
//
// Note        :  1. Invoked by Init_GAMER() and EvolveLevel()
//                2. Enabled by the runtime options "OPT__EXT_ACC" and "OPT__EXT_POT"
//                3. Function pointers Init_ExtAcc_Ptr and Init_ExtPot_Ptr must be set in advance by a
//                   test problem initializer
//                4. Must invoke either CUAPI_SetConstMemory() or CUAPI_SetConstMemory_ExtAccPot() afterward
//                   to set the GPU constant memory
//
// Parameter   :  OnlySetAuxArray : true:  Set the auxiliary array but NOT the CPU/GPU routines
//                                         --> Used by EvolveLevel()
//                                  false: Set both the auxiliary array and CPU/GPU routines
//                                         --> Used by Init_GAMER()
//
// Return      :  None
//-------------------------------------------------------------------------------------------------------
void Init_ExtAccPot( const bool OnlySetAuxArray )
{

// external acceleration
   if ( OPT__EXT_ACC )
   {
      if ( Init_ExtAcc_Ptr != NULL )   Init_ExtAcc_Ptr( OnlySetAuxArray );
      else                             Aux_Error( ERROR_INFO, "Init_ExtAcc_Ptr == NULL !!\n" );
   }

// external potential
   if ( OPT__EXT_POT )
   {
      if ( Init_ExtPot_Ptr != NULL )   Init_ExtPot_Ptr();
      else                             Aux_Error( ERROR_INFO, "Init_ExtPot_Ptr == NULL !!\n" );
   }

} // FUNCTION : Init_ExtAccPot



#endif // #ifdef GRAVITY
