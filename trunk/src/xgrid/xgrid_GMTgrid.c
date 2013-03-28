
/*
	Hugh Fisher
	Dec 91
	
	Operations for GMT grids. The I/O code
	has a remarkable similarity to that
	in gmt_grdio.c, but uses different
	parameters and error handling.
					*/

#include "gmt_dev.h"
#include <stdio.h>
#include <errno.h>
#include <X11/Intrinsic.h>

#include "xgrid_utility.h"
#include "xgrid_GMTgrid.h"

static void clear (GMTGrid *grid)
{
  grid->methods.width     = 0;
  grid->methods.height    = 0;
  grid->methods.coordType = GridCoordNodes;
  
  grid->value = NULL;
  
  grid->header.wesn[XLO] = 0.0;
  grid->header.wesn[XHI] = 0.0;
  grid->header.wesn[YLO] = 0.0;
  grid->header.wesn[YHI] = 0.0;
  grid->header.z_min = 0.0;
  grid->header.z_max = 0.0;
  grid->header.inc[GMT_X] = 0.0;
  grid->header.inc[GMT_Y] = 0.0;
  grid->header.z_scale_factor	= 1.0;
  grid->header.z_add_offset	= 0.0;
  grid->header.nx = 0;
  grid->header.ny = 0;
  grid->header.registration = GridCoordNodes;

  strcpy(grid->header.x_units, "x");
  strcpy(grid->header.y_units, "y");
  strcpy(grid->header.z_units, "z");
  strcpy(grid->header.title, "Untitled");
  memset(grid->header.command, 0, sizeof(grid->header.command));
  memset(grid->header.remark, 0, sizeof(grid->header.remark));
}

static void readFromFile (Grid *_grid, String fileName, int *status)
{
    GMTGrid *grid = (GMTGrid *)_grid;
    if ((*status = GMT_read_grd_info (grid->GMT, fileName, &grid->header)))
      return;

    grid->value = calloc (grid->header.nm, sizeof (float));
    GMT_memset (grid->GMT->current.io.pad, 4, int);
    *status = GMT_read_grd (grid->GMT, fileName, &grid->header, grid->value, NULL,
	grid->GMT->current.io.pad, false);

    /* Update generic grid values */    
    grid->methods.width  = grid->header.nx;
    grid->methods.height = grid->header.ny;
    grid->methods.coordType = grid->header.registration; 

  return;
}

static void writeToFile (Grid *_grid, String fileName, int *status)
{
	GMTGrid *grid = (GMTGrid *)_grid;
	double wesn[4];
	GMT_memset (wesn, 4, double);
	GMT_memset (grid->GMT->current.io.pad, 4, int);
	*status = GMT_write_grd (grid->GMT, fileName, &grid->header, grid->value, wesn, grid->GMT->current.io.pad, false);
}

static void dispose (Grid *_grid)
{
  GMTGrid *grid = (GMTGrid *)_grid;
  free(grid->value);
  grid->value = NULL;
}

static void getIndexes (Grid *_grid, GridPoint *coord, XPoint *index)
{
  int column, row;
  GMTGrid *grid = (GMTGrid *)_grid;
  
  column = (int)lrint ((coord->x - grid->header.wesn[XLO]) / grid->header.inc[GMT_X]);
  row    = (int)lrint ((coord->y - grid->header.wesn[YLO]) / grid->header.inc[GMT_Y]);
  index->x = column;
  /* Allow for origin being at the bottom rather than the top */
  index->y = grid->header.ny - row;
}

static void getCoords (Grid *_grid, XPoint *index, GridPoint *coord)
{
  GMTGrid *grid = (GMTGrid *)_grid;
  coord->x = grid->header.wesn[XLO] + index->x * grid->header.inc[GMT_X];
  coord->y = grid->header.wesn[YHI] - index->y * grid->header.inc[GMT_Y];
}

static void set (Grid *_grid, int xIndex, int yIndex, GridValue value)
{
	GMTGrid *grid = (GMTGrid *)_grid;
	int k = yIndex * grid->header.nx + xIndex;
	grid->value[k] = value;
}

static GridValue get (Grid *_grid, int xIndex, int yIndex)
{
  GMTGrid *grid = (GMTGrid *)_grid;
/* Original Version had "grid->header.ny": should be "grid->header.nx */
/* Fixed 8/4/93 by J. Lillibrige @ NOAA/NOS to eliminate core dumps!  */

  return grid->value[yIndex * grid->header.nx + xIndex];
}

GMTGrid * CreateGMTGrid (struct GMT_CTRL *GMT)
{
  GMTGrid * result;
	/* avoid cast from 'char *' to 'GMTGrid *'
		 -result = (GMTGrid *)XtMalloc(sizeof(GMTGrid));
		 */
	result = malloc(sizeof(GMTGrid));
  clear(result);
  /* Assign methods */
  result->methods.dispose	= dispose;
  result->methods.readFromFile	= readFromFile;
  result->methods.writeToFile	= writeToFile;
  result->methods.getIndexes	= getIndexes;
  result->methods.getCoords	= getCoords;
  result->methods.set		= set;
  result->methods.get		= get;
  result->GMT		= GMT;	/* Pass CTRL struct pointer */
  
  return result;
}
