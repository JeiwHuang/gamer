#include "GAMER.h"

#if ( MODEL == ELBDM )




//-------------------------------------------------------------------------------------------------------
// Function    :  ELBDM_Flag_EngyDensity 
// Description :  Flag according to the energy density criterion 
//
// Note        :  1. Flag the input cell if its energy density exceeds Angle^2  
//                2. A soften factor "Eps" is added to the denominator when evaluating energy density 
//                   to prevent flagging in the extremely low density region
//                3. Size of the input arrays "Real & Imag" should be PATCH_SIZE^3 
//
// Parameter   :  i,j,k       : Indices of the target cell in the arrays "Real & Imag"
//                Real_Array  : Real     -part array of the target patch
//                Imag_Array  : Imaginary-part array of the target patch
//                Angle_2pi   : Angle/2/PI (refinement threshold = Angle^2)
//                Eps         : Soften factor
//
// Return      :  "true"  if the flag criterion is     fulfilled
//                "false" if the flag criterion is NOT fulfilled
//-------------------------------------------------------------------------------------------------------
bool ELBDM_Flag_EngyDensity( const int i, const int j, const int k, const real Real1_Array[], 
                             const real Imag1_Array[], const real Real2_Array[], const real Imag2_Array[], const double Angle_2pi, const double Eps )
{

// check
#  ifdef GAMER_DEBUG
   if (  i < 0  ||  i >= PS1  ||  j < 0  ||  j >= PS1  ||  k < 0  ||  k >= PS1  )
      Aux_Error( ERROR_INFO, "incorrect index (i,j,k) = (%d,%d,%d) !!\n", i, j, k );
#  endif


   const double Angle     = Angle_2pi*2.0*M_PI;
   const double Threshold = Angle*Angle;
   const int    ijk[3]    = { i, j, k };
   const int    Idx       = k*PS1*PS1 + j*PS1 + i;
   const int    dIdx[3]   = { 1, PS1, PS1*PS1 };
   const real   Real1     = Real1_Array[Idx];
   const real   Imag1     = Imag1_Array[Idx];
   const real   Real2     = Real2_Array[Idx];
   const real   Imag2     = Imag2_Array[Idx];
   const real   Deno1     = Real1*Real1 + Imag1*Imag1 + Eps;
   const real   Deno2     = Real2*Real2 + Imag2*Imag2 + Eps;

   int  Idx_p, Idx_m;
   real _dh, Grad_Real1[3], Grad_Imag1[3], Grad_Real2[3], Grad_Imag2[3], Nume1, Nume2, EngyDensity;
   bool Flag;


// evaluate gradients
   for (int d=0; d<3; d++)
   {
      switch ( ijk[d] )
      {
         case 0     : Idx_m = Idx;           Idx_p = Idx+dIdx[d];    _dh = (real)1.0;  break;
         case PS1-1 : Idx_m = Idx-dIdx[d];   Idx_p = Idx;            _dh = (real)1.0;  break;
         default    : Idx_m = Idx-dIdx[d];   Idx_p = Idx+dIdx[d];    _dh = (real)0.5;  break;
      } 

      Grad_Real1[d] = _dh*( Real1_Array[Idx_p] - Real1_Array[Idx_m] );
      Grad_Imag1[d] = _dh*( Imag1_Array[Idx_p] - Imag1_Array[Idx_m] );
      Grad_Real2[d] = _dh*( Real2_Array[Idx_p] - Real2_Array[Idx_m] );
      Grad_Imag2[d] = _dh*( Imag2_Array[Idx_p] - Imag2_Array[Idx_m] );

   } // for (int d=0; d<3; d++)


// evaluate energy density and check the flag criterion
   Nume1        = Grad_Real1[0]*Grad_Real1[0] + Grad_Real1[1]*Grad_Real1[1] + Grad_Real1[2]*Grad_Real1[2] +
                 Grad_Imag1[0]*Grad_Imag1[0] + Grad_Imag1[1]*Grad_Imag1[1] + Grad_Imag1[2]*Grad_Imag1[2] ;
   Nume2        = Grad_Real2[0]*Grad_Real2[0] + Grad_Real2[1]*Grad_Real2[1] + Grad_Real2[2]*Grad_Real2[2] +
                 Grad_Imag2[0]*Grad_Imag2[0] + Grad_Imag2[1]*Grad_Imag2[1] + Grad_Imag2[2]*Grad_Imag2[2];
   EngyDensity = Nume1/Deno1 + Nume2/Deno2;
   Flag        = EngyDensity > Threshold;

   return Flag;

} // FUNCTION : ELBDM_Flag_EngyDensity



#endif // #if ( MODEL == ELBDM )
