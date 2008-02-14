
#include <stdio.h>


/* Integer square root by Halleck's method, with Legalize's speedup */
long isqrt (x) long x;{
  long   squaredbit, remainder, root;

   if (x<1) return 0;

   /* Load the binary constant 01 00 00 ... 00, where the number
    * of zero bits to the right of the single one bit
    * is even, and the one bit is as far left as is consistant
    * with that condition.)
    */
   squaredbit  = (long) ((((unsigned long) ~0L) >> 1) &
                        ~(((unsigned long) ~0L) >> 2));
   /* This portable load replaces the loop that used to be
    * here, and was donated by  legalize@xmission.com
    */

   /* Form bits of the answer. */
   remainder = x;  root = 0;
   while (squaredbit > 0) {
     if (remainder >= (squaredbit | root)) {
         remainder -= (squaredbit | root);
         root >>= 1; root |= squaredbit;
     } else {
         root >>= 1;
     }
     squaredbit >>= 2;
   }

   return root;
}
