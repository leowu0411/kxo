#ifndef KXO_PKG_H
#define KXO_PKG_H

struct package {
    char val;
    int move;
};

#define PKG_PUT_AI(pkg, c) (READ_ONCE(pkg.val) & 0x80) | c
#define PKG_SET_END(pkg) (READ_ONCE(pkg.val) & 0x7F) | 0x80
#define PKG_CLR_END(pkg) (READ_ONCE(pkg.val) & 0x7F) | 0x00
#define PKG_GET_AI(val) val & 0x7F
#define PKG_GET_END(val) (((val) & 0x80) != 0)
#endif /* KXO_PKG_H */
