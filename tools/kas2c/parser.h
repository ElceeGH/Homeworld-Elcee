
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
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


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
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
     POINT = 287,
     VOLUME = 288,
     THISTEAM = 289,
     THISTEAMSHIPS = 290,
     THISTEAMSPOINT = 291,
     JUMP = 292,
     TRUE = 293,
     FALSE = 294,
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 42 "parser.y"

    char    *string;    /* string buffer */
    int     number;     /* numeric value */



/* Line 1676 of yacc.c  */
#line 110 "parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


