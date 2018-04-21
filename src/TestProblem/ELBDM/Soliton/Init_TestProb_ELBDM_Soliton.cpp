#include "GAMER.h"
#include "TestProb.h"



// problem-specific global variables
// =======================================================================================
static int      Soliton_N;                               // total number of solitons
static int      Soliton_RSeed;                           // random seed for setting Soliton_Center (<0 --> set manually)
static double   Soliton_FixedScale;                      // fixed scale factor for all solitons (<=0.0 --> set manually)
static double   Soliton_EmptyRegion;                     // soliton-free region from the boundary (useful only when Soliton_RSeed>=0)
static char     Soliton_DensProf_Filename[MAX_STRING];   // filename of the soliton density profile

static int      Soliton_DensProf_NBin;                   // number of radial bins of the soliton density profile
static double  *Soliton_DensProf   = NULL;               // soliton density profile [radius/density]
static double  *Soliton_Scale      = NULL;               // scale factor of each soliton
static double (*Soliton_Center)[3] = NULL;               // center coordinates of each soliton
// =======================================================================================




//-------------------------------------------------------------------------------------------------------
// Function    :  Validate
// Description :  Validate the compilation flags and runtime parameters for this test problem
//
// Note        :  None
//
// Parameter   :  None
//
// Return      :  None
//-------------------------------------------------------------------------------------------------------
void Validate()
{

   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Validating test problem %d ...\n", TESTPROB_ID );


// errors
#  if ( MODEL != ELBDM )
   Aux_Error( ERROR_INFO, "MODEL != ELBDM !!\n" );
#  endif

#  ifndef GRAVITY
   Aux_Error( ERROR_INFO, "GRAVITY must be enabled !!\n" );
#  endif

#  ifdef COMOVING
   Aux_Error( ERROR_INFO, "COMOVING must be disabled !!\n" );
#  endif

#  ifdef PARTICLE
   Aux_Error( ERROR_INFO, "PARTICLE must be disabled !!\n" );
#  endif

#  ifdef GRAVITY
   if ( OPT__BC_POT != BC_POT_ISOLATED )
      Aux_Error( ERROR_INFO, "must adopt isolated BC for gravity --> reset OPT__BC_POT !!\n" );
#  endif


// warnings
   if ( MPI_Rank == 0 )
   {
      if ( !OPT__INIT_RESTRICT )
         Aux_Message( stderr, "WARNING : it's recommended to enable OPT__INIT_RESTRICT !!\n" );
   }


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Validating test problem %d ... done\n", TESTPROB_ID );

} // FUNCTION : Validate



#if ( MODEL == ELBDM  &&  defined GRAVITY )
//-------------------------------------------------------------------------------------------------------
// Function    :  SetParameter
// Description :  Load and set the problem-specific runtime parameters
//
// Note        :  1. Filename is set to "Input__TestProb" by default
//                2. Major tasks in this function:
//                   (1) load the problem-specific runtime parameters
//                   (2) set the problem-specific derived parameters
//                   (3) reset other general-purpose parameters if necessary
//                   (4) make a note of the problem-specific parameters
//
// Parameter   :  None
//
// Return      :  None
//-------------------------------------------------------------------------------------------------------
void SetParameter()
{

   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Setting runtime parameters ...\n" );


// (1) load the problem-specific runtime parameters
   const char FileName[] = "Input__TestProb";
   ReadPara_t *ReadPara  = new ReadPara_t;

// (1-1) add parameters in the following format:
// --> note that VARIABLE, DEFAULT, MIN, and MAX must have the same data type
// --> some handy constants (e.g., Useless_bool, Eps_double, NoMin_int, ...) are defined in "include/ReadPara.h"
// ********************************************************************************************************************************
// ReadPara->Add( "KEY_IN_THE_FILE",           &VARIABLE,                   DEFAULT,       MIN,              MAX               );
// ********************************************************************************************************************************
   ReadPara->Add( "Soliton_N",                 &Soliton_N,                 -1,             1,                NoMax_int         );
   ReadPara->Add( "Soliton_RSeed",             &Soliton_RSeed,              0,             NoMin_int,        NoMax_int         );
   ReadPara->Add( "Soliton_FixedScale",        &Soliton_FixedScale,         1.0,           NoMin_double,     NoMax_double      );
   ReadPara->Add( "Soliton_EmptyRegion",       &Soliton_EmptyRegion,        0.0,           NoMin_double,     NoMax_double      );
   ReadPara->Add( "Soliton_DensProf_Filename",  Soliton_DensProf_Filename,  Useless_str,   Useless_str,      Useless_str       );

   ReadPara->Read( FileName );

   delete ReadPara;

// (1-2) set the default values

// (1-3) check the runtime parameters
   if ( Soliton_RSeed >= 0  &&  Soliton_EmptyRegion < 0.0 )
      Aux_Error( ERROR_INFO, "Soliton_EmptyRegion (%14.7e) < 0.0 !!\n", Soliton_EmptyRegion );


// (2) set the problem-specific derived parameters
// (2-1) allocate memory
   Soliton_Scale  = new double [Soliton_N];
   Soliton_Center = new double [Soliton_N][3];

// (2-2) soliton scale factors
   if ( Soliton_FixedScale > 0.0 )
   {
      for (int t=0; t<Soliton_N; t++)  Soliton_Scale[t] = Soliton_FixedScale;
   }

   else
   {
//    for Soliton_FixedScale <= 0.0, comment out the following line and hard code the scale factor of each soliton
      Aux_Error( ERROR_INFO, "for Soliton_FixedScale <= 0.0, please comment out this error check and hard code "
                             "the scale factor of each soliton !!\n" );
//    for (int t=0; t<Soliton_N; t++)  Soliton_Scale[t] = XXX;
   }

// (2-3) soliton centers
   if ( Soliton_RSeed >= 0 )
   {
      const double Coord_Min[3] = { Soliton_EmptyRegion, Soliton_EmptyRegion, Soliton_EmptyRegion };
      const double Coord_Max[3] = { amr->BoxSize[0] - Soliton_EmptyRegion,
                                    amr->BoxSize[1] - Soliton_EmptyRegion,
                                    amr->BoxSize[2] - Soliton_EmptyRegion };
      srand( Soliton_RSeed );

      for (int t=0; t<Soliton_N; t++)
      for (int d=0; d<3; d++)
         Soliton_Center[t][d] = ( (double)rand()/RAND_MAX )*(Coord_Max[d]-Coord_Min[d]) + Coord_Min[d];
   }

   else
   {
      if ( Soliton_N == 1 )
      {
         for (int d=0; d<3; d++)    Soliton_Center[0][d] = 0.5*amr->BoxSize[d];
      }

      else
      {
//       for Soliton_RSeed<0, comment out the following line and hard code the center of each soliton
         Aux_Error( ERROR_INFO, "for Soliton_RSeed < 0 and Soliton_N > 1, please comment out this error check and hard code "
                                "the center of each soliton !!\n" );

         /*
         for (int t=0; t<Soliton_N; t++)
         for (int d=0; d<3; d++)          Soliton_Center[t][d] = XXX;
         */
      }
   } // if ( Soliton_RSeed >= 0 ) ... else ...


// (3) load the soliton density profile
   if ( OPT__INIT != INIT_BY_RESTART )
   {
      const bool RowMajor_No  = false;    // load data into the column-major order
      const bool AllocMem_Yes = true;     // allocate memory for Soliton_DensProf
      const int  NCol         = 2;        // total number of columns to load
      const int  Col[NCol]    = {0, 1};   // target columns: (radius, density)

      Soliton_DensProf_NBin = Aux_LoadTable( Soliton_DensProf, Soliton_DensProf_Filename, NCol, Col, RowMajor_No, AllocMem_Yes );
   }


// (4) reset other general-purpose parameters
//     --> a helper macro PRINT_WARNING is defined in TestProb.h
   const long   End_Step_Default = __INT_MAX__;
   const double End_T_Default    = 1.0e3;

   if ( END_STEP < 0 ) {
      END_STEP = End_Step_Default;
      PRINT_WARNING( "END_STEP", END_STEP, FORMAT_LONG );
   }

   if ( END_T < 0.0 ) {
      END_T = End_T_Default;
      PRINT_WARNING( "END_T", END_T, FORMAT_REAL );
   }


// (5) make a note
   if ( MPI_Rank == 0 )
   {
      Aux_Message( stdout, "=============================================================================\n" );
      Aux_Message( stdout, "  test problem ID                           = %d\n",     TESTPROB_ID                );
      Aux_Message( stdout, "  total number of solitons                  = %d\n",     Soliton_N                  );
      Aux_Message( stdout, "  random seed for setting the center coord. = %d\n",     Soliton_RSeed              );
      Aux_Message( stdout, "  size of the soliton-free zone             = %13.7e\n", Soliton_EmptyRegion        );
      Aux_Message( stdout, "  density profile filename                  = %s\n",     Soliton_DensProf_Filename  );
      Aux_Message( stdout, "  number of bins of the density profile     = %d\n",     Soliton_DensProf_NBin      );
      Aux_Message( stdout, "\n" );
      Aux_Message( stdout, "  Soliton info:\n" );
      Aux_Message( stdout, "  %7s  %13s  %13s  %13s  %13s\n", "ID", "Scale", "Center_X", "Center_Y", "Center_Z" );
      for (int t=0; t<Soliton_N; t++)
      Aux_Message( stdout, "  %7d  %13.6e  %13.6e  %13.6e  %13.6e\n",
                   t, Soliton_Scale[t], Soliton_Center[t][0], Soliton_Center[t][1], Soliton_Center[t][2] );
      Aux_Message( stdout, "=============================================================================\n" );
   }


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "   Setting runtime parameters ... done\n" );

} // FUNCTION : SetParameter



//-------------------------------------------------------------------------------------------------------
// Function    :  SetGridIC
// Description :  Set the problem-specific initial condition on grids
//
// Note        :  1. This function may also be used to estimate the numerical errors when OPT__OUTPUT_USER is enabled
//                   --> In this case, it should provide the analytical solution at the given "Time"
//                2. This function will be invoked by multiple OpenMP threads when OPENMP is enabled
//                   --> Please ensure that everything here is thread-safe
//
// Parameter   :  fluid    : Fluid field to be initialized
//                x/y/z    : Physical coordinates
//                Time     : Physical time
//                lv       : Target refinement level
//                AuxArray : Auxiliary array
//
// Return      :  fluid
//-------------------------------------------------------------------------------------------------------
void SetGridIC( real fluid[], const double x, const double y, const double z, const double Time,
                const int lv, double AuxArray[] )
{

   const double *Table_Radius  = Soliton_DensProf + 0*Soliton_DensProf_NBin;  // radius
   const double *Table_Density = Soliton_DensProf + 1*Soliton_DensProf_NBin;  // density

   double r, Dens;


// initialize density as zero since there may be multiple solitons
   fluid[DENS] = 0.0;

// loop over all solitons to get the total density
   for (int t=0; t<Soliton_N; t++)
   {
      r = sqrt( SQR(x-Soliton_Center[t][0]) + SQR(y-Soliton_Center[t][1]) + SQR(z-Soliton_Center[t][2]) );

//    rescale radius (~scale^-1)
//    --> we rescale "r" instead of "Table_Radius" just for convenience
      r *= Soliton_Scale[t];

//    linear interpolation
      Dens = Mis_InterpolateFromTable( Soliton_DensProf_NBin, Table_Radius, Table_Density, r );

      if ( Dens == NULL_REAL )
      {
         if      ( r <  Table_Radius[0] )                         Dens = Table_Density[0];
         else if ( r >= Table_Radius[Soliton_DensProf_NBin-1] )   Dens = Table_Density[Soliton_DensProf_NBin-1];
         else
            Aux_Error( ERROR_INFO, "interpolation failed at radius %13.7e (min/max radius = %13.7e/%13.7e) !!\n",
                       r, Table_Radius[0], Table_Radius[Soliton_DensProf_NBin-1] );
      }

//    rescale density (~scale^4) and add to the fluid array
      fluid[DENS] += Dens*SQR( Soliton_Scale[t] )*SQR( Soliton_Scale[t] );
   } // for (int t=0; t<Soliton_N; t++)


// set the real and imaginary parts
   fluid[REAL] = sqrt( fluid[DENS] );
   fluid[IMAG] = 0.0;                  // imaginary part is always zero --> no initial velocity

} // FUNCTION : SetGridIC



//-------------------------------------------------------------------------------------------------------
// Function    :  End_Soliton
// Description :  Free memory before terminating the program
//
// Note        :  1. Linked to the function pointer "End_User_Ptr" to replace "End_User()"
//
// Parameter   :  None
//-------------------------------------------------------------------------------------------------------
void End_Soliton()
{

   delete [] Soliton_DensProf;
   delete [] Soliton_Scale;
   delete [] Soliton_Center;

} // FUNCTION : End_Soliton



//-------------------------------------------------------------------------------------------------------
// Function    :  BC
// Description :  Set the extenral boundary condition
//
// Note        :  1. Linked to the function pointer "BC_User_Ptr"
//
// Parameter   :  fluid    : Fluid field to be set
//                x/y/z    : Physical coordinates
//                Time     : Physical time
//                lv       : Refinement level
//                AuxArray : Auxiliary array
//
// Return      :  fluid
//-------------------------------------------------------------------------------------------------------
void BC( real fluid[], const double x, const double y, const double z, const double Time,
         const int lv, double AuxArray[] )
{

   fluid[REAL] = (real)0.0;
   fluid[IMAG] = (real)0.0;
   fluid[DENS] = (real)0.0;

} // FUNCTION : BC
#endif // #if ( MODEL == ELBDM  &&  defined GRAVITY )



//-------------------------------------------------------------------------------------------------------
// Function    :  Init_TestProb_ELBDM_Soliton
// Description :  Test problem initializer
//
// Note        :  None
//
// Parameter   :  None
//
// Return      :  None
//-------------------------------------------------------------------------------------------------------
void Init_TestProb_ELBDM_Soliton()
{

   if ( MPI_Rank == 0 )    Aux_Message( stdout, "%s ...\n", __FUNCTION__ );


// validate the compilation flags and runtime parameters
   Validate();


#  if ( MODEL == ELBDM  &&  defined GRAVITY )
// set the problem-specific runtime parameters
   SetParameter();


   Init_Function_User_Ptr   = SetGridIC;
   Flag_User_Ptr            = NULL;
   Mis_GetTimeStep_User_Ptr = NULL;
   BC_User_Ptr              = BC;
   Flu_ResetByUser_Func_Ptr = NULL;
   Output_User_Ptr          = NULL;
   Aux_Record_User_Ptr      = NULL;
   End_User_Ptr             = End_Soliton;
   Init_ExternalAcc_Ptr     = NULL;
   Init_ExternalPot_Ptr     = NULL;
#  endif // #if ( MODEL == ELBDM  &&  defined GRAVITY )


   if ( MPI_Rank == 0 )    Aux_Message( stdout, "%s ... done\n", __FUNCTION__ );

} // FUNCTION : Init_TestProb_ELBDM_Soliton
