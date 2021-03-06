/* *********************************************************************** *
 *   File:  mob_fix.c                                    Part of CircleMUD *
 *  Usage:  equalizing the mob files                                       *
 *  Writer: Angus Mezick, parts taken from db.c                            *
 * *********************************************************************** */

/*
 * here is some code for a mob file equalizer.   Since we spend a lot of
 * time porting in zones, we noticed that some zones were really powerful
 * and some were weak.  this mainly had to do with mobs.  This program
 * takes in a .mob file, and then equalizes some of the stats based on
 * level, writing out to another file.  A lot of this code is taken from
 * db.c, so you should recognize it :)  I just don't want anyone making
 * money with it :).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>


int stats[][13]=
{
/*lev    hp        exp  thaco  ac   #d#+dam      gold    at2   at3   at4   hit*/
  {0,   10,      191     ,  20,  100,  1,2,1,        100,    0,    0,    0,    1}, 
  {1,   13,      366     ,  19,   99,  1,3,1,        150,    0,    0,    0,    1}, 
  {2,   18,      650     ,  19,   97,  1,4,1,        250,    3,    0,    0,    1}, 
  {3,   23,      1058    ,  19,   96,  1,5,1,        350,    3,    0,    0,    1},
  {4,   30,      1605    ,  19,   94,  1,6,2,        500,    3,    0,    0,    2}, 
  {5,   37,      2305    ,  19,   93,  1,7,2,        650,    6,    0,    0,    2}, 
  {6,   44,      3173    ,  18,   91,  1,8,3,        800,    6,    0,    0,    3},
  {7,   53,      4223    ,  18,   90,  1,9,3,       1000,    6,    0,    0,    3},
  {8,   62,      5470    ,  18,   88,  1,10,4,      1200,    9,    0,    0,    4},
  {9,   71,      6928    ,  18,   87,  1,11,4,      1400,    9,    0,    0,    4},
  {10,  80,      8612    ,  18,   85,  1,12,5,      1600,    9,    0,    0,    5},
  {11,  91,      10537   ,  17,   84,  1,13,5,      1850,   12,    0,    0,    5},
  {12,  102,     12718   ,  17,   82,  1,14,6,      2100,   12,    0,    0,    6},
  {13,  113,     15168   ,  17,   81,  1,15,6,      2350,   12,    0,    0,    6},
  {14,  124,     17902   ,  17,   79,  1,16,7,      2600,   15,    0,    0,    7},
  {15,  135,     20935   ,  17,   78,  1,17,7,      2850,   15,    0,    0,    7},
  {16,  148,     24282   ,  16,   76,  1,18,8,      3150,   15,    0,    0,    8},
  {17,  161,     27957   ,  16,   75,  1,19,8,      3450,   18,    0,    0,    8},
  {18,  174,     31975   ,  16,   73,  1,20,9,      3750,   18,    0,    0,    9},
  {19,  187,     36350   ,  16,   72,  1,21,9,      4050,   18,    0,    0,    9},
  {20,  200,     41097   ,  16,   70,  1,22,10,      4350,   21,    0,    0,   10},
  {21,  213,     46230   ,  15,   69,  1,23,10,      4650,   21,    0,    0,   10},
  {22,  228,     51764   ,  15,   67,  1,24,11,      5000,   21,    0,    0,   11},
  {23,  243,     57714   ,  15,   66,  1,25,11,      5350,   24,    0,    0,   11},
  {24,  258,     64094   ,  15,   64,  1,26,12,      5700,   24,    0,    0,   12},
  {25,  273,     70919   ,  15,   63,  1,27,12,      6050,   24,    0,    0,   12},
  {26,  288,     78204   ,  14,   61,  1,28,13,      6400,   27,    0,    0,   13},
  {27,  303,     85962   ,  14,   60,  1,29,13,      6750,   27,    0,    0,   13},
  {28,  318,     94209   ,  14,   58,  1,30,14,      7100,   27,    0,    0,   14},
  {29,  335,     102959  ,  14,   57,  1,31,14,      7500,   30,    0,    0,   14},
  {30,  352,     112227  ,  14,   55,  1,32,15,     7900,   30,    0,    0,   15},
  {31,  369,     122027  ,  13,   54,  1,33,15,     8300,   30,    0,    0,   15},
  {32,  386,     132373  ,  13,   52,  1,34,16,     8700,   33,    0,    0,   16},
  {33,  403,     143282  ,  13,   51,  1,35,16,     9100,   33,    3,    0,   16},
  {34,  420,     154766  ,  13,   49,  1,36,17,     9500,   33,    3,    0,   17},
  {35,  437,     166841  ,  13,   48,  1,37,17,     9900,   36,    3,    0,   17},
  {36,  454,     179521  ,  12,   46,  1,38,18,    10300,   36,    3,    0,   18},
  {37,  473,     192821  ,  12,   45,  1,39,18,    10750,   39,    3,    0,   18},
  {38,  492,     206756  ,  12,   43,  1,40,19,    11200,   39,    6,    0,   19},
  {39,  511,     221339  ,  12,   42,  1,41,19,    11650,   39,    6,    0,   19},
  {40,  530,     236586  ,  12,   40,  1,42,20,    12100,   42,    6,    0,   20},
  {41,  549,     252511  ,  11,   39,  1,43,20,    12550,   42,    6,    0,   20},
  {42,  568,     269128  ,  11,   37,  1,44,21,    13000,   45,    6,    0,   21},
  {43,  587,     286453  ,  11,   36,  1,45,21,    13450,   45,    9,    0,   21},
  {44,  606,     304500  ,  11,   34,  1,46,22,    13900,   45,    9,    0,   22},
  {45,  625,     323284  ,  11,   33,  1,47,22,    14350,   48,    9,    0,   22},
  {46,  646,     342818  ,  10,   31,  1,48,23,    14850,   48,    9,    0,   23},
  {47,  667,     363118  ,  10,   30,  1,49,23,    15350,   51,    9,    0,   23},
  {48,  688,     384198  ,  10,   28,  1,50,24,    15850,   51,   12,    0,   24},
  {49,  709,     406073  ,  10,   27,  1,51,24,    16350,   51,   12,    0,   24},
  {50,  730,     428757  ,  10,   25,  1,52,25,    16850,   54,   12,    0,   25},
  {51,  751,     452266  ,   9,   24,  1,53,25,    17350,   54,   12,    0,   25},
  {52,  772,     476613  ,   9,   22,  1,54,26,    17850,   57,   12,    0,   26},
  {53,  793,     501813  ,   9,   21,  1,55,26,    18350,   57,   15,    0,   26},
  {54,  814,     527880  ,   9,   19,  1,56,27,    18850,   57,   15,    0,   27},
  {55,  835,     554830  ,   9,   18,  1,57,27,    19350,   60,   15,    0,   27},
  {56,  858,     582677  ,   8,   16,  1,58,28,    19900,   60,   15,    0,   28},
  {57,  881,     611435  ,   8,   15,  1,59,28,    20450,   63,   15,    0,   28},
  {58,  904,     641120  ,   8,   13,  1,60,29,    21000,   63,   18,    0,   29},
  {59,  927,     671745  ,   8,   12,  1,61,29,    21550,   63,   18,    0,   29},
  {60,  950,     703325  ,   8,   10,  1,62,30,    22100,   66,   18,    0,   30},
  {61,  973,     735875  ,   7,    9,  1,63,30,    22650,   66,   18,    0,   30},
  {62,  996,     769409  ,   7,    7,  1,64,31,    23200,   69,   18,    0,   31},
  {63,  1019,    803943  ,   7,    6,  1,65,31,    23750,   69,   21,    0,   31},
  {64,  1042,    839489  ,   7,    4,  1,66,32,    24300,   69,   21,    0,   32},
  {65,  1065,    876064  ,   7,    3,  1,67,32,    24850,   72,   21,    0,   32},
  {66,  1088,    913682  ,   6,    1,  1,68,33,    25400,   72,   21,    0,   33},
  {67,  1113,    952357  ,   6,    0,  1,69,33,    26000,   75,   21,    0,   33}, 
  {68,  1138,    992104  ,   6,   -2,  1,70,34,    26600,   75,   24,    0,   34}, 
  {69,  1163,    1032937 ,   6,   -3,  1,71,34,    27200,   75,   24,    0,   34}, 
  {70,  1188,    1074872 ,   6,   -5,  1,72,35,    27800,   78,   24,    0,   35},
  {71,  1213,    1117922 ,   5,   -6,  1,73,35,    28400,   78,   24,    0,   35}, 
  {72,  1238,    1162102 ,   5,   -8,  1,74,36,    29000,   81,   24,    0,   36}, 
  {73,  1263,    1207427 ,   5,   -9,  1,75,36,    29600,   81,   27,    0,   36},
  {74,  1288,    1253911 ,   5,  -11,  1,76,37,    30200,   81,   27,    0,   37},
  {75,  1313,    1301569 ,   5,  -12,  1,77,37,    30800,   84,   27,    0,   37},
  {76,  1338,    1350416 ,   4,  -14,  1,78,38,    31400,   84,   27,    0,   38},
  {77,  1363,    1400466 ,   4,  -15,  1,79,38,    32000,   87,   27,    0,   38},
  {78,  1388,    1451734 ,   4,  -17,  1,80,39,    32600,   87,   30,    0,   39},
  {79,  1415,    1504234 ,   4,  -18,  1,81,39,    33250,   87,   30,    0,   39},
  {80,  1442,    1557981 ,   4,  -20,  1,82,40,    33900,   90,   30,    0,   40},
  {81,  1469,    1612989 ,   3,  -21,  1,83,40,    34550,   90,   30,    0,   40},
  {82,  1496,    1669273 ,   3,  -23,  1,84,41,    35200,   90,   30,    0,   41},
  {83,  1523,    1726848 ,   3,  -24,  1,85,41,    35850,   90,   32,    0,   41},
  {84,  1550,    1785729 ,   3,  -26,  1,86,42,    36500,   92,   32,    0,   42},
  {85,  1577,    1845929 ,   3,  -27,  1,87,42,    37150,   92,   32,    0,   42},
  {86,  1604,    1907463 ,   2,  -29,  1,88,43,    37800,   92,   32,    0,   43},
  {87,  1631,    1970346 ,   2,  -30,  1,89,43,    38450,   92,   34,    0,   43},
  {88,  1658,    2034593 ,   2,  -32,  1,90,44,    39100,   92,   34,    0,   44},
  {89,  1685,    2100218 ,   2,  -33,  1,91,44,    39750,   92,   34,    0,   44},
  {90,  1712,    2167236 ,   2,  -35,  1,92,45,    40400,   92,   36,    0,   45},
  {91,  1739,    2235661 ,   1,  -36,  1,93,45,    41050,   94,   36,    0,   45},
  {92,  1768,    2305508 ,   1,  -38,  1,94,46,    41750,   94,   36,    0,   46},
  {93,  1797,    2376791 ,   1,  -39,  1,95,46,    42450,   94,   36,    0,   46},
  {94,  1826,    2449525 ,   1,  -41,  1,96,47,    43150,   94,   38,    0,   47},
  {95,  1855,    2523725 ,   1,  -42,  1,97,47,    43850,   94,   38,    0,   47},
  {96,  1884,    2599405 ,   0,  -44,  1,98,48,    44550,   94,   38,    0,   48},
  {97,  1913,    2676580 ,   0,  -45,  1,99,48,    45250,   94,   40,    0,   48},
  {98,  1942,    2755265 ,   0,  -47,  1,100,49,   45950,   96,   40,    0,   49},
  {99,  1971,    2835473 ,   0,  -48,  1,101,49,   46650,   96,   40,    0,   49},
  {100, 2000,    2917220 ,   0,  -50,  1,102,50,   47350,   96,   40,    0,   50},
  {101, 2029,    3000520 ,  -1,  -51,  1,103,50,   48050,   96,   42,    0,   50},
  {102, 2058,    3085388 ,  -1,  -53,  1,104,51,   48750,   96,   42,    0,   51},
  {103, 2087,    3171838 ,  -1,  -54,  1,105,51,   49450,   96,   42,    0,   51},
  {104, 2116,    3259884 ,  -1,  -56,  1,106,52,   50150,   96,   44,    0,   52},
  {105, 2145,    3349543 ,  -1,  -57,  1,107,52,   50850,   98,   44,    0,   52},
  {106, 2176,    3440827 ,  -2,  -59,  1,108,53,   51600,   98,   44,    0,   53},
  {107, 3207,    3533752 ,  -2,  -60,  1,109,53,   52350,   98,   44,    0,   53},
  {108, 2238,    3628332 ,  -2,  -62,  1,110,54,   53100,   98,   46,    0,   54},
  {109, 2269,    3724582 ,  -2,  -63,  1,111,54,   53850,   98,   46,    0,   54},
  {110, 2300,    3822517 ,  -2,  -65,  1,112,55,   54600,   98,   46,    0,   55},
  {111, 2331,    3922150 ,  -3,  -66,  1,113,55,   55350,   98,   48,    0,   55},
  {112, 2362,    4023497 ,  -3,  -68,  1,114,56,   56100,  100,   48,    0,   56},
  {113, 2393,    4126572 ,  -3,  -69,  1,115,56,   56850,  100,   48,    0,   56},
  {114, 2424,    4231389 ,  -3,  -71,  1,116,57,   57600,  100,   48,    0,   57},
  {115, 2455,    4337964 ,  -3,  -72,  1,117,57,   58350,  100,   50,    0,   57},
  {116, 2486,    4446311 ,  -4,  -74,  1,118,58,   59100,  100,   50,    0,   58},
  {117, 2517,    4556445 ,  -4,  -75,  1,119,58,   59850,  100,   53,    0,   58},
  {118, 2548,    4668379 ,  -4,  -77,  1,120,59,   60600,  100,   53,    0,   59},
  {119, 2579,    4782129 ,  -4,  -78,  1,121,59,   61350,  100,   53,    3,   59},
  {120, 2610,    4897709 ,  -4,  -80,  1,122,60,   62100,  100,   53,    3,   60},
  {121, 2643,    5015134 ,  -5,  -81,  1,123,60,   62900,  100,   56,    3,   60},
  {122, 2676,    5134418 ,  -5,  -83,  1,124,61,   63700,  100,   56,    3,   61},
  {123, 2709,    5255577 ,  -5,  -84,  1,125,61,   64500,  100,   56,    6,   61},
  {124, 2742,    5378624 ,  -5,  -86,  1,126,62,   65300,  100,   56,    6,   62},
  {125, 2775,    5503574 ,  -5,  -87,  1,127,62,   66100,  100,   59,    6,   62},
  {126, 2808,    5630441 ,  -6,  -89,  1,128,63,   66900,  100,   59,    6,   63},
  {127, 2841,    5759241 ,  -6,  -90,  1,129,63,   67700,  100,   59,    9,   63},
  {128, 2874,    5889988 ,  -6,  -92,  1,130,64,   68500,  100,   59,    9,   64},
  {129, 2907,    6022696 ,  -6,  -93,  1,131,64,   69300,  100,   62,    9,   64},
  {130, 2940,    6157381 ,  -6,  -95,  1,132,65,   70100,  100,   62,    9,   65},
  {131, 2973,    6294056 ,  -7,  -96,  1,133,65,   70900,  100,   62,   12,   65},
  {132, 3006,    6432736 ,  -7,  -98,  1,134,66,   71700,  100,   62,   12,   66},
  {133, 3039,    6573436 ,  -7,  -99,  1,135,66,   72500,  100,   65,   12,   66},
  {134, 3072,    6716170 ,  -7, -101,  1,136,67,   73300,  100,   65,   12,   67}, 
  {135, 3105,    6860954 ,  -7, -102,  1,137,67,   74100,  100,   65,   15,   67}, 
  {136, 3138,    7007800 ,  -8, -104,  1,138,68,   74900,  100,   65,   15,   68}, 
  {137, 3173,    7156725 ,  -8, -105,  1,139,68,   75750,  100,   68,   15,   68},
  {138, 3208,    7307743 ,  -8, -107,  1,140,69,   76600,  100,   68,   15,   69}, 
  {139, 3243,    7460868 ,  -8, -108,  1,141,69,   77450,  100,   68,   18,   69}, 
  {140, 3278,    7616115 ,  -8, -110,  1,142,70,   78300,  100,   68,   18,   70},
  {141, 3313,    7773498 ,  -9, -111,  1,143,70,   79150,  100,   71,   18,   70},
  {142, 3348,    7933033 ,  -9, -113,  1,144,71,   80000,  100,   71,   18,   71},
  {143, 3383,    8094733 ,  -9, -114,  1,145,71,   80850,  100,   71,   21,   71},
  {144, 3418,    8258613 ,  -9, -116,  1,146,72,   81700,  100,   71,   21,   72},
  {145, 3453,    8424688 ,  -9, -117,  1,147,72,   82550,  100,   74,   21,   72},
  {146, 3488,    8592972 , -10, -119,  1,148,73,   83400,  100,   74,   21,   73},
  {147, 3523,    8763480 , -10, -120,  1,149,73,   84250,  100,   74,   24,   73},
  {148, 3558,    8936227 , -10, -122,  1,150,74,   85100,  100,   74,   24,   74},
  {149, 3593,    9111227 , -10, -123,  1,151,74,   85950,  100,   77,   24,   74},
  {150, 3628,    9288495 , -10, -125,  1,152,75,   86800,  100,   77,   24,   75},
  {151, 3663,    9468045 , -11, -126,  1,153,75,   87650,  100,   77,   27,   75},
  {152, 3698,    9649892 , -11, -128,  1,154,76,   88500,  100,   77,   27,   76},
  {153, 3733,    9834050 , -11, -129,  1,155,76,   89350,  100,   80,   27,   76},
  {154, 3770,    10020534, -11, -131,  1,156,77,   90250,  100,   80,   27,   77},
  {155, 3807,    10209359, -11, -132,  1,157,77,   91150,  100,   80,   30,   77},
  {156, 3844,    10400540, -12, -134,  1,158,78,   92050,  100,   80,   30,   78},
  {157, 3881,    10594090, -12, -135,  1,159,78,   92950,  100,   80,   33,   78},
  {158, 3918,    10790024, -12, -137,  1,160,79,   93850,  100,   82,   33,   79},
  {159, 3955,    10988357, -12, -138,  1,161,79,   94750,  100,   82,   33,   79},
  {160, 3992,    11189104, -12, -140,  1,162,80,   95650,  100,   82,   36,   80},
  {161, 4029,    11392279, -13, -141,  1,163,80,   96550,  100,   82,   36,   80},
  {162, 4066,    11597897, -13, -143,  1,164,81,   97450,  100,   82,   39,   81},
  {163, 4103,    11805972, -13, -144,  1,165,81,   98350,  100,   84,   39,   81},
  {164, 4140,    12016519, -13, -146,  1,166,82,   99250,  100,   84,   39,   82},
  {165, 4177,    12229552, -13, -147,  1,167,82,  101050,  100,   84,   42,   82},
  {166, 4214,    12445086, -14, -149,  1,168,83,  101950,  100,   84,   42,   83},
  {167, 4251,    12663136, -14, -150,  1,169,83,  102850,  100,   84,   45,   83},
  {168, 4288,    12883716, -14, -152,  1,170,84,  103750,  100,   86,   45,   84},
  {169, 4325,    13106841, -14, -153,  1,171,84,  104650,  100,   86,   45,   84},
  {170, 4362,    13332526, -14, -155,  1,172,85,  105550,  100,   86,   48,   85},
  {171, 4399,    13560784, -15, -156,  1,173,85,  106450,  100,   86,   48,   85},
  {172, 4438,    13791631, -15, -158,  1,174,86,  107400,  100,   86,   51,   86},
  {173, 4477,    14025081, -15, -159,  1,175,86,  108350,  100,   88,   51,   86},
  {174, 4516,    14261149, -15, -161,  1,176,87,  109300,  100,   88,   51,   87},
  {175, 4555,    14499849, -15, -162,  1,177,87,  110250,  100,   88,   54,   87},
  {176, 4594,    14741195, -16, -164,  1,178,88,  111200,  100,   88,   54,   88},
  {177, 4633,    14985204, -16, -165,  1,179,88,  112150,  100,   88,   57,   88},
  {178, 4672,    15231888, -16, -167,  1,180,89,  113100,  100,   90,   57,   89},
  {179, 4711,    15481263, -16, -168,  1,181,89,  114050,  100,   90,   57,   89},
  {180, 4750,    15733343, -16, -170,  1,182,90,  115000,  100,   90,   60,   90},
  {181, 4789,    15988143, -17, -171,  1,183,90,  115950,  100,   90,   60,   90},
  {182, 4828,    16245678, -17, -173,  1,184,91,  116900,  100,   92,   60,   91},
  {183, 4867,    16505961, -17, -174,  1,185,91,  117850,  100,   92,   60,   91},
  {184, 4906,    16769008, -17, -176,  1,186,92,  118800,  100,   92,   63,   92},
  {185, 4945,    17034833, -17, -177,  1,187,92,  119750,  100,   92,   63,   92},
  {186, 4984,    17303450, -18, -179,  1,188,93,  120700,  100,   94,   63,   93},
  {187, 5023,    17574875, -18, -180,  1,189,93,  121650,  100,   94,   63,   93},
  {188, 5062,    17849122, -18, -182,  1,190,94,  122600,  100,   94,   66,   94},
  {189, 5101,    18126206, -18, -183,  1,191,94,  123550,  100,   94,   66,   94},
  {190, 5140,    18406140, -18, -185,  1,192,95,  124500,  100,   96,   66,   95},
  {191, 5181,    18688940, -19, -186,  1,193,95,  125500,  100,   96,   66,   95},
  {192, 5222,    18974620, -19, -188,  1,194,96,  126500,  100,   96,   69,   96},
  {193, 5263,    19263195, -19, -189,  1,195,96,  127500,  100,   96,   69,   96},
  {194, 5304,    19554679, -19, -191,  1,196,97,  128500,  100,   98,   69,   97},
  {195, 5345,    19849088, -19, -192,  1,197,97,  129500,  100,   98,   69,   97},
  {196, 5386,    20146435, -20, -194,  1,198,98,  130500,  100,   98,   72,   98},
  {197, 5427,    20446735, -20, -195,  1,199,98,  131500,  100,   98,   72,   98},
  {198, 5468,    20750002, -20, -197,  1,200,99,  132500,  100,  100,   72,   99},
  {199, 5509,    21056252, -20, -198,  1,201,99,  133500,  100,  100,   72,   99},
  {200, 5550,    21950627, -20, -200,  1,202,99,  134500,  100,  100,   75,  100},
  {201, 7000,    22500000, -20, -200,  1,220,40,  200000,  100,  100,   80,  140},
  {202, 10000,   23000000, -20, -200,  1,240,50,  300000,  100,  100,   84,  150},
  {203, 15000,   23500000, -20, -200,  1,270,50,  400000,  100,  100,   88,  150},
  {204, 20000,   24000000, -20, -200,  1,300,50,  500000,  100,  100,   91,  150},
  {205, 30000,   25000000, -20, -200,  1,350,50,  600000,  100,  100,   94,  150},
  {206, 40000,   26000000, -20, -200,  1,400,50,  700000,  100,  100,   96,  150},
  {207, 50000,   27000000, -20, -200,  1,450,50,  800000,  100,  100,   98,  150},
  {208, 60000,   28000000, -20, -200,  1,500,50, 1000000,  100,  100,  100,  150},
  {209, 80000,   29000000, -20, -200,  1,600,50, 1200000,  100,  100,  100,  150},
  {210,100000,   30000000, -20, -200,  1,700,50, 1500000,  100,  100,  100,  150},
};

int level;

#define MAX_STRING_LENGTH 8192
#define READ_LINE  get_line(input_file,output_file,line)
#define HP(lev)  stats[(lev)][1]
#define EXP(lev) stats[(lev)][2]
#define THACO(lev) stats[(lev)][3]
#define AC(lev) stats[(lev)][4]
#define NUM_DICE(lev) stats[(lev)][5]
#define TYPE_DICE(lev) stats[(lev)][6]
#define DAMAGE(lev) stats[(lev)][7]
#define GOLD(lev) stats[(lev)][8]
#define CREATE(result, type, number)  do {\
        if (!((result) = (type *) calloc ((number), sizeof(type))))\
	 { perror("malloc failure");abort();}}while(0)
#define LOWER(c) (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))


int get_line(FILE *input_file, FILE *output_file, char *buf)
{
  char temp[256];
  int lines = 0;
  do
  {
    lines++;
    fgets(temp, 256, input_file);
    if(*temp=='*')
      fprintf(output_file,temp);
    else if (*temp)
      temp[strlen(temp) - 1] = '\0';
    else if(!*temp)
      fprintf(output_file,temp);
  }
  while (!feof(input_file)&&((*temp=='*')||(!*temp)));
  
  if (feof(input_file))
    return 0;
  else
  {
    strcpy(buf, temp);
    return lines;
  }
}


char *asciiflag_conv(char *flag)
{
  unsigned long flags = 0;
  int is_number = 1;
  char *p='\0';
  char flag_list[70];
  int i;
  char temp[40];
  char hold='\0';

  bzero(flag_list,70);
  bzero(temp,40);
  
  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
    {
      flags |= 1 << (26 + (*p - 'A'));
      }
    if (!isdigit(*p))
      is_number = 0;
  }
  
  if (is_number)
    flags = atol(flag);
  
  if(flags>0)
  {
    for(i=31;i>=0;i--)
    {
      if(i>25)
      {
	if(flags>=(1<<i))
	{
	  hold = 'A'+(i-26);
	  sprintf(temp,"%c",hold);
	  strcat(flag_list,temp);
	  flags = flags-(1<<i);
	}
      }
      else if(flags>=(1<<i))
      {
	hold = 'a'+(i);
	sprintf(temp,"%c",hold);
	strcat(flag_list,temp);
	flags = flags-(1<<i);
      }
    }
    flag = flag_list;
  }
  return flag;
}

char fread_letter(FILE *input_file)
{
  char c;
  do
  {
    c=getc(input_file);
  }
  while(isspace(c));
  return c;
}


/* read and allocate space for a '~'-terminated string from a given file */
void  fread_string(FILE *fl,FILE *output_file, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[500];
  register char        *point;
  int  flag;
  
  bzero(buf, MAX_STRING_LENGTH);
  
  do {
    if (!fgets(tmp, MAX_STRING_LENGTH, fl)) {
      fprintf(stderr, "fread_string: format error at or near %s\n", error);
      exit(0);
    }
    
    fprintf(output_file,tmp);
    
    for (point = tmp + strlen(tmp) - 2; point >= tmp && isspace(*point);
	 point--)
      ;
    flag = (*point == '~');
  } while (!flag);
  
}

char    *fname(char *namelist)
{
  static char  holder[30];
  register char        *point;
  
  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;
  
  *point = '\0';
  
  return(holder);
}

void parse_simple_mob(FILE *input_file, FILE *output_file, int nr)
{
  int t[10];
  char line[256];
  int thaco, maxhit, armor, numdice, damdice;
  time_t ti;
  int temp;
  

  READ_LINE;
  if(sscanf(line,"%d %d %d %dd%d+%d %dd%d+%d\n",
	    t,t+1,t+2,t+3,t+4,t+5,t+6,t+7,t+8)!=9)
   {
     printf("Format error in mob #%d, first line after S flag\n ... expecting line of form '# # # #d#+# #d#+#'\n",nr);
     exit(1);
   }
  level=t[0];
  thaco = (stats[level][12]-20)*(-1);
  maxhit = stats[level][1];
  armor = stats[level][4]/10; 
  temp = ((stats[level][6]) / 4);
  numdice = stats[level][5];
  damdice = stats[level][6];
  
  fprintf(output_file,"%d %d %d %dd%d+%d %dd%d+%d\n",
	  t[0],thaco, armor,1,1,  maxhit,
	  numdice, damdice, stats[level][7]);
  READ_LINE;
  sscanf(line," %d %d %d",t,t+1,t+2);
  fprintf(output_file,"%d %d %d\n",stats[level][8]/10,stats[level][2], t[2]);
  READ_LINE;
  if (sscanf(line," %d %d %d %d %d", t, t+1, t+2, t+3, t+4 )<3) 
  {
    printf("Format error in mob #%d, second line after S flag\n"
	   "...expecting line of form '# # #'\n",nr);
    exit(1);
  }
  fprintf(output_file,"%s\n",line);
}


void parse_enhanced_mob(FILE *input_file, FILE *output_file, int nr)
{
  char line[256];
  char temp[256];
  parse_simple_mob(input_file, output_file, nr);
  while(READ_LINE)
  {
    if(!strcmp(line,"E"))/*end of enhanced section*/
    {
      if (stats[level][9] != 0)
       fprintf(output_file,"Att2: %d\n",stats[level][9]);
      if (stats[level][10] != 0) 
       fprintf(output_file,"Att3: %d\n",stats[level][10]);
      if (stats[level][11] != 0)
       fprintf(output_file,"Att4: %d\n",stats[level][11]);
      fprintf(output_file,"E\n");
      return;
    }
    else if (*line =='#')
    {
      printf("Unterminated E section in mob #%d\n",nr);
      exit(1);
    }
    else
     {
      strncpy(temp,line,3);
      temp[3] = '\0';
      if(strcmp(temp,"Att")!=0)  
      {
       strcat(line,"\n");  
       fprintf(output_file,line);
      } 
     } 
  }
  printf("Unexpected end of file reached after mob #%d\n",nr);
  exit(1);
}

void parse_mob(FILE *input_file, FILE *output_file, int nr)
{
  char line[256];
  char letter;
  int t[10];
  static int i = 0;
  char f1[128],f2[128];
  char buf2[MAX_STRING_LENGTH];
  char *f1p, *f2p;
  long temp;
  
  /** string data **/
  /*get the name*/
  fread_string(input_file,output_file,buf2);
  
  /*do the short desc*/
  fread_string(input_file,output_file,buf2);
    
  /*long_desc*/
  fread_string(input_file,output_file,buf2);
    
  /*description*/
  fread_string(input_file,output_file,buf2);
  
  /** numeric data **/
  READ_LINE;
  sscanf(line,"%s %s %d %c",f1,f2,t+2,&letter);
  //f1p=asciiflag_conv(f1);
  //strcpy(f1,f1p);
  //f2p=asciiflag_conv(f2);
  //strcpy(f2,f2p);
  temp = atol(f2);
  temp=temp & 2143289343;
  sprintf(line,"%s %d %d %c\n",f1,temp,t[2],letter);
  fprintf(output_file,line);
  
  switch(letter)
  {
   case 'S':
     parse_simple_mob(input_file, output_file,nr);
     break;
   case 'E':
     parse_enhanced_mob(input_file,output_file,nr);
     break;
   default:
     printf("Unsupported mob type '%c' in mob #%d\n",letter,nr);
     exit(1);
     break;
  }
  
  letter=fread_letter(input_file);
  if(letter=='>')
  {
    ungetc(letter,input_file);
    do
    {
      READ_LINE;
      fprintf(output_file,line);
    }
    while(*line!='|');
    fprintf(output_file,"|\n");
  }
  else
    ungetc(letter,input_file);
  
  i++;
}




int main(int arg_count, char **arg_vector)
{
  FILE *input_file=0;/*input file*/
  FILE *output_file=0;/*the changed file*/
  int nr=-1;
  int last=0;
  char line[256];
  int end=1;
  char file_name[256];
  
  if(arg_count!=3)
  {
    printf("USAGE:  %s file_to_be_changed.mob new_file.mob\n",arg_vector[0]);
    exit(0);
  }
  
  sprintf(file_name,"./%s",arg_vector[1]);
  if(!(input_file=fopen(file_name,"r")))
  {
    printf("%s is not an existing file.\n",arg_vector[1]);
    exit(0);
  }
  
  sprintf(file_name,"./%s",arg_vector[2]);
  if(!(output_file=fopen(file_name,"w")))
  {
    printf("Could not open the output file: %s\n",arg_vector[2]);
    exit(0);
  }
  
  while(end)
  {
    if(!READ_LINE)
    {
      printf("Format error after mob #%d.(get_line)\n",nr);
      exit(1);
    }
    
    if(*line=='$')
    {
      fprintf(output_file,"$~\n");
      end=0;
      continue;
    }
    
    if(*line=='#')
    {
      last=nr;
      if(sscanf(line,"#%d",&nr)!=1)
      {
	printf("Format error after mob #%d (no number after #)\n",last);
	exit(1);
      }
      fprintf(output_file,"#%d\n",nr);
      if(nr>=99999)
      {
	fprintf(output_file,"$~\n");
	end=0;
	continue;
      }
      else
    	parse_mob(input_file,output_file,nr);
    }
    else
    { if ((*line=='T') || (*line=='Q')) //trigger and quest
       { 
       	strcat(line,"\n");  
        fprintf(output_file,line);   
       }
      else
       {
         printf("Format error in mob near #%d. \n",nr);
         printf("Offending line: '%s'\n",line);
       }  
    }
  }
  fclose(input_file);
  fclose(output_file);
  return (1);
}
