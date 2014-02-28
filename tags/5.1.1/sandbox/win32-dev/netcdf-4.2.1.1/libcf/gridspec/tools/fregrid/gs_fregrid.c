#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include "globals.h"
#include "constant.h"
#include "read_mosaic.h"
#include "mpp_io.h"
#include "conserve_interp.h"
#include "bilinear_interp.h"
#include "fregrid_util.h"

char *usage[] = {
   "",
   "  fregrid --input_mosaic input_mosaic --input_file input_file                         ",
   "          [--scalar_field scalar_fld] [--u_field u_fld]  [--v_field v_fld]            ",
   "          [--output_mosaic output_mosaic] [--lonBegin #decimal] [--lonEnd #decimal]   ",
   "          [--latBegin #decimal] [--latEnd #decimal] [--nlon #integer]                 ",
   "          [--nlat #integer] [--KlevelBegin #integer] [--KlevelEnd #integer]           ",
   "          [--LstepBegin #integer] [--LstepEnd #integer]                               ",
   "          [--output_file output_file] [--input_dir input_dir]                         ",
   "          [--output_dir output_dir] [--remap_file remap_file]                         ",
   "          [--interp_method method] [--grid_type grid_type] [--test_case test_case]    ",
   "          [--symmetry] [--target_grid] [--finer_step #] [--fill_missing]              ",
   "          [--center_y] [--check_conserve]                                             ",
   "fregrid remap data (scalar or vector) from input_mosaic onto output_mosaic (The       ",
   "target grid also could be specified through lonBegin, lonEnd, latBegin, latEnd, nlon  ",
   "and nlat). Currently only T-cell scalar regridding and AGRID vector regridding (only  ",
   "bilinear interpolation is implemented for cubic grid vector interpolation). The       ",
   "interpolation algorithm used is controlled by --interp_method with default            ",
   "'conserve_order1'. Currently only 'conserve_order1', 'conserve_order2' and 'bilinear' ",
   "remapping scheme are implemented. 'bilinear' is only used to remap data from cubic    ",
   "grid to latlon grid. We will add more scheme in the future if needed. fregrid expects ",
   "NetCDF format input. scalar_field and/or u_field/v_field must be specified. u_fld and ",
   "v_fld must be paired together.    ",
   "                                                                                      ",
   "fregrid takes the following flags:                                                    ",
   "                                                                                      ",
   "REQUIRED:                                                                             ",
   "                                                                                      ",
   "--input_mosaic  input_mosaic  specify the input mosaic information. This file         ",
   "                              contains list of tile files which specify the grid      ",
   "                              information for each tile.                              ",
   "                                                                                      ",
   "OPTIONAL FLAGS                                                                        ",
   "                                                                                      ",  
   "--input_file    input_file    specify the input file name. The suffix '.nc' can be    ",
   "                              omitted. The suffix 'tile#' should not present for      ",
   "                              multiple-tile files. The number of files must be 1 for  ",
   "                              scalar regridding and can be 1 or 2 for vector          ",
   "                              regridding. File path should not be includes.           ",
   "                                                                                      ",
   "--scalar_field    scalar_fld  specify the scalar field name to be regridded. The      ",
   "                              multiple entry field names are seperated by comma.      ",
   "                                                                                      ",    
   "--u_field         u_fld       specify the vector field u-componentname to be          ",
   "                              regridded. The multiple entry field names are seperated ",
   "                              by comma. u_field must be paired together with v_field. ",
   "                                                                                      ",    
   "--v_field         v_fld       specify the vector field v-componentname to be          ",
   "                              regridded. The multiple entry field names are seperated ",
   "                              by comma. v_field must be paired together with u_field. ",
   "                                                                                      ",  
   "--output_mosaic output_mosaic specify the output mosaic information. This file        ",
   "                              contains list of tile files which specify the grid      ",
   "                              information for each tile. If output_mosaic is not      ",
   "                              specified, nlon and nlat must be specified.             ",
   "                                                                                      ",
   "--lonBegin  #decimal          specify the starting longitude(in degree) of the        ",
   "                              geographical region of the target grid on which the     ",
   "                              output is desired. The default value is 0.              ",
   "                                                                                      ",
   "--lonEnd   #decimal           specify the ending longitude(in degree) of the          ",
   "                              geographical region of the target grid on which the     ",
   "                              output is desired. The default value is 360.            ",
   "                                                                                      ",
   "--latBegin  #decimal          specify the starting latitude(in degree) of the         ",
   "                              geographical region of the target grid on which the     ",
   "                              output is desired. The default value is -90.            ",
   "                                                                                      ",
   "--latEnd   #decimal           specify the ending latitude(in degree) of the           ",
   "                              geographical region of the target grid on which the     ",
   "                              output is desired. The default value is 90.             ",  
   "                                                                                      ",
   "--nlon #integer               specify number of grid box cells in x-direction for a   ",
   "                              regular lat-lon grid.                                   ",
   "                                                                                      ",
   "--nlat #integer               specify number of grid box cells in y-direction for a   ",
   "                              regular lat-lon grid.                                   ",
   "                                                                                      ",  
   "--KlevelBegin #integer        specify begin index of the k-level (depth axis) that    ",
   "                              to be regridded.                                        ",
   "                                                                                      ",
   "--KlevelEnd #integer          specify end index of the k-level (depth axis) that      ",
   "                              to be regridded.                                        ",  
   "                                                                                      ",
   "--LstepBegin #integer         specify the begin index of L-step (time axis) that      ",
   "                              to be regridded.                                        ",
   "                                                                                      ",
   "--LstepEnd #integer           specify the end index of L-step (time axis) that        ",
   "                              to be regridded.                                        ",  
   "                                                                                      ",  
   "--output_file   output_file   specify the output file name. If not presented,         ",
   "                              output_file will take the value of input_file. The      ",
   "                              suffix '.nc' can be omitted. The suffix 'tile#' should  ",
   "                              not present for multiple-tile files. The number of      ",
   "                              files must be 1 for scalar regridding and can be 1 or 2 ",
   "                              for vector regridding. File path should not be includes.",  
   "                                                                                      ",
   "--input_dir     input_dir     specify the path that stores input_file. If not         ",
   "                              presented, the input file is assumed to be stored in    ",
   "                              current diretory.                                       ",
   "                                                                                      ",
   "--output_dir   output_dir     specify the path that will store output file. If not    ",
   "                              presented, the output file will be stored in current    ",
   "                              diretory.                                               ",
   "                                                                                      ",
   "--remap_file   remap_file     specify the file name that saves remapping information. ",
   "                              If remap_file is specified and the file does not exist, ",
   "                              remapping information will be calculated ans stored in  ",
   "                              remap_file. If remap_file is specified and the file     ",
   "                              exists, remapping information will be read from         ",
   "                              remap_file.                                             ",
   "                                                                                      ",
   "--interp_method interp_method specify the remapping algorithm to be used. Default is  ",
   "                              'conserve_order1'. Currently only 'conserve_order1',    ",
   "                              'conserve_order2' and 'bilinear' remapping scheme are   ",
   "                              implemented in this tool. The bilinear scheme can only  ",
   "                              be used to remap data from cubic grid to regular latlon ",
   "                              grid. When interp_method is 'bilinear', nlon and nlat   ",
   "                              must be specified and the output data in y-direction    ",
   "                              will be located at the center of cell or bound of the   ",
   "                              cell depending on the setting of y_center.              ",
   "                                                                                      ",
   "--test_case test_case         specify the test function to be used for testing.       ",
   "                                                                                      ",
   "--grid_type     grid_type     specify the vector field grid location. default is      ",
   "                              AGRID and only AGRID is implemented yet.                ",
   "                                                                                      ",
   "--symmetry                    indicate the grid is symmetry or not.                   ",
   "                                                                                      ",
   "--target_grid                 use input cell area instead of calculating based on     ",
   "                              exchange grid area. default is off.                     ",
   "                                                                                      ",
   "---finer_step #integer        This is used only for bilinear interpolation. Set       ",
   "                              finer_step to a positive integer to reduce noise in     ",
   "                              interpolation and get a relatively smooth output. The   ",
   "                              default value is 0. When finer_step is greater than 0,  ",
   "                              fregrid will first remap data from source grid onto a   ",
   "                              finer grid with resolution that is power of 2 of        ",
   "                              destination grid resolution using bilinear              ",
   "                              interpolation, then using volume averaging to remap     ",
   "                              data from finer grid onto destination grid.             ",
   "                                                                                      ",
   "--center_y                    output latitude will locate at cell center, i.e., the   ",
   "                              starting latitude will be -89 when nlat = 90. when      ",
   "                              center_y is not set, starting latitude will be -90. for ",
   "                              bilinear interpolation. For conservative interpolation, ",
   "                              center_y is assumed.                                    ",
   "                                                                                      ",
   "--check_conserve              check the conservation of conservative interpolation.   ",
   "                              The area sum will be printed out for input and output   ",
   "                              mosaic.                                                 ",
   "                                                                                      ",
   "  fregrid --input_mosaic input_mosaic.nc --output_mosaic output_mosaic.nc             ",
   "          --input_dir input_dir --input_file input_file --scalar_field temp,salt      ",
   "                                                                                      ",
   NULL};

const double D2R = M_PI/180.;
char tagname[] = "$Name:  $";

int main(int argc, char* argv[])
{
   unsigned int opcode = 0;
   char    *mosaic_in=NULL;            /* input mosaic file name */
   char    *mosaic_out=NULL;           /* input mosaic file name */
   char    *dir_in=NULL;               /* input file location */
   char    *dir_out=NULL;              /* output file location */
   int     ntiles_in = 0;              /* number of tiles in input mosaic */
   int     ntiles_out = 0;             /* number of tiles in output mosaic */
   int     nfiles     = 0;              /* number of input file */
   int     nfiles_out = 0;             /* number of output file */
   char    input_file [NFILE][STRING];
   char    output_file[NFILE][STRING];
   char    scalar_name[NVAR] [STRING];
   char    u_name     [NVAR] [STRING];
   char    v_name     [NVAR] [STRING];
   char    *test_case = NULL;
   double  test_param = 1;
   int     check_conserve = 0; /* 0 means no check */
   double  lonbegin = 0, lonend = 360;
   double  latbegin = -90, latend = 90;			  
   int     nlon = 0, nlat = 0;
   int     kbegin = 0, kend = -1; 
   int     lbegin = 0, lend = -1;
   char    *remap_file = NULL;
   char    interp_method[STRING] = "conserve_order1";
   int     y_at_center = 0;
   int     grid_type = AGRID;
   int     nscalar=0, nvector=0, nvector2;
   int     option_index, c, i, n, m, l;
   char    entry[MAXSTRING];  /* should be long enough */
   char    txt[STRING];
   char    history[MAXATT];
   int     fill_missing = 0;
   unsigned int  finer_step = 0;

   Grid_config   *grid_in    = NULL;   /* store input grid  */
   Grid_config   *grid_out   = NULL;   /* store output grid */
   Field_config  *scalar_in  = NULL;   /* store input scalar data */
   Field_config  *scalar_out = NULL;   /* store output scalar data */
   Field_config  *u_in       = NULL;   /* store input vector u-component */
   Field_config  *v_in       = NULL;   /* store input vector v-component */
   Field_config  *u_out      = NULL;   /* store input vector u-component */
   Field_config  *v_out      = NULL;   /* store input vector v-component */
   File_config   *file_in    = NULL;   /* store input file information */
   File_config   *file_out   = NULL;   /* store output file information */
   File_config   *file2_in   = NULL;   /* store input file information */
   File_config   *file2_out  = NULL;   /* store output file information */
   Bound_config  *bound_T    = NULL;   /* store halo update information for T-cell*/
   Interp_config *interp     = NULL;   /* store remapping information */
   int save_weight_only      = 0;
  
   int errflg = (argc == 1);
   int fid;
   int ret;
  
   static struct option long_options[] = {
      {"input_mosaic",     required_argument, NULL, 'a'},
      {"output_mosaic",    required_argument, NULL, 'b'},
      {"input_dir",        required_argument, NULL, 'c'},
      {"output_dir",       required_argument, NULL, 'd'},
      {"input_file",       required_argument, NULL, 'e'},
      {"output_file",      required_argument, NULL, 'f'},
      {"remap_file",       required_argument, NULL, 'g'},
      {"test_case",        required_argument, NULL, 'i'},
      {"interp_method",    required_argument, NULL, 'j'},
      {"test_parameter",   required_argument, NULL, 'k'},
      {"symmetry",         no_argument,       NULL, 'l'},
      {"grid_type",        required_argument, NULL, 'm'},
      {"target_grid",      no_argument,       NULL, 'n'},
      {"finer_step",       required_argument, NULL, 'o'},
      {"fill_missing",     no_argument,       NULL, 'p'},
      {"nlon",             required_argument, NULL, 'q'},
      {"nlat",             required_argument, NULL, 'r'},
      {"scalar_field",     required_argument, NULL, 's'},
      {"check_conserve",   no_argument,       NULL, 't'},
      {"u_field",          required_argument, NULL, 'u'},
      {"v_field",          required_argument, NULL, 'v'},
      {"center_y",         no_argument,       NULL, 'y'},
      {"lonBegin",         required_argument, NULL, 'A'},
      {"lonEnd",           required_argument, NULL, 'B'},
      {"latBegin",         required_argument, NULL, 'C'},
      {"latEnd",           required_argument, NULL, 'D'},
      {"KlevelBegin",      required_argument, NULL, 'E'},
      {"KlevelEnd",        required_argument, NULL, 'F'},
      {"LstepBegin",       required_argument, NULL, 'G'},
      {"LstepEnd",         required_argument, NULL, 'H'},
      {"help",             no_argument,       NULL, 'h'},
      {0, 0, 0, 0},
   };  
  
   /* start parallel */
   mpp_init(&argc, &argv);
   mpp_domain_init();
  
   while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {
      switch (c) {
	 case 'a':
	    mosaic_in  = optarg;
	    break;
	 case 'b':
	    mosaic_out = optarg;
	    break;
	 case 'c':
	    dir_in = optarg;
	    break;
	 case 'd':
	    dir_out = optarg;
	    break;
	 case 'e':
	    if(strlen(optarg) >= MAXSTRING) mpp_error("fregrid: the entry is not long for option -e");
	    strcpy(entry, optarg);
	    tokenize(entry, ",", STRING, NFILE, input_file, &nfiles);
	    break;
	 case 'f':
	    if(strlen(optarg) >= MAXSTRING)  mpp_error("fregrid: the entry is not long for option -f");      
	    strcpy(entry, optarg);
	    tokenize(entry, ",", STRING, NFILE, output_file, &nfiles_out);
	    break;
	 case 'g':
	    remap_file = optarg;
	    break;
	 case 's':
	    if(strlen(optarg) >= MAXSTRING) mpp_error("fregrid: the entry is not long for option -s");      
	    strcpy(entry, optarg);
	    tokenize(entry, ",", STRING, NVAR, scalar_name, &nscalar);
	    break;
	 case 'u':
	    if(strlen(optarg) >= MAXSTRING) mpp_error("fregrid: the entry is not long for option -u");      
	    strcpy(entry, optarg);
	    tokenize(entry, ",", STRING, NVAR, u_name, &nvector);
	    break;        
	 case 'v':
	    if(strlen(optarg) >= MAXSTRING) mpp_error("fregrid: the entry is not long for option -v");      
	    strcpy(entry, optarg);
	    tokenize(entry, ",", STRING, NVAR, v_name, &nvector2);
	    break;      
	 case 'j':
	    strcpy(interp_method, optarg);
	    break;
	 case 'i':
	    test_case = optarg;
	    break;
	 case 'k':
	    test_param = atof(optarg);
	    break;      
	 case 'l':
	    opcode |= SYMMETRY;
	    break;
	 case 'm':
	    if(strcmp(optarg, "AGRID") == 0)
	       grid_type = AGRID;
	    else if(strcmp(optarg, "BGRID") == 0)
	       grid_type = BGRID;
	    else
	       mpp_error("fregrid: only AGRID and BGRID vector regridding are implmented, contact developer");
	    break;
	 case 'n':
	    opcode |= TARGET;
	    break;
	 case 'o':
	    finer_step = atoi(optarg);
	    break;
	 case 'p':
	    fill_missing = 1;
	    break;
	 case 'q':
	    nlon = atoi(optarg);
	    break;
	 case 'r':
	    nlat = atoi(optarg);
	    break;
	 case 't':
	    check_conserve = 1;
	    break;
	 case 'y':
	    y_at_center = 1;
	    break;
	 case 'A':
	    lonbegin = atof(optarg);
	    break;
	 case 'B':
	    lonend = atof(optarg);
	    break;
	 case 'C':
	    latbegin = atof(optarg);
	    break;
	 case 'D':
	    latend = atof(optarg);
	    break;
	 case 'E':
	    kbegin = atoi(optarg);
	    break;
	 case 'F':
	    kend = atoi(optarg);
	    break;
	 case 'G':
	    lbegin = atoi(optarg);
	    break;
	 case 'H':
	    lend = atoi(optarg);
	    break;
	 case '?':
	    errflg++;
	    break;
      }
   }

   if (errflg) {
      char **u = usage;
      while (*u) { fprintf(stderr, "%s\n", *u); u++; }
      exit(2);
   }      

   /* define history to be the history in the grid file */
   strcpy(history,argv[0]);
   for(i=1;i<argc;i++) {
      strcat(history, " ");
      if(strlen(argv[i]) > MAXENTRY) { /* limit the size of each entry, here we are assume the only entry that is longer than
					  MAXENTRY= 256 is the option --scalar_field --u_field and v_field */
	 if(strcmp(argv[i-1], "--scalar_field") && strcmp(argv[i-1], "--u_field") && strcmp(argv[i-1], "--v_field") )
	    mpp_error("fregrid: the entry ( is not scalar_field, u_field, v_field ) is too long, need to increase parameter MAXENTRY");
	 strcat(history, "(**please see the field list in this file**)" );
      }
      else
	 strcat(history, argv[i]);
   }
  

   if ((ret = gs_fregrid(history, mosaic_in, mosaic_out, dir_in, 
                         dir_out, input_file, nfiles, output_file, nfiles_out, 
                         remap_file, scalar_name, nscalar, u_name, nvector, 
                         v_name, nvector2, interp_method, test_case, test_param, 
                         opcode, grid_type, finer_step, fill_missing, nlon, nlat, check_conserve, 
                         y_at_center, lonbegin, lonend, latbegin, latend, 
                         lbegin, kend, lbegin, lend)))
      return ret;
      
   mpp_end();
   return 0;
  
} /* end of main */
  
