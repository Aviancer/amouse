#ifndef VERSION_H_   /* Include guard */
#define VERSION_H_

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define V_MAJOR 1
#define V_MINOR 5
#define V_REVISION 1
#define V_FULL STR(V_MAJOR) STR(.) STR(V_MINOR) STR(.) STR(V_REVISION)

#endif // VERSION_H_
