
;; Function main (main, funcdef_no=0, decl_uid=2344, cgraph_uid=1, symbol_order=0)

int main ()
{
  int f;
  int n;
  int i;
  int k;
  int D.2356;

  k = 1000;
  printf ("%d\n", k);
  scanf ("%d", &n);
  i = 2;
  f = 1;
  goto <D.2351>;
  <D.2352>:
  f = f * i;
  i = i + 1;
  <D.2351>:
  n.0_1 = n;
  if (i <= n.0_1) goto <D.2352>; else goto <D.2350>;
  <D.2350>:
  if (0 != 0) goto <D.2354>; else goto <D.2355>;
  <D.2354>:
  printf ("fool");
  <D.2355>:
  printf ("%d\n", f);
  D.2356 = 0;
  goto <D.2358>;
  <D.2358>:
  n = {CLOBBER};
  goto <D.2357>;
  D.2356 = 0;
  goto <D.2357>;
  <D.2357>:
  return D.2356;
}


