/*
 *	$Id: coast_io.c,v 1.4 2004-09-09 20:17:47 pwessel Exp $
 */
#define COASTLIB 1
#include "wvs.h"

#if WORDS_BIGENDIAN == 0
int swab_polheader (struct GMT3_POLY *h);
int swab_polpoints (struct LONGPAIR *p, int n);
#endif

int pol_readheader (struct GMT3_POLY *h, FILE *fp)
{
	int n;
	n = fread ((void *)h, sizeof (struct GMT3_POLY), 1, fp);
#if WORDS_BIGENDIAN == 0
	swab_polheader (h);
#endif
	return (n);
}

int pol_writeheader (struct GMT3_POLY *h, FILE *fp)
{
	int n;
	struct GMT3_POLY *use_h;
#if WORDS_BIGENDIAN == 0
	struct GMT3_POLY tmp_h;
	tmp_h = *h;
	swab_polheader (&tmp_h);
	use_h = &tmp_h;
#else
	use_h = h;
#endif
	n = fwrite ((void *)use_h, sizeof (struct GMT3_POLY), 1, fp);
	return (n);
}

int pol_fread (struct LONGPAIR *p, size_t n_items, FILE *fp)
{
	int n;

	n = fread ((void *)p, sizeof (struct LONGPAIR), n_items, fp);
#if WORDS_BIGENDIAN == 0
	swab_polpoints (p, n_items);
#endif
	return (n);
}

int pol_fwrite (struct LONGPAIR *p, size_t n_items, FILE *fp)
{
	int n;
#if WORDS_BIGENDIAN == 0
	swab_polpoints (p, n_items);
#endif
	n = fwrite ((void *)p, sizeof (struct LONGPAIR), n_items, fp);
	return (n);
}

#if WORDS_BIGENDIAN == 0
int swab_polheader (struct GMT3_POLY *h)
{
	unsigned int *i, j;

	h->id = GMT_swab4 (h->id);
	h->n = GMT_swab4 (h->n);
	h->greenwich = GMT_swab4 (h->greenwich);
	h->level = GMT_swab4 (h->level);
	h->datelon = GMT_swab4 (h->datelon);
	h->checked[0] = GMT_swab4 (h->checked[0]);
	h->checked[1] = GMT_swab4 (h->checked[1]);
	h->source = GMT_swab4 (h->source);
	i = (unsigned int *)&h->west;
	j = GMT_swab4 (i[0]);
	i[0] = GMT_swab4 (i[1]);
	i[1] = j;
	i = (unsigned int *)&h->east;
	j = GMT_swab4 (i[0]);
	i[0] = GMT_swab4 (i[1]);
	i[1] = j;
	i = (unsigned int *)&h->south;
	j = GMT_swab4 (i[0]);
	i[0] = GMT_swab4 (i[1]);
	i[1] = j;
	i = (unsigned int *)&h->north;
	j = GMT_swab4 (i[0]);
	i[0] = GMT_swab4 (i[1]);
	i[1] = j;
	i = (unsigned int *)&h->area;
	j = GMT_swab4 (i[0]);
	i[0] = GMT_swab4 (i[1]);
	i[1] = j;
}

int swab_polpoints (struct LONGPAIR *p, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		p[i].x = GMT_swab4 (p[i].x);
		p[i].y = GMT_swab4 (p[i].y);
	}
}
#endif
