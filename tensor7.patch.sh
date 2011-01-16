#!/bin/sh
# A self-applying patch for FFTW 3.3alpha1's tensor_compress_contiguous().
# The patch must be applied to 3.3alpha1 otherwise underling will not work.
#
# From Steven G. Johnson on 18 February 2010:
#     Unfortunately, FFTW does not currently recognize the equivalence of a 7 x
#     (3 x 5) transpose to a 7 x 15 transpose, so you have to reformulate it
#     this way yourself.  Probably it should be possible to patch FFTW to do
#     this -- the right place would be the tensor_compress_contiguous function
#     in kernel/tensor7.c ... a simple change to the strides_contig function
#     might do it, but some careful thought is required to get something that
#     is correct in all cases.
#
# This patch allows tensor_compress_contiguous() to detect such situations.  It
# must be run from FFTW's topmost source directory.  The patch has been
# provided to Steven G. Johnson and Matteo Frigo in private correspondence.

if [ ! -f kernel/tensor7.c ]; then
    echo File kernel/tensor7.c not detected.
    echo $0 must be run within FFTW\'s topmost source directory!
    exit 1
fi

patch -p0 kernel/tensor7.c << ENDOFFILE
--- tensor7.c	2010-02-19 01:12:17.000000000 -0600
+++ tensor7.c	2010-02-21 17:51:24.000000000 -0600
@@ -93,9 +93,20 @@
 }
 
 /* Return whether the strides of a and b are such that they form an
-   effective contiguous 1d array.  Assumes that a.is >= b.is. */
+   effective contiguous 1d array.  Argument order is irrelevant. */
 static int strides_contig(iodim *a, iodim *b)
 {
+     if (a->is < b->is) {
+          if (a->is >= 0) {                    /* positive stride(s?) */
+               iodim *tmp = a; a = b; b = tmp; /* swap */
+          }
+     } else {
+          if (a->is < 0) {                     /* negative stride(s?) */
+               iodim *tmp = a; a = b; b = tmp; /* swap */
+          }
+     }
+
+     /* For n > 0, can only pass if strides have same signs */
      return (a->is == b->is * b->n && a->os == b->os * b->n);
 }
 
@@ -104,7 +115,7 @@
    some stride.  (This can safely be done for transform vector sizes.) */
 tensor *X(tensor_compress_contiguous)(const tensor *sz)
 {
-     int i, rnk;
+     int i, j, rnk;
      tensor *sz2, *x;
 
      if (X(tensor_sz)(sz) == 0) 
@@ -116,21 +127,44 @@
      if (sz2->rnk < 2)		/* nothing to compress */
           return sz2;
 
-     for (i = rnk = 1; i < sz2->rnk; ++i)
-          if (!strides_contig(sz2->dims + i - 1, sz2->dims + i))
-               ++rnk;
+     /* Holds that sz2->dims[i].n > 0 for all i due to tensor_compress above.
+      * Use sz2->dims[i] == 0 as an its-been-compressed flag below here.
+      * Merge all as-yet-uncompressed-and-strides_contig direction pairs. */
+     for (i = rnk = 0; i < sz2->rnk; ++i) {
+          if (sz2->dims[i].n == 0) continue;
+
+          ++rnk;
+          for (j = i + 1; j < sz2->rnk; ++j) {
+               if (sz2->dims[j].n == 0) continue;
+
+               if (strides_contig(sz2->dims + i, sz2->dims + j)) {
+                    /* strides_contig forces is, os to have consistent signs */
+                    A(signof(sz2->dims[i].is) == signof(sz2->dims[j].is));
+                    A(signof(sz2->dims[i].os) == signof(sz2->dims[j].os));
+
+                    /* Merge direction pair accounting for stride signs
+                     * and strides_contig check being transitive */
+                    sz2->dims[i].n *= sz2->dims[j].n;
+                    sz2->dims[i].is =   signof(sz2->dims[i].is)
+                                      * X(imin)(X(iabs)(sz2->dims[i].is),
+                                                X(iabs)(sz2->dims[j].is));
+                    sz2->dims[i].os =   signof(sz2->dims[i].os)
+                                      * X(imin)(X(iabs)(sz2->dims[i].os),
+                                                X(iabs)(sz2->dims[j].os));
+
+                    /* Mark j flagged as compressed away */
+                    sz2->dims[j].n = 0;
+               }
+          }
+     }
 
+     /* Pack the remaining, now compressed directions into return value */
      x = X(mktensor)(rnk);
-     x->dims[0] = sz2->dims[0];
-     for (i = rnk = 1; i < sz2->rnk; ++i) {
-          if (strides_contig(sz2->dims + i - 1, sz2->dims + i)) {
-               x->dims[rnk - 1].n *= sz2->dims[i].n;
-               x->dims[rnk - 1].is = sz2->dims[i].is;
-               x->dims[rnk - 1].os = sz2->dims[i].os;
-          } else {
-               A(rnk < x->rnk);
-               x->dims[rnk++] = sz2->dims[i];
-          }
+     for (i = rnk = 0; i < sz2->rnk; ++i) {
+          if (sz2->dims[i].n == 0) continue;
+
+          A(rnk < x->rnk);
+          x->dims[rnk++] = sz2->dims[i];
      }
 
      X(tensor_destroy)(sz2);
ENDOFFILE
