#ifndef __AWDS_DEBUG_H_
#define __AWDS_DEBUG_H_

/*
 * kernel debugging
 */
#ifdef DEBUG

    #define AWDS_INDENT_LENGTH 128

    static int  awds_current_depth;
    static char awds_indent[AWDS_INDENT_LENGTH+1];

    #define AWDS_DEBUG(fmt, args...) \
        printk(KERN_DEBUG "awds %s:%s:%d:  " fmt,__FILE__, __FUNCTION__ , __LINE__ , ## args)

    #define AWDS_INDENT(depth) \
        do { \
        if(depth*2 < AWDS_INDENT_LENGTH) { \
        memset(awds_indent, 0x20, AWDS_INDENT_LENGTH); \
        memset(awds_indent+depth*2, 0x0, 1); \
        } \
        } while (0)

    #define AWDS_FUNC_IN(dbg) \
        int dbglevel = dbg; \
        do { \
        if(dbglevel <= debuglevel) {\
        AWDS_INFO("+%s()\n",__FUNCTION__); \
        awds_current_depth++; \
        if(awds_current_depth > AWDS_INDENT_LENGTH/2) \
        { \
        awds_current_depth = AWDS_INDENT_LENGTH/2; \
        } \
        AWDS_INDENT(awds_current_depth);\
        } \
        } while (0)

    #define AWDS_FUNC_OUT() \
        do { \
        if(dbglevel <= debuglevel) {\
        awds_current_depth--; \
        if(awds_current_depth < 0) \
        { \
        awds_current_depth = 0; \
        } \
        AWDS_INDENT(awds_current_depth); \
        AWDS_INFO("-%s()\n",__FUNCTION__); \
        } \
        } while (0)

    #define AWDS_RETURN \
        AWDS_FUNC_OUT(); \
        return

    #define AWDS_INFO(fmt, args...) \
        do { \
        if(dbglevel <= debuglevel) {\
        printk(KERN_INFO "awds: %s" fmt,awds_indent , ## args); \
        } \
        } while (0)

#else

    #define AWDS_DEBUG(fmt, args...) \
        do {} while (0)

    #define AWDS_FUNC_IN(fmt, args...) \
        do {} while (0)

    #define AWDS_INFO(dbg, fmt, args...) \
        printk(KERN_INFO "awds: " fmt, ## args)

    #define AWDS_FUNC_OUT(fmt, arg) \
        do {} while (0)

    #define AWDS_RETURN \
        return 

#endif

#endif


