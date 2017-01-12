/* scfio.h
 * header file for ncurses structured field i/o routines
 * Jim Jackson Nov. 1996
 */

#ifndef __SCFIO_H
#define __SCFIO_H

/*#include <ncurses/ncurses.h>*/
#include <curses.h>

/* CONFIGURATION OPTION - either here or in your make file
 * I like the 'open field' functioning - others may not.
 * Simply uncomment the define here for 'closed' fields
 */

/*#define   CLOSED_FIELDS  */

/* Some misc. defines.........
 */

#define RANGE(L,X,U) ((X<=L)?L:((X>=U)?U:X))
#define MAXINTWIDTH 9

static int POWERS10[]= 
   { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 
     1000000000, 0 };

#define LIMITS(N)  (POWERS10[N]-1)

#define MAXCOL 256

/* ====================================================================
 * Functions calls where everything is passed as separate parameters
 */

/* macro function calls.......
 */

#define mvactfint(L,C,K,A,IP,W,O) ((move(L,C)==ERR)?ERR:actfrac(K,A,IP,W,O,0))
#define mvputfint(L,C,A,N,W)  ((move(L,C)==ERR)?ERR:putfrac(A,N,W,0))
#define mvactfrac(L,C,K,A,IP,W,O,DP) ((move(L,C)==ERR)?ERR:actfrac(K,A,IP,W,O,DP))
#define mvputfrac(L,C,A,N,W,DP)  ((move(L,C)==ERR)?ERR:putfrac(A,N,W,DP))
#define mvactfstr(L,C,K,A,P,PP,W) ((move(L,C)==ERR)?ERR:actfstr(K,A,P,PP,W))
#define mvputfstr(L,C,A,P,W) ((move(L,C)==ERR)?ERR:putfstr(A,P,W))

#define actfint(K,A,IP,W,O) actfrac(K,A,IP,W,O,0)
#define putfint(A,N,W)  putfrac(A,N,W,0)
#define putfopt(A,S,I,W)   putfstr(A,S[I],W)
#define mvactfopt(L,C,K,A,P,I,W) ((move(L,C)==ERR)?ERR:actfopt(K,A,P,I,W))
#define mvputfopt(L,C,A,P,I,W) ((move(L,C)==ERR)?ERR:putfstr(A,P[I],W))

/* Main io field structure definition .......
 */

struct SCField {
/*   WINDOW *win; */               /* Window this field is in                */
   int type;                   /* type of field - see below              */
   int row;                    /* row and col of start of field          */
   int col;
   int attr;                   /* attribute to use for field             */
   union {
      void *tp;                /* general void ptr  */
      char *s;                 /* or pointer to string                   */
      char **sp;               /* or a ptr to an array of options        */
      int *i;                  /* integer value of field - see below     */
      float *f;                /* or float value                         */
   } val;
   int W;                      /* width in chars of field                */
   int adj;                    /* a 2nd parameter for some fields        */
   int id;                     /* field identifier for e.g. tying fields
                                  together or any special application use */
   int dp;                     /* In integer/fractions number of dec. places */
};

/* values for the type field define the type of value defined
 * and some simple attributes of the value
 * 
 * integer  - is a cash till type integer entry. Cursor stays fixed on right
 * fraction - is a fixed decimal point fractional value.
 * string   - simple string value
 * option   - allows one of a fixed set of values to be choosen
 */

#define SCF_type     0xff
#define SCF_integer  1         /* A Fraction with dp == 0 ! */
#define SCF_fraction 1
#define SCF_string   2
#define SCF_real     3         /* NOT YET IMPLEMENTED */
#define SCF_option   4
#define SCF_fixed    0x100
                               /* field has a fixed value - cannot be altered */
                               /*  default is NOT fixed                    */
#define SCF_justify  0x200
                               /* field is justified opposite to default   */
                               /* string right justified, int and real left*/
#define SCF_overwrite 0x400    /* data entered into field does not insert  */
                               /* but overwites existing data */

/* macros acting on SCField.type field flags above. These macros act on
 * the given SCF_type and return the new value of the SCF_type.
 *
 *  - test SCField type to see if field is not fixed, i.e. an input field
 *  - to force field to be fixed
 *  - to make field an input field, i.e. remove the fixed value
 *  - to put field in overwrite mode
 *  - to put field in insert mode (default condition)
 */

#define CAN_INPUT(T)  (!((T) & SCF_fixed))
#define INSERTING(T)  (!((T) & SCF_overwrite))

#define FIXED_FIELD(T) ((T) | SCF_fixed)
#define INPUT_FIELD(T) ((T) & (~SCF_fixed))
#define OVERWRITE_FIELD(T) ((T) | SCF_overwrite)
#define INSERT_FIELD(T) ((T) & (~SCF_overwrite))

/* These macros take a struct SCField * and manipulate it's contents
 * as if they were functions. For all those that change SCF_type field
 * the returned value is the new value of the field.
 */

/* these interrogate the structure and return values...
 * First these check true or false....
 */

#define CanInput(S) (CAN_INPUT(S->type))
#define Inserting(S) (INSERTING(S->type))
#define IsFieldInt(S) ((((S->type)&SCF_type) == SCF_integer) && ((S->dp)==0))
#define IsFieldFrac(S) ((((S->type)&SCF_type) == SCF_integer) && (S->dp))
#define IsFieldStr(S) (((S->type)&SCF_type) == SCF_string)
#define IsFieldOption(S) (((S->type)&SCF_type) == SCF_option)

/* These set values in the field structure........
 */

#define FixedField(S) (S->type = FIXED_FIELD(S->type))
#define InputField(S) (S->type = INPUT_FIELD(S->type))
#define OverwriteField(S) (S->type = OVERWRITE_FIELD(S->type))
#define InsertField(S) (S->type = INSERT_FIELD(S->type))
#define SetAttrib(S,A) (S->attr = A)
#define SetWidth(S,WIDTH)  (S->W = WIDTH)
#define SetDP(S,D)     (S->dp = D)
#define SetOffset(S,O) (S->adj = O)
#define SetOptionIndex(S,O) (S->adj = O)

#define SetInt(S,I)    (*(S->val.i) = I)
#define SetStr(S,ST)   ((S->val.s) = ST)
#define SetOption(S,OP) ((S->val.sp) = OP)

/* These return values from the field structure.......
 */

#define GetType(SP)   ((SP->type)&SCF_type)
#define GetInt(SP)    (*(SP->val.i))
#define GetStr(SP)    (SP->val.s)
#define GetOption(SP) ((SP->val.sp)[SP->adj])
#define GetOptionIndex(SP) (SP->adj)
#define GetFrac(SP)   (((double)*(SP->val.i))/(double)POWERS10[RANGE(0,SP->dp,MAXINTWIDTH)])
#define GetOffset(S)  (S->adj)
#define GetDP(S)      (S->dp)
#define GetAttrib(S)  (S->attr)
#define GetWidth(S)  (S->W)

/* ====================================================================
 * Function calls where everything is passed thru' a structure
 */

struct SCField *newfield(int type,int att,int COL,int ROW,
                                           int W,void *D,int id,int dp);
int actfield(int k, struct SCField *F);
int putfield(struct SCField *F);
struct SCField *dupfield(struct SCField *F);

/* ======================================================================  */

#ifndef CLOSED_FIELDS
# define OPEN_FIELDS
#endif

#endif   /*  __SCFIO_H  */
