/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */


#ifdef _MACOSX
    #include <malloc/malloc.h>
#else
    #include <malloc.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined _MSC_VER
#include <strings.h>
#endif

#include "unistd.h"
#include "../../src/Game/Types.h"
#include "kas2c.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif


int parseLevel = LEVEL_LEVEL;

int ifOnceIndex;  // up to 2^32 IFONCE statements per WATCH or INIT routine

extern FILE *yyout;
extern char *yytext;

extern int lineNum;
extern char curFilename[];



# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_PARSER_H_INCLUDED
# define YY_YY_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    QSTRING = 258,
    ID = 259,
    COMMENT2 = 260,
    FSM = 261,
    ENDF = 262,
    STATES = 263,
    WATCH = 264,
    ENDW = 265,
    STATE = 266,
    ENDS = 267,
    INITIALIZE = 268,
    ENDI = 269,
    IF = 270,
    ELSEIF = 271,
    ELSE = 272,
    ENDIF = 273,
    IFONCE = 274,
    ENDIFONCE = 275,
    NUMBER = 276,
    AND = 277,
    OR = 278,
    NOT = 279,
    TEAM = 280,
    SHIPS = 281,
    TEAMSHIPS = 282,
    SHIPSPOINT = 283,
    TEAMSPOINT = 284,
    VOLUMEPOINT = 285,
    PATH = 286,
    XPOINT = 287,
    VOLUME = 288,
    THISTEAM = 289,
    THISTEAMSHIPS = 290,
    THISTEAMSPOINT = 291,
    JUMP = 292,
    XTRUE = 293,
    XFALSE = 294,
    LT = 295,
    LTE = 296,
    EQ = 297,
    NEQ = 298,
    GTE = 299,
    GT = 300,
    FSMCREATE = 301,
    LOCALIZATION = 302,
    ENDL = 303,
    LSTRING = 304,
    BANG = 305,
    UMINUS = 306
  };
#endif
/* Tokens.  */
#define QSTRING 258
#define ID 259
#define COMMENT2 260
#define FSM 261
#define ENDF 262
#define STATES 263
#define WATCH 264
#define ENDW 265
#define STATE 266
#define ENDS 267
#define INITIALIZE 268
#define ENDI 269
#define IF 270
#define ELSEIF 271
#define ELSE 272
#define ENDIF 273
#define IFONCE 274
#define ENDIFONCE 275
#define NUMBER 276
#define AND 277
#define OR 278
#define NOT 279
#define TEAM 280
#define SHIPS 281
#define TEAMSHIPS 282
#define SHIPSPOINT 283
#define TEAMSPOINT 284
#define VOLUMEPOINT 285
#define PATH 286
#define XPOINT 287
#define VOLUME 288
#define THISTEAM 289
#define THISTEAMSHIPS 290
#define THISTEAMSPOINT 291
#define JUMP 292
#define XTRUE 293
#define XFALSE 294
#define LT 295
#define LTE 296
#define EQ 297
#define NEQ 298
#define GTE 299
#define GT 300
#define FSMCREATE 301
#define LOCALIZATION 302
#define ENDL 303
#define LSTRING 304
#define BANG 305
#define UMINUS 306

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{


    char    *string;    /* string buffer */
    int     number;     /* numeric value */



};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_PARSER_H_INCLUDED  */



#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   336

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  62
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  65
/* YYNRULES -- Number of rules.  */
#define YYNRULES  119
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  202

#define YYUNDEFTOK  2
#define YYMAXUTOK   306


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    61,     2,     2,     2,     2,     2,     2,
      59,    60,    52,    51,    58,    50,     2,    53,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    57,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    56,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    54,    55
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    62,    62,    65,    66,    69,    72,    75,    76,    83,
      86,    89,    90,    93,    94,    97,   100,   103,   104,   107,
     108,   108,   111,   112,   115,   116,   119,   122,   125,   126,
     129,   132,   140,   141,   144,   145,   146,   147,   148,   150,
     154,   154,   161,   164,   165,   165,   168,   169,   169,   169,
     172,   172,   172,   175,   178,   178,   178,   182,   182,   183,
     183,   184,   184,   185,   185,   186,   186,   187,   187,   188,
     188,   189,   189,   190,   190,   191,   191,   192,   192,   193,
     193,   194,   194,   195,   195,   196,   196,   197,   197,   198,
     199,   199,   200,   201,   204,   205,   206,   206,   209,   210,
     212,   214,   219,   221,   223,   225,   227,   229,   231,   233,
     235,   237,   241,   243,   247,   248,   251,   254,   257,   258
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "QSTRING", "ID", "COMMENT2", "FSM",
  "ENDF", "STATES", "WATCH", "ENDW", "STATE", "ENDS", "INITIALIZE", "ENDI",
  "IF", "ELSEIF", "ELSE", "ENDIF", "IFONCE", "ENDIFONCE", "NUMBER", "AND",
  "OR", "NOT", "TEAM", "SHIPS", "TEAMSHIPS", "SHIPSPOINT", "TEAMSPOINT",
  "VOLUMEPOINT", "PATH", "XPOINT", "VOLUME", "THISTEAM", "THISTEAMSHIPS",
  "THISTEAMSPOINT", "JUMP", "XTRUE", "XFALSE", "LT", "LTE", "EQ", "NEQ",
  "GTE", "GT", "FSMCREATE", "LOCALIZATION", "ENDL", "LSTRING", "'-'",
  "'+'", "'*'", "'/'", "BANG", "UMINUS", "'_'", "';'", "','", "'('", "')'",
  "'!'", "$accept", "level", "localization", "localization_start",
  "localization_end", "lstring_defs", "lstring_def", "lstring_def_name",
  "lstring_value_list", "fsms", "fsm", "fsm_start", "fsm_end",
  "state_list", "$@1", "state_id_list", "initialize_block",
  "initialize_start", "initialize_end", "watch_block", "watch_start",
  "watch_end", "statements", "statement", "fsmcreate", "$@2",
  "ifstatement", "else", "$@3", "elseif", "$@4", "$@5", "ifcondition",
  "$@6", "$@7", "ifoncestatement", "ifoncecondition", "$@8", "$@9",
  "expression", "$@10", "$@11", "$@12", "$@13", "$@14", "$@15", "$@16",
  "$@17", "$@18", "$@19", "$@20", "$@21", "$@22", "$@23", "$@24", "$@25",
  "$@26", "param_list", "$@27", "param", "paramteam", "states", "state",
  "state_start", "state_end", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
      45,    43,    42,    47,   305,   306,    95,    59,    44,    40,
      41,    33
};
# endif

#define YYPACT_NINF (-63)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -43,   -63,    24,   -63,   -63,   -63,     9,     3,    62,   -63,
     -63,    67,    61,   -63,   -63,    20,   -63,   -63,    74,   -63,
     -63,    71,   -63,   -63,   -63,    68,    76,   -63,    78,    81,
      61,   111,   -63,   -63,    27,    32,   -63,   -63,    89,   -63,
     -63,    35,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,    75,   259,   -63,   -63,   -63,   -41,   -63,   -63,
     -63,    37,   -63,   -63,   116,    40,    94,   116,   116,   116,
      14,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,    96,    18,    29,   116,
     116,   -63,   -63,   -63,   -63,   163,   -63,    42,   -63,    84,
     116,   116,   116,   116,   116,   116,   116,   116,   116,   116,
     116,   116,   -63,    99,   100,   -63,   -63,    71,   -63,    52,
      53,    54,    56,    57,    60,    63,    66,    77,   -63,   -63,
     -63,    80,   283,    11,   -63,   -63,   187,   211,    65,   -63,
     -63,   -63,   -63,   -32,   -32,   -16,   -16,   -16,   -16,   -16,
     -16,    21,    21,   -63,   -63,   -63,   -63,    61,   127,   130,
     134,   135,   137,   138,   139,   140,   142,   143,   -63,   -63,
     -63,   -63,   -11,   116,   141,   146,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,    29,   -63,   -63,    91,
     235,   148,   -63,   -63,   141,   141,   102,   -63,   -63,   -63,
     -63,   141
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       3,     5,     0,    13,     7,     1,    24,     0,     0,    26,
      14,    19,    28,    32,     6,     0,     4,     8,     0,    16,
      20,    24,    30,     2,    32,     0,     0,    11,     9,     0,
      28,     0,    90,    27,     0,     0,    89,    85,     0,    92,
      93,     0,    81,    34,    87,    83,    25,    33,    39,    37,
      46,    38,     0,     0,    10,    12,    22,     0,   114,    31,
      29,     0,    50,    54,     0,     0,     0,     0,     0,     0,
      43,    53,    65,    67,    77,    79,    69,    71,    75,    73,
      59,    57,    61,    63,    35,    21,     0,     0,    94,     0,
       0,    86,    36,    40,    82,     0,    84,     0,    44,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    23,    17,     0,    15,   115,    24,    99,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   113,   110,
     111,     0,    98,     0,    95,   101,     0,     0,     0,    88,
      47,    32,    42,    66,    68,    78,    80,    70,    72,    76,
      74,    60,    58,    62,    64,    18,   117,    28,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    96,    91,
      51,    55,     0,     0,    45,     0,   112,   102,   106,   107,
     108,   109,   103,   104,   105,   100,     0,    32,    32,     0,
       0,   118,   116,    97,    52,    56,     0,    48,   119,    41,
      32,    49
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -18,   -63,   -63,   -29,
     -63,   -63,   -24,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -62,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -33,
      -5,   -63,   -63,   -63,   -63
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,    16,     7,    17,    18,    28,     6,
      10,    11,   115,    21,    29,    57,    12,    13,    46,    23,
      24,    60,    25,    47,    48,   138,    49,    99,   141,    70,
     173,   200,    50,    89,   187,    51,    52,    90,   188,    53,
     109,   108,   110,   111,   100,   101,   104,   105,   107,   106,
     102,   103,    67,    69,    64,    68,    61,   133,   186,   134,
     135,    87,   116,   117,   192
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      31,    58,    91,    30,     1,    94,    95,    96,    74,    75,
      76,    77,    78,    79,   119,     8,    85,    86,    80,    81,
      82,    83,     9,   128,     5,   113,   132,   136,   137,   114,
      97,    98,   118,    32,    80,    81,    82,    83,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
      36,    14,    15,    37,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,    19,    39,    40,   168,
      22,   169,    32,    82,    83,    20,    26,    27,   131,    42,
      54,    55,    33,    34,     9,    56,    62,    35,    44,    36,
      45,    63,    37,    65,    66,    71,    88,    92,    93,   157,
     112,   140,   142,   155,   156,    38,    39,    40,   158,   159,
     160,   190,   161,   162,    41,    32,   163,   174,    42,   164,
      32,    59,   165,   172,   132,    43,    34,    44,   175,    45,
      35,   176,    36,   166,   177,    37,   167,    36,   178,   179,
      37,   180,   181,   182,   183,    32,   184,   185,    38,    39,
      40,   196,   198,   193,    39,    40,    34,    41,   191,   199,
      35,    42,    36,   194,   195,    37,    42,   189,    43,     0,
      44,     0,    45,     0,     0,    44,   201,    45,    38,    39,
      40,     0,     0,     0,     0,    72,    73,    41,     0,     0,
       0,    42,     0,     0,     0,     0,     0,     0,    43,     0,
      44,     0,    45,    74,    75,    76,    77,    78,    79,    72,
      73,     0,     0,    80,    81,    82,    83,     0,     0,     0,
       0,     0,     0,   139,     0,     0,     0,    74,    75,    76,
      77,    78,    79,    72,    73,     0,     0,    80,    81,    82,
      83,     0,     0,     0,     0,     0,     0,   170,     0,     0,
       0,    74,    75,    76,    77,    78,    79,    72,    73,     0,
       0,    80,    81,    82,    83,     0,     0,     0,     0,     0,
       0,   171,     0,     0,     0,    74,    75,    76,    77,    78,
      79,    72,    73,     0,     0,    80,    81,    82,    83,     0,
       0,     0,     0,     0,     0,   197,     0,     0,     0,    74,
      75,    76,    77,    78,    79,    72,    73,     0,     0,    80,
      81,    82,    83,     0,     0,     0,    84,     0,     0,     0,
       0,     0,     0,    74,    75,    76,    77,    78,    79,     0,
       0,     0,     0,    80,    81,    82,    83
};

static const yytype_int16 yycheck[] =
{
      24,    30,    64,    21,    47,    67,    68,    69,    40,    41,
      42,    43,    44,    45,    25,     6,    57,    58,    50,    51,
      52,    53,    13,    34,     0,     7,    88,    89,    90,    11,
      16,    17,     3,     4,    50,    51,    52,    53,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      21,    48,    49,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,     4,    38,    39,    58,
       9,    60,     4,    52,    53,     8,    56,     3,    49,    50,
       4,     3,    14,    15,    13,     4,    59,    19,    59,    21,
      61,    59,    24,     4,    59,    20,    59,    57,     4,   117,
       4,    59,    18,     4,     4,    37,    38,    39,    56,    56,
      56,   173,    56,    56,    46,     4,    56,   141,    50,    56,
       4,    10,    56,    58,   186,    57,    15,    59,   157,    61,
      19,     4,    21,    56,     4,    24,    56,    21,     4,     4,
      24,     4,     4,     4,     4,     4,     4,     4,    37,    38,
      39,    60,     4,   186,    38,    39,    15,    46,    12,    57,
      19,    50,    21,   187,   188,    24,    50,   172,    57,    -1,
      59,    -1,    61,    -1,    -1,    59,   200,    61,    37,    38,
      39,    -1,    -1,    -1,    -1,    22,    23,    46,    -1,    -1,
      -1,    50,    -1,    -1,    -1,    -1,    -1,    -1,    57,    -1,
      59,    -1,    61,    40,    41,    42,    43,    44,    45,    22,
      23,    -1,    -1,    50,    51,    52,    53,    -1,    -1,    -1,
      -1,    -1,    -1,    60,    -1,    -1,    -1,    40,    41,    42,
      43,    44,    45,    22,    23,    -1,    -1,    50,    51,    52,
      53,    -1,    -1,    -1,    -1,    -1,    -1,    60,    -1,    -1,
      -1,    40,    41,    42,    43,    44,    45,    22,    23,    -1,
      -1,    50,    51,    52,    53,    -1,    -1,    -1,    -1,    -1,
      -1,    60,    -1,    -1,    -1,    40,    41,    42,    43,    44,
      45,    22,    23,    -1,    -1,    50,    51,    52,    53,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    -1,    -1,    -1,    40,
      41,    42,    43,    44,    45,    22,    23,    -1,    -1,    50,
      51,    52,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    41,    42,    43,    44,    45,    -1,
      -1,    -1,    -1,    50,    51,    52,    53
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    47,    63,    64,    65,     0,    71,    67,     6,    13,
      72,    73,    78,    79,    48,    49,    66,    68,    69,     4,
       8,    75,     9,    81,    82,    84,    56,     3,    70,    76,
      78,    84,     4,    14,    15,    19,    21,    24,    37,    38,
      39,    46,    50,    57,    59,    61,    80,    85,    86,    88,
      94,    97,    98,   101,     4,     3,     4,    77,    81,    10,
      83,   118,    59,    59,   116,     4,    59,   114,   117,   115,
      91,    20,    22,    23,    40,    41,    42,    43,    44,    45,
      50,    51,    52,    53,    57,    57,    58,   123,    59,    95,
      99,   101,    57,     4,   101,   101,   101,    16,    17,    89,
     106,   107,   112,   113,   108,   109,   111,   110,   103,   102,
     104,   105,     4,     7,    11,    74,   124,   125,     3,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    49,   101,   119,   121,   122,   101,   101,    87,    60,
      59,    90,    18,   101,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,     4,     4,    78,    56,    56,
      56,    56,    56,    56,    56,    56,    56,    56,    58,    60,
      60,    60,    58,    92,    84,    81,     4,     4,     4,     4,
       4,     4,     4,     4,     4,     4,   120,    96,   100,   122,
     101,    12,   126,   121,    84,    84,    60,    60,     4,    57,
      93,    84
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    62,    63,    64,    64,    65,    66,    67,    67,    68,
      69,    70,    70,    71,    71,    72,    73,    74,    74,    75,
      76,    75,    77,    77,    78,    78,    79,    80,    81,    81,
      82,    83,    84,    84,    85,    85,    85,    85,    85,    85,
      87,    86,    88,    89,    90,    89,    91,    92,    93,    91,
      95,    96,    94,    97,    99,   100,    98,   102,   101,   103,
     101,   104,   101,   105,   101,   106,   101,   107,   101,   108,
     101,   109,   101,   110,   101,   111,   101,   112,   101,   113,
     101,   114,   101,   115,   101,   116,   101,   117,   101,   101,
     118,   101,   101,   101,   119,   119,   120,   119,   121,   121,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,   121,   122,   122,   123,   123,   124,   125,   126,   126
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     4,     0,     3,     1,     1,     0,     2,     2,
       3,     1,     2,     0,     2,     6,     2,     1,     2,     0,
       0,     4,     1,     3,     0,     3,     1,     1,     0,     3,
       1,     1,     0,     2,     1,     2,     3,     1,     1,     1,
       0,     8,     4,     0,     0,     3,     0,     0,     0,     8,
       0,     0,     7,     2,     0,     0,     7,     0,     4,     0,
       4,     0,     4,     0,     4,     0,     4,     0,     4,     0,
       4,     0,     4,     0,     4,     0,     4,     0,     4,     0,
       4,     0,     3,     0,     3,     0,     3,     0,     4,     1,
       0,     5,     1,     1,     0,     1,     0,     4,     1,     1,
       3,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       1,     1,     3,     1,     0,     2,     4,     2,     1,     2
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 5:
                                 { kasLocalizationStart(); }
    break;

  case 6:
                         { kasLocalizationEnd(); }
    break;

  case 9:
                                                    { kasLStringDefineEnd(); }
    break;

  case 10:
                                   { kasLStringDefineStart((yyvsp[0].string)); }
    break;

  case 11:
                            { kasLStringValue((yyvsp[0].string)); }
    break;

  case 12:
                                   { kasLStringValue((yyvsp[0].string)); }
    break;

  case 16:
                   { kasFSMStart((yyvsp[0].string)); }
    break;

  case 17:
                    { kasFSMEnd(""); }
    break;

  case 18:
                    { kasFSMEnd((yyvsp[0].string)); }
    break;

  case 19:
             { kasStateListClear(); kasStateListEnd(); }
    break;

  case 20:
                   { kasStateListClear(); }
    break;

  case 21:
                                                              { kasStateListEnd(); }
    break;

  case 22:
                                        { kasStateListAdd((yyvsp[0].string)); }
    break;

  case 23:
                                        { kasStateListAdd((yyvsp[0].string)); }
    break;

  case 24:
                    { kasInitializeStart(); kasInitializeEnd(); }
    break;

  case 26:
                                   { kasInitializeStart(); }
    break;

  case 27:
                                    { kasInitializeEnd(); }
    break;

  case 28:
                { kasWatchStart(); kasWatchEnd(); }
    break;

  case 30:
                                    { kasWatchStart(); }
    break;

  case 31:
                                    { kasWatchEnd(); }
    break;

  case 34:
                    { fprintf(yyout, ";\n\t"); }
    break;

  case 35:
                               { fprintf(yyout, ";\n\t"); }
    break;

  case 36:
                               { kasJump((yyvsp[-1].string)); fprintf(yyout, ";\n\treturn;\n\t"); }
    break;

  case 40:
                                 { kasFSMCreateStart((yyvsp[0].string)); }
    break;

  case 41:
                                                                                  { kasFSMCreateEnd(); }
    break;

  case 44:
                     { fprintf(yyout, "else\n\t{\n\t"); }
    break;

  case 45:
                                                                     { fprintf(yyout, "}\n\t"); }
    break;

  case 47:
                                  { fprintf(yyout, "else if ("); }
    break;

  case 48:
                                                                                  { fprintf(yyout, ")\n\t{\n\t"); }
    break;

  case 49:
                                                                                                                               { fprintf(yyout, "}\n\t"); }
    break;

  case 50:
                       { fprintf(yyout, "if ("); }
    break;

  case 51:
                                                                  { fprintf(yyout, ")\n\t{\n\t"); }
    break;

  case 52:
                                                                                                               { fprintf(yyout, "}\n\t"); }
    break;

  case 54:
                               { fprintf(yyout, "if (!kasfVarValueGet(\"_VIFONCE%03d\") && (", ifOnceIndex+1); }
    break;

  case 55:
                                                                                                                                { fprintf(yyout, ")) // \"ifonce\" #%d\n\t{\n\tkasfVarCreateSet(\"_VIFONCE%03d\", 1);\n\t", ifOnceIndex+1, ifOnceIndex+1); }
    break;

  case 56:
                                                                                                                                                                                                                                                                        { fprintf(yyout, "}\n\t"); ++ifOnceIndex; }
    break;

  case 57:
                           { fprintf(yyout, " + "); }
    break;

  case 59:
                           { fprintf(yyout, " - "); }
    break;

  case 61:
                           { fprintf(yyout, " * "); }
    break;

  case 63:
                           { fprintf(yyout, " / "); }
    break;

  case 65:
                           { fprintf(yyout, " && "); }
    break;

  case 67:
                           { fprintf(yyout, " || "); }
    break;

  case 69:
                           { fprintf(yyout, " == "); }
    break;

  case 71:
                           { fprintf(yyout, " != "); }
    break;

  case 73:
                           { fprintf(yyout, " > "); }
    break;

  case 75:
                           { fprintf(yyout, " >= "); }
    break;

  case 77:
                           { fprintf(yyout, " < "); }
    break;

  case 79:
                           { fprintf(yyout, " <= "); }
    break;

  case 81:
                    { fprintf(yyout, "-"); }
    break;

  case 83:
                    { fprintf(yyout, "!"); }
    break;

  case 85:
                    { fprintf(yyout, "!"); }
    break;

  case 87:
                { fprintf(yyout, "("); }
    break;

  case 88:
                                                        { fprintf(yyout, ")"); }
    break;

  case 89:
                   { fprintf(yyout, "%d", (yyvsp[0].number)); }
    break;

  case 90:
               { kasFunctionStart((yyvsp[0].string)); }
    break;

  case 91:
                                                            { kasFunctionEnd(); }
    break;

  case 92:
                 { fprintf(yyout, "1"); }
    break;

  case 93:
                  { fprintf(yyout, "0"); }
    break;

  case 96:
                           { fprintf(yyout, ", "); }
    break;

  case 98:
                       { kasFunctionParamNumber(); }
    break;

  case 99:
                       { fprintf(yyout, "\"%s\"", (yyvsp[0].string));
                         kasFunctionParamCharPtr(); }
    break;

  case 100:
                            { kasLStringReference((yyvsp[0].string));
                              kasFunctionParamCharPtr(); }
    break;

  case 102:
                           { fprintf(yyout, "kasGrowSelectionPtr(\"%s\")", (yyvsp[0].string));
                             kasFunctionParamSelectCommandPtr(); }
    break;

  case 103:
                           { fprintf(yyout, "kasPathPtr(\"%s\")", (yyvsp[0].string));
                             kasFunctionParamPathPtr(); }
    break;

  case 104:
                            { fprintf(yyout, "kasVectorPtr(\"%s\")", (yyvsp[0].string));
                              kasFunctionParamVectorPtr(); }
    break;

  case 105:
                            { fprintf(yyout, "kasVolumePtr(\"%s\")", (yyvsp[0].string));
                              kasFunctionParamVolumePtr(); }
    break;

  case 106:
                                { fprintf(yyout, "kasAITeamShipsPtr(\"%s\")", (yyvsp[0].string));
                                  kasFunctionParamSelectCommandPtr(); }
    break;

  case 107:
                                 { fprintf(yyout, "kasShipsVectorPtr(\"%s\")", (yyvsp[0].string));
                                   kasFunctionParamVectorPtr(); }
    break;

  case 108:
                                 { fprintf(yyout, "kasTeamsVectorPtr(\"%s\")", (yyvsp[0].string));
                                   kasFunctionParamVectorPtr(); }
    break;

  case 109:
                                  { fprintf(yyout, "kasVolumeVectorPtr(\"%s\")", (yyvsp[0].string));
                                   kasFunctionParamVectorPtr(); }
    break;

  case 110:
                           { fprintf(yyout, "(&kasThisTeamPtr->shipList)");
                        kasFunctionParamSelectCommandPtr(); }
    break;

  case 111:
                           { fprintf(yyout, "kasThisTeamsVectorPtr()");
                                kasFunctionParamVectorPtr();  }
    break;

  case 112:
                           { fprintf(yyout, "kasAITeamPtr(\"%s\")", (yyvsp[0].string));
                             kasFunctionParamAITeamPtr(); }
    break;

  case 113:
                      { fprintf(yyout, "kasThisTeamPtr");
                        kasFunctionParamAITeamPtr(); }
    break;

  case 117:
                            { kasStateStart((yyvsp[0].string)); }
    break;

  case 118:
                        { kasStateEnd(""); }
    break;

  case 119:
                        { kasStateEnd((yyvsp[0].string)); }
    break;



      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}



int lineNum = 1;
char curFilename[256]; // current input filename
char levelName[MAX_LEVEL_NAME_LENGTH+1]; // current input filename
FILE *yyhout, *yyfout;
unsigned int functionCount;  // number of FSM functions

int yyerror (char *s)
{
    if (!strcasecmp(s, "parse error"))
        fprintf(stderr, "%s(%d) error : at '%s', %s\n", curFilename, lineNum, yytext, stateHelpGet());
    else
        fprintf(stderr, "%s(%d) error : at '%s', %s (%s)\n", curFilename, lineNum, yytext, stateHelpGet(), s);
    return 99;
}

char *levelNameGet(void)
{
    return levelName;
}

int lineNumGet(void)
{
    return lineNum;
}

char *curFilenameGet()
{
    return curFilename;
}

int main(int argc, char **argv)
{
    printf( "Main()\n" );

    char infilename[256], outfilename[256], houtfilename[256], foutfilename[256];
    char *shortinfilename, tempfilename[256];
    extern FILE *yyin, *yyout;
    int i;

    if (argc >= 2 && !strcasecmp(argv[1], "-F"))
    {
        kasHeaders(0);
        exit(1);
    }
    else if (argc >= 2 && !strcasecmp(argv[1], "-FC"))
    {
        kasHeaders(1);
        exit(1);
    }
    else if (argc != 4 && argc != 5)
    {
        fprintf(stderr, "\nKAS2C Version %s\n", KAS2C_VERSION);
        fprintf(stderr, "Copyright (C) 1998 Relic Entertainment Inc.  All rights reserved.\n");
        fprintf(stderr, "Modifications for Homeworld SDL by Ted Cipicchio <ted@thereisnospork.com>\n");
        fprintf(stderr, "\n Usage:\n\n");
        fprintf(stderr, " KAS2C mission.kas mission.c mission.h [func.c]\n");
        fprintf(stderr, " - generate mission.c and mission.h (and, optionally, func.c)\n");
        fprintf(stderr, " KAS2C -f\n");
        fprintf(stderr, " - list function headers (english style)\n");
        fprintf(stderr, " KAS2C -fc\n");
        fprintf(stderr, " - list function headers (C-style)\n");
        exit(1);
    }

    strcpy(infilename, argv[1]);

    strcpy(outfilename, argv[2]);

    strcpy(houtfilename, argv[3]);

    strcpy(foutfilename, (argc == 5 ? argv[4] : ""));

    if (!(yyin = fopen(infilename, "r")))
    {
        fprintf(stderr, "%s: can't open\n", infilename);
        exit(1);
    }

    if (!(yyout = fopen(outfilename, "w")))
    {
        fprintf(stderr, "%s: can't open\n", outfilename);
        exit(1);
    }

    if (!(yyhout = fopen(houtfilename, "w")))
    {
        fprintf(stderr, "%s: can't open\n", houtfilename);
        exit(1);
    }

    if (argc == 5 && !(yyfout = fopen(foutfilename, "w")))
    {
        fprintf(stderr, "%s: can't open\n", foutfilename);
        exit(1);
    }

    // hack off leading paths for display of infilename
    shortinfilename = infilename+strlen(infilename);
    while (shortinfilename > infilename)
    {
        --shortinfilename;
        if (*shortinfilename == '\\' || *shortinfilename == '/')
        {
            ++shortinfilename;
            break;
        }
    }
    fprintf(stderr, "%s\n", shortinfilename);

    strcpy(tempfilename, shortinfilename);
    strcpy(infilename, tempfilename);

    strcpy(levelName, shortinfilename);
    i = 0;
    while (i < strlen(levelName) && levelName[i] != '.')
        ++i;
    if (i < strlen(levelName))
        levelName[i] = 0;  // knock off extension

    strcpy(curFilename, infilename);

    fprintf(yyout, "//\n");
    fprintf(yyout, "//  %s\n", outfilename);
    fprintf(yyout, "//\n");
    fprintf(yyout, "//  Finite state machine routines for \"%s\" mission\n", levelName);
    fprintf(yyout, "//\n");
    fprintf(yyout, "//  Copyright (C) 1998 Relic Entertainment Inc.\n");
    fprintf(yyout, "//  All rights reserved\n");
    fprintf(yyout, "//\n");
    fprintf(yyout, "//  This code was autogenerated from %s by KAS2C Version %s\n",
        infilename, KAS2C_VERSION);
    fprintf(yyout, "//\n\n\n");
    fprintf(yyout, "#include \"%s\"  // prototypes and #includes for exposed game functions\n", houtfilename);
    fprintf(yyout, "\n");
    fprintf(yyout, "extern AITeam *CurrentTeamP;\n");
    fprintf(yyout, "#define kasThisTeamPtr CurrentTeamP\n");
    fprintf(yyout, "\n");
    fprintf(yyout, "//  for run-time scoping of symbols (variables, timers, etc.)\n");
    fprintf(yyout, "extern sdword CurrentMissionScope;\n");
    fprintf(yyout, "extern char CurrentMissionScopeName[];\n");
    fprintf(yyout, "\n");

    fprintf(yyout, "//  for displaying localized strings\n");
    fprintf(yyout, "extern udword strCurLanguage;\n");
    fprintf(yyout, "\n");

    fprintf(yyhout, "//\n");
    fprintf(yyhout, "//  %s\n", houtfilename);
    fprintf(yyhout, "//\n");
    fprintf(yyhout, "//  Finite state machine for \"%s\" mission\n", levelName);
    fprintf(yyhout, "//\n");
    fprintf(yyhout, "//  Copyright (C) 1998 Relic Entertainment Inc.\n");
    fprintf(yyhout, "//  All rights reserved\n");
    fprintf(yyhout, "//\n");
    fprintf(yyhout, "//  This code was autogenerated from %s by KAS2C Version %s\n",
        infilename, KAS2C_VERSION);
    fprintf(yyhout, "//\n\n\n");
    fprintf(yyhout, "#ifndef __%s_H\n", levelName);
    fprintf(yyhout, "#define __%s_H\n\n\n", levelName);

    // add any other required #includes here
    fprintf(yyhout, "//\n");
    fprintf(yyhout, "//  types and exposed game functions\n");
    fprintf(yyhout, "//\n");
    fprintf(yyhout, "#include <string.h>\n");
    fprintf(yyhout, "#include \"AIMoves.h\"\n");
    fprintf(yyhout, "#include \"AITeam.h\"\n");
    fprintf(yyhout, "#include \"Attributes.h\"\n");
    fprintf(yyhout, "#include \"CommandWrap.h\"\n");
    fprintf(yyhout, "#include \"KASFunc.h\"\n");
    fprintf(yyhout, "#include \"Objectives.h\"\n");
    fprintf(yyhout, "#include \"Timer.h\"\n");
    fprintf(yyhout, "#include \"TradeMgr.h\"\n");
    fprintf(yyhout, "#include \"Types.h\"\n");
    fprintf(yyhout, "#include \"Vector.h\"\n");
    fprintf(yyhout, "#include \"Volume.h\"\n");

    // declarations of function list information
    if (yyfout)
    {
        fprintf(yyhout, "\n\n");
        fprintf(yyhout, "//\n");
        fprintf(yyhout, "//  level function pointer list\n");
        fprintf(yyhout, "//\n");
        fprintf(yyhout, "extern const void* %s_FunctionPointers[];\n", levelName);
        fprintf(yyhout, "extern const unsigned int %s_FunctionPointerCount;\n", levelName);
    }

    fprintf(yyhout, "\n\n");
    fprintf(yyhout, "//\n");
    fprintf(yyhout, "//  FSM prototypes\n");
    fprintf(yyhout, "//\n");

    if (yyfout)
    {
        fprintf(yyfout, "//\n");
        fprintf(yyfout, "//  %s\n", foutfilename);
        fprintf(yyfout, "//\n");
        fprintf(yyfout, "//  Finite state machine function pointers for \"%s\" mission\n", levelName);
        fprintf(yyfout, "//\n");
        fprintf(yyfout, "//  This code was autogenerated from %s by KAS2C Version %s\n",
            infilename, KAS2C_VERSION);
        fprintf(yyfout, "//\n\n\n");
        fprintf(yyfout, "#include \"%s\"  // prototypes and #includes for exposed game functions\n", houtfilename);
        fprintf(yyfout, "\n");
        fprintf(yyfout, "// FSM init/watch function pointers.\n");
        fprintf(yyfout, "const void* %s_FunctionPointers[] =\n", levelName);
        fprintf(yyfout, "{\n");
    }

    ifOnceIndex = 0;  // reset counter for each source file parsed

    functionCount = 0;  // number of FSM init/watch functions created

    // normal yyin and yyout interaction
    yyparse();

    fprintf(yyhout, "\n\n#endif\n");

    if (yyfout)
    {
        fprintf(yyfout, "};\n");
        fprintf(yyfout, "\n");
        fprintf(yyfout, "// Number of FSM init/watch function pointers.\n");
        fprintf(yyfout, "const unsigned int %s_FunctionPointerCount = %u;\n", levelName, functionCount);
    }

    fclose(yyin);
    fclose(yyout);
    fclose(yyhout);
    if (yyfout)
        fclose(yyfout);

    // to be done: unlink output files on error?
    if (yynerrs)
    {
        unlink(outfilename);
        unlink(houtfilename);
        if (foutfilename[0]);
            unlink(foutfilename);
    }
    return 0;
}
